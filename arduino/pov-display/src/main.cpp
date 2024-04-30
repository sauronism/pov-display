#include <FastLED.h>
#include <SD.h>
#include <SPI.h>
#include <math.h>
#include <iterator>
#include <algorithm>

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
  LowPassFilter(float alpha)
  {
    this->_alpha = alpha;
  }
  float apply(float x)
  {
    x = (x * _alpha) + (_oldValue * (1 - _alpha));
    _oldValue = x;
    return x;
  }

private:
  float _alpha;
  float _oldValue;
};

LowPassFilter encoderSpdLP(0.01);
float angle = 0;
double precise_millis;
float timeFromLastTick;
float degPerMilli;
float encoderGetPosition()
{
  // calc speed
  encoder.millisPerRotation = encoderSpdLP.apply(encoder.tickTime - encoder.oldTickTime);
  degPerMilli = 360.0f / encoder.millisPerRotation;

  // calc position
  precise_millis = micros() / 1000.0f;
  timeFromLastTick = precise_millis - encoder.tickTime;
  angle = degPerMilli * timeFromLastTick;
  angle = fmod(angle, 180);

  return angle;
}

int ledsAngleToYCurser(float angle)
{
  angle -= IMAGE_START_ANGLE; // some offset, so 10 degrees is our starting position.
  angle = constrain(angle, 0, IMAGE_SPREAD_DEGREES);

  int yCureser = (angle / (float)IMAGE_SPREAD_DEGREES) * (float)IMAGE_HEIGHT_PIXELS;

  return yCureser;
}

void setup()
{
  delay(3000); // 3 second delay for good measure

  SDSetup();
  ledsSetup();
  encoderSetup(); // interrupt pin definition

  // for tests only - read only one frame
  updateImageFrame(0);
  Serial.printf("setup end\n");
}

void loop()
{

  int ledsAngle = encoderGetPosition();

  // EVERY_N_MILLISECONDS(1000 / VIDEO_FPS)
  // {
  //   frameIndex++;
  //   if (frameIndex > ANIMATION_NUM_FRAMES)
  //     frameIndex = 0;
  //   // updateImageFrame(frameIndex);
  // }

  int imageCurserY = ledsAngleToYCurser(ledsAngle);
  std::copy(std::begin(imageFrame[imageCurserY]), std::end(imageFrame[imageCurserY]), std::begin(ledStrip));
  FastLED.show();
}
