#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <TFT_eSPI.h>

#include "WifiSecrets.h"

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;

// Your Domain name with URL path or IP address with path
String serverName = FULL_BAUBLE_URL;
AsyncWebServer server(80); // Object of WebServer(HTTP port, 80 is defult)

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
unsigned long timerDelay = 60000 * 30;

TFT_eSPI tft = TFT_eSPI(); // Invoke library, pins defined in User_Setup.h

void drawPixel(int row, int col, uint16_t colour)
{
  row = TFT_WIDTH - row - 1;
  tft.drawPixel(row, col, colour); // blue green red
}

void setLightColour(int index, const String *hex)
{
  int colourHex = (int)strtol(hex->c_str(), NULL, 16);

  // Split them up into r, g, b values
  int r = colourHex >> 16;
  int g = colourHex >> 8 & 0xFF;
  int b = colourHex & 0xFF;
  // const id = width * currentClickPos.row + currentClickPos.col;
  int col = index % (TFT_HEIGHT);
  int row = index / (TFT_HEIGHT);

  Serial.printf("Updating light at col %d and row %d to colour %s\n", col, row, hex);
  drawPixel(row, col, tft.color565(r, g, b)); // blue green red
}

void handleRoot(AsyncWebServerRequest *request)
{
  if (!request->hasHeader("Authorization"))
  {
    request->send(401);
    return;
  }

  AsyncWebHeader *h = request->getHeader("Authorization");
  if (h->value() != API_KEY) // does it have the api key
  {
    request->send(401);
    return;
  }

  if (!request->hasParam("colour") || !request->hasParam("light-id")) // does it have the required params
  {
    request->send(400);
    return;
  }

  AsyncWebParameter *colour = request->getParam("colour");
  AsyncWebParameter *bulbId = request->getParam("light-id");
  int bulbIdInt = bulbId->value().toInt();
  setLightColour(bulbIdInt, &colour->value());
  request->send(200);
}

void initialiseBoard()
{
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(serverName.c_str());

  // If you need Node-RED/server authentication, insert user and password below
  // http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");

  // Send HTTP GET request
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    const char *payload = (http.getString()).c_str(); // get the stream of bytes

    const int offset = 54; // TODO dynamically get this from the file and then everything should work!
    int counter = 0;
    for (int32_t i = 0; i < TFT_WIDTH; i++)
    {
      for (int32_t j = 0; j < TFT_HEIGHT; j++)
      {
        // tft.drawPixel(j, i, tft.color565(0, 255, 0)); // blue green red)
        int currentIndex = offset + (counter++ * 3);
        drawPixel(i, j, tft.color565(payload[currentIndex + 2], payload[currentIndex + 1], payload[currentIndex])); // blue green red
      }
    }
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
}

void setup()
{
  Serial.begin(9600);
  tft.init();
  tft.fillScreen(TFT_GREEN); // BLUE = RED // RED = BLUE

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);

  server.begin();
  delay(100);
  initialiseBoard();

  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

void loop()
{
  // for (int32_t i = 0; i < TFT_WIDTH; i++)
  // {
  //   drawPixel(0, i, TFT_BLUE); // blue green red
  // }
  // Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay)
  {
    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {
      initialiseBoard(); // get a full refresh
    }
    else
    {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}
