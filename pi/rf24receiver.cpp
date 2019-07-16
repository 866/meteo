#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <iostream>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sqlite3.h> 


#define METEO 0
#define VERBOSE 0
#define MAX_TRIES 5
/**
 * g++ -L/usr/lib main.cc -I/usr/include -o main -lrrd
 **/

// CE Pin, CSN Pin, SPI Speed
RF24 radio(RPI_BPLUS_GPIO_J8_15,RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_8MHZ);

RF24Network network(radio);

// Constants that identifies this node
const uint16_t pi_node = 0;

// Time between checking for packets (in ms)
const unsigned long interval = 2000;

unsigned int count = 0;

// Structure of our message
struct message_t {
    float temperature;
    float humidity;
    float pressure;
    float lux;
};

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
   int i;
   for(i = 0; i<argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

sqlite3* opentable() {
   char *zErrMsg = 0;
 
   /* Open database */
   sqlite3* db;
   int rc = sqlite3_open("test.db", &db);
   
   if( rc ) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(0);
   } else {
      fprintf(stderr, "Opened database successfully\n");
   }

   
    /* Create SQL statement */
   const char* query = "CREATE TABLE meteo (" \
     "timestamp INTEGER," \
     "temperature REAL," \
     "humidity REAL," \
     "pressure REAL," \
     "light REAL);";

   /* Execute SQL statement */
   rc = sqlite3_exec(db, query, callback, 0, &zErrMsg);

   if( rc == SQLITE_OK ){
      fprintf(stdout, "Table created successfully\n");
   }

   return db;
}


int writeValues(message_t& msg, sqlite3* db) {
   char *zErrMsg = 0;
   char query[500];
   long int t = static_cast<long int>(std::time(NULL)); 

   sprintf(query, "INSERT INTO meteo (timestamp, temperature, humidity, pressure, light) "  \
         "VALUES (%li, %.1f, %.1f, %.1f, %.1f); ", t, msg.temperature, msg.humidity, 
         msg.pressure, msg.lux);

   /* Execute SQL statement */
   int rc = sqlite3_exec(db, query, callback, 0, &zErrMsg);
   
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
      return -1;
   }
   return 0; 
}

int main(int argc, char** argv)
{
        sqlite3 *db;
	// Open the table
        db = opentable();
        if (db == 0) {
           fprintf(stdout, "Aborting");
           return 0;
        }
  
	// Initialize all radio related modules
	radio.begin();
	delay(5);
	network.begin(90, pi_node);
	
	// Print some radio details (for debug purposes)
	radio.printDetails();
	printf("Ready to receive...\n");
	
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
			// Have a peek at the data to see the header type
			network.peek(header);
			count++;
			if (VERBOSE) {
                            printf("Messages: %i\n", count);
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
				while ((writeValues(message, db) == -1) && (failed <= MAX_TRIES)){
					delay(500);
					failed++;
				}
				if (failed > MAX_TRIES) {
				   fprintf(stdout, "Aborting. Exceeded number of unsuccessful tries.");
                                   sqlite3_close(db);
				   return 0;				 
				}
			}
		}
	
		// Wait a bit before we start over again
		delay(interval);
	}

	// last thing we do before we end things
        sqlite3_close(db);
	return 0;
}
