#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>

// debugging
#define DEBUG_ENABLED true
#define DEBUG_PRINTF(...)       \
  if (DEBUG_ENABLED)            \
  {                             \
    Serial.printf(__VA_ARGS__); \
  }

const byte commRx = D5;
const byte commTx = D6;
// Set up a new SoftwareSerial object
SoftwareSerial teensy_serial(commRx, commTx);

const char *ssid = "EspAcessPoint";
const char *password = "12345678";
ESP8266WebServer server(80);

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