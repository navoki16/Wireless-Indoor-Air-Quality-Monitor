#include <Arduino.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <ccs811.h>
#include <Adafruit_BME280.h>

#define INTERVAL 1000
#define CO2_THRESHOLD 1000
#define TVOC_THRESHOLD 1000
#define TVOC_THRESHOLD1 100
#define TVOC_THRESHOLD2 10
#define PORT 1611

unsigned long runTime = 0;
uint16_t lastCO2 = 0;
uint16_t lastTVOC = 0;
const char *ssid = "*your wifi ssid";
const char *password = "your wifi password";
const char *serverAddress = "*your reader device's ip address*";

CCS811 ccs811(-1, CCS811_SLAVEADDR_0);
Adafruit_BME280 bme;
LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiClient TCPsender;

void setup()
{
  unsigned status;

  lcd.init();
  lcd.backlight();

  status = Wire.begin();
  while (!status)
  {
    lcd.setCursor(0, 0);
    lcd.print("problem with I2C");
  }

  status = bme.begin();
  while (!status)
  {
    lcd.setCursor(0, 0);
    lcd.print("problem with BME");
  }
  bme.setSampling(bme.MODE_NORMAL, bme.SAMPLING_X2, bme.SAMPLING_X16, bme.SAMPLING_X1, bme.FILTER_X16, bme.STANDBY_MS_500);

  status = ccs811.begin();
  while (!status)
  {
    lcd.setCursor(0, 0);
    lcd.print("problem with CCS");
  }
  ccs811.start(CCS811_MODE_1SEC);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    lcd.setCursor(0, 0);
    lcd.print("Connecting WiFi..");
  }
  lcd.clear();
  TCPsender.connect(serverAddress, PORT);

  setCpuFrequencyMhz(160); //MHz
}

void loop()
{
  if (millis() - runTime > INTERVAL)
  {
    if (!TCPsender.connected())
    {
      TCPsender.connect(serverAddress, PORT);
    }

    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();

    ccs811.set_envdata_Celsius_percRH(temperature, humidity);
    uint16_t eco2, etvoc, errstat, raw;
    ccs811.read(&eco2, &etvoc, &errstat, &raw);

    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temperature);
    lcd.setCursor(0, 1);
    lcd.print("H:");
    lcd.print(humidity);
    lcd.print("%");
    lcd.setCursor(8, 0);
    lcd.print("CO2:");
    lcd.print(eco2);
    lcd.setCursor(9, 1);
    lcd.print("VC:");
    lcd.print(etvoc);

    if ((eco2 >= CO2_THRESHOLD && lastCO2 <= CO2_THRESHOLD) ||
        (etvoc >= TVOC_THRESHOLD && lastTVOC <= TVOC_THRESHOLD) ||
        (etvoc >= TVOC_THRESHOLD1 && lastTVOC <= TVOC_THRESHOLD1) ||
        (etvoc >= TVOC_THRESHOLD2 && lastTVOC <= TVOC_THRESHOLD2))
    {
      lastCO2 = eco2;
      lastTVOC = etvoc;
    }
    else if ((eco2 < CO2_THRESHOLD && lastCO2 >= CO2_THRESHOLD) ||
             (etvoc < TVOC_THRESHOLD && lastTVOC >= TVOC_THRESHOLD) ||
             (etvoc < TVOC_THRESHOLD1 && lastTVOC >= TVOC_THRESHOLD1) ||
             (etvoc < TVOC_THRESHOLD2 && lastTVOC >= TVOC_THRESHOLD2))
    {
      lcd.clear();
      lastCO2 = eco2;
      lastTVOC = etvoc;
    }

    TCPsender.print("Temperature: " + (String)temperature + "°C," + "Humidity: " + (String)humidity + "%," + "CO2: " + (String)eco2 + "ppm," + "TVOC: " + (String)etvoc + "ppb");

    runTime = millis();
  }
}