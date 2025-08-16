#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

const char* GITHUB_OWNER = " "; // <--- Update this with your GitHub username
const char* GITHUB_REPO = " "; // <--- Update this with your repo name
const char* GITHUB_TOKEN = " "; // ⚠️ Never expose in public repos

const char* ssid = " ";  //
const char* password = " ";
String currentVersion = "v3.0";

String firmwareDownloadURL;

String getLatestReleaseTag() {
  WiFiClientSecure client;
  client.setInsecure(); // skip SSL cert validation
  HTTPClient http;

  String apiUrl = "https://api.github.com/repos/" + String(GITHUB_OWNER) + "/" + String(GITHUB_REPO) + "/releases/latest";

  http.begin(client, apiUrl);
  http.addHeader("User-Agent", "ESP32");   // GitHub requires UA
  http.addHeader("Authorization", "token " + String(GITHUB_TOKEN));

  int httpCode = http.GET();
  String tagName = "";

  if (httpCode == 200) {
    String payload = http.getString();
    // Serial.println("GitHub API Response:");
    // Serial.println(payload);

    // Parse JSON
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      tagName = doc["tag_name"].as<String>();
      firmwareDownloadURL = doc["assets"][0]["browser_download_url"].as<String>(); 
    } else {
      Serial.println("Failed to parse JSON!");
    }
  } else {
    Serial.printf("Failed to fetch release, HTTP code: %d\n", httpCode);
  }

  http.end();
  return tagName;
}

void performOTA(String firmwareURL) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  Serial.printf("Downloading firmware from: %s\n", firmwareURL.c_str());

  http.begin(client, firmwareURL);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);   // follow 302 redirects
  int httpCode = http.GET();

  if (httpCode == 200) {
    int contentLength = http.getSize();
    if (contentLength <= 0) {
      Serial.println("Invalid content length");
      return;
    }

    if (!Update.begin(contentLength)) {
      Serial.println("Not enough space for OTA");
      return;
    }

    WiFiClient *stream = http.getStreamPtr();
    uint8_t buff[512];
    int written = 0;

    Serial.println("Starting OTA update...");

    while (http.connected() && written < contentLength) {
      size_t available = stream->available();
      if (available) {
        int bytesRead = stream->readBytes(buff, ((available > sizeof(buff)) ? sizeof(buff) : available));
        written += Update.write(buff, bytesRead);

        // Progress
        int progress = (written * 100) / contentLength;
        Serial.printf("Progress: %d%%\r", progress);
      }
      delay(1); // yield to WiFi stack
    }
    Serial.println();

    if (Update.end()) {
      if (Update.isFinished()) {
        Serial.println("OTA update successful! Rebooting...");
        ESP.restart();
      } else {
        Serial.println("OTA update not finished. Something went wrong!");
      }
    } else {
      Serial.printf("Update failed. Error #: %d\n", Update.getError());
    }

  } else {
    Serial.printf("Firmware download failed, code: %d\n", httpCode);
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
}

void loop() {
  String latestTag = getLatestReleaseTag();

  if (latestTag.length() > 0 && latestTag != currentVersion) {
    Serial.printf("New version available: %s (current: %s)\n", latestTag.c_str(), currentVersion.c_str());
    performOTA(firmwareDownloadURL);
  } else {
    Serial.println("No update available.");
  }
  Serial.print("Current version: ");
  Serial.println(currentVersion);
  delay(60000); // check every 60s
}
