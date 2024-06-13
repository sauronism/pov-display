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
#define DEBUG_SIMULATE_INTERRUPTS false


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
#define IMAGE_START_ANGLE 40

// --- video details --- //
#define VIDEO_FPS 20
#define ANIMATION_NUM_FRAMES 667
#define BASE_FILE_PATH "vid_center/img_"

// --- leds --- //
FASTLED_USING_NAMESPACE
#define LED1_CLK_PIN SPI_CLOCK
#define LED1_DATA_PIN SPI_DATA
#define LED_TYPE APA102
#define COLOR_ORDER BGR

// 8 MHz is the fastest rate we can sustain before we lose signal integrity
#define LED_DATA_RATE DATA_RATE_MHZ(7)

#define NUM_LEDS 288
CRGB ledStrip[NUM_LEDS];
#define BRIGHTNESS 150

// --- sd reading --- //
// TODO: Read image size directly from file and don't assume it is constant
CRGB imageFrame[IMAGE_HEIGHT_PIXELS][NUM_LEDS] = {0};
const CRGB blackRow[NUM_LEDS] = {0};

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
  FastLED.addLeds<LED_TYPE, LED1_DATA_PIN, LED1_CLK_PIN, COLOR_ORDER, LED_DATA_RATE>(ledStrip, NUM_LEDS)
      .setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxRefreshRate(0, false);  // No upper limit on FPS
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
  Serial.printf("parsing packet: %s\n", pktString.c_str());
  if (parseJsonString(pktString, json_packet)) {
    parseJsonCmd(json_packet, command);
//    Serial.printf("command.display: %d, az: %d", command.display_on, command.eye_azimuth);
//    Serial.printf("command display_on is %d\n", command.display_on);
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

static const char *base_dirs[] = {
    "vid_center",
    "vid_left",
//    "vid_strong_left",
    "vid_right",
//    "vid_strong_right",
    "eye_loop",
    "this_is_fine",
    "nyan_cat",
    "poop_emoji",
};

auto display_video(int video_index) {
  static auto frameIndex = 0;
  static char filename[256];

  const auto encoderPosition = encoderGetPosition();

  EVERY_N_MILLISECONDS(200) {
    frameIndex = (frameIndex + 1) % ANIMATION_NUM_FRAMES;
    snprintf(filename, sizeof(filename), "%s/frame_%08d.bin", base_dirs[video_index], frameIndex);
    updateImageFrame(filename);
  }

  // TODO: Move drawing to separate function
  const float angle = fmod(encoderPosition, 180);
  const auto y_cursor_opt = ledsAngleToYCurser(angle);
  if (!y_cursor_opt) {
    std::copy(std::begin(blackRow), std::end(blackRow), std::begin(ledStrip));
  } else {
    const auto cursor = y_cursor_opt.value();
    std::copy(std::begin(imageFrame[cursor]), std::end(imageFrame[cursor]), ledStrip);
  }
}

bool esp_connected = false;
unsigned long time_considered_as_disconnection = 0;
const int esp_dead_timeout = 5000;

bool read_esp_data(cmd_t &esp_packet) {
  bool valid_packet = 0;

  if (esp_serial.available()) {
    time_considered_as_disconnection = millis() + esp_dead_timeout;
//    Serial.println("received command");
    esp_connected = true;
    valid_packet = read_esp_command(esp_packet);
//    Serial.printf("packet is %d", valid_packet);
  }

  if (esp_connected && millis() > time_considered_as_disconnection) {
    esp_connected = false;
    Serial.printf("esp disconnect");
  }
  esp_packet.esp_connected = esp_connected;
  return valid_packet;
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

int video_index = 0;

void loop() {
//  read_esp_data(esp_command);
//  if (esp_command.esp_connected) {
//    Serial.printf("display is %d\n", bool(esp_command.display_on));
//    esp_controller_loop(esp_command);
//  } else {
  EVERY_N_SECONDS(120) {
    const auto should_troll = random(10) >= 8;
    const auto should_display_center_eye = random(10) < 8;
    const auto next_index =
        should_troll ? random(4, 8) :
        should_display_center_eye ? 0
                                  : random(1, 4);

    video_index = next_index;
  }
  display_video(video_index);
//  }

  // Uncomment to simulate interrupts
#if DEBUG_SIMULATE_INTERRUPTS
  EVERY_N_MILLISECONDS(100) {
    encoderInterrupt();
  }
#endif  // DEBUG_SIMULATE_INTERRUPTS

  FastLED.show();
}
