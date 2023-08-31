#include <FastLED.h>

#define IMAGE_HEIGHT 80          // in pixels
#define IMAGE_HEIGHT_DEGREES 45  // image height in degrees (like Field Of View)

#define VIDEO_FPS 20

void setup() {
  delay(3000);  // 3 second delay for recovery

  ledsSetup();
  encoderSetup();  //interrupt pin definition
}

uint32_t imageFrame[IMAGE_HEIGHT][NUM_LEDS];


void loop() {
  int ledsAngle = encoderGetPosition();  // in degrees from the top

  EVERY_N_MILLISECONDS(1000 / VIDEO_FPS) {
    // change the image being used.
    // this func creates the next image.
    // TODO make sure it's fast
    updateImageFrame(imageFrame);
  }

  int imageCurserY = (double)(360 / ledsAngle) * IMAGE_HEIGHT_DEGREES;  // get current leds pos. starting from the top.
  ledStrip = imageFrame[imageCurserY];


  updateLedStrip();
}
