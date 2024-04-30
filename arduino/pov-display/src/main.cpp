#include <FastLED.h>
#include <SD.h>
#include <SPI.h>
#include <math.h>

// --- LOGGING --- //
#define LOG_SD true

// --- image details --- //
#define IMAGE_HEIGHT_PIXELS 80  // in pixels
#define IMAGE_SPREAD_DEGREES 90 // image height in degrees (like vertical Field Of View)
#define IMAGE_START_ANGLE 40
#define IMAGE_END_ANGLE IMAGE_START_ANGLE + IMAGE_SPREAD_DEGREES

// --- video details --- //
#define VIDEO_FPS 20
#define ANIMATION_NUM_FRAMES 3200
#define BASE_FILE_PATH "img_"

// --- leds --- //
FASTLED_USING_NAMESPACE
#define DATA_PIN 3
#define CLK_PIN 4
#define LED_TYPE APA102
#define COLOR_ORDER BGR
#define NUM_LEDS 300

CRGB ledStrip[NUM_LEDS];
#define BRIGHTNESS 100

// --- sd reading --- //
const long imageBufferSize = 3 * NUM_LEDS * IMAGE_HEIGHT_PIXELS;
byte imageBuffer[imageBufferSize]; // R G B

CRGB imageFrame[IMAGE_HEIGHT_PIXELS][NUM_LEDS];
int frameIndex;

void SDSetup()
{
  if (LOG_SD)
    Serial.print("SD | Initializing filesystem...");

  if (!SD.begin(BUILTIN_SDCARD))
  {
    Serial.println("\t begin() failed!!!");
    return;
  }
  if (LOG_SD)
    Serial.println("\t initialized!!");
}

void ledsSetup()
{

  FastLED.addLeds<LED_TYPE, DATA_PIN, CLK_PIN, COLOR_ORDER>(ledStrip, NUM_LEDS); //.setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
}

SdFile bmpFile; // set filesystem

void updateImageFrame(int frameIndex)
{
  String filename = BASE_FILE_PATH + String(frameIndex);
  // Open the binary file
  File file = SD.open(filename.c_str(), FILE_READ);

  if (!file)
  {
    printf("SD | Failed to open file for reading\n");
    return;
  }

  int pixelIndex = 0;
  if (file.available())
  {
    file.read(imageBuffer, imageBufferSize);

    for (int y = 0; y < IMAGE_HEIGHT_PIXELS; y++)
      for (int x = 0; x < NUM_LEDS; x++)
      {
        // Extract RGB values from the buffer
        byte red = imageBuffer[pixelIndex];
        byte green = imageBuffer[pixelIndex + 1];
        byte blue = imageBuffer[pixelIndex + 2];

        CRGB pixel_value = CRGB(red, green, blue);
        imageFrame[y][x] = pixel_value;

        pixelIndex += 3;
      }
  }

  file.close();
}

// --- ENCODER --- //
#define ENCODER_PIN 5
#define ENCODER_ACTIVE_STATE LOW
#define ENCODER_INTERRUPT_MODE FALLING
#define MIN_ACCEPTABLE_ENCODER_TICK_INTERVAL 60

typedef struct
{
  unsigned long tickTime;
  unsigned long oldTickTime;
  unsigned long minTimeForNextTick;
  float millisPerRotation;
} EncoderData;

EncoderData encoder = {0, 0, 0, 0};

void encoderInterrupt()
{
  unsigned long _current_time = millis();
  if (_current_time < encoder.minTimeForNextTick)
  {
    return;
  }

  encoder.oldTickTime = encoder.tickTime;
  encoder.tickTime = _current_time;
  encoder.minTimeForNextTick = _current_time + MIN_ACCEPTABLE_ENCODER_TICK_INTERVAL;
}

void encoderSetup()
{
  pinMode(ENCODER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), encoderInterrupt, ENCODER_INTERRUPT_MODE);
}

class LowPassFilter
{
public:
  float alpha;
  float old_x_value;
  LowPassFilter(float alpha)
  {
    this->alpha = alpha;
  }
  float apply(float x)
  {
    x = (x * alpha) + (old_x_value * (1 - alpha));
    old_x_value = x;
    return x;
  }
};

LowPassFilter timesLP(0.01);
double position_millis_counter;
unsigned long millis_true_position_offset = 0;
float k_offset = 0.1;
float angle = 0;
double precise_millis;
float encoderGetPosition()
{
  // encoderGetRotationSpeed();

  unsigned long dt = encoder.tickTime - encoder.oldTickTime;
  encoder.millisPerRotation = timesLP.apply(dt);
  float deg_per_millis = 360.0f / encoder.millisPerRotation;

  precise_millis = micros() / 1000.0f;
  float time_from_last_tick = precise_millis - encoder.tickTime;
  float angle_based_on_last_tick = deg_per_millis * time_from_last_tick;
  angle = angle_based_on_last_tick;
  // angle = (deg_per_millis + millis_true_position_offset) * precise_millis;
  angle = fmod(angle, 180);

  // millis_true_position_offset = k_offset * (angle_based_on_last_tick - angle);

  // angle = encoder.millis_per_rotation position_millis_counter;
  // double millis_per_rotation = encoderGetRotationSpeed()
  // angle of the device from the top - in degrees

  // angle = encoderGetRotationSpeed() * position_millis_counter; // min(360, encoderGetRotationSpeed() * (float)timeFromLastTick);

  // angle = angle > 180 ? angle - 180 : angle; // faster that angle % 180

  return angle;
}

int ledsAngleToYCurser(float angle)
{
  angle -= IMAGE_START_ANGLE; // some offset, so 10 degrees is our starting position.
  angle = constrain(angle, 0, IMAGE_SPREAD_DEGREES);

  int yPos = (angle / (float)IMAGE_SPREAD_DEGREES) * (float)IMAGE_HEIGHT_PIXELS;

  return yPos;
}

void setup()
{
  delay(3000); // 3 second delay for recovery

  SDSetup();
  ledsSetup();
  encoderSetup(); // interrupt pin definition
  // unsigned long start_time = micros();
  updateImageFrame(0);
  Serial.printf("setup end\n");

  for (int i = 0; i < 300; i++)
  {
    ledStrip[i] = CRGB::Red;
    FastLED.show();
    delay(10);
  }
  for (int i = 0; i < 300; i++)
  {
    ledStrip[i] = CRGB::Black;
    FastLED.show();
    delay(10);
  }
}

uint32_t color; // s[] = {CRGB::Black, CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Yellow, CRGB::Purple};

double millis_per_rotation_for_printing = 400;
void loop()
{
  // if (encoder.millis_per_rotation > 60)
  // {
  //   if (encoder.millis_per_rotation < millis_per_rotation_for_printing)
  //     millis_per_rotation_for_printing = encoder.millis_per_rotation;
  // }
  // if (millis() % 10000)
  //   Serial.println(millis_per_rotation_for_printing);

  int ledsAngle = encoderGetPosition();
  if (ledsAngle > IMAGE_END_ANGLE || ledsAngle < IMAGE_START_ANGLE)
    fill_solid(ledStrip, NUM_LEDS, CRGB::Black);

  // // int color_index = (ledsAngle / 180.0f) * 6;÷
  // if (ledsAngle < 90)
  //   color = CRGB::Black;
  // else
  //   color = CRGB::Red;
  // fill_solid(ledStrip, NUM_LEDS, color);
  // FastLED.show();

  // int imageCurserY = ledsAngleToYCurser(ledsAngle);
  // ledStrip[imageCurserY] = CRGB::Red;
  // FastLED.show();

  // ledStrip[constrain(ledsAngle, 0, 300)] = CRGB::Red;
  // ledStrip[constrain(last_angle, 0, 300)] = CRGB::Black;

  // last_angle = ledsAngle;

  // EVERY_N_MILLISECONDS(1000 / VIDEO_FPS)
  // {
  //   frameIndex++;
  //   if (frameIndex > ANIMATION_NUM_FRAMES)
  //     frameIndex = 0;
  //   // updateImageFrame(frameIndex);
  // }

  int imageCurserY = ledsAngleToYCurser(ledsAngle);
  // Serial.printf("imageCurserY: %d\n", imageCurserY);
  // ledStrip = imageFrame[imageCurserY];
  for (int i = 0; i < 300; i++)
  {
    ledStrip[i] = imageFrame[imageCurserY][i];
  }
  // // memcpy(imageFrame[imageCurserY], ledStrip, sizeof(ledStrip)); // copy each row at the time to the leds
  FastLED.show();
}
