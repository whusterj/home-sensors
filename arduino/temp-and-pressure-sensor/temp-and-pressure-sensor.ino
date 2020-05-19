/*
   May 2017 - William Huster
   Derived from examples by TMRh20
   Derived from examples by J. Coliz <maniacbug@ymail.com>
*/
/**
 * Wireless light and soil humidity sensor.
 */

/* Enable/disable Serial prints */
#define MY_DEBUG

// Libraries for all sensors
#include <SPI.h>
#include "RF24.h"
#include <printf.h>
#include "LowPower.h"
#include "Vcc.h"

// Library for Temp and Pressure sensor
#include <SFE_BMP180.h>
#include <Wire.h>

/****************** User Config ***************************/
/* Give this device a unique address. */
uint8_t DEVICE_ADDRESS = 0x01;

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 9 & 10 */
RF24 radio(9,10);
const int RADIO_POWER = RF24_PA_MIN;

/* Set up voltage  */
const float VCC_MIN        = 2.0*0.6;  // Minimum expected Vcc level, in Volts. Example for 2xAA Alkaline.
const float VCC_MAX        = 2.0*1.5;  // Maximum expected Vcc level, in Volts. Example for 2xAA Alkaline.
const float VCC_CORRECTION = 1.0/1.0;  // Measured Vcc by multimeter divided by reported Vcc

/**********************************************************/

//
Vcc vcc(VCC_CORRECTION);

//
SFE_BMP180 pressure_sensor;                     // BMP180 Pressure Sensor object

// Topology
byte RX_PIPE[6] = "RECVR";
byte TX_PIPE[6] = "TRANS";

byte counter = 1;                               // A single byte to enumerate packets

typedef struct Packet {
  uint8_t packet_id;      // Byte 1
  uint8_t source_addr;    // Byte 2
  uint8_t temp_val;       // Byte 3
  uint8_t pressure_val;   // Byte 4
  uint8_t battery;        // Byte 5
}
Packet;

//
// Utility function: pretty-print a packet.
// 
void print_packet(struct Packet* packet)
{
  printf("Packet: { %i %i %i %i %i }\n",
         packet->packet_id,
         packet->source_addr,
         packet->temp_val,
         packet->pressure_val,
         packet->battery);
}

void setup(){

  Serial.begin(9600);
  printf_begin();

#ifdef MY_DEBUG
  Serial.println(F("Light and Soil Sensor"));
#endif
 
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

  if (pressure_sensor.begin()) {
#ifdef MY_DEBUG
    Serial.println("BMP180 init success");
#endif
  } else {
#ifdef MY_DEBUG
    Serial.println("BMP180 init fail\n\n");     // Something went wrong attempting to initialize the temp & pressure sensor.
#endif
    while(1);                                   // Pause forever.
  }

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

  if (loop_count >= 1) {                                   // Read the sensors approx every 5 minutes (37 * 8 secs)
  
    //float v = vcc.Read_Volts();
    float battery_perc = vcc.Read_Perc(VCC_MIN, VCC_MAX);
    uint8_t battery = (uint8_t)(battery_perc * 2.55);

#ifdef MY_DEBUG
    Serial.print("VCC_perc = ");
    Serial.print(battery_perc);
    Serial.println(" %");
    Serial.flush();
#endif

    char status;
    double T,P;

    status = pressure_sensor.startTemperature();
    if (status != 0) {

      delay(status);                                 // Wait for the measurement to complete

      status = pressure_sensor.getTemperature(T);    // Get temperature as `T`
      if (status != 0) {                             // status is 1 if successful, 0 if failure.

#ifdef MY_DEBUG
        Serial.print("temperature: ");
        Serial.print(T, 2);
        Serial.print(" deg C, ");
        Serial.print((9.0/5.0)*T+32.0,2);
        Serial.println(" deg F");
#endif

        status = pressure_sensor.startPressure(3);          // 3 is hi-res sampling
        if (status != 0) {

          delay(status);                             // Wait for the measurement to complete:

          status = pressure_sensor.getPressure(P, T);       // Store pressure in `P`, requires a tempt value `T`
          if (status != 0) {

#ifdef MY_DEBUG
            Serial.print("absolute pressure: ");
            Serial.print(P, 2);
            Serial.print(" mb, ");
            Serial.print(P*0.0295333727,2);
            Serial.println(" inHg");
#endif
          } else {
#ifdef MY_DEBUG
            Serial.println("error retrieving pressure measurement\n");
#endif
          }
        } else {
#ifdef MY_DEBUG
          Serial.println("error starting pressure measurement\n");
#endif
        }
      } else {
#ifdef MY_DEBUG
        Serial.println("error retrieving temperature measurement\n");
#endif
      }
    } else {
#ifdef MY_DEBUG
      Serial.println("error starting temperature measurement\n");
#endif
    }

    byte gotByte;                                           // Initialize a variable for the incoming response
    radio.stopListening();                                  // First, stop listening so we can talk.      

#ifdef MY_DEBUG
    Serial.print(F("Now sending "));                        // Use a simple byte counter as payload
    Serial.println(counter);
    Serial.flush();
#endif

    unsigned long time = micros();                          // Record the current microsecond count   
  
    // Build the Packet
    Packet packet;
    packet.packet_id = counter;
    packet.source_addr = DEVICE_ADDRESS;
    packet.temp_val = T;
    packet.pressure_val = P;
    packet.battery = battery;

    char temp_str[16],
         pressure_str[16];

    dtostrf(T, 5, 2, temp_str);
    dtostrf(P, 5, 2, pressure_str);

    char message_buffer[255];
    sprintf(message_buffer, "%i,%i,%s,%s,%i",
            counter,
            DEVICE_ADDRESS,
            temp_str,
            pressure_str,
            battery);

#ifdef MY_DEBUG
    // Serial.print(F("Transmitting packet: "));
    // print_packet(&packet);
    Serial.print(F("Transmitting message: "));
    Serial.println(message_buffer);
    Serial.flush();
#endif

    if ( radio.write(&message_buffer, sizeof(message_buffer)) ) {                       // Send the counter variable to the other radio 

        if(!radio.available()) {                                        // If nothing in the buffer, we got an ack but it is blank

#ifdef MY_DEBUG
            Serial.print(F("Got blank response. round-trip delay: "));
            Serial.print(micros()-time);
            Serial.println(F(" microseconds"));
#endif

        } else {

            while(radio.available() ){                      // If an ack with payload was received

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
#endif

    }

    loop_count = 0;
  }
}
