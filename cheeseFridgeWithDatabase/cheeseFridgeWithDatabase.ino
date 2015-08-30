#include "DHT.h"
#include <SPI.h>
#include <Ethernet.h>

// Sensor setup
DHT dhtOne(2, DHT22);
DHT dhtTwo(3, DHT22);

// Ethernet setup
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 99);  
EthernetClient client;
IPAddress dbIP(192, 168, 1, 65);
const int dbPort = 7777;

// Time interval
const long interval = 10000;

// Relay pins
const int fridgeOnePin = 5;
const int pumpOnePin = 6;
const int fridgeTwoPin = 7;
const int pumpTwoPin = 8;

// Min / max temperatures and humidity
const float minTemp1 = 8;
const float maxTemp1 = 15;
const float minHumidity1 = 80;
const float maxHumidity1 = 95;
const float minTemp2 = 5;
const float maxTemp2 = 10;
const float minHumidity2 = 90;
const float maxHumidity2 = 95;

void setup()
{
  Serial.begin(9600);

  // Initialise ethernet
  startEthernet();
  
  // Initialise sensors
  dhtOne.begin();
  dhtTwo.begin();

  // Initialise everything to OFF (HIGH)
  pinMode(fridgeOnePin, OUTPUT);
  digitalWrite(fridgeOnePin, HIGH);
  pinMode(pumpOnePin, OUTPUT);
  digitalWrite(pumpOnePin, HIGH);
  pinMode(fridgeTwoPin, OUTPUT);
  digitalWrite(fridgeTwoPin, HIGH);
  pinMode(pumpTwoPin, OUTPUT);
  digitalWrite(pumpTwoPin, HIGH);
}

void loop()
{
  // Read sensor data
  float currentHumidity1  = dhtOne.readHumidity();
  float currentTemp1 = dhtOne.readTemperature();
  float currentHumidity2 = dhtTwo.readHumidity();
  float currentTemp2 = dhtTwo.readTemperature();
  
  // Check temperatures and humidity, switching on where necessary
  // Only switch one thing on per interval
  boolean justSwitchedOn = false;     
  justSwitchedOn = checkTemperature(fridgeOnePin, currentTemp1, minTemp1, maxTemp1);
  if (!justSwitchedOn)  { justSwitchedOn = checkHumidity(pumpOnePin, currentHumidity1, minHumidity1, maxHumidity1);  }
  if (!justSwitchedOn)  { justSwitchedOn = checkTemperature(fridgeTwoPin, currentTemp2, minTemp2, minTemp2);  }
  if (!justSwitchedOn)  { checkHumidity(pumpTwoPin, currentHumidity2, minHumidity2, maxHumidity2);  }

  // Send to server if available, otherwise write to log
  sendData(currentTemp1, currentHumidity1, currentTemp2, currentHumidity2);

  delay(interval);
}

boolean checkTemperature(int fridgePin, float temperature, float minTemp, float maxTemp)
{
  // Figure out what to do with fridge
  if (temperature >= maxTemp && digitalRead(fridgePin) == HIGH)
  {
    Serial.println(F("Turning fridge ON as temperature >= maxTemp"));
    // Temperature above maximum, fridge pin HIGH (i.e. off)
    digitalWrite(fridgePin, LOW);
    return true;
  }
  else if (temperature < minTemp && digitalRead(fridgePin) == LOW)
  {
    Serial.println(F("Turning fridge OFF as temperature < maxTemp"));
    // Temperature below minimum, fridge pin LOW (i.e. on)
    digitalWrite(fridgePin, HIGH);
  }
  return false;
}

boolean checkHumidity(int pumpPin, float humidity, float minHumidity, float maxHumidity)
{
  if (humidity < minHumidity && digitalRead(pumpPin) == HIGH)
  {
    // Humidity below minimum, pump pin HIGH (i.e. off)
    Serial.println(F("Turning pump ON as humidity < minHumidity"));
    digitalWrite(pumpPin, LOW);
    return true;
  }
  else if (humidity >= maxHumidity && digitalRead(pumpPin) == LOW)
  {
    Serial.println(F("Turning pump OFF as humidity >= maxHumidity"));
    // Humidity above maximum, pump pin LOW (i.e. on)
    digitalWrite(pumpPin, HIGH);
  } 
  return false;
}

void sendData(float currentTemp1, float currentHumidity1, float currentTemp2, float currentHumidity2)
{
  // Don't do anything if junk data
  if (isnan(currentTemp1) || isnan(currentHumidity1) || isnan(currentTemp2) || isnan(currentHumidity2))
  {
    Serial.println(F("Received invalid data from sensor")); 
  }

  // Send data to server  
  if (client.connect(dbIP, dbPort) == 1)
  {
    Serial.println(F("Sending to server ..."));
    char dataString[32], strTemp1[6], strTemp2[6], strHumidity1[6], strHumidity2[6];
    int fridgeOne = 0, fridgeTwo = 0, pumpOne = 0, pumpTwo = 0;

    // sprintf doesn't support floats
    dtostrf(currentTemp1, 4, 2, strTemp1);
    dtostrf(currentTemp2, 4, 2, strTemp2);
    dtostrf(currentHumidity1, 4, 2, strHumidity1);
    dtostrf(currentHumidity2, 4, 2, strHumidity2);

    // Relay is HIGH when it's off, LOW when it's on
    if (digitalRead(fridgeOnePin) == LOW)  { fridgeOne = 1;  }
    if (digitalRead(pumpOnePin) == LOW)    { pumpOne = 1;    }
    if (digitalRead(fridgeTwoPin) == LOW)  { fridgeTwo = 1;  }
    if (digitalRead(pumpTwoPin) == LOW)    { pumpTwo = 1;    }        
    
    sprintf(dataString, "%s,%d,%s,%d,%s,%d,%s,%d", strTemp1, fridgeOne, strHumidity1, pumpOne, strTemp2, fridgeTwo, strHumidity2, pumpTwo);
    client.println(dataString);
    delay(500);
    client.stop();
  }
  else
  {
     Serial.println(F("Could not connect to server"));
     startEthernet();
  }
}

void startEthernet()
{
  Serial.println(F("Connecting ethernet ..."));
  Ethernet.begin(mac, ip);
  delay(500);
}
