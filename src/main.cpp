#include <Arduino.h>
#include <ESP8266WiFiMulti.h>
#include <EventDispatcher.hpp>
#include <LittleFS.h>
#include <SingleHostHTTPSClient.hpp>
#include <Timer.hpp>
#include <WiFiManager.hpp>

#define SSID "myssid"
#define PASSWORD "mypassword"

const char *cert PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ
RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD
VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX
DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y
ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy
VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr
mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr
IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK
mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu
XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy
dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye
jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1
BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3
DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92
9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx
jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0
Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz
ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS
R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp
-----END CERTIFICATE-----
)EOF";

EventDispatcher dispatcher;
Timer timer;
ESP8266WiFiMulti wifiMulti;
WiFiManager wifiManager(&wifiMulti, &dispatcher, &timer, SSID, PASSWORD);

SingleHostHTTPSClient httpsClient("100daysofcode.s3-eu-west-1.amazonaws.com",
                                  cert, &wifiManager, &timer);

const char *filename = "/file-read-as-stream.txt";

void readFile() {
  LittleFS.begin();

  File file = LittleFS.open(filename, "r");

  Serial.println("File contents: ");

  while (file.available()) {
    char buf[256];

    size_t bytesRead = file.readBytes(buf, sizeof(buf) - 1);
    buf[bytesRead] = '\0';

    Serial.print(buf);
  }

  file.close();

  LittleFS.end();
}

void onResponse(SingleHostHTTPSClient::Response res) {
  if (res.error != nullptr) {
    Serial.printf("request error: %s\n", res.error);
    return;
  }

  LittleFS.begin();

  File file = LittleFS.open(filename, "w");

  if (!file) {
    Serial.println("could not open the file for write");
    return;
  }

  char error[64] = "";

  while (res.body->available()) {
    uint8_t buffer[256];

    auto bytesRead = res.body->readBytes(buffer, sizeof(buffer));
    Serial.printf("bytesRead: %d\n", bytesRead);

    if (bytesRead == 0) {
      strcpy(error, "could not read form the response body");
      break;
    }

    auto bytesWritten = file.write(buffer, bytesRead);
    Serial.printf("bytesWritten: %d\n", bytesWritten);

    if (bytesWritten == 0) {
      strcpy(error, "could not write to the file");
      break;
    }
  }

  file.close();
  LittleFS.end();

  if (strcmp("", error) != 0) {
    Serial.printf("stream error: %s\n", error);
    return;
  }

  readFile();
}

void setup() {
  Serial.begin(115200);
  delay(5000);

  httpsClient.sendRequest(SingleHostHTTPSClient::Request{"/rfc2616.txt"},
                          onResponse);
}

void loop() { timer.tick(); }