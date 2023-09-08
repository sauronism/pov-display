#include <FastLED.h>

#define IMAGE_HEIGHT_PIXELS 80    // in pixels
#define IMAGE_SPREAD_DEGREES 160  // image height in degrees (like Field Of View)
#define IMAGE_START_ANGLE 10
#define IMAGE_END_ANGLE IMAGE_START_ANGLE + IMAGE_SPREAD_DEGREES


#define VIDEO_FPS 20

void setup() {
  delay(3000);  // 3 second delay for recovery

  ledsSetup();
  encoderSetup();  //interrupt pin definition
}

uint32_t imageFrame[IMAGE_HEIGHT_PIXELS][NUM_LEDS];


void loop() {
  int ledsAngle = encoderGetPosition();  // in degrees from the top TEST:(0 - 360)

  EVERY_N_MILLISECONDS(1000 / VIDEO_FPS) {
    // change the image being used.
    // this func creates the next image.
    // TEST make sure it's fast
    getNewImageFrame(imageFrame);
  }

  int imageCurserY = ledsAngleToYCurser(ledsAngle);

  ledStrip = imageFrame[imageCurserY];
  updateLedStrip();
}
