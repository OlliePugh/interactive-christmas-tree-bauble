#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "WifiSecrets.h"
#include <SPI.h>
#include <TFT_eSPI.h>

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;

// Your Domain name with URL path or IP address with path
String serverName = FULL_BAUBLE_URL;

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

TFT_eSPI tft = TFT_eSPI(); // Invoke library, pins defined in User_Setup.h

void setup()
{
  Serial.begin(9600);
  tft.init();
  tft.fillScreen(TFT_BLACK);

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

  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

void loop()
{
  // Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay)
  {
    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
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
            tft.drawPixel(i, j, tft.color565(payload[currentIndex], payload[currentIndex + 1], payload[currentIndex + 2])); // blue green red
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
    else
    {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}