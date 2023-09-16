SdFile bmpFile;  // set filesystem

void updateImageFrame(int frameIndex) {
  String filename = BASE_FILE_PATH + frameIndex;
  // Open the binary file
  File file = SD.open(filename.c_str(), FILE_READ);

  if (!file) {
    Serial.println("SD | Failed to open file for reading");
    return;
  }
  
  int pixelIndex = 0;

  if (file.available()) {
    file.read(imageBuffer, imageBufferSize);

    for (int y = 0; y < IMAGE_HEIGHT_PIXELS; y++)
      for (int x = 0; x < NUM_LEDS; x++) {
        // Extract RGB values from the buffer
        byte red = imageBuffer[pixelIndex];
        byte green = imageBuffer[pixelIndex + 1];
        byte blue = imageBuffer[pixelIndex + 2];
        CRGB pixel_value = (red << 16) | (green << 8) | blue;
        imageFrame[y][x] = pixel_value;

        pixelIndex++;
      }
  }
  file.close();
}