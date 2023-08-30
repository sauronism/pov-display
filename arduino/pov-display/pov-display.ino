

#define IMAGE_WIDTH 80

void setup() {
  encoderSetup(); //interrupt pin definition
}

void loop() {
  unsigned int rotationSpeed = calculateRotationSpeed();

}
