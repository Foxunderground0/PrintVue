#include <Arduino.h>
//#include <BluetoothSerial.h>

bool printHasStarted = false;
bool supportsManualShots = false;
float lastZ = -1000;

extern void createNewSequence();
extern void endSequence();
extern void appendSequence();
extern void setUploadSequenceShotToServer();
static void positionZNotifyCallback(float positionZ)
{
    // Process the change
    Serial.print("Position Z:");
    Serial.println(positionZ);
    if (!printHasStarted)
    {
        if (supportsManualShots) {
            Serial.println("Skipping auto shot in favor of manual");
        } else if (positionZ - lastZ <= 0.6 && positionZ - lastZ >= 0.05)
        {
            Serial.println("[Shot] Persistent Z change. We might have missed Begin");
            printHasStarted = true;
            createNewSequence();
            appendSequence();

            // Temporary function for testing the upload to the health check service
            setUploadSequenceShotToServer();
        } else
            Serial.println("Skipping auto shot because print hasn't started.");
        lastZ = positionZ;
    } else
    {
        if (supportsManualShots)
        {
            Serial.println("Skipping auto shot in favor of manual");
        } else {
            appendSequence();
        }
    }
}
static void requestShotNotifyCallback(float positionZ)
{
    // Process the change
    Serial.print("[Shot] Request shot @ position Z:");
    Serial.println(positionZ);
    supportsManualShots = true;
    appendSequence();
}
static void printingStatusNotifyCallback(int printingStatus)
{
    Serial.print("Printing Status:");
    if (printingStatus < 0)
    {
        Serial.println("End of print");
        supportsManualShots = false;
        lastZ = -1000;
        endSequence();
    } else
    {
        lastZ = 0;
        Serial.print("Print started @");
        printHasStarted = true;
        createNewSequence();
    }
    Serial.println(printingStatus);
}


//BluetoothSerial comSerial;
#define comSerial Serial
void commSetup()
{
    Serial.println("Test with serial comm");
    //comSerial.begin(115200, SERIAL_8N1, 2);
    // comSerial.begin("PrintVue BT", true);
    // comSerial.connect("M3D Enabler PV");
    // Connect to the Bluetooth server
    // Serial.print("Connecting to server: ");
    // Serial.println("M3D Enabler PV");
}

void commLoop()
{
    // if (!comSerial.connected())
    // {
    //     while (!comSerial.connected())
    //     {
    //         Serial.print(".");
    //         delay(500);
    //         comSerial.connect("M3D Enabler PV");
    //     }
    //     Serial.println("Connected!");
    // }

    if (comSerial.available())
    {
        String com = comSerial.readStringUntil('\n');
        Serial.print("Comm: ");
        Serial.println(com);
        if (com.startsWith("begin")) {
            printingStatusNotifyCallback(com.substring(6).toInt());
        } else if (com.startsWith("inc")) {
            requestShotNotifyCallback(com.substring(4).toFloat());
        } else if (com.startsWith("end")) {
            printingStatusNotifyCallback(-1);
        } else if (com.startsWith("z")) {
            positionZNotifyCallback(com.substring(2).toFloat());
        } else if (com.startsWith("reset")) {
            ESP.restart();
        } else
            Serial.println("Com not processed");
    }
}
