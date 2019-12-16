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

#define METEO 0

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

// Structure of our message
struct message_t {
    float temp;
    float humidity;
    float pressure;
    float lux;
};

message_t message;

// The network header initialized for this node
RF24NetworkHeader header(parent_node);



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
  message = (message_t){t, h, float(P), l};
  Serial.print(t);
 
  // Writing the message to the network means sending it
  if (!network.write(header, &message, sizeof(message))) {
    Serial.print("Could not send message\n"); 
  }
 
    /* Delay. */
   delay(5000); 
}
