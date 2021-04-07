#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFiMulti.h>
#include <EventDispatcher.hpp>
#include <LittleFS.h>
#include <Timer.hpp>
#include <WiFiManager.hpp>

#define SSID "myssid"
#define PASSWORD "password"

EventDispatcher dispatcher;
Timer timer;
ESP8266WiFiMulti wifiMulti;
WiFiManager wifiManager(&wifiMulti, &dispatcher, &timer, SSID, PASSWORD);

const char *filename = "/file.txt";

void readFile() {
  LittleFS.begin();

  File file = LittleFS.open(filename, "r");

  Serial.println(file.readString().c_str());

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
                      "schedule.txt")) {
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

  auto body = httpClient.getString();

  LittleFS.begin();

  File file = LittleFS.open(filename, "w");

  size_t bytesWritten = file.write(body.c_str());

  if (bytesWritten == 0) {
    Serial.println("could not write to the file");
    return;
  }

  file.close();
  LittleFS.end();

  timer.setTimeout(readFile, 3000);
}

void setup() {
  Serial.begin(115200);
  delay(5000);

  wifiManager.connect(onConnectFinished);
}

void loop() { timer.tick(); }