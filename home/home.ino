#include <RF24.h>
#include <SPI.h>
#include <Wire.h>
#include <dht.h>
#include <RF24Network.h>

#define dht_apin A0 // Analog Pin sensor is connected to 
#define moisture_pin A1

// Define the connections pins:
#define CLK 2
#define DIO 3

// Define message types
#define METEO        0
#define MOISTURE1    1
#define HUMIDITY1    2
#define TEMPERATURE1 3
#define CSS811_TEMP  4
#define CSS811_CO2   5 
#define CSS811_TVOC  6 


int sensor_pin = A0;
// Relay's pin
int hSensorSwitch = 5;

// Humidity/temperature sensor
dht DHT;

// Radio with CE & CSN connected to 7 & 8
RF24 radio(7, 8);
RF24Network network(radio);

// Pumping time in ms
const uint16_t pumpTime = 25000;

// Wait time after pump in seconds
const uint16_t waitAfterPump = 60000;

// Measure delay in ms 
const uint32_t measureDelay = 120000;//240000;

// ThresholLOWd below which the pump is activated
const int moistureThreshold = 40;

// Constants that identify this node and the node to send data to
const uint16_t this_node = 2;
const uint16_t parent_node = 0;

// Time between packets (in ms)
const unsigned long interval = 2000;

// Structure of our message
struct message_t {
  float value;
};

message_t message;

// The network header initialized for this node
RF24NetworkHeader header(parent_node);


void send_message(int htype, float value) {
  delay(interval);
  network.update();
  header.type = htype;
  message = (message_t){value};
  // Writing the message to the network means sending it
  if (!network.write(header, &message, sizeof(message))) {
    Serial.print("Could not send the message\n");
  } else {
    Serial.print("Message sent.\n");
  }
}

void setup(void)
{
  Serial.begin(9600);
  Serial.println("Started");
  pinMode(hSensorSwitch, OUTPUT);
  // Initialize all radio related modules
  SPI.begin();
  radio.begin();
  delay(200);
  network.begin(90, this_node);
  delay(200);
}

void loop() {
  // Do moisture reading
  digitalWrite(hSensorSwitch, LOW);
  delay(50);
  int moisture = analogRead(moisture_pin);
  Serial.println(moisture);
  delay(10);
  digitalWrite(hSensorSwitch, HIGH);
  // Send the moisture message
  send_message(MOISTURE1, moisture);
  // Do temp/humidity reading
  DHT.read11(dht_apin);
  float temperature =  DHT.temperature;
  float humidity = DHT.humidity;
  // Send temperature message
  send_message(TEMPERATURE1, temperature);
  // Send the humidity message
  send_message(HUMIDITY1, humidity);
  /* Delay. */
  delay(measureDelay);
}
