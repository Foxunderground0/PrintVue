#include <ArduinoBLE.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEScan.h>

// Device name and service UUID
const char* deviceName = "YOUR_DEVICE_NAME";
const char* serviceUUID = "8e088cd2-8100-11ee-b9d1-0242ac120002";

// Characteristic UUIDs
const char* positionXCharUUID = "8e088cd2-7101-11ee-b9d1-0242ac120002";
const char* positionYCharUUID = "8e088cd2-7102-11ee-b9d1-0242ac120002";
const char* positionZCharUUID = "8e088cd2-7103-11ee-b9d1-0242ac120002";
const char* positionECharUUID = "8e088cd2-7104-11ee-b9d1-0242ac120002";
const char* temperatureCharUUID = "8e088cd2-7105-11ee-b9d1-0242ac120002";
const char* printingStatusCharUUID = "8e088cd2-7106-11ee-b9d1-0242ac120002";

// Function prototypes
void setup();
void loop();
void onResult(BLEAdvertisedDevice device);
void connectToDevice(BLEClient* pClient, BLEAddress address);
void onCharacteristicChanged(BLERemoteCharacteristic* characteristic);

// Global variables to store characteristic pointers
BLEFloatCharacteristic* positionXChar = nullptr;
BLEFloatCharacteristic* positionYChar = nullptr;
BLEFloatCharacteristic* positionZChar = nullptr;
BLEFloatCharacteristic* positionEChar = nullptr;
BLEFloatCharacteristic* temperatureChar = nullptr;
BLEIntCharacteristic* printingStatusChar = nullptr;


void setup() {
  Serial.begin(115200);

  // Start BLE
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();

  // Set scan filters based on device name (if applicable)
  if (deviceName) {
    pBLEScan->setFilterName(deviceName);
    pBLEScan->setActiveScan(true); // Active scan for faster results
  }

  // Start scan and handle callbacks
  BLECallback callback = new MyScanCallback();
  pBLEScan->onResults(callback);
  pBLEScan->start(5); // Scan for 5 seconds
}

void loop() {
  // Process incoming data and display values
  if (positionXChar && positionYChar && positionZChar && positionEChar && temperatureChar && printingStatusChar) {
    float positionX = positionXChar->readValue();
    float positionY = positionYChar->readValue();
    float positionZ = positionZChar->readValue();
    float positionE = positionEChar->readValue();
    float temperature = temperatureChar->readValue();
    int printingStatus = printingStatusChar->readValue();

    Serial.print("X: ");
    Serial.print(positionX);
    Serial.print(", Y: ");
    Serial.print(positionY);
    Serial.print(", Z: ");
    Serial.print(positionZ);
    Serial.print(", E: ");
    Serial.print(positionE);
    Serial.print(", Temperature: ");
    Serial.print(temperature);
    Serial.print(", Printing Status: ");
    Serial.println(printingStatus);
  } else {
    Serial.println("Waiting for characteristics...");
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