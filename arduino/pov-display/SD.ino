
void SDSetup() {
  if (LOG_SD) Serial.print("SD | Initializing filesystem...");

  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("\t begin() failed!!!");
    return;
  }
  if (LOG_SD) Serial.println("\t initialized!!");
}