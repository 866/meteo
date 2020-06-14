#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <iostream>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mysql.h> 


#define METEO 0
#define MOISTURE1 1
#define VERBOSE 0
#define MAX_TRIES 20

using namespace std; 

// MySQL variables
MYSQL *conn, mysql;
MYSQL_RES *res;
MYSQL_ROW row;

// CE Pin, CSN Pin, SPI Speed
RF24 radio(RPI_BPLUS_GPIO_J8_15,RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_8MHZ);

RF24Network network(radio);

// Constants that identifies this node
const uint16_t pi_node = 0;

// Time between checking for packets (in ms)
const unsigned long interval = 20;

// Number of messages sent(successful + failed)
unsigned int count = 0;

// Structure of our message
struct message_t {
    float temperature;
    float humidity;
    float pressure;
    float lux;
};

struct message_moisture {
    float value;
};


MYSQL* opendb() {
   // Opens the connection to MySQL database
   const char *server="localhost";
   const char *user="meteopi";
   const char *password="ipoetem";
   const char *database="home";
   mysql_init(&mysql);
   conn = mysql_real_connect(&mysql, server, user, password, database, 0, 0, 0);
   if(conn == NULL)
   {
       cout << mysql_error(&mysql) << endl << endl;
      return 0;
   }
   return conn;
}


int executeQuery(MYSQL* conn, char* query) {
   /* Execute SQL statement */
   int query_state = mysql_query(conn,	query);
   if(query_state != 0){
      cout << mysql_error(conn) << endl << endl; 
      return -1;
   }
   return 0; 
}


int writeValues(message_t& msg, MYSQL* conn) {
   char query[500];
   long int t = static_cast<long int>(std::time(NULL)); 
   sprintf(query, "INSERT INTO meteo (timestamp, temperature, humidity, pressure, light) "  \
         "VALUES (%li, %.1f, %.1f, %.1f, %.1f); ", t, msg.temperature, msg.humidity, 
         msg.pressure, msg.lux);
   return executeQuery(conn, query);
}


int writeValues(message_moisture& msg, int sensor, MYSQL* conn) {
   char query[500];
   long int t = static_cast<long int>(std::time(NULL)); 
   sprintf(query, "INSERT INTO home (timestamp, sensor, value) "  \
         "VALUES (%li, %d, %.0f); ", t, sensor, msg.value);
   return executeQuery(conn, query);
}


int main(int argc, char** argv)
{
	// Open the database
        MYSQL* conn = opendb();
        if (conn == 0) {
           fprintf(stdout, "Aborting");
           return 0;
        }
  
	// Initialize all radio related modules
	radio.begin();
	delay(5);
	network.begin(90, pi_node);
	
	// Print some radio details (for debug purposes)
	radio.printDetails();
	cout << "Ready to receive...\n";
	
	// Now do this forever (until cancelled by user)
	while(1)
	{
		// Get the latest network info
		network.update();
		// Enter this loop if there is data available to be read,
		// and continue it as long as there is more data to read
		while ( network.available() ) {
			RF24NetworkHeader header;
			message_t message;
			message_moisture message_moisture1;
			// Have a peek at the data to see the header type
			network.peek(header);
			count++;
			if (VERBOSE) {
                            printf("Messages: %i\nheader type: %d\nnode: %d\n", count, header.type, header.from_node);

			}
			// We can only handle the Temperature type for now
			if (header.type == METEO) {
				// Read the message
				network.read(header, &message, sizeof(message));
				if (VERBOSE) {
				   // Print it out
				   printf("Message received from node %i:\nTemperature: %0.1f*C\nHumidity: %0.1f%%\nPressure: %0.1f mb\nLight: %0.2f lx\n\n", header.from_node, message.temperature, message.humidity, message.pressure, message.lux);
				}
				int failed=0;
				while ((writeValues(message, conn) == -1) && (failed <= MAX_TRIES)){
					delay(500);
					failed++;
				}
				if (failed > MAX_TRIES) {
				   fprintf(stdout, "Aborting. Exceeded number of unsuccessful tries.");
                                   mysql_close(conn);
				   return 0;				 
				}
			}
			else if (header.type == MOISTURE1) {
				network.read(header, &message_moisture1, sizeof(message_moisture1));
				if (VERBOSE) {
					printf("Message received from node %i:\nMoisture: %0.0f\n\n", header.from_node, message_moisture1.value);
				}
				int failed = 0;
				while ((writeValues(message_moisture1, MOISTURE1, conn) == -1) && (failed <= MAX_TRIES)) {
					delay(500);
					failed++;
				}
				if (failed > MAX_TRIES) {
				   fprintf(stdout, "Aborting. Exceeded number of unsuccessful tries.");
                                   mysql_close(conn);
				   return 0;				 
				}

			}
		}
	
		// Wait a bit before we start over again
		delay(interval);
	}

	// last thing we do before we end things
        mysql_close(conn);
	return 0;
}
