/*
  May 2017 - William Huster

  Code for a battery-powered arduino with a light sensor (photoresistor)
  and soil humidity sensor (custom built) attached.

  Thanks to:
   - Examples by TMRh20
   - Examples by J. Coliz <maniacbug@ymail.com>
*/

/* Enable/disable Serial prints */
#define MY_DEBUG

#include <SPI.h>
#include "RF24.h"
#include <printf.h>
#include "LowPower.h"
#include "Vcc.h"

/****************** User Config ***************************/
/* Give this device a unique address. */
uint8_t DEVICE_ADDRESS = 0x00;

/* The soil sensor must be powered by a digital output */
int SOIL_SENSOR_POWER_PIN = 5;

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 9 & 10 */
RF24 radio(9,10);
const int RADIO_POWER = RF24_PA_MIN;

/* Set up voltage measurements */
const float VCC_MIN        = 4.0*0.6;  // Minimum expected Vcc level, in Volts. Example for 2xAA Alkaline.
const float VCC_MAX        = 4.0*1.5;  // Maximum expected Vcc level, in Volts. Example for 2xAA Alkaline.
const float VCC_CORRECTION = 1.0/1.0;  // Measured Vcc by multimeter divided by reported Vcc

/**********************************************************/

//
Vcc vcc(VCC_CORRECTION);

// Topology
byte RX_PIPE[6] = "RECVR";
byte TX_PIPE[6] = "TRANS";

byte counter = 1;                               // A single byte to enumerate data packets


void setup(){

  Serial.begin(9600);
  printf_begin();

#ifdef MY_DEBUG
  Serial.println(F("Light and Soil Sensor"));
#endif

  pinMode(SOIL_SENSOR_POWER_PIN, OUTPUT);
 
  // Setup and configure radio

  radio.begin();

  radio.enableAckPayload();                     // Allow optional ack payloads
  radio.enableDynamicPayloads();                // Ack payloads are dynamic payloads
  radio.setChannel(0x60);
  radio.setPALevel(RADIO_POWER);

  radio.openWritingPipe(TX_PIPE);
  radio.openReadingPipe(1, RX_PIPE);

#ifdef MY_DEBUG
  radio.printDetails();                         // NOTE: printDetails() will only work with printf included
  Serial.flush();
#endif

  radio.startListening();                       // Start listening  

  radio.writeAckPayload(1,&counter,1);          // Pre-load an ack-payload into the FIFO buffer for pipe 1
 
}

int loop_count = 37;                            // Variable to count the number of sleep cycles, start by reporting status
void loop(void) {

  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);            // The sensor is usually asleep

  loop_count++;

#ifdef MY_DEBUG
  Serial.print(F("Woke Up, Loop Count: "));
  Serial.println(loop_count);
  Serial.flush();
#endif

  if (loop_count >= 37) {                                   // Read the sensors approx every 5 minutes (37 * 8 secs)
  
    // Optionall read the voltage as float
    //float v = vcc.Read_Volts();
    float battery_perc = vcc.Read_Perc(VCC_MIN, VCC_MAX);
    uint8_t battery = (uint8_t)(battery_perc * 2.55);       // Coerce percentage to a 8-bit int

#ifdef MY_DEBUG
    Serial.print("VCC_perc = ");
    Serial.print(battery_perc);
    Serial.println(" %");
    Serial.print("battery = ");
    Serial.println(battery);
    Serial.flush();
#endif

    digitalWrite(SOIL_SENSOR_POWER_PIN, HIGH);
    delay(250);
    int light_val = analogRead(A7) / 4,                     // Take sensor readings
        soil_val = analogRead(A3) / 4;                      // Divide by 4 so they fit in a byte.
    digitalWrite(SOIL_SENSOR_POWER_PIN, LOW);

#ifdef MY_DEBUG
    Serial.print(F("Light value "));
    Serial.println(light_val);
    Serial.print(F("Soil value "));
    Serial.println(soil_val);
    Serial.flush();
#endif

    byte gotByte;                                           // Initialize a variable for the incoming response
  
    radio.stopListening();                                  // First, stop listening so we can talk.      

#ifdef MY_DEBUG
    Serial.print(F("Now sending "));                        // Use a simple byte counter as payload
    Serial.println(counter);
    Serial.flush();
#endif

    unsigned long time = micros();                          // Record the current microsecond count   
  
    // Build a "packet." It's just a comma-delimited string of values
    char message_buffer[255];
    sprintf(message_buffer, "%i,%i,%i,%i,%i",
            counter,
            DEVICE_ADDRESS,
            light_val,
            soil_val,
            battery);

#ifdef MY_DEBUG
    Serial.print(F("Transmitting message: "));
    Serial.println(message_buffer);
    Serial.flush();
#endif

    if ( radio.write(&message_buffer, sizeof(message_buffer)) ) {

        if(!radio.available()){                            // If nothing in the buffer, we got an ack but it is blank

#ifdef MY_DEBUG
            Serial.print(F("Got blank response. round-trip delay: "));
            Serial.print(micros()-time);
            Serial.println(F(" microseconds"));
            Serial.flush();
#endif

        } else {

            while ( radio.available() ) {                   // If an ack with payload was received

                radio.read( &gotByte, 1 );                  // Read it, and display the response time
                unsigned long timer = micros();

#ifdef MY_DEBUG
                Serial.print(F("Got response "));
                Serial.print(gotByte);
                Serial.print(F(" round-trip delay: "));
                Serial.print(timer-time);
                Serial.println(F(" microseconds"));
                Serial.flush();
#endif
                counter++;                                  // Increment the counter variable
            }
        }
  
    } else {
#ifdef MY_DEBUG
      Serial.println(F("Sending failed."));                 // If no ack response, sending failed
      Serial.flush();
#endif
    }

    loop_count = 0;
  }
}
