#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

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

const byte commRx = 2;
const byte commTx = 3;
// Set up a new SoftwareSerial object
SoftwareSerial teensy_serial(commRx, commTx);

const char *ssid = "EspAcessPoint";
const char *password = "12345678";
ESP8266WebServer server(80);

// void sendCommand(cmd_t command)
// {
//   DEBUG_PRINTF("sending command: display_on=%d, eye_azimuth=%d, display_custom_text=%d, custom_text_data=%s\n", command.display_on, command.eye_azimuth, command.display_custom_text, command.custom_text_data.c_str());
//   JsonDocument doc;
//   doc["display_on"] = command.display_on;
//   doc["eye_azimuth"] = command.eye_azimuth;
//   doc["display_custom_text"] = command.display_custom_text;
//   doc["custom_text_data"] = command.custom_text_data;

//   String output;
//   serializeJson(doc, output);
//   teensy_serial.write(output.c_str());
// }

void handleJsonClient()
{

  DEBUG_PRINTF("received request - ");
  if (server.method() != HTTP_POST)
  {
    DEBUG_PRINTF("but cant handle non-post reqs!\n");
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  if (!server.hasArg("plain"))
  {
    DEBUG_PRINTF("but the packet is empty\n");
    server.send(400, "text/plain", "Bad Request: No data received");
    return;
  }
  DEBUG_PRINTF("\n");

  String json_string = server.arg("plain");
  teensy_serial.write(json_string.c_str());

  server.send(200, "text/plain", "OK");
}

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.println("Access point created");
  Serial.println("IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/json_client", handleJsonClient);
  server.begin();
}

void loop()
{
  server.handleClient();
}