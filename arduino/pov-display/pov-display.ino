include<FastLED.h>

#define IMAGE_HEIGHT 80          // in pixels
#define IMAGE_HEIGHT_DEGREES 45  // image height in degrees (like Field Of View)
#define NUM_LEDS 300

#define VIDEO_FPS 20

  void setup() {
  encoderSetup();  //interrupt pin definition
}

uint32_t imageFrame[IMAGE_HEIGHT][NUM_LEDS];
void loop() {
  int ledsAngle = encoderGetPosition();  // in degrees from the top

  // TODO
  // create a displayFrame(ImageFrame)
  // -- nonblocking
  // -- only updates the imageParser (TBD) with a new image to show.
  // -- should be called 20 times per second (every 1000/VIDEO_FPS ms)
  EVERY_N_MILLISECONDS(1000 / VIDEO_FPS) {
    imageFrame = updateImageFrame();
  }

  int imageCurserY = (double)(360 / ledsAngle) * IMAGE_HEIGHT_DEGREES;  //starting from the top
  ledStrip = imageFrame[imageCurserY];

  
}
