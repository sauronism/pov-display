
#define STRIP_NUM_LEDS 280
#define IMAGE_WIDTH 80


class videoFrame
{
  public:
  uint32_t timeForNextRow;
  uint nextRowIndex;
  uint32_t rowData[STRIP_NUM_LEDS];
  uint32_t frameData[IMAGE_WIDTH][STRIP_NUM_LEDS];
};

void encoder_trigger(){
  rotationStartedTime = millis();
}


void setup() {
  // put your setup code here, to run once:

}

void loop() {

}


