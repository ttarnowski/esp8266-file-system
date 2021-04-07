#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFiMulti.h>
#include <EventDispatcher.hpp>
#include <LittleFS.h>
#include <Timer.hpp>
#include <WiFiManager.hpp>

#define SSID "VM3549886"
#define PASSWORD "mc7RsdnxV4qp"

EventDispatcher dispatcher;
Timer timer;
ESP8266WiFiMulti wifiMulti;
WiFiManager wifiManager(&wifiMulti, &dispatcher, &timer, SSID, PASSWORD);

const char *filename = "/file-22.txt";

void readFile() {
  LittleFS.begin();

  File file = LittleFS.open(filename, "r");

  Serial.println("File contents: ");

  while (file.available()) {
    char buf[64];

    size_t bytesRead = file.readBytes(buf, sizeof(buf) - 1);
    buf[bytesRead] = '\0';

    Serial.print(buf);
  }

  file.close();

  LittleFS.end();
}

void onConnectFinished(wl_status_t status) {
  if (status != WL_CONNECTED) {
    Serial.println("could not connect to WiFi");
    return;
  }

  WiFiClient wifiClient;
  HTTPClient httpClient;

  if (!httpClient.begin(
          wifiClient, "http://100daysofcode.s3-website-eu-west-1.amazonaws.com/"
                      "rfc2616.txt")) {
    Serial.println("could not connect to the internet");
    return;
  }

  int statusCode = httpClient.GET();

  if (statusCode < 0) {
    Serial.printf("GET failed, error: %s\n",
                  httpClient.errorToString(statusCode).c_str());
    return;
  }

  if (statusCode != HTTP_CODE_OK) {
    Serial.printf("invalid status code: %d\n", statusCode);
    return;
  }

  LittleFS.begin();

  File file = LittleFS.open(filename, "w");

  int len = httpClient.getSize();

  uint8_t buff[128] = {0};

  auto stream = httpClient.getStreamPtr();

  while (httpClient.connected() && (len > 0 || len == -1)) {
    int c = stream->readBytes(buff, std::min((size_t)len, sizeof(buff)));
    Serial.printf("readBytes: %d\n", c);
    if (!c) {
      Serial.println("read timeout");
    }

    size_t bytesWritten = file.write(buff, c);

    if (bytesWritten == 0) {
      Serial.println("could not write to the file");
      file.close();
      LittleFS.end();
      return;
    }

    if (len > 0) {
      len -= c;
    }
  }

  file.close();

  LittleFS.end();

  httpClient.end();

  timer.setTimeout(readFile, 3000);
}

void setup() {
  Serial.begin(115200);
  delay(5000);

  wifiManager.connect(onConnectFinished);
}

void loop() { timer.tick(); }