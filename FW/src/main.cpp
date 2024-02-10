#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Arduino.h>
// Define the name of the BLE device you want to connect to
const char *bleServerName = "M3D Enabler";
bool printHasStarted = false;
bool supportsManualShots = false;
float lastZ = -1000;

// Activate notify
const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};

static void positionZNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                                    uint8_t *pData, size_t length, bool isNotify)
{
  // Process the change
  float positionZ = *reinterpret_cast<float *>(pData);
  Serial.print("Position Z:");
  Serial.println(positionZ);
  if (supportsManualShots)
  {
    Serial.println("Skipping auto shot in favor of manual");
  }
  if (!printHasStarted)
  {
    if (positionZ - lastZ <= 0.6 && positionZ - lastZ >= 0.05)
    {
      Serial.println("[Shot] Persistent Z change. We might have skipped Begin");
    }
    else
      Serial.println("Skipping auto shot because print hasn't started.");
    lastZ = positionZ;
  }
}
static void requestShotNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                                      uint8_t *pData, size_t length, bool isNotify)
{
  // Process the change
  float positionZ = *reinterpret_cast<float *>(pData);
  Serial.print("[Shot] Request shot @ position Z:");
  Serial.println(positionZ);
  supportsManualShots = true;
}
static void printingStatusNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                                         uint8_t *pData, size_t length, bool isNotify)
{
  //
  int printingStatus = *reinterpret_cast<int *>(pData);
  Serial.print("Printing Status:");
  if (printingStatus < 0)
  {
    Serial.println("End of print");
    supportsManualShots = false;
    lastZ = -1000;
  }
  else
  {
    lastZ = 0;
    Serial.print("Print started @");
    printHasStarted = true;
  }
  Serial.println(printingStatus);
}

// create service and characteristics:
static BLEUUID bmeServiceUUID("8e088cd2-8100-11ee-b9d1-0242ac120002");
static BLEUUID positionXCharactristicUUID("8e088cd2-7101-11ee-b9d1-0242ac120002");
static BLEUUID positionYCharactristicUUID("8e088cd2-7102-11ee-b9d1-0242ac120002");
static BLEUUID positionZCharactristicUUID("8e088cd2-7103-11ee-b9d1-0242ac120002");
static BLEUUID positionECharactristicUUID("8e088cd2-7104-11ee-b9d1-0242ac120002");
static BLEUUID requestShotCharactristicUUID("8e088cd2-7105-11ee-b9d1-0242ac120002");
static BLEUUID temperatureCharactristicUUID("8e088cd2-7106-11ee-b9d1-0242ac120002");
static BLEUUID printingStatusCharactristicUUID("8e088cd2-7107-11ee-b9d1-0242ac120002");

// create service and characteristics:
BLERemoteCharacteristic *positionXCharactristic;
BLERemoteCharacteristic *positionYCharactristic;
BLERemoteCharacteristic *positionZCharactristic;
BLERemoteCharacteristic *positionECharactristic;
BLERemoteCharacteristic *requestShotCharactristic;
BLERemoteCharacteristic *temperatureCharactristic;
BLERemoteCharacteristic *printingStatusCharactristic;

// Flags stating if should begin connecting and if the connection is up
static boolean doConnect = false;
static boolean bleConnected = false;

// Address of the peripheral device. Address will be found during scanning...
static BLEAddress *pServerAddress;

class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient *pClient)
  {
    Serial.println("Connected to BLE device");
  }

  void onDisconnect(BLEClient *pClient)
  {
    Serial.println("Disconnected from BLE device");
    doConnect = true;
    // Set a flag or perform other actions to handle disconnection
  }
};
// Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer(BLEAddress pAddress)
{
  Serial.println("Connect to server");
  BLEClient *pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(bmeServiceUUID);
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(bmeServiceUUID.toString().c_str());
    return (false);
  }

  // Obtain a reference to the characteristics in the service of the remote BLE server.
  positionXCharactristic = pRemoteService->getCharacteristic(positionXCharactristicUUID);
  positionYCharactristic = pRemoteService->getCharacteristic(positionYCharactristicUUID);
  positionZCharactristic = pRemoteService->getCharacteristic(positionZCharactristicUUID);
  positionECharactristic = pRemoteService->getCharacteristic(positionECharactristicUUID);
  requestShotCharactristic = pRemoteService->getCharacteristic(requestShotCharactristicUUID);
  temperatureCharactristic = pRemoteService->getCharacteristic(temperatureCharactristicUUID);
  printingStatusCharactristic = pRemoteService->getCharacteristic(printingStatusCharactristicUUID);

  if (positionXCharactristic == nullptr)
  {
    Serial.print("Failed to find positionXCharactristic UUID");
    return false;
  }
  if (positionYCharactristic == nullptr)
  {
    Serial.print("Failed to find positionYCharactristic UUID");
    return false;
  }
  if (positionZCharactristic == nullptr)
  {
    Serial.print("Failed to find positionZCharactristic UUID");
    return false;
  }
  if (positionECharactristic == nullptr)
  {
    Serial.print("Failed to find positionECharactristic UUID");
    return false;
  }
  if (temperatureCharactristic == nullptr)
  {
    Serial.print("Failed to find temperatureCharactristic UUID");
    return false;
  }
  if (printingStatusCharactristic == nullptr)
  {
    Serial.print("Failed to find printingStatusCharactristic UUID");
    return false;
  }
  if (positionXCharactristic == nullptr || positionYCharactristic == nullptr || positionZCharactristic == nullptr || positionECharactristic == nullptr || temperatureCharactristic == nullptr || printingStatusCharactristic == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID");
    return false;
  }
  Serial.println(" - Found our characteristics");

  // Assign callback functions for the Characteristics
  positionZCharactristic->registerForNotify(positionZNotifyCallback);
  printingStatusCharactristic->registerForNotify(printingStatusNotifyCallback);
  requestShotCharactristic->registerForNotify(requestShotNotifyCallback);
  return true;
}
// Callback function that gets called, when another device's advertisement has been received
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    Serial.print("On Result: ");
    if (String(advertisedDevice.getName().c_str()) == bleServerName)
    {
      Serial.print("[");
      Serial.print(advertisedDevice.getName().c_str());
      Serial.print("] == [");
      Serial.print(bleServerName);
      Serial.print("]");                                              // Check if the name of the advertiser matches
      advertisedDevice.getScan()->stop();                             // Scan can be stopped, we found what we are looking for
      pServerAddress = new BLEAddress(advertisedDevice.getAddress()); // Address of advertiser is the one we need
      doConnect = true;                                               // Set indicator, stating that we are ready to connect
      Serial.println("Device found. Connecting!");
    }
    else
    {
      Serial.print("[");
      Serial.print(advertisedDevice.getName().c_str());
      Serial.print("] != [");
      Serial.print(bleServerName);
      Serial.println("]");
    }
  }
};
void BeginScan()
{
  Serial.println("Begin Scan");
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for one hour
  BLEDevice::init("");
  auto pBLEScan = BLEDevice::getScan(); // create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // less or equal setInterval value
  pBLEScan->start(3600);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");

  // Init BLE device
  BLEDevice::init("");
  BeginScan();
}

void loop()
{
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true)
  {
    if (connectToServer(*pServerAddress))
    {
      Serial.println("We are now connected to the BLE Server.");
      // Activate the Notify property of each Characteristic
      positionZCharactristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t *)notificationOn, 2, true);
      printingStatusCharactristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t *)notificationOn, 2, true);
      bleConnected = true;
      doConnect = false;
    }
    else
    {
      Serial.println("We have failed to connect to the server; Restart your device to scan for nearby BLE server again.");
      bleConnected = false;
      doConnect = true;
    }
  }
}

// #include <DNSServer.h>
// #include <WiFi.h>
// // #include <AsyncTCP.h>
// #include "Camera.h"
// #include "FS.h"     // SD Card ESP32
// #include "SD_MMC.h" // SD Card ESP32
// #include <EEPROM.h> // read and write from flash memory
// #include "CamWebServer.h"
// #include <ESPAsyncWebServer.h>
// #include <ESP-FTP-Server-Lib.h>
// #include <FTPFilesystem.h>
// #include <ArduinoJson.h>
// #include <Preferences.h>

// #define FTP_USER "ftp"
// #define FTP_PASSWORD "ftp"

// #define systemRoot "/PrintVue"
// #define wwwRoot systemRoot "/www"
// #define galleryRoot wwwRoot "/gallery"
// #define settingsRoot systemRoot "/settings"

// FTPServer ftp;
// AsyncWebServer server = AsyncWebServer(80);
// int lastSeqIndex = 0;
// int lastShotIndex = 0;
// float lastZ = 0;

// void Error(int period, int onFor = 50)
// {

//   const int freq = 5000;
//   const int ledChannel = LEDC_CHANNEL_6;
//   ledcSetup(ledChannel, freq, 8); // vacate pin 4
//   ledcAttachPin(4, ledChannel);   // vacate pin 4
//   ledcWrite(ledChannel, 0);       // vacate pin 4
//   while (true)
//   {
//     ledcWrite(ledChannel, 255);
//     delay(onFor);
//     ledcWrite(ledChannel, 0);
//     delay(period);
//   }
// }

// String ReadSetting(String key, String defaultValue)
// {
//   String fName = String(systemRoot "/") + key;
//   if (!SD_MMC.exists(fName))
//   {
//     Serial.print("Couldn't find file: ");
//     Serial.print(fName);
//     Serial.print(", returning default");
//     Serial.println(defaultValue);
//     return defaultValue;
//   }

//   File f = SD_MMC.open(fName, FILE_WRITE);
//   if (!f)
//   {
//     Serial.print("Couldn't open setting file: ");
//     Serial.println(fName);
//     return;
//   }
//   String data = f.readString();
//   f.close();
//   return data;
// }
// void SaveSetting(String key, String value)
// {
//   String fName = String(systemRoot "/") + key;
//   if (SD_MMC.exists(fName))
//     SD_MMC.remove(fName);

//   File f = SD_MMC.open(fName, FILE_WRITE);
//   if (!f)
//   {
//     Serial.print("Couldn't open setting file: ");
//     Serial.println(fName);
//     return;
//   }
//   f.print(value);
//   f.close();
// }

// void resumeSession()
// {
//   lastZ = ReadSetting("last z", "0").toFloat();
//   lastSeqIndex = ReadSetting("last seq", "0").toInt();
//   lastShotIndex = ReadSetting("last shot", "0").toInt();
// }
// void saveSession()
// {
//   SaveSetting("last z", String(lastZ));
//   SaveSetting("last seq", String(lastSeqIndex));
//   SaveSetting("last shot", String(lastShotIndex));
// }
// int remainingCamBytes = 0;
// int sentCamBytes = 0;
// bool camImageIsFresh = false;
// #define tempCamImageName "/PrintVue/temp-cam.jpg"
// camera_fb_t *fb = NULL;
// uint8_t *_jpg_buf = NULL;
// char *part_buf[64];

// void setup()
// {
//   Serial.begin(115200);
//   delay(100);

//   Serial.println("Starting SD Card");
//   if (!SD_MMC.begin("/sdcard", true))
//   {
//     Serial.println("SD Card Mount Failed");
//     Error(1000);
//   }
//   else
//   {
//     uint8_t cardType = SD_MMC.cardType();
//     if (cardType == CARD_NONE)
//     {
//       Serial.println("No SD Card attached");
//       Error(1500);
//     }
//     else
//     {
//       Serial.println("SD Card present");
//     }
//   }

//   if (!CameraSetup())
//     Error(500);
//   Serial.println("Camera present");
//   // start FTP

//   WiFi.softAP("M3D PrintVue", "12345678");
//   // dnsServer.start(53, "markhor.local", WiFi.softAPIP());
//   // server.serveStatic("/", SD_MMC, "www");
//   // server.on("/live-cam.jpg", HTTP_POST, liveCamHandler);

//   ftp.addUser(FTP_USER, FTP_PASSWORD);
//   ftp.addFilesystem("SD", &SD_MMC);
//   ftp.begin();

//   server.serveStatic("/live-cam.jpg", SD_MMC, tempCamImageName);
//   server.on("/refresh-cam", HTTP_POST, [](AsyncWebServerRequest *req)
//             {
//     camImageIsFresh = false;
//     while (!camImageIsFresh) //
//     {
//       delay(10);
//     }

//     req->send(200); });
//   // server.on("/cam-image.jpg", HTTP_GET, camImageHandler);

//   server.on("/gallery/sequences.json", HTTP_GET, [](AsyncWebServerRequest *request)
//             {
//     Serial.println("Get sequences");
//     DynamicJsonDocument doc(1024);  // Allocate memory for JSON document
//     JsonArray list = doc.createNestedArray("list");

//     String rootDir = String(galleryRoot);
//     File root = SD_MMC.open(rootDir);  // Open root directory
//     if (!root) {
//       Serial.println("Failed to open /www directory");
//       request->send(500, "text/plain", "Error accessing directory");
//       return;
//     }

//     File file;
//     while (file = root.openNextFile()) {
//       if (file.isDirectory()) {
//         String dirName = file.name();
//         if (dirName.startsWith("s") && dirName.length() > 1 && isDigit(dirName.charAt(1))) {
//           // Check for seq_info.json inside the directory
//           String fName = rootDir + "/" + String(file.name()) + "/seq_info.json";
//           file = SD_MMC.open(fName);
//           if (file) {
//             Serial.println("Adding: " + dirName);
//             file.close();  // Close seq_info.json file
//             list.add(dirName);  // Add directory name to JSON list
//           }
//         }
//       }
//       file.close();
//     }

//     root.close();

//     String response;
//     serializeJson(doc, response);
//     request->send(200, "application/json", response); });

//   server.serveStatic("/", SD_MMC, "/PrintVue/www").setDefaultFile("index.html");
//   ;
//   server.onNotFound([](AsyncWebServerRequest *request)
//                     { request->send(404); });

//   Serial.println("Starting prefs");

//   // restore session
//   resumeSession();

//   server.begin();
// }
// void handleZChange(float z)
// {
//   if (z == lastZ)
//   {
//     Serial.print("No change in z: ");
//     Serial.println(z);
//     return;
//   }
//   Serial.print("Z = ");
//   Serial.print(z);
//   Serial.print(", lastZ = ");
//   Serial.println(lastZ);
//   if (z > lastZ)
//   {
//     if (z - lastZ <= 0.5)
//     {
//       Serial.println("Layer increment");
//       lastZ = z;
//       String dir = String(galleryRoot) + "/s" + String(lastSeqIndex);
//       String fName = dir + "/" + String(lastSeqIndex) + ".jpg";
//       if (!SD_MMC.exists(dir))
//         SD_MMC.mkdir(dir);
//       SavePhoto(fName);
//       lastShotIndex++;
//       saveSession();
//     }
//     else
//     {
//       Serial.println("Probably a manual move, beginning or of print");
//     }
//   }
//   else
//   {
//     if (z <= 0.5)
//     { // first layer
//       Serial.println("First layer, could be a manual move");
//       bool savedShot = false;
//       for (int i = 10000; i > 0 && !savedShot; i--)
//       {
//         if (sequenceExists(i))
//         {
//           DynamicJsonDocument doc(1024);
//           if (getSeqInfo(i, doc))
//           {
//             Serial.print("Got start index: ");
//             Serial.println(doc["startFrameIndex"].as<int>());
//             Serial.print("Got end index: ");
//             Serial.println(doc["endFrameIndex"].as<int>());
//             if (doc["endFrameIndex"].as<int>() == doc["startFrameIndex"].as<int>())
//             {
//               Serial.println("Only one file in the sequence. Lets over write.");
//               String shotName = MakeShotName(i, doc["startFrameIndex"].as<int>());
//               Serial.print("File to over write: ");
//               Serial.println(shotName);
//               SavePhoto(shotName);
//               savedShot = true;
//             }
//           }
//           else
//           {
//             Serial.print("No sequence info: ");
//             Serial.println(i);
//           }
//         }
//       }
//       if (!savedShot)
//       { // no sequences found or not empty sequences empty
//         for (int i = 1; i <= 10000; i++)
//         {
//           if (!sequenceExists(i))
//           {
//             // Create this sequence
//             DynamicJsonDocument doc(1024);
//             Serial.print("Got start index: ");
//             Serial.println(doc["startFrameIndex"].as<int>());
//             Serial.print("Got end index: ");
//             Serial.println(doc["endFrameIndex"].as<int>());
//             if (doc["endFrameIndex"].as<int>() == doc["startFrameIndex"].as<int>())
//             {
//               Serial.println("Only one file in the sequence. Lets over write.");
//               String shotName = MakeShotName(i, doc["startFrameIndex"].as<int>());
//               Serial.print("File to over write: ");
//               Serial.println(shotName);
//               SavePhoto(shotName);
//               savedShot = true;
//             }
//           }
//         }
//       }
//     }
//   }

//   lastZ = z;
// }
// void loop()
// {
//   ftp.handle();
//   if (!camImageIsFresh)
//   {
//     Serial.println("Cam Image not fresh");
//     if (!SavePhoto(tempCamImageName))
//     {
//       Serial.println("Save photo failed");
//       return;
//     }
//     camImageIsFresh = true;
//     Serial.println("Image taken");
//   }
//   if (Serial.available())
//   {
//     String com = Serial.readStringUntil('\n');
//     com.trim();
//     float z = com.toFloat();
//   }
// }