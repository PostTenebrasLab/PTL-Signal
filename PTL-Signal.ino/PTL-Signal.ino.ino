/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>


 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */


/**
 * Example RF Radio Ping Pair
 *
 * This is an example of how to use the RF24 class.  Write this sketch to 
two different nodes,
 * connect the role_pin to ground on one.  The ping node sends the current 
time to the pong node,
 * which responds by sending the value back.  The ping node can then see 
how long the whole cycle
 * took.
 */


#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"



//
// Hardware configuration
//


// Set up nRF24L01 radio on SPI bus plus pins 9 & 10


RF24 radio(9,10);


// sets the role of this unit in hardware.  Connect to GND to be the 'pong' receiver
// Leave open to be the 'ping' transmitter
const int role_pin = 7;

const int CH1817_pin = 6;
const int relay_pin = 6;

const int led_pin = 5;

unsigned long started_flashing_at; //to count flashing time in the pong role



//
// Topology
//


// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };


//
// Role management
//
// Set up role.  This sketch uses the same software for all the nodes
// in this system.  Doing so greatly simplifies testing.  The hardware itself specifies
// which node it is.
//
// This is done through the role_pin
//


// The various roles supported by this sketch
typedef enum { role_ping_out = 1, role_pong_back } role_e;


// The debug-friendly names of those roles
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back"};


// The role of the current running sketch
role_e role;


void setup(void)
{
  //
  // Role
  //


  // set up the role pin
  pinMode(role_pin, INPUT);
  digitalWrite(role_pin,HIGH);
  
  delay(20); // Just to get a solid reading on the role pin
 
  // set up led as out  
  pinMode(led_pin, OUTPUT);


  // read the address pin, establish our role
  if (  digitalRead(role_pin) )
  {
    role = role_ping_out;
    pinMode(CH1817_pin, INPUT); 
    digitalWrite(CH1817_pin,HIGH);
  }
  else
  {
    role = role_pong_back;
    pinMode(relay_pin, OUTPUT); 
  }

  //role = role_pong_back; 
  //pinMode(relay_pin, OUTPUT);


  //
  // Print preamble
  //


  Serial.begin(57600);
  printf_begin();
  printf("\n\rRF24/examples/pingpair/\n\r");
  printf("ROLE: %s\n\r",role_friendly_name[role]);


  //
  // Setup and configure rf radio
  //


  radio.begin();

  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.disableCRC();


  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15,15);  // AEK already max!


  // optionally, reduce the payload size.  seems to
  // improve reliability
  radio.setPayloadSize(8); // SET TO LOWER PAYLOAD 1 byte is enough? default is 8


  //
  // Open pipes to other nodes for communication
  //


  // This simple sketch opens two pipes for these two nodes to communicate
  // back and forth.
  // Open 'our' pipe for writing
  // Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading)


  if ( role == role_ping_out )
  {
    radio.openWritingPipe(pipes[0]);
    radio.openReadingPipe(1,pipes[1]);
  }
  else
  {
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1,pipes[0]);
  }


  //
  // Start listening
  //


  radio.startListening();


  //
  // Dump the configuration of the rf unit for debugging
  //


  radio.printDetails();
}


void loop(void)
{
  //
  // Ping out role.  Repeatedly send the current time
  //

 

  if (role == role_ping_out)
  {
        //read if we're ringing or not 
        bool ringing = digitalRead(CH1817_pin); 
 
    // First, stop listening so we can talk.
    radio.stopListening();


    // Take the time, and send it.  This will block until complete
    //unsigned long time = millis();
    printf("Now sending %d...",ringing);        //we send ringing as d because it's promoted to int
    //bool ok = radio.write( &ringing, sizeof(unsigned long) ); //original line AEK
    bool ok = radio.write( &ringing, 1 );
 
    if (ok)
    {
      printf("ok...");
      digitalWrite(led_pin, !digitalRead(led_pin)); //toggle led, only if everything is ok
    }
    else
      printf("failed.\n\r");


    // Now, continue listening
    radio.startListening();


    // Wait here until we get a response, or timeout (250ms)
    unsigned long started_waiting_at = millis();
    bool timeout = false;
    while ( ! radio.available() && ! timeout )
      if (millis() - started_waiting_at > 200 )
        timeout = true;


    // Describe the results
    if ( timeout )
    {
      printf("Failed, response timed out.\n\r");
    }
    else
    {
      // Grab the response, compare, and send to debugging spew
      //unsigned long got_time;
      bool response;
      //radio.read( &got_time, sizeof(unsigned long) );
      radio.read( &response, 1 );


      // Spew it
      //printf("Got response at %lu, round-trip delay: %lu\n\r",got_time,millis()-got_time);
      printf("Got response at: %d -- now is: %lu\n\r",response,millis());
    }


    // Try again 1s later
    delay(500);
  }


  //
  // Pong back role.  Receive each packet, dump it out, and send it back
  //


  if ( role == role_pong_back )
  {
    // if there is data ready
    if ( radio.available() )
    {
      // Dump the payloads until we've gotten everything
      //unsigned long got_time;
      bool msg;
      bool done = false;
      while (!done)
      {
        // Fetch the payload, and see if this was the last one.
        //done = radio.read( &got_time, sizeof(unsigned long) );
        done = radio.read( &msg, 1 );

 
        // Spew it
        printf("Got payload %d...",msg);
        if (!msg)       //we have a ringing phone
        {
                Serial.println("RINGING");
                digitalWrite(relay_pin, HIGH); //start flashing the light
                started_flashing_at = millis(); //count when we started
        }
        else
                            if (millis() - started_flashing_at > 10000 ) //has the ringing stopped or we passed the 10 seconds of flashing per ring?
                              digitalWrite(relay_pin, LOW); //stop flashing the light
                        }
 

                        // Delay just a little bit to let the other unit
                        // make the transition to receiver
                        delay(20);
 


      // First, stop listening so we can talk
      radio.stopListening();


      // Send the final one back.
      radio.write( &msg, 1);
      printf("Sent response.\n\r");


      // Now, resume listening so we catch the next packets.
      radio.startListening();
      
      
      //flash the led
      digitalWrite(led_pin, !digitalRead(led_pin)); //toggle led, when we get something
      
    } // radio.available
  } //role pong
} // main loop

