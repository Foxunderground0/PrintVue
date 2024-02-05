#include <DNSServer.h>
#include <WiFi.h>
// #include <AsyncTCP.h>
#include "Camera.h"
#include "FS.h"     // SD Card ESP32
#include "SD_MMC.h" // SD Card ESP32
#include <EEPROM.h> // read and write from flash memory
#include "CamWebServer.h"
#include <ESPAsyncWebServer.h>
#include <ESP-FTP-Server-Lib.h>
#include <FTPFilesystem.h>
#include <ArduinoJson.h>

#define FTP_USER "ftp"
#define FTP_PASSWORD "ftp"

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

FTPServer ftp;
AsyncWebServer server = AsyncWebServer(80);

void Error(int period, int onFor = 50)
{

  const int freq = 5000;
  const int ledChannel = LEDC_CHANNEL_6;
  ledcSetup(ledChannel, freq, 8); // vacate pin 4
  ledcAttachPin(4, ledChannel);   // vacate pin 4
  ledcWrite(ledChannel, 0);       // vacate pin 4
  while (true)
  {
    ledcWrite(ledChannel, 255);
    delay(onFor);
    ledcWrite(ledChannel, 0);
    delay(period);
  }
}

int remainingCamBytes = 0;
int sentCamBytes = 0;
bool camImageIsFresh = false;
#define tempCamImageName "/PrintVue/temp-cam.jpg"
camera_fb_t *fb = NULL;
uint8_t *_jpg_buf = NULL;
char *part_buf[64];
int sendCamChunks(uint8_t *buffer, size_t maxLen, size_t index)
{
  Serial.printf("Send chunk, max: %i, sent: %i\r\n", maxLen, index);
  int res = ESP_OK;
  if (remainingCamBytes <= 0)
  {
    Serial.println("Create more bytes");
    // create more data
    fb = esp_camera_fb_get();
    if (!fb)
    {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    }
    else
    {
      if (fb->width > 400)
      {
        Serial.printf("fb->width > 400 => %i\r\n", fb->width);
        if (fb->format != PIXFORMAT_JPEG)
        {
          Serial.printf("Not JPEG: %i\r\n", fb->format);
          size_t _jpg_buf_len = 0;
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          remainingCamBytes = _jpg_buf_len;
          sentCamBytes = 0;
          esp_camera_fb_return(fb);
          fb = NULL;
          if (!jpeg_converted)
          {
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        }
        else
        {
          Serial.println("JPEG");
          remainingCamBytes = fb->len;
          sentCamBytes = 0;
          _jpg_buf = fb->buf;
        }
      }
    }
    if (res != ESP_OK)
    {
      Serial.println("End chunked response [1]");
      return 0;
    }
    if (remainingCamBytes == 0)
    {
      Serial.println("End chunked response [2]]");
      return 0;
    }
  }
  //

  if (maxLen > remainingCamBytes)
    maxLen = remainingCamBytes;

  Serial.printf("Sending chunk. remainingCamBytes: %i, sentCamBytes %i\r\n", remainingCamBytes, sentCamBytes);
  for (int i = 0; i < maxLen; i++)
    buffer[i] = _jpg_buf[sentCamBytes + i];

  sentCamBytes += maxLen;
  remainingCamBytes -= maxLen;
  return maxLen;
}
void liveCamHandler(AsyncWebServerRequest *request)
{
  Serial.println("Live cam request");
  remainingCamBytes = 0;

  String contentType = "multipart/x-mixed-replace;boundary=123456789000000000000987654321";
  int remaining = 0;
  auto resp = request->beginChunkedResponse(contentType, sendCamChunks);
  request->send(resp);
}
void camImageHandler(AsyncWebServerRequest *request)
{
  File f = SD_MMC.open(tempCamImageName);
  if (!f)
  {
    Serial.println("file open failed");
    request->send(500);
    return;
  }
  Serial.printf("Respond with file: %i\r\n", f.size());
  auto resp = request->beginResponse(f, "image/jpg", f.size());
  request->send(resp);
  camImageIsFresh = false;
}

void setup()
{
  Serial.begin(115200);
  delay(100);

  Serial.println("Starting SD Card");
  if (!SD_MMC.begin("/sdcard", true))
  {
    Serial.println("SD Card Mount Failed");
    Error(1000);
  }
  else
  {
    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE)
    {
      Serial.println("No SD Card attached");
      Error(1500);
    }
    else
    {
      Serial.println("SD Card present");
    }
  }

  if (!CameraSetup())
    Error(500);
  Serial.println("Camera present");
  // start FTP

  WiFi.softAP("M3D PrintVue", "12345678");
  // dnsServer.start(53, "markhor.local", WiFi.softAPIP());
  // server.serveStatic("/", SD_MMC, "www");
  // server.on("/live-cam.jpg", HTTP_POST, liveCamHandler);

  ftp.addUser(FTP_USER, FTP_PASSWORD);
  ftp.addFilesystem("SD", &SD_MMC);
  ftp.begin();

  server.serveStatic("/live-cam.jpg", SD_MMC, tempCamImageName);
  server.on("/refresh-cam", HTTP_POST, [](AsyncWebServerRequest *req)
            {
    camImageIsFresh = false; 
    while (!camImageIsFresh) //
    {
      delay(10);
    }
    
    req->send(200); });
  // server.on("/cam-image.jpg", HTTP_GET, camImageHandler);

  server.on("/gallery/sequences.json", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Get sequences");
    DynamicJsonDocument doc(1024);  // Allocate memory for JSON document
    JsonArray list = doc.createNestedArray("list");

    String rootDir = String("/PrintVue/www/gallery");
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
  ;
  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->send(404); });
  server.begin();
}
void loop()
{
  ftp.handle();
  if (!camImageIsFresh)
  {
    Serial.println("Cam Image not fresh");
    if (!SavePhoto(tempCamImageName))
    {
      Serial.println("Save photo failed");
      return;
    }
    camImageIsFresh = true;
    Serial.println("Image taken");
  }
}