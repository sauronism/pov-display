// class videoFrame
// {
//   public:
//   uint32_t timeForNextRow;
//   uint nextRowIndex;
//   uint32_t rowData[STRIP_NUM_LEDS];
//   uint32_t frameData[IMAGE_WIDTH][STRIP_NUM_LEDS];
// };

int ledsAngleToYCurser(int angle){
  angle -= IMAGE_START_ANGLE; //some offset, so 0 degrees is our starting position.
  angle = constrain(angle, 0, IMAGE_SPREAD_DEGREES); 
  
  double yPos = (double)(angle/IMAGE_SPREAD_DEGREES) * IMAGE_HEIGHT_PIXELS;
  
  return yPos;
}


// class SauronEyeAnimation {
//   void updateImageFrame(int (&imageResult)[IMAGE_HEIGHT_PIXELS][NUM_LEDS], int x, int y) {
//     this.drawBaseEye(x, y, imageResult);

//     for (int i = 0; i < IMAGE_HEIGHT_PIXELS; i++)
//       for (int j = 0; j < NUM_LEDS; j++) {
//         this.drawOutwardSin(x, y, imageResult);
//       }
//   }

//   void drawBaseEye(int x, int y, int (&imageResult)[IMAGE_HEIGHT_PIXELS][NUM_LEDS]) {
//     imageResult = 
//   }
// };