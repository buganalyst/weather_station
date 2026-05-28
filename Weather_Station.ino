#include <ESP8266WiFi.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <DHT.h>
#include <Adafruit_BMP085.h>

// WIFI
const char* ssid = "Moto";
const char* password = "12345678";

// THINGSPEAK
String apiKey = "IJNYVAH0TQAPMKP4";
const char* server = "api.thingspeak.com";

// OLED
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// DHT
#define DHTPIN D3
#define DHTTYPE DHT11
#define BUZZER D5
DHT dht(DHTPIN, DHTTYPE);

// BMP180
Adafruit_BMP085 bmp;

WiFiClient client;

void setup() {
  Serial.begin(115200);

  Wire.begin(D2, D1);

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW); // OFF (ACTIVE LOW BUZZER)

  u8g2.begin();
  u8g2.setI2CAddress(0x3C * 2);

  dht.begin();

  if (!bmp.begin()) {
    Serial.println("BMP ERROR");
    while (1);
  }

  // ===== WIFI FIX =====
  WiFi.mode(WIFI_STA);   // force station mode
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    timeout++;

    if (timeout > 40) { // ~20 sec timeout
      Serial.println("\nWiFi FAILED");
      return;
    }
  }

  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0, 20, "WiFi Connected");
  u8g2.sendBuffer();
}

void loop() {

  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  float pressure = bmp.readPressure() / 100.0;
  float altitude = bmp.readAltitude();

  // ===== DHT FIX =====
  if (isnan(temp) || isnan(hum)) {
    Serial.println("DHT ERROR");
    return; // don't use garbage values
  }

  Serial.print("Temp: ");
  Serial.println(temp);

  // ===== BUZZER FIX (ACTIVE LOW) =====
  if (temp > 30) digitalWrite(BUZZER, HIGH);  // ON
  else digitalWrite(BUZZER, LOW);           // OFF

  // OLED
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tr);

  u8g2.drawStr(3, 10, "Weather Station");

  u8g2.setCursor(3, 25);
  u8g2.print("Temp: "); u8g2.print(temp); u8g2.print(" C");

  u8g2.setCursor(3, 38);
  u8g2.print("Hum : "); u8g2.print(hum); u8g2.print(" %");

  u8g2.setCursor(3, 51);
  u8g2.print("Pres: "); u8g2.print(pressure); u8g2.print(" hPa");

  u8g2.setCursor(3, 64);
  u8g2.print("Alt : "); u8g2.print(altitude); u8g2.print(" m");

  u8g2.sendBuffer();

  // ===== THINGSPEAK =====
  if (WiFi.status() == WL_CONNECTED) {

    if (client.connect(server, 80)) {

      String url = "/update?api_key=" + apiKey +
                   "&field1=" + String(temp) +
                   "&field2=" + String(hum) +
                   "&field3=" + String(pressure) +
                   "&field4=" + String(altitude);

      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: " + server + "\r\n" +
                   "Connection: close\r\n\r\n");

      Serial.println("Data sent to ThingSpeak");
    } else {
      Serial.println("Connection to server failed");
    }
  } else {
    Serial.println("WiFi disconnected");
  }

  client.stop();

  delay(15000);
}