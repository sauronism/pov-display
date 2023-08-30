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

int encoderGetRotationSpeed() {
  int dt = encoder.lastTickTime - encoder.previousTickTime;
  unsigned int speed = 0;
  if (dt > 0)
    speed = (double)(1 / ENCODER_TICKS_PER_ROTATION) * 1000.0 / dt;  //speed is in rotations per second -- RPS

  return speed;
}

int encoderGetPosition() {
  // angle of the device from the top - in degrees
  double rotSpeedDps = (double)encoderGetRotationSpeed() / 360.0;  // degrees per second
  long timeFromLastTick = millis() - encoder.lastTickTime;          // TODO - consider using time_from_*startTick* (using an absulute start position for the device)

  int angle = (double)(rotSpeedDps * timeFromLastTick / 1000.0);
  return angle;
}

