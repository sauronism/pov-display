#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

const byte commRx = 2;
const byte commTx = 3;
// Set up a new SoftwareSerial object
SoftwareSerial teensy_serial(commRx, commTx);

const char *ssid = "EspAcessPoint";
const char *password = "12345678";

ESP8266WebServer server(80);

void sendSerialData(String data)
{
  teensy_serial.write(data.c_str());
}
void handleJsonClient()
{
  Serial.println("received request");
  if (server.method() != HTTP_POST)
  {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  if (!server.hasArg("plain"))
  {
    server.send(400, "text/plain", "Bad Request: No data received");
    return;
  }

  String json = server.arg("plain");
  sendSerialData(json);
  Serial.println(json);

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