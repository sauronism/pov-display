#include <FastLED.h>
#include <SD.h>
#include <SPI.h>
#include <math.h>
#include <iterator>
#include <algorithm>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

// debugging
#define DEBUG_ENABLED true
#define DEBUG_PRINTF(...)       \
  if (DEBUG_ENABLED)            \
  {                             \
    Serial.printf(__VA_ARGS__); \
  }

// command constants
#define TRUE 1
#define FALSE 0
#define NO_OP -1

// --- LOGGING --- //
#define LOG_SD true

// --- image details --- //
// TODO: Increase vertical resolution to 240
#define IMAGE_HEIGHT_PIXELS 80   // in pixels
#define IMAGE_SPREAD_DEGREES 150 // image height in degrees (like vertical Field Of View)
#define IMAGE_START_ANGLE 20

// --- video details --- //
#define VIDEO_FPS 20
#define ANIMATION_NUM_FRAMES 20
#define BASE_FILE_PATH "img_"

// --- leds --- //
FASTLED_USING_NAMESPACE
#define LED1_DATA_PIN 3
#define LED1_CLK_PIN 4
#define LED_TYPE APA102
#define COLOR_ORDER BGR
// TODO: Number of LEDs is actually 2 * 144 = 288
#define NUM_LEDS 300
// The default data rate (12MHz) is too high and we see some signal integrity issues
// 6 MHz works perfectly fine (but we lose a lot of bandwidth)
// TODO: We should test and see if 10Mhz or 8MHz works smoothly as well
#define LED_DATA_RATE DATA_RATE_MHZ(6)

/*
 * LED Refresh Rate
 * We need to call FastLED.show() at least once for each *ROW* of our image.
 * So our effective refresh rate is:
 * Image height * target FPS * 2 (number of led strips) * spread correction * nyquist sampling theorm
 * */
static const auto NUM_LED_STRIPS = 2;
static const auto MOTOR_RPS_TARGET = 10;
static const auto DISPLAY_SPREAD_RATIO = 180.0 / IMAGE_SPREAD_DEGREES;
// Actually anything > 2.0 is enough, but we've got extra data rate to spare
static const auto NYQUIST_FACTOR = 4;
static const uint16_t LED_TARGET_REFRESH_RATE = std::ceil(
    NUM_LED_STRIPS * MOTOR_RPS_TARGET * DISPLAY_SPREAD_RATIO * IMAGE_HEIGHT_PIXELS * NYQUIST_FACTOR);

CRGB ledStrip[NUM_LEDS];
#define BRIGHTNESS 50

// --- sd reading --- //
// TODO: Read image size directly from file and don't assume it is constant
CRGB imageFrame[IMAGE_HEIGHT_PIXELS][NUM_LEDS] = {0};
CRGB blackRow[NUM_LEDS] = {0};

/*
 * Image encoding suggestion (not yet implemented)
 *
 * | Byte 1..4    | Byte 5..6   | 7..8          | 9 ... 9 + H * W * 3 |
 * | Magic header | Frame Width | Frame Height  | Bitmap Data         |
 * | 0x4559_4531  | ------------| --------------| --------------------|
 *
 * Better yet: use actual .BMP file format
 * */

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
  /* TODO: Setup 2 LED strips on different pins
   * Currently the 2 strips ar driven by the same data channel
   * */
  FastLED.addLeds<LED_TYPE, LED1_DATA_PIN, LED1_CLK_PIN, COLOR_ORDER, LED_DATA_RATE>(ledStrip, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxRefreshRate(LED_TARGET_REFRESH_RATE);
}

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

  if (file.available())
  {
    // Read directly into the imageFrame buffer
    // Only works because the buffer and image are exactly in the same dimensions
    file.read(imageFrame, std::min((uint64_t)sizeof imageFrame, file.size()));
  }
  else if (LOG_SD)
  {
    Serial.println(String("SD | Could not open ") + filename);
  }

  file.close();
}
//----------------------------------------------------------------
#define esp_serial Serial2

typedef StaticJsonDocument<256> Packet;
typedef struct
{
  short display_on;
  short eye_azimuth;
  short display_custom_text;
  String custom_text_data;
} cmd_t;

short getIntJsonProperty(Packet pkt, char *propertyName)
{
  return pkt.containsKey(propertyName) ? pkt[propertyName].as<short>() : NO_OP;
}

String getStringJsonProperty(Packet pkt, char *propertyName)
{
  return pkt.containsKey(propertyName) ? pkt[propertyName].as<String>() : "";
}

cmd_t parseJsonCmd(Packet json_data)
{
  cmd_t command;
  command.display_on = getIntJsonProperty(json_data, "display_on");
  command.eye_azimuth = getIntJsonProperty(json_data, "eye_azimuth");
  command.display_custom_text = getIntJsonProperty(json_data, "display_custom_text");
  command.custom_text_data = getStringJsonProperty(json_data, "custom_text_data");
}

bool parseJsonString(String pktString, Packet &pkt)
{
  DeserializationError error = deserializeJson(pkt, pktString);
  if (error)
  {
    DEBUG_PRINTF("Error parsing JSON: %s", error);
    return false;
  }
  return true;
}
cmd_t read_esp_command()
{
  Packet json_packet;
  cmd_t command;

  String pktString = esp_serial.readStringUntil('\0');
  if (parseJsonString(pktString, json_packet))
  {
    command = parseJsonCmd(json_packet);
  }
  return command;
}
//----------------------------------------------------------------

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

int ledsAngleToYCurser(const double alpha)
{
  const auto angleWithOffset = alpha - IMAGE_START_ANGLE; // some offset, so 10 degrees is our starting position.
  const auto yCursor = (angleWithOffset / IMAGE_SPREAD_DEGREES) * IMAGE_HEIGHT_PIXELS;
  const auto clamped = constrain(yCursor, 0, IMAGE_HEIGHT_PIXELS);
  // return -1 to signal an out-of-bounds index.
  if (yCursor != clamped)
  {
    return -1;
  }
  return round(yCursor);
}

void display_video(int azimuth)
{
  static auto frameIndex = 0;

  int ledsAngle = encoderGetPosition();

  EVERY_N_MILLISECONDS(1000 / VIDEO_FPS)
  {
    frameIndex = (frameIndex + 1) % ANIMATION_NUM_FRAMES;
    updateImageFrame(frameIndex);
  }

  // TODO: Move drawing to separate function
  int imageCurserY = ledsAngleToYCurser(ledsAngle);
  // TODO: don't encode out-of-bounds result as -1 and use std::variant or something similar
  if (imageCurserY == -1)
  {
    std::copy(std::begin(blackRow), std::end(blackRow), std::begin(ledStrip));
  }
  else
  {
    std::copy(std::begin(imageFrame[imageCurserY]), std::end(imageFrame[imageCurserY]), std::begin(ledStrip));
  }
  FastLED.show();
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
  cmd_t esp_command;
  if (esp_serial.available())
  {
    esp_command = read_esp_command();
  }

  if (esp_command.display_on != 1)
  {
    fill_solid(ledStrip, NUM_LEDS, 0);
  }
  else if (esp_command.display_custom_text == 1)
  {
    // TODO display custom text (esp_command.custom_text_data)
  }
  else
  {
    display_video(esp_command.eye_azimuth);
  }

  FastLED.show();
}
