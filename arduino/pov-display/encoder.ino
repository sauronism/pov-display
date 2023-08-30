// --- ENCODER --- //
#define ENCODER_PIN 1
#define ENCODER_INTERRUPT_MODE RISING
#define ENCODER_TICKS_PER_ROTATION 2

struct EncoderData {
  unsigned long lastTickTime;
  unsigned long previousTickTime;
};

EncoderData encoder;

void encoderSetup() {
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), encoderInterrupt, ENCODER_INTERRUPT_MODE);
}

void encoderInterrupt() {
  encoder.previousTickTime = encoder.lastTickTime;
  encoder.lastTickTime = millis();
}

int calculateRotationSpeed() {
  int dt = encoder.lastTickTime - encoder.previousTickTime;
  unsigned int speed = 0;

  if (dt > 0)
    speed = (float)(1 / ENCODER_TICKS_PER_ROTATION) * 1000.0 / dt;  //spped is in rotations per second -- RPS

  return speed;
}
