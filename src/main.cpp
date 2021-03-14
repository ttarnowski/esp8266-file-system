#include <Arduino.h>
#include <LittleFS.h>

const char *filename = "/example-big";

void setup() {
  Serial.begin(115200);

  delay(5000);

  LittleFS.begin();

  if (!LittleFS.exists(filename)) {
    Serial.println("file does not exist, creating...");

    File file = LittleFS.open(filename, "w");

    if (!file) {
      Serial.println("could not open the file for writing");
      LittleFS.end();
      return;
    }

    auto bytesWritten = file.write("hello\n");

    Serial.printf("bytes written: %d\n", bytesWritten);

    if (bytesWritten == 0) {
      Serial.println("could not write to the file");
    }

    for (int i = 0; i < 2000; i++) {
      file.write("i am example file content i am example file content i am "
                 "example file content i am example file ..\n");

      Serial.println(i);
      yield();
    }

    file.write("this is the last line\n");

    file.close();
  }

  LittleFS.end();
}

void loop() {
  LittleFS.begin();

  File file = LittleFS.open(filename, "r");

  if (!file) {
    // handle the error
  }

  while (file.available()) {
    auto buf = file.readStringUntil('\n');

    Serial.println(buf.c_str());

    yield();
  }

  file.close();

  LittleFS.end();
  delay(5000);
}