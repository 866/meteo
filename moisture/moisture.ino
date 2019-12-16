#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <Wire.h>

#define METEO 0
#define MOISTURE1 1

int sensor_pin = A0;
int moisture;

// Radio with CE & CSN connected to 7 & 8
RF24 radio(7, 8);
RF24Network network(radio);

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



void setup(void)
{
  // Initialize all radio related modules
  SPI.begin();
  radio.begin();
  delay(5);
  network.begin(90, this_node);
}

void loop() {
  // Update network data
  network.update();
  header.type = MOISTURE1;

  // Read values from sensor
  moisture = analogRead(sensor_pin);
  moisture = map(moisture,740,355,0,100);
  // Send the message
  message = (message_t){moisture};
  // Writing the message to the network means sending it
  if (!network.write(header, &message, sizeof(message))) {
    Serial.print("Could not send message\n"); 
  }
   /* Delay. */
   delay(10000); 
}
