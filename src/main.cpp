#include <Arduino.h>
#include <LittleFS.h>

#include "FS.h"

#define FORMAT_LITTLEFS_IF_FAILED true

static const char *PARTITION_LABEL = "storage";
static const char *BASE_PATH = "/data";

void setup() {
  Serial.begin(115200);
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED, BASE_PATH, 10,
                      PARTITION_LABEL)) {
    Serial.println("LittleFS mount failed");
    return;
  }
}

void loop() {
  Serial.printf("Reading file: %s\n", "/hello.txt");
  File file = LittleFS.open("/hello.txt");
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
  }
  Serial.println("- read from file:");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
  delay(1000);
}
