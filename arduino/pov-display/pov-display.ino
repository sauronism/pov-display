#include <FastLED.h>
#include <SD.h>
#include <SPI.h>

// --- LOGGING --- //
#define LOG_SD true

// --- image details --- //
#define IMAGE_HEIGHT_PIXELS 80    // in pixels
#define IMAGE_SPREAD_DEGREES 160  // image height in degrees (like Field Of View)
#define IMAGE_START_ANGLE 10
#define IMAGE_END_ANGLE IMAGE_START_ANGLE + IMAGE_SPREAD_DEGREES

// --- video details --- //
#define VIDEO_FPS 20
#define ANIMATION_NUM_FRAMES 3200
#define BASE_FILE_PATH "./animation1/img_"

// --- leds --- //
FASTLED_USING_NAMESPACE
#define DATA_PIN 3
#define CLK_PIN 4
#define LED_TYPE APA102
#define COLOR_ORDER GRB
#define NUM_LEDS 300

CRGB ledStrip[NUM_LEDS];
#define BRIGHTNESS 100

// --- sd reading --- //
const int imageBufferSize = 3 * NUM_LEDS * IMAGE_HEIGHT_PIXELS;
PROGMEM byte imageBuffer[imageBufferSize];  // R G B

void setup() {
  delay(3000);  // 3 second delay for recovery

  SDSetup();
  ledsSetup();
  encoderSetup();  // interrupt pin definition
}

CRGB imageFrame[IMAGE_HEIGHT_PIXELS][NUM_LEDS];
int frameIndex;
void loop() {
  int ledsAngle = encoderGetPosition();  // in degrees from the top TEST:(0 - 360)

  EVERY_N_MILLISECONDS(1000 / VIDEO_FPS) {
    frameIndex++;
    if (frameIndex > ANIMATION_NUM_FRAMES)
      frameIndex = 0;
    updateImageFrame(frameIndex);
  }

  int imageCurserY = ledsAngleToYCurser(ledsAngle);
  // ledStrip = imageFrame[imageCurserY];
  memcpy(imageFrame[imageCurserY], ledStrip, sizeof(ledStrip)); // copy each row at the time to the leds

  updateLedStrip();
}
