// class videoFrame
// {
//   public:
//   uint32_t timeForNextRow;
//   uint nextRowIndex;
//   uint32_t rowData[STRIP_NUM_LEDS];
//   uint32_t frameData[IMAGE_WIDTH][STRIP_NUM_LEDS];
// };


void updateImageFrame(uint32_t (*imageFrame)[NUM_LEDS], int rows, int columns) {
  // You can update the array here
}


class SauronEyeAnimation {
  void updateImageFrame(int (&imageResult)[IMAGE_HEIGHT][NUM_LEDS], int x, int y) {
    this.drawBaseEye(x, y, imageResult);

    for (int i = 0; i < IMAGE_HEIGHT; i++)
      for (int j = 0; j < NUM_LEDS; j++) {
        this.drawOutwardSin(x, y, imageResult);
      }
  }

  void drawBaseEye(int x, int y, int (&imageResult)[IMAGE_HEIGHT][NUM_LEDS]) {
    imageResult = 
  }
};




void updateImageFrame(int (&imageResult)[IMAGE_HEIGHT][NUM_LEDS],
                      uint32_t (*imageEffect)(uint32_t, int, int)) {
  for (int i = 0; i < IMAGE_HEIGHT; i++)
    for (int j = 0; j < NUM_LEDS; j++) {
      imageResult[i][j] = imageEffect(imageResult[i][j], i, j);
    }
}