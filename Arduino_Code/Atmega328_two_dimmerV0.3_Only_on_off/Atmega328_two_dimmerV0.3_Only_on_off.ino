
/*Code for Wifi two triac 2 amps mini board
The Board has two Triacs both can be used for on off mode only 
no dimming 

This code is for Atmega328p 
Firmware Version: 0.3
Hardware Version: 0.1

Code Edited By :Naren N Nayak
Date: 09/11/2017
Last Edited By:Naren N Nayak
Date: 24/10/2017

*/ 


#include <TimerOne.h>

//Relay no.
#define NON_DIMMABLE_TRIAC_1 8 //Gpio 8             
#define NON_DIMMABLE_TRIAC_2 9 //Gpio 9 

/*Dual colour LED*/ 
#define DLED_RED   3
#define DLED_GREEN 4

//manual switch
#define SWITCH_INPIN1 A5 //switch 1 
#define SWITCH_INPIN2 A4 //switch 2



/*Serial Data variables*/
String serialReceived;
String serialReceived1;
String serialReceived2;





void setup() 
{

  Serial.begin(115200);
  Serial.println("WiFi-2A- 2 channel ON/OFF only");
  pinMode(NON_DIMMABLE_TRIAC_1, OUTPUT); //relay1 output
  pinMode(NON_DIMMABLE_TRIAC_2, OUTPUT); //Dimmer output
  
  pinMode(DLED_RED, OUTPUT);
  pinMode(DLED_GREEN, OUTPUT);

  pinMode(SWITCH_INPIN1, INPUT); //manual switch 1 input
  pinMode(SWITCH_INPIN2, INPUT); //manual switch 1 input
  
}



void loop() 
{

/*############### Uart Data ###########################*/
  
  if (Serial.available() > 0) 
  {   // is a character available
    
    serialReceived = Serial.readStringUntil('\n');
    Serial.println(serialReceived);
    
    if (serialReceived.substring(0, 9) == "Dimmer:99")
    {
      serialReceived2 = "R_2 switched via web request to 1";     
    }

     if (serialReceived.substring(0, 8) == "Dimmer:0")
    {
      serialReceived2 = "R_2 switched via web request to 0";     
    }

    if (serialReceived.substring(0, 33) == "R_1 switched via web request to 1")
    {
      serialReceived1 = serialReceived;
    }
    
    if (serialReceived.substring(0, 33) == "R_1 switched via web request to 0")
    {
      serialReceived1 = serialReceived;
    }

  }



/*##################### Non Dimmable Triac 1##############################*/
  
  if (((serialReceived1.substring(0, 33) == "R_1 switched via web request to 1") && (!(digitalRead(SWITCH_INPIN1)))) || ((!(serialReceived1.substring(0, 33) == "R_1 switched via web request to 1")) && ((digitalRead(SWITCH_INPIN1))))) //exor logic
  {
    if(digitalRead(NON_DIMMABLE_TRIAC_1)==HIGH)
    {
      Serial.println("Load 1 is OFF");
    }
    digitalWrite(NON_DIMMABLE_TRIAC_1, LOW);
    digitalWrite(DLED_RED, HIGH);
    
  }
  else
  {
    if(digitalRead(NON_DIMMABLE_TRIAC_1)==LOW)
    {
      Serial.println("Load 1 is ON");
    }
    digitalWrite(NON_DIMMABLE_TRIAC_1, HIGH);
    digitalWrite(DLED_RED, LOW);
   
  }


  /*##################### Non Dimmable Triac 2##############################*/
  
  if (((serialReceived2.substring(0, 33) == "R_2 switched via web request to 1") && (!(digitalRead(SWITCH_INPIN2)))) || ((!(serialReceived2.substring(0, 33) == "R_2 switched via web request to 1")) && ((digitalRead(SWITCH_INPIN2))))) //exor logic
  {
    if(digitalRead(NON_DIMMABLE_TRIAC_2)==HIGH)
    {
      Serial.println("Load 2 is OFF");
    }
    digitalWrite(NON_DIMMABLE_TRIAC_2, LOW);
    digitalWrite(DLED_GREEN, HIGH);
    
  }
  else
  {
    if(digitalRead(NON_DIMMABLE_TRIAC_2)==LOW)
    {
      Serial.println("Load 2 is ON");
    }
    digitalWrite(NON_DIMMABLE_TRIAC_2, HIGH);
    digitalWrite(DLED_GREEN, LOW);
   
  }

  
}


