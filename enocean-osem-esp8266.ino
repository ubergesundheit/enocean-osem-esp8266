#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>

#include "arduino_secrets.h"
#include <ArduinoHttpClient.h>

#include <WiFiClientSecure.h>

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
/////// Wifi Settings ///////
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

char serverAddress[] = "ingress.opensensemap.org"; // server address
int port = 443;

WiFiClientSecure wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);

SoftwareSerial swSer(13, 12); // D7, D6

String response;
int statusCode = 0;

byte rx_buffer[256];
char url_buffer[256];
char data_buffer[256];

void setup() {
  Serial.begin(115200);
  swSer.begin(57600);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

void postData() {
  Serial.println("making POST request");
  Serial.println(url_buffer);
  Serial.println(data_buffer);

  client.post(url_buffer, "text/csv", data_buffer);

  // read the status code and body of the response
  statusCode = client.responseStatusCode();
  response = client.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
}

const char url_buffer_template[] = "/boxes/%s/data";
const char data_buffer_template[] = "%s,%s\n%s,%s\n%s,%s";

uint8_t decodeRxBuffer() {
  Serial.println(F("start decoding"));
  // validate some bytes
  if (rx_buffer[0] != 0x55) {
    return 0;
  }

  Serial.println(F("has correct header"));

  // volt
  float volt = 0.0258039 * rx_buffer[7];
  Serial.println(F("decoded volt"));
  Serial.println(volt);

  // humidity
  float hum = (float)rx_buffer[8] / 2.5;
  Serial.println(F("decoded hum"));
  Serial.println(hum);

  // temperature
  float temp = 0.32 * (float)rx_buffer[9] - 20.0;
  Serial.println(F("decoded temp"));
  Serial.println(temp);

  Serial.println(F("determining device"));
  if (rx_buffer[11] == OUTDOOR_ADDRESS_BYTE_0 &&
      rx_buffer[12] == OUTDOOR_ADDRESS_BYTE_1 &&
      rx_buffer[13] == OUTDOOR_ADDRESS_BYTE_2 &&
      rx_buffer[14] == OUTDOOR_ADDRESS_BYTE_3) {
    Serial.println(F("is outdoor"));
    sprintf(url_buffer, url_buffer_template, OUTDOOR_SENSEBOX_ID);
    Serial.println(url_buffer);
    sprintf(data_buffer, data_buffer_template, OUTDOOR_VOLT_ID,
            String(volt, 1).c_str(), OUTDOOR_TEMP_ID, String(temp, 1).c_str(),
            OUTDOOR_HUMI_ID, String(hum, 1).c_str());
    Serial.println(data_buffer);

    return 1;
  }

  if (rx_buffer[11] == INDOOR_ADDRESS_BYTE_0 &&
      rx_buffer[12] == INDOOR_ADDRESS_BYTE_1 &&
      rx_buffer[13] == INDOOR_ADDRESS_BYTE_2 &&
      rx_buffer[14] == INDOOR_ADDRESS_BYTE_3) {
    Serial.println(F("is indoor"));
    sprintf(url_buffer, url_buffer_template, INDOOR_SENSEBOX_ID);
    Serial.println(url_buffer);
    sprintf(data_buffer, data_buffer_template, INDOOR_VOLT_ID,
            String(volt, 1).c_str(), INDOOR_TEMP_ID, String(temp, 1).c_str(),
            INDOOR_HUMI_ID, String(hum, 1).c_str());
    Serial.println(data_buffer);

    return 1;
  }

  return 0;
}

void loop() {
  while (swSer.available() > 0) {
    swSer.readBytes(rx_buffer, 24);
    // uncomment for datagram debugging
    // for (int i = 0; i < 24; i++) {
    //   Serial.print(i);
    //   Serial.print(": ");
    //   Serial.println(rx_buffer[i], HEX);
    // }
    Serial.println(F("call decoding"));
    if (decodeRxBuffer()) {
      postData();
      Serial.println(F("done uploading"));
    }
  }
}
