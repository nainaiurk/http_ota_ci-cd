#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

const char* ssid = "SUST WiFi";
const char* password = "SUST10s10";
const char* firmwareURL = "http://10.101.13.227:5000/firmware.bin";

void performOTA() {
  WiFiClient client;
  HTTPClient http;

  Serial.println("Checking for new firmware...");

  http.begin(client, firmwareURL);
  int httpCode = http.GET();

  if (httpCode == 200) { // OK
    int contentLength = http.getSize();
    bool canBegin = Update.begin(contentLength);

    if (canBegin) {
      Serial.println("Starting OTA update...");
      WiFiClient* stream = http.getStreamPtr();
      size_t written = 0;
      uint8_t buf[512];

      while (http.connected() && written < contentLength) {
        size_t available = stream->available();
        if (available) {
          size_t toRead = available > sizeof(buf) ? sizeof(buf) : available;
          int c = stream->readBytes(buf, toRead);
          Update.write(buf, c);
          written += c;
          Serial.printf("Progress: %d%%\r", (written * 100) / contentLength);
        }
        delay(1);
      }

      if (Update.end()) {
        if (Update.isFinished()) {
          Serial.println("\nOTA update finished! Rebooting...");
          ESP.restart();
        } else {
          Serial.println("\nOTA update not finished? Something went wrong!");
        }
      } else {
        Serial.printf("\nUpdate failed. Error #: %d\n", Update.getError());
      }
    } else {
      Serial.println("Not enough space to begin OTA");
    }
  } else {
    Serial.printf("HTTP GET failed, code: %d\n", httpCode);
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
}

void loop() {
  performOTA();
  delay(30000); // check every 30 sec
}