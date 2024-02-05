#include <DNSServer.h>
#include <WiFi.h>
//#include <AsyncTCP.h>
#include "Camera.h"
#include "FS.h"     // SD Card ESP32
#include "SD_MMC.h" // SD Card ESP32
#include <EEPROM.h> // read and write from flash memory
#include "CamWebServer.h"
#include <ESPAsyncWebServer.h>
#include <ESP-FTP-Server-Lib.h>
#include <FTPFilesystem.h>

#define FTP_USER     "ftp"
#define FTP_PASSWORD "ftp"

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

  server.serveStatic("/", SD_MMC, "/PrintVue/www");
  server.begin();
  
}
void loop()
{
  ftp.handle();
}