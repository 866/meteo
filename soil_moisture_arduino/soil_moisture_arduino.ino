#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <Wire.h>

// Define the connections pins:
#define CLK 2
#define DIO 3
#define METEO 0
#define MOISTURE1 1

int sensor_pin = A0;
// Relay's pin
int pumpSwitch = 6;
int hSensorSwitch = 5;

// Radio with CE & CSN connected to 7 & 8
RF24 radio(7, 8);
RF24Network network(radio);

// Pumping time in seconds
const uint16_t pumpTime = 30;

// Measure delay in ms 
const uint16_t measureDelay = 1000;

// Threshold below which the pump is activated
const int moistureThreshold = 40;

// Constants that identify this node and the node to send data to
const uint16_t this_node = 2;
const uint16_t parent_node = 0;

// Time between packets (in ms)
const unsigned long interval = 2000;

// Structure of our message
struct message_t {
  float moisture;
};

message_t message;

// The network header initialized for this node
RF24NetworkHeader header(parent_node);

int getMoistureReading() {
  // Turns on the reading sensors and reads the 
  // moisture value. Then turns it off
  digitalWrite(hSensorSwitch, LOW);
  // Wait for current flow and stabilizing of readings  
  delay(500);
  // Read the value
  int moisture = analogRead(sensor_pin);
  // Normalise the read value
  moisture = map(moisture, 800, 420, 0, 100);
  moisture = min(moisture, 100);
  moisture = max(moisture, 0);
  // Turn off the sensor
  digitalWrite(hSensorSwitch, HIGH);
  return moisture;
}

void setup(void)
{
  Serial.begin(9600);
  Serial.println("Started");

  // Initialize all radio related modules
  SPI.begin();
  radio.begin();
  delay(5);
  network.begin(90, this_node);
  
  // Pin for relay module set as output
  pinMode(pumpSwitch, OUTPUT);
  pinMode(hSensorSwitch, OUTPUT);
  digitalWrite(pumpSwitch, HIGH);
  digitalWrite(hSensorSwitch, HIGH);

}

void loop() {
  // Do moisture reading
  int moisture = getMoistureReading();

 // Update network data
 network.update();
 header.type = MOISTURE1;

  message = (message_t){moisture};
  // Writing the message to the network means sending it
  if (!network.write(header, &message, sizeof(message))) {
    Serial.print("Could not send the message\n");
  } else {
    Serial.print("Message sent.\n");
  }

  // Delay for successful sending
  delay(1000);
  
  // Turn off/on the pump if we need soil humification
  if (moisture < moistureThreshold) {
      // Turn off the pump
      digitalWrite(pumpSwitch, LOW);
  } else {
      // Turn on the pump
      digitalWrite(pumpSwitch, HIGH);
      delay(pumpTime);
  }
  
  /* Delay. */
  delay(measureDelay);
}
