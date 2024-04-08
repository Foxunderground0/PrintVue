//#include <DNSServer.h>
#include <WiFi.h>
// #include <AsyncTCP.h>
#include "Camera.h"
#include "FS.h"     // SD Card ESP32
#include "SD_MMC.h" // SD Card ESP32
#include <EEPROM.h> // read and write from flash memory
#include <ESPAsyncWebServer.h>
#include <ESP-FTP-Server-Lib.h>
#include <FTPFilesystem.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <AsyncWebSocket.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
//#include <esp_http_client.h>

#define FTP_USER "ftp"
#define FTP_PASSWORD "ftp"

#define systemRoot "/PrintVue"
#define settingsRoot "/PrintVue/Settings"
#define wwwRoot systemRoot "/www"
#define galleryRoot wwwRoot "/gallery"
#define serverUrl "http://192.168.1.101:3000/"

FTPServer ftp;
AsyncWebServer server = AsyncWebServer(80);
int currentSeqIndex = 0;
int nextShotIndex = 0;
bool WIFIConnected = false;
bool uploadSequenceShotToServerForLayerHealthRequest = false;

struct imageInfo {
  uint32_t fileSize;
};

int remainingCamBytes = 0;
int sentCamBytes = 0;
bool camImageIsFresh = false;
bool takeSequenceShotRequest = false;
#define tempCamImageName "/PrintVue/temp-cam.jpg"
camera_fb_t* fb = NULL;
uint8_t* _jpg_buf = NULL;
char* part_buf[64];
void commLoop();
void commSetup();

void LEDBlinkLoop(long period)
{
  int v = 255 - abs((((long)millis() % period) * 510) / period - 255); // 0 > 255 > 0
  analogWrite(33, v);
}
void Error(int period)
{
  while (1) {
    LEDBlinkLoop(period);
    delay(1);
  }
}

String ReadSetting(String key, String defaultValue)
{
  String fName = String(settingsRoot "/") + key;
  if (!SD_MMC.exists(fName))
  {
    Serial.print("Couldn't find file: ");
    Serial.print(fName);
    Serial.print(", returning default");
    Serial.println(defaultValue);
    return defaultValue;
  }

  File f = SD_MMC.open(fName, FILE_WRITE);
  if (!f)
  {
    Serial.print("Couldn't open setting file: ");
    Serial.println(fName);
    return defaultValue;
  }
  String data = f.readString();
  f.close();
  return data;
}
void SaveSetting(String key, String value)
{
  String fName = String(settingsRoot "/") + key;
  if (SD_MMC.exists(fName))
    SD_MMC.remove(fName);

  File f = SD_MMC.open(fName, FILE_WRITE);
  if (!f)
  {
    Serial.print("Couldn't open setting file: ");
    Serial.println(fName);
    return;
  }
  f.print(value);
  f.close();
}

void resumeSession()
{
  Serial.println("Resume Session");
  currentSeqIndex = ReadSetting("current seq", "1").toInt();
  nextShotIndex = ReadSetting("next shot", "1").toInt();
}
void saveSession()
{
  Serial.println("Save Session");
  SaveSetting("current seq", String(currentSeqIndex));
  SaveSetting("next shot", String(nextShotIndex));
}

void setup()
{
  Serial.begin(115200);
  delay(100);

  commSetup();

  Serial.println("Starting SD Card");
  if (!SD_MMC.begin("/sdcard", true))
  {
    Serial.println("SD Card Mount Failed");
    Error(500);
  } else
  {
    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE)
    {
      Serial.println("No SD Card attached");
      Error(1000);
    } else
    {
      Serial.println("SD Card present");
      if (!SD_MMC.exists(systemRoot)) {
        Serial.println("systen root doesn't exist.");
        SD_MMC.mkdir(systemRoot);
      }
      if (!SD_MMC.exists(wwwRoot)) {
        SD_MMC.mkdir(wwwRoot);
        Serial.println("www doesn't exist.");
      }
      if (!SD_MMC.exists(galleryRoot)) {
        Serial.println("gallery doesn't exist.");
        SD_MMC.mkdir(galleryRoot);
      }
      if (!SD_MMC.exists(settingsRoot)) {
        Serial.println("settings doesn't exist.");
        SD_MMC.mkdir(settingsRoot);
      }
    }
  }

  if (!CameraSetup())
    Error(1500);
  Serial.println("Camera present");

  //Start the ESP in AP and STA mode
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin("Storm PTCL", "35348E80687?!");
  WiFi.softAP("M3D PrintVue", "12345678");

  // Timeout if not connected to wifi in 10 seconds
  long WIFIConnectTimeout = millis() + 10000;

  Serial.print("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED && millis() < WIFIConnectTimeout)
  {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("");

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to WiFi, Continuing in AP mode");
  } else {
    Serial.print("Connected to WiFi: ");
    Serial.println(WiFi.localIP());
    WIFIConnected = true;
  }

  // dnsServer.start(53, "markhor.local", WiFi.softAPIP());
  server.serveStatic("/", SD_MMC, "www");
  // server.on("/live-cam.jpg", HTTP_POST, liveCamHandler);

  // start FTP
  ftp.addUser(FTP_USER, FTP_PASSWORD);
  ftp.addFilesystem("SD", &SD_MMC);
  ftp.begin();

  server.serveStatic("/live-cam.jpg", SD_MMC, tempCamImageName);
  server.on("/refresh-cam", HTTP_POST, [](AsyncWebServerRequest* req)
    {
      camImageIsFresh = false;
      while (!camImageIsFresh) //
      {
        delay(10);
      }

      req->send(200); });
  // server.on("/cam-image.jpg", HTTP_GET, camImageHandler);

  server.on("/gallery/sequences.json", HTTP_GET, [](AsyncWebServerRequest* request)
    {
      Serial.println("Get sequences");
      DynamicJsonDocument doc(1024);  // Allocate memory for JSON document
      JsonArray list = doc.createNestedArray("list");

      String rootDir = String(galleryRoot);
      File root = SD_MMC.open(rootDir);  // Open root directory
      if (!root) {
        Serial.println("Failed to open /www directory");
        request->send(500, "text/plain", "Error accessing directory");
        return;
      }

      File file;
      while (file = root.openNextFile()) {
        if (file.isDirectory()) {
          String dirName = file.name();
          if (dirName.startsWith("s") && dirName.length() > 1 && isDigit(dirName.charAt(1))) {
            // Check for seq_info.json inside the directory
            String fName = rootDir + "/" + String(file.name()) + "/seq_info.json";
            file = SD_MMC.open(fName);
            if (file) {
              Serial.println("Adding: " + dirName);
              file.close();  // Close seq_info.json file
              list.add(dirName);  // Add directory name to JSON list
            }
          }
        }
        file.close();
      }

      root.close();

      String response;
      serializeJson(doc, response);
      request->send(200, "application/json", response); });

  server.serveStatic("/", SD_MMC, "/PrintVue/www").setDefaultFile("index.html");
  server.onNotFound([](AsyncWebServerRequest* request)
    { request->send(404); });

  // restore session
  resumeSession();

  server.begin();
}

void updateCurrentSequenceInfo() {
  Serial.print("Update current sequence info: ");
  String seqInfoFile = String(galleryRoot) + "/s" + String(currentSeqIndex) + "/seq_info.json";
  Serial.println(seqInfoFile);

  File f = SD_MMC.open(seqInfoFile, FILE_WRITE);
  if (!f) {
    Serial.println("Could not save sequence file");
    return;
  }
  DynamicJsonDocument json(1024);
  json["startFrameIndex"] = 1;
  json["endFrameIndex"] = nextShotIndex - 1;
  json["seed"] = "";
  json["extension"] = ".jpg";
  json["sequenceTitle"] = String("Sequence ") + String(currentSeqIndex);
  String serialJson;
  serializeJson(json, serialJson);
  Serial.print("Info: ");
  Serial.println(serialJson);
  f.print(serialJson);
  f.close();
}

void createNewSequence()
{
  Serial.println("createNewSequence()");
  nextShotIndex = 0;
  saveSession();
  // tolerate if a sqeuence or shot already exists

  String seqName;
  // Find the current available sequence dir index
  for (int i = currentSeqIndex; i < 10000; i++)
  {
    seqName = String(galleryRoot) + "/s" + String(i);
    if (SD_MMC.exists(seqName))
    {
      continue;
    } else
    {
      currentSeqIndex = i;
      Serial.print("Create new sequence: ");
      Serial.println(seqName);
      SD_MMC.mkdir(seqName);
      saveSession();
      break;
    }
  }

  // Find the current available shot file index
  for (int i = 1; i < 10000; i++)
  {
    String shotName = seqName + String(i) + ".jpg";
    if (SD_MMC.exists(shotName))
    {
      continue;
    } else
    {
      nextShotIndex = i;
      saveSession();
      break;
    }
  }
  updateCurrentSequenceInfo();
}
void endSequence()
{
  Serial.println("endSequence()");
  currentSeqIndex++;
  nextShotIndex = 1;
  saveSession();
}
void appendSequence()
{
  Serial.println("appendSequence()");
  takeSequenceShotRequest = true;
}

// Temporary function to test file upload
void setUploadSequenceShotToServer()
{
  Serial.println("[Call to upload sequence shot to server for layer health]");
  uploadSequenceShotToServerForLayerHealthRequest = true;
}

void uploadSequenceShotToServerForLayerHealth(String shotName) {
  Serial.print("[Upload sequence shot to server for layer health] Image at: ");
  Serial.println(shotName);

  // Open the image file
  File imageFile = SD_MMC.open(shotName, FILE_READ);
  if (!imageFile) {
    Serial.println("Failed to open image file");
    return;
  }

  WiFiClient client;
  HTTPClient http;

  // Start a POST request
  http.begin(client, serverUrl);
  http.addHeader("Content-Type", "image/jpeg");

  // Send the image data in chunks
  const size_t chunkSize = 1024;
  uint8_t buffer[chunkSize];

  while (http.connected() && imageFile.available()) {
    size_t bytesRead = imageFile.readBytes(reinterpret_cast<char*>(buffer), chunkSize);
    if (bytesRead > 0) {
      client.write(buffer, bytesRead);
    }
  }

  // Finalize the request
  http.end();


  // Close the image file
  imageFile.close();
}




void loop()
{
  ftp.handle();
  commLoop();
  if (!camImageIsFresh)
  {
    Serial.println("Cam Image not fresh");
    if (!SavePhoto(tempCamImageName))
    {
      Serial.println("Save photo failed");
      camImageIsFresh = true; // stop forever loop
      return;
    }
    camImageIsFresh = true;
    Serial.println("Image taken");
  }
  if (takeSequenceShotRequest)
  {
    Serial.println("Sequence shot request");
    String shotName = String(galleryRoot) + "/s" + String(currentSeqIndex) + "/" + String(nextShotIndex) + ".jpg";
    if (!SavePhoto(shotName))
    {
      Serial.print("Save photo failed: ");
      Serial.println(shotName);
      takeSequenceShotRequest = false;
      return;
    }
    nextShotIndex++;
    takeSequenceShotRequest = false;
    Serial.print("Image taken: ");
    Serial.println(shotName);
    updateCurrentSequenceInfo();
    uploadSequenceShotToServerForLayerHealth(shotName);
  }
}