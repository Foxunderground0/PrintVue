#include <Arduino.h>
//#include <BluetoothSerial.h>

bool printHasStarted = false;
bool supportsManualShots = false;
float lastZ = -1000;

extern void createNewSequence();
extern void endSequence();
extern void appendSequence();

static void positionZNotifyCallback(float positionZ)
{
    // Process the change
    Serial.print("Position Z:");
    Serial.println(positionZ);
    if (!printHasStarted)
    {
        if (supportsManualShots){
            Serial.println("Skipping auto shot in favor of manual");
        }
        else if (positionZ - lastZ <= 0.6 && positionZ - lastZ >= 0.05)
        {
            Serial.println("[Shot] Persistent Z change. We might have missed Begin");
            printHasStarted = true;
            createNewSequence();
            appendSequence();
        }
        else
            Serial.println("Skipping auto shot because print hasn't started.");
        lastZ = positionZ;
    }
    else
    {
        if (supportsManualShots)
        {
            Serial.println("Skipping auto shot in favor of manual");
        }
        else {
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
    //
    Serial.print("Printing Status:");
    if (printingStatus < 0)
    {
        Serial.println("End of print");
        supportsManualShots = false;
        lastZ = -1000;
        endSequence();
    }
    else
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
        if(com.startsWith("begin")){
            printingStatusNotifyCallback(com.substring(6).toInt());
        }
        else if(com.startsWith("inc")){
            requestShotNotifyCallback(com.substring(4).toFloat());
        }
        else if(com.startsWith("end")){
            printingStatusNotifyCallback(-1);
        }
        else if(com.startsWith("z")){
            requestShotNotifyCallback(com.substring(2).toFloat());
        }
        else
            Serial.println("Com not processed");
    }
}

// #include <BLEDevice.h>
// #include <BLEUtils.h>
// #include <BLEScan.h>
// #include <BLEAdvertisedDevice.h>
// #include <Arduino.h>

// extern void createNewSequence();
// extern void endSequence();
// extern void appendSequence();

// // Define the name of the BLE device you want to connect to
// const char *bleServerName = "M3D Enabler";
// bool printHasStarted = false;
// bool supportsManualShots = false;
// float lastZ = -1000;

// // Activate notify
// const uint8_t notificationOn[] = {0x1, 0x0};
// const uint8_t notificationOff[] = {0x0, 0x0};

// static void positionZNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
//                                     uint8_t *pData, size_t length, bool isNotify)
// {
//     // Process the change
//     float positionZ = *reinterpret_cast<float *>(pData);
//     Serial.print("Position Z:");
//     Serial.println(positionZ);
//     if (!printHasStarted)
//     {
//         if (positionZ - lastZ <= 0.6 && positionZ - lastZ >= 0.05)
//         {
//             Serial.println("[Shot] Persistent Z change. We might have missed Begin");
//             printHasStarted = true;
//             createNewSequence();
//             appendSequence();
//         }
//         else
//             Serial.println("Skipping auto shot because print hasn't started.");
//         lastZ = positionZ;
//     }
//     else
//     {
//         if (supportsManualShots)
//         {
//             Serial.println("Skipping auto shot in favor of manual");
//         }
//         else {
//             appendSequence();
//         }
//     }
// }
// static void requestShotNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
//                                       uint8_t *pData, size_t length, bool isNotify)
// {
//     // Process the change
//     float positionZ = *reinterpret_cast<float *>(pData);
//     Serial.print("[Shot] Request shot @ position Z:");
//     Serial.println(positionZ);
//     supportsManualShots = true;
//     appendSequence();
// }
// static void printingStatusNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
//                                          uint8_t *pData, size_t length, bool isNotify)
// {
//     //
//     int printingStatus = *reinterpret_cast<int *>(pData);
//     Serial.print("Printing Status:");
//     if (printingStatus < 0)
//     {
//         Serial.println("End of print");
//         supportsManualShots = false;
//         lastZ = -1000;
//         endSequence();
//     }
//     else
//     {
//         lastZ = 0;
//         Serial.print("Print started @");
//         printHasStarted = true;
//         createNewSequence();
//     }
//     Serial.println(printingStatus);
// }

// // create service and characteristics:
// static BLEUUID bmeServiceUUID("8e088cd2-8100-11ee-b9d1-0242ac120002");
// static BLEUUID positionXCharactristicUUID("8e088cd2-7101-11ee-b9d1-0242ac120002");
// static BLEUUID positionYCharactristicUUID("8e088cd2-7102-11ee-b9d1-0242ac120002");
// static BLEUUID positionZCharactristicUUID("8e088cd2-7103-11ee-b9d1-0242ac120002");
// static BLEUUID positionECharactristicUUID("8e088cd2-7104-11ee-b9d1-0242ac120002");
// static BLEUUID requestShotCharactristicUUID("8e088cd2-7105-11ee-b9d1-0242ac120002");
// static BLEUUID temperatureCharactristicUUID("8e088cd2-7106-11ee-b9d1-0242ac120002");
// static BLEUUID printingStatusCharactristicUUID("8e088cd2-7107-11ee-b9d1-0242ac120002");

// // create service and characteristics:
// BLERemoteCharacteristic *positionXCharactristic;
// BLERemoteCharacteristic *positionYCharactristic;
// BLERemoteCharacteristic *positionZCharactristic;
// BLERemoteCharacteristic *positionECharactristic;
// BLERemoteCharacteristic *requestShotCharactristic;
// BLERemoteCharacteristic *temperatureCharactristic;
// BLERemoteCharacteristic *printingStatusCharactristic;

// // Flags stating if should begin connecting and if the connection is up
// static boolean doConnect = false;
// static boolean bleConnected = false;

// // Address of the peripheral device. Address will be found during scanning...
// static BLEAddress *pServerAddress;

// class MyClientCallback : public BLEClientCallbacks
// {
//     void onConnect(BLEClient *pClient)
//     {
//         Serial.println("Connected to BLE device");
//     }

//     void onDisconnect(BLEClient *pClient)
//     {
//         Serial.println("Disconnected from BLE device");
//         doConnect = true;
//         // Set a flag or perform other actions to handle disconnection
//     }
// };
// // Connect to the BLE Server that has the name, Service, and Characteristics
// bool connectToServer(BLEAddress pAddress)
// {
//     Serial.println("Connect to server");
//     BLEClient *pClient = BLEDevice::createClient();
//     pClient->setClientCallbacks(new MyClientCallback());

//     // Connect to the remove BLE Server.
//     pClient->connect(pAddress);
//     Serial.println(" - Connected to server");

//     // Obtain a reference to the service we are after in the remote BLE server.
//     BLERemoteService *pRemoteService = pClient->getService(bmeServiceUUID);
//     if (pRemoteService == nullptr)
//     {
//         Serial.print("Failed to find our service UUID: ");
//         Serial.println(bmeServiceUUID.toString().c_str());
//         return (false);
//     }

//     // Obtain a reference to the characteristics in the service of the remote BLE server.
//     positionXCharactristic = pRemoteService->getCharacteristic(positionXCharactristicUUID);
//     positionYCharactristic = pRemoteService->getCharacteristic(positionYCharactristicUUID);
//     positionZCharactristic = pRemoteService->getCharacteristic(positionZCharactristicUUID);
//     positionECharactristic = pRemoteService->getCharacteristic(positionECharactristicUUID);
//     requestShotCharactristic = pRemoteService->getCharacteristic(requestShotCharactristicUUID);
//     temperatureCharactristic = pRemoteService->getCharacteristic(temperatureCharactristicUUID);
//     printingStatusCharactristic = pRemoteService->getCharacteristic(printingStatusCharactristicUUID);

//     if (positionXCharactristic == nullptr)
//     {
//         Serial.print("Failed to find positionXCharactristic UUID");
//         return false;
//     }
//     if (positionYCharactristic == nullptr)
//     {
//         Serial.print("Failed to find positionYCharactristic UUID");
//         return false;
//     }
//     if (positionZCharactristic == nullptr)
//     {
//         Serial.print("Failed to find positionZCharactristic UUID");
//         return false;
//     }
//     if (positionECharactristic == nullptr)
//     {
//         Serial.print("Failed to find positionECharactristic UUID");
//         return false;
//     }
//     if (temperatureCharactristic == nullptr)
//     {
//         Serial.print("Failed to find temperatureCharactristic UUID");
//         return false;
//     }
//     if (printingStatusCharactristic == nullptr)
//     {
//         Serial.print("Failed to find printingStatusCharactristic UUID");
//         return false;
//     }
//     if (positionXCharactristic == nullptr || positionYCharactristic == nullptr || positionZCharactristic == nullptr || positionECharactristic == nullptr || temperatureCharactristic == nullptr || printingStatusCharactristic == nullptr)
//     {
//         Serial.print("Failed to find our characteristic UUID");
//         return false;
//     }
//     Serial.println(" - Found our characteristics");

//     // Assign callback functions for the Characteristics
//     positionZCharactristic->registerForNotify(positionZNotifyCallback);
//     printingStatusCharactristic->registerForNotify(printingStatusNotifyCallback);
//     requestShotCharactristic->registerForNotify(requestShotNotifyCallback);
//     return true;
// }
// // Callback function that gets called, when another device's advertisement has been received
// class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
// {
//     void onResult(BLEAdvertisedDevice advertisedDevice)
//     {
//         Serial.print("On Result: ");
//         if (String(advertisedDevice.getName().c_str()) == bleServerName)
//         {
//             Serial.print("[");
//             Serial.print(advertisedDevice.getName().c_str());
//             Serial.print("] == [");
//             Serial.print(bleServerName);
//             Serial.print("]");                                              // Check if the name of the advertiser matches
//             advertisedDevice.getScan()->stop();                             // Scan can be stopped, we found what we are looking for
//             pServerAddress = new BLEAddress(advertisedDevice.getAddress()); // Address of advertiser is the one we need
//             doConnect = true;                                               // Set indicator, stating that we are ready to connect
//             Serial.println("Device found. Connecting!");
//         }
//         else
//         {
//             Serial.print("[");
//             Serial.print(advertisedDevice.getName().c_str());
//             Serial.print("] != [");
//             Serial.print(bleServerName);
//             Serial.println("]");
//         }
//     }
// };
// void BeginScan()
// {
//     Serial.println("Begin Scan");
//     // Retrieve a Scanner and set the callback we want to use to be informed when we
//     // have detected a new device.  Specify that we want active scanning and start the
//     // scan to run for one hour
//     BLEDevice::init("");
//     auto pBLEScan = BLEDevice::getScan(); // create new scan
//     pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
//     pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
//     pBLEScan->setInterval(100);
//     pBLEScan->setWindow(99); // less or equal setInterval value
//     pBLEScan->start(3600);
// }

// void bluetoothSetup()
// {
//     // Serial.begin(115200);
//     Serial.println("Starting Arduino BLE Client application...");

//     // Init BLE device
//     BLEDevice::init("");
//     BeginScan();
// }

// void bluetoothLoop()
// {
//     // If the flag "doConnect" is true then we have scanned for and found the desired
//     // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
//     // connected we set the connected flag to be true.
//     if (doConnect == true)
//     {
//         if (connectToServer(*pServerAddress))
//         {
//             Serial.println("We are now connected to the BLE Server.");
//             // Activate the Notify property of each Characteristic
//             positionZCharactristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t *)notificationOn, 2, true);
//             printingStatusCharactristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t *)notificationOn, 2, true);
//             bleConnected = true;
//             doConnect = false;
//         }
//         else
//         {
//             Serial.println("We have failed to connect to the server; Restart your device to scan for nearby BLE server again.");
//             bleConnected = false;
//             doConnect = true;
//         }
//     }
// }