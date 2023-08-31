// --- leds_hal --- //

FASTLED_USING_NAMESPACE


#define DATA_PIN 3
#define CLK_PIN 4
#define LED_TYPE APA102
#define COLOR_ORDER GRB
#define NUM_LEDS 300
CRGB ledStrip[NUM_LEDS];

#define BRIGHTNESS 100

void ledsSetup() {
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLK_PIN, COLOR_ORDER>(ledStrip, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
}

void updateLedStrip() {
  FastLED.show();
}
