#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <DHT.h>
#include <SFE_BMP180.h>
#include <Wire.h>
#include <BH1750.h>

// The DHT data line is connected to pin 2 on the Arduino
#define DHTPIN 2

// Leave as is if you're using the DHT22. Change if not.
//#define DHTTYPE DHT11   // DHT 11 
#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define ALTITUDE 180

// Wind measuring parameters
#define WIND_TIME 1000
#define WIND_SENSOR_PIN 3
#define WIND_DELAY 1

// Define message types
#define METEO              0
#define MOISTURE1          1
#define HUMIDITY1          2
#define TEMPERATURE1       3
#define CSS811_TEMP        4
#define CSS811_CO2         5 
#define CSS811_TVOC        6
#define WIND_SPEED_SENSOR  7

SFE_BMP180 pressure;
BH1750 lightMeter;


DHT dht(DHTPIN, DHTTYPE);

// Radio with CE & CSN connected to 7 & 8
RF24 radio(7, 8);
RF24Network network(radio);

// Constants that identify this node and the node to send data to
const uint16_t this_node = 1;
const uint16_t parent_node = 0;
volatile int f_wdt=1;

// Time between packets (in ms)
const unsigned long interval = 2000;
int counts = 0;

// Structure of meteo message
struct message_t {
    float temp;
    float humidity;
    float pressure;
    float lux;
} meteoMsg;

// Structure of wind speed message
struct message_w {
    float windSpeed;
} windMsg;

// The network header initialized for this node
RF24NetworkHeader header(parent_node);


// Measures wind speed based on number of pulses in NPN sensor
float measureWindSpeed() {
  int total = 0, pulses = 0;
  float val = digitalRead(WIND_SENSOR_PIN), lastval;
  // Measure every single signal change in WIND_TIME ms
  while (total < WIND_TIME) {
    delay(WIND_DELAY);
    val = digitalRead(WIND_SENSOR_PIN); //connect sensor to Analog 0
    if (val != lastval) {
      pulses++;
    }
    lastval = val;
    total += WIND_DELAY;
  }
  return pulses * 1.75 / (20 * WIND_TIME / 1000);
}


void setup(void)
{
  
  // Initialize all radio related modules
  SPI.begin();
  radio.begin();
  delay(5);
  network.begin(90, this_node);

  // Initialize the DHT library
  
  dht.begin();
  
  if (lightMeter.begin())
    Serial.println("BH1750 init success");
  else
  {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.

    Serial.println("BH1750 init fail\n\n");
    while(1); // Pause forever.
  }
  
  if (pressure.begin())
    Serial.println("SFE_BMP180 init success");
  else
  {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.

    Serial.println("BMP180 init fail\n\n");
    while(1); // Pause forever. 
  }
  
}

void loop() {
  // Update network data
  network.update();
  
  char status;
  double T,P;
  uint16_t id;
  float p = 0;
  
  status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Function returns 1 if successful, 0 if failure.

    status = pressure.getTemperature(T);
    if (status != 0)
    {
      status = pressure.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = pressure.getPressure(P,T);
        if (status != 0)
        {
          P = pressure.sealevel(P, ALTITUDE);    
        }
        else Serial.println("error retrieving pressure measurement\n");
      }
      else Serial.println("error starting pressure measurement\n");
    }
    else Serial.println("error retrieving temperature measurement\n");
  }
  else Serial.println("error starting temperature measurement\n");
  
  
  // Read humidity (percent)
  float h = dht.readHumidity();
  // Read temperature as Celsius
  float t = dht.readTemperature();
  header.type = METEO;
  float l = lightMeter.readLightLevel();
  meteoMsg = (message_t){t, h, float(P), l};
  
  // Writing the message to the network
  if (!network.write(header, &meteoMsg, sizeof(meteoMsg))) {
    Serial.print("Could not send the meteo message\n"); 
  }

  // Measure the wind speed
  float windSpeed = measureWindSpeed();
  header.type = WIND_SPEED_SENSOR;
  windMsg = (message_w){windSpeed};
  // Writing the message to the network
  if (!network.write(header, &windMsg, sizeof(windMsg))) {
    Serial.print("Could not send the wind speed message\n"); 
  }
  /* Delay. */
  delay(120000); 
}
