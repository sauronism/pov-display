#include <FastLED.h>
#include <SD.h>
#include <SPI.h>
#include <math.h>
#include <iterator>
#include <algorithm>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <algorithm>
#include <numeric>
#include <optional>

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
#define IMAGE_HEIGHT_PIXELS 128   // in pixels
#define IMAGE_SPREAD_DEGREES 120 // image height in degrees (like vertical Field Of View)
#define IMAGE_START_ANGLE 30

// --- video details --- //
#define VIDEO_FPS 20
#define ANIMATION_NUM_FRAMES 2400
#define BASE_FILE_PATH "vid_center/img_"

// --- leds --- //
FASTLED_USING_NAMESPACE
#define LED2_CLK_PIN 4
#define LED2_DATA_PIN 3
#define LED1_CLK_PIN 7
#define LED1_DATA_PIN 6
#define LED_TYPE APA102
#define COLOR_ORDER BGR
// TODO: Number of LEDs is actually 2 * 144 = 288
// The default data rate (12MHz) is too high and we see some signal integrity issues
// 6 MHz works perfectly fine (but we lose a lot of bandwidth)
// TODO: We should test and see if 10Mhz or 8MHz works smoothly as well
#define LED_DATA_RATE DATA_RATE_MHZ(6)

#define NUM_LEDS 288
CRGB ledStrip1[NUM_LEDS];
CRGB ledStrip2[NUM_LEDS];
#define BRIGHTNESS 50

/*
 * LED Refresh Rate
 * We need to call FastLED.show() at least once for each *ROW* of our image.
 * So our effective refresh rate is:Á
 * Image height * target FPS * 2 (number of led strips) * spread correction * nyquist sampling theorm
 * */
static const auto NUM_LED_STRIPS = 2;
static const auto MOTOR_RPS_TARGET = 10;
static const auto DISPLAY_SPREAD_RATIO = 180.0 / IMAGE_SPREAD_DEGREES;
// Actually anything > 2.0 is enough, but we've got extra data rate to spare
static const auto NYQUIST_FACTOR = 4;
static const uint16_t LED_TARGET_REFRESH_RATE = 10000; // std::ceil(NUM_LED_STRIPS * MOTOR_RPS_TARGET * DISPLAY_SPREAD_RATIO * IMAGE_HEIGHT_PIXELS * NYQUIST_FACTOR);

// --- sd reading --- //
// TODO: Read image size directly from file and don't assume it is constant
CRGB imageFrame[IMAGE_HEIGHT_PIXELS][NUM_LEDS] = {0};
const CRGB blackRow[NUM_LEDS] = {0};

/*
 * Image encoding suggestion (not yet implemented)
 *
 * | Byte 1..4    | Byte 5..6   | 7..8          | 9 ... 9 + H * W * 3 |
 * | Magic header | Frame Width | Frame Height  | Bitmap Data         |
 * | 0x4559_4531  | ------------| --------------| --------------------|
 *
 * Better yet: use actual .BMP file format
 * */

void SDSetup() {
  if (LOG_SD)
    Serial.print("SD | Initializing filesystem...");

  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("\t begin() failed!!!");
    return;
  }
  if (LOG_SD)
    Serial.println("\t initialized!!");
}

void ledsSetup() {
  /* TODO: Setup 2 LED strips on different pins
   * Currently the 2 strips ar driven by the same data channel
   * */
  FastLED.addLeds<LED_TYPE, LED1_DATA_PIN, LED1_CLK_PIN, COLOR_ORDER, LED_DATA_RATE>(ledStrip1, NUM_LEDS);
  FastLED.addLeds<LED_TYPE, LED2_DATA_PIN, LED2_CLK_PIN, COLOR_ORDER, LED_DATA_RATE>(ledStrip2, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxRefreshRate(std::numeric_limits<uint16_t>::max());
}

void updateImageFrame(const char *filename) {

  // Open the binary file
  File file = SD.open(filename, FILE_READ);

  if (!file) {
    Serial.printf("SD | Failed to open file for reading\n");
    return;
  }

  if (file.available()) {
//    Serial.printf("Playing: %s\n", filename);
    // Read directly into the imageFrame buffer
    // Only works because the buffer and image are exactly in the same dimensions
    file.read(imageFrame, std::min((uint64_t) sizeof imageFrame, file.size()));
  } else if (LOG_SD) {
    Serial.println(String("SD | Could not open ") + filename);
  }

  file.close();
}
//----------------------------------------------------------------
#define esp_serial Serial4

typedef StaticJsonDocument<256> Packet;
typedef struct {
  int display_on;
  int eye_azimuth;
  int display_custom_text;
  String custom_text_data;
  bool esp_connected;
} cmd_t;

short getIntJsonProperty(Packet pkt, char *propertyName) {
  return pkt.containsKey(propertyName) ? pkt[propertyName].as<short>() : NO_OP;
}

String getStringJsonProperty(Packet pkt, char *propertyName) {
  return pkt.containsKey(propertyName) ? pkt[propertyName].as<String>() : "";
}

bool parseJsonCmd(Packet json_data, cmd_t &command) {
  command.display_on = getIntJsonProperty(json_data, "display_on");
  command.eye_azimuth = getIntJsonProperty(json_data, "eye_azimuth");
  command.display_custom_text = getIntJsonProperty(json_data, "display_custom_text");
  command.custom_text_data = getStringJsonProperty(json_data, "custom_text_data");
  return true;
}

bool parseJsonString(String pktString, Packet &pkt) {
  DeserializationError error = deserializeJson(pkt, pktString);
  if (error) {
    DEBUG_PRINTF("Error parsing JSON: %s", error);
    return false;
  }
  return true;
}

bool read_esp_command(cmd_t &command) {
  Packet json_packet;

  String pktString = esp_serial.readStringUntil('\0');
  // Serial.printf("packet looks like this: ");
  Serial.println(pktString);
  // Serial.printf("packet length is %d", pktString.length());
  if (pktString.length() < 5)
    return false;

  Serial.printf("parsing packet");
  if (parseJsonString(pktString, json_packet)) {
    parseJsonCmd(json_packet, command);
    Serial.printf("command display_on is %d", command.display_on);
  }
  return true;
}
//----------------------------------------------------------------

// --- ENCODER --- //
#define ENCODER_PIN 5
#define ENCODER_ACTIVE_STATE LOW
#define ENCODER_INTERRUPT_MODE FALLING
#define MIN_ACCEPTABLE_ENCODER_TICK_INTERVAL 60
#define MAX_ACCEPTABLE_ENCODER_TICK_INTERVAL 1000

typedef struct {
  unsigned long tickTime;
  unsigned long oldTickTime;
  unsigned long minTimeForNextTick;
  unsigned long maxTimeForNextTick;
  float millisPerRotation;
} EncoderData;


class LowPassFilter {
public:
  LowPassFilter(float alpha) {
    this->_alpha = alpha;
  }

  float apply(float x) {
    x = (x * _alpha) + (_oldValue * (1 - _alpha));
    _oldValue = x;
    return x;
  }

private:
  float _alpha;
  float _oldValue;
};

LowPassFilter encoderSpdLP(0.2);

EncoderData encoder = {0, 0, 0, 0};

void encoderInterrupt() {
  unsigned long _current_time = millis();
  if (_current_time < encoder.minTimeForNextTick) {
    return;
  }

  encoder.oldTickTime = encoder.tickTime;
  encoder.tickTime = _current_time;
  encoder.minTimeForNextTick = _current_time + MIN_ACCEPTABLE_ENCODER_TICK_INTERVAL;
  encoder.maxTimeForNextTick = _current_time + MAX_ACCEPTABLE_ENCODER_TICK_INTERVAL;
  if (_current_time < encoder.maxTimeForNextTick) {
    encoder.millisPerRotation = encoderSpdLP.apply(encoder.tickTime - encoder.oldTickTime);
  }
}

void encoderSetup() {
  pinMode(ENCODER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), encoderInterrupt, ENCODER_INTERRUPT_MODE);
}


float angle = 0;
double precise_millis;
float timeFromLastTick;
float degPerMilli;

float encoderGetPosition() {
  // calc speed
  degPerMilli = 360.0f / encoder.millisPerRotation;

  // calc position
  precise_millis = micros() / 1000.0f;
  timeFromLastTick = precise_millis - encoder.tickTime;
  const auto angle = degPerMilli * timeFromLastTick;
  return fmod(angle, (float) 360.0);
}

std::optional<int> ledsAngleToYCurser(const double alpha) {
  const auto angleWithOffset = alpha - IMAGE_START_ANGLE; // some offset, so 10 degrees is our starting position.
  const auto yCursor = (angleWithOffset / IMAGE_SPREAD_DEGREES) * IMAGE_HEIGHT_PIXELS;
  const auto clamped = constrain(yCursor, 0, IMAGE_HEIGHT_PIXELS);
  // return -1 to signal an out-of-bounds index.
  if (yCursor != clamped) {
    return {};
  }
  return round(yCursor);
}

auto display_video(int azimuth) {
  static auto frameIndex = 0;
  static char filename[256];

  const auto encoderPosition = encoderGetPosition();

  EVERY_N_MILLISECONDS(50) {
    frameIndex = (frameIndex + 1) % ANIMATION_NUM_FRAMES;
    snprintf(filename, sizeof(filename), "out/frame_%08d.bin", frameIndex);
    updateImageFrame(filename);
  }

  // TODO: Move drawing to separate function
  const float angle = fmod(encoderPosition, 180);
  const auto y_cursor_opt = ledsAngleToYCurser(angle);
  // TODO: don't encode out-of-bounds result as -1 and use std::variant or something similar
  if (!y_cursor_opt) {
    std::copy(std::begin(blackRow), std::end(blackRow), std::begin(ledStrip1));
    std::copy(std::begin(blackRow), std::end(blackRow), std::begin(ledStrip2));
  } else {
    const auto cursor = y_cursor_opt.value();
    const auto target = angle != encoderPosition ? ledStrip1 : ledStrip2;
    const auto other = angle != encoderPosition ? ledStrip2 : ledStrip1;
    std::copy(std::begin(imageFrame[cursor]), std::end(imageFrame[cursor]), target);
    std::copy(std::begin(blackRow), std::end(blackRow), other);
  }
}

bool esp_connected = false;
unsigned long time_considered_as_disconnection = 0;
const int esp_dead_timeout = 5000;

bool read_esp_data(cmd_t &esp_packet) {
  bool valid_packet = 0;

  if (esp_serial.available()) {
    time_considered_as_disconnection = millis() + esp_dead_timeout;
    Serial.println("received command");
    esp_connected = true;
    valid_packet = read_esp_command(esp_packet);
    Serial.printf("packet is %d", valid_packet);
  }

  if (esp_connected && millis() > time_considered_as_disconnection) {
    esp_connected = false;
    Serial.printf("esp disconnect");
  }
  esp_packet.esp_connected = esp_connected;
  return valid_packet;
}

void esp_controller_loop(cmd_t esp_command) {

  if (esp_command.display_on != 1) {
    fill_solid(ledStrip1, NUM_LEDS, 0);
    fill_solid(ledStrip2, NUM_LEDS, 0);
  } else if (esp_command.display_custom_text == 1) {
    // TODO display custom text (esp_command.custom_text_data)
  } else {
    Serial.printf("running esp video command");
    esp_command.eye_azimuth = 0;
    display_video(esp_command.eye_azimuth);
  }
}

void setup() {
  Serial.begin(115200);
  esp_serial.begin(115200);
  delay(3000); // 3 second delay for good measure

  SDSetup();
  ledsSetup();
  encoderSetup(); // interrupt pin definition

  // for tests only - read only one frame
  updateImageFrame("out/frame_00000000.bin");
  Serial.printf("setup end\n");
}

cmd_t esp_command;

void loop() {
//  read_esp_data(esp_command);
//  if (esp_command.esp_connected)
//  {
//    Serial.printf("display is %d\n", bool(esp_command.display_on));
//    esp_controller_loop(esp_command);
//  }
//  else
//  {
//  }
  display_video(0);

  // Uncomment to simulate interrupts
//  EVERY_N_MILLISECONDS(100) {
//    encoderInterrupt();
//  }

  FastLED.show();
}
