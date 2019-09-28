
/*Code for Wifi two triac 2 amps mini board
The Board has two Triacs both dimmable using this code

This code is for Atmega328p 
Firmware Version: 0.5T
Hardware Version: 0.1

Code Edited By :Naren N Nayak
Date: 07/12/2017
Last Edited By:Karthik B
Date: 27/9/19

V0.5T(tasmota)
--->> Push button instead of R-pot
--->> Virtual push button status update bug fixed

*/ 

/*
Programming header J1

 1_VCC * * 2_Dummy
3_TXDE * * 4_RXDA
5_RXDE * * 6_TXDA
7_DTRE * * 8_DTRA
9RTSE * * 10_GND


*/

#include <TimerOne.h>
#define version_no "FVer 0.5T ,HVer 0.2"
//Relay no.
#define DIMMABLE_TRIAC_2     8 //Gpio 8             
#define DIMMABLE_TRIAC_1     9 //Gpio 9 


/*Dual colour LED*/ 
#define DLED_RED   3
#define DLED_GREEN 4
/* ZCD */

#define ZCD_INT 0  //Arduino GPIO2 
#define Dimmer_width 100

/*I/P Switch*/

#define INPIN_REGULATOR A5  // input pin (push button)
#define INPIN_REGULATOR2 A4  // input pin (push button)
#define HUMANPRESSDELAY 50 // the delay in ms untill the press should be handled as a normal push by human. Button debounce.

unsigned long count_regulator = 0; //Button press time counter
unsigned long dimval = 0; //Button press time counter
int button_press_flag =1;
byte tarBrightness = 0;
byte curBrightness = 0;

unsigned long count_regulator2 = 0; //Button press time counter
unsigned long dimval2 = 0; //Button press time counter
int button_press_flag2 =1;
byte tarBrightness2 = 0;
byte curBrightness2 = 0;

/*Serial Data variables*/
String serialReceived;
String Dimmer_value_temp_two;
String Dimmer_value_temp_one;

String Dimmer_value_one;
String Dimmer_value_two;

String regulator_value_temp_one;
String regulator_value_temp_two;
/*POT Variable */
String regulator_value_one;
String regulator_value_two;

/*ZCD Variables */
int freqStep = 85;//75*5 as prescalar is 16 for 80MHZ
volatile int dim_value_one = 0;
volatile int dim_value_two = 0;
int dimming_one = 100;
int dimming_two = 100;
volatile boolean zero_cross_one = 0;
volatile boolean zero_cross_two = 0;

volatile int int_regulator_one=0;
volatile int int_regulator_two=0;

volatile int int_regulator_temp_one;
volatile int int_regulator_temp_two;

volatile int i=1;
/*Flags for Dimmer virtual switch concept */
volatile boolean dimmer_value_changed_one =false; 
volatile boolean dimmer_value_changed_two =false;

volatile boolean regulator_value_changed_one =false;
volatile boolean regulator_value_changed_two =false;

volatile boolean dimmer_status =false;
int dimvalue_one;
int dimvalue_two;


void setup() {

  Serial.begin(115200);
  Serial.println("WiFi-2A-Dimmer");
  Serial.println(version_no);
  pinMode(DIMMABLE_TRIAC_2, OUTPUT); //relay1 output
  pinMode(DIMMABLE_TRIAC_1, OUTPUT); //Dimmer output
  
  pinMode(DLED_RED, OUTPUT);
  pinMode(DLED_GREEN, OUTPUT);

  attachInterrupt(ZCD_INT, zero_cross_detect, CHANGE);    // Attach an Interupt to Pin 2 (interupt 0) for Zero Cross Detection
  Timer1.initialize(freqStep);                      // Initialize TimerOne library for the freq we need
  Timer1.attachInterrupt(dim_check, freqStep);
  
  
}


void btn_handle()
{
if(count_regulator<=9)
{
  count_regulator=0;
  tarBrightness =count_regulator;
}

if(!digitalRead(INPIN_REGULATOR))
{
  if(button_press_flag ==1)  
{
button_press_flag =0;
if(count_regulator<=9)
{
tarBrightness =count_regulator+10;
count_regulator=count_regulator+10;
}
else
{
count_regulator=count_regulator+10;
tarBrightness =count_regulator;
}
if(count_regulator<=90)
     {
      //Serial.print("Reg VAL:");
      //Serial.println(count_regulator);
      int_regulator_one = count_regulator;
     }
     else if(count_regulator<=100 && count_regulator >90 )
     {
      dimval = count_regulator -1;
      //Serial.print("Reg VAL:");
      //Serial.println(dimval);
      int_regulator_one = dimval;
     }
     else
     {
      count_regulator=0;
      //Serial.print("Reg VAL:");
      //Serial.println(count_regulator);
      int_regulator_one = count_regulator;
     }
}
}
else
{
  button_press_flag =1;
  if (count_regulator > 1 && count_regulator < HUMANPRESSDELAY/5) 
    { 
    if(count_regulator<=99)
     {
      //Serial.println(count_regulator);
     }
     else
     {
      count_regulator=0;
     }
    }
}
/*####*/
if(count_regulator2<=9)
{
  count_regulator2=0;
  tarBrightness2 =count_regulator2;
}

if(!digitalRead(INPIN_REGULATOR2))
{
  if(button_press_flag2 ==1)  
{
button_press_flag2 =0;
if(count_regulator2<=9)
{
tarBrightness2 =count_regulator2+10;
count_regulator2=count_regulator2+10;
}
else
{
count_regulator2=count_regulator2+10;
tarBrightness2 =count_regulator2;
}
if(count_regulator2<=90)
     {
      //Serial.print("Reg VAL2:");
      //Serial.println(count_regulator2);
      int_regulator_two = count_regulator2;
     }
     else if(count_regulator2<=100 && count_regulator2 >90 )
     {
      dimval = count_regulator2 -1;
      //Serial.print("Reg VAL2:");
      //Serial.println(dimval2);
      int_regulator_two = dimval2;
     }
     else
     {
      count_regulator2=0;
      //Serial.print("Reg VAL2:");
      //Serial.println(count_regulator2);
      int_regulator_two =count_regulator2;
     }
}
}
else
{
  button_press_flag2 =1;
  if (count_regulator2 > 1 && count_regulator2 < HUMANPRESSDELAY/5) 
    { 
    if(count_regulator2<=99)
     {
      //Serial.println(count_regulator2);
     }
     else
     {
      count_regulator2=0;
     }
    }
}

}


/*ZCD Interrupt Function*/
void zero_cross_detect() 
{
  zero_cross_one = true;               // set the boolean to true to tell our dimming function that a zero cross has occured
  zero_cross_two = true;               // set the boolean to true to tell our dimming function that a zero cross has occured
  dim_value_one = 0;
  dim_value_two = 0;
  digitalWrite(DIMMABLE_TRIAC_1, LOW);      // turn off TRIAC (and AC)
  digitalWrite(DIMMABLE_TRIAC_2, LOW);      // turn off TRIAC (and AC)
  
  digitalWrite(DLED_GREEN, HIGH);
}

/*Timer Interrupt Function used to trigger the triac for Dimming*/
void dim_check() 
{
  /*For Dimmer */
  if (zero_cross_one == true) 
  {
    if (dim_value_one >= dimming_one) 
    {
      digitalWrite(DIMMABLE_TRIAC_1, HIGH); // turn on Triac 
      digitalWrite(DLED_GREEN, LOW);
      dim_value_one = 0; // reset time step counter
      zero_cross_one = false; //reset zero cross detection
    }
    else 
    {
      dim_value_one++; // increment time step counter
    }
  }


  if (zero_cross_two == true) 
  {
    if (dim_value_two >= dimming_two) 
    {
      digitalWrite(DIMMABLE_TRIAC_2, HIGH); // turn on Triac 
      digitalWrite(DLED_GREEN, LOW);
      dim_value_two = 0; // reset time step counter
      zero_cross_two = false; //reset zero cross detection
    }
    else 
    {
      dim_value_two++; // increment time step counter
    }
  }

  
}


void loop() 
{
   btn_handle();
    
   i++;
   
   if(i>=100)
   {
    regulator_value_temp_one="Dimmer1:"+String((int_regulator_one));
    regulator_value_temp_two="Dimmer2:"+String((int_regulator_two));
    i=0;
   }
   




  
/*############### Flag setting for Dimmable Triac one through Pot ###############*/
  
  if(regulator_value_temp_one!=regulator_value_one)
  {
    regulator_value_one=regulator_value_temp_one;
    regulator_value_changed_one =true;
  }
  else
  {
    regulator_value_changed_one =false;
  }

/*############### Flag setting for Dimmable Triac two through Pot ###############*/
  
  if(regulator_value_temp_two!=regulator_value_two)
  {
    regulator_value_two=regulator_value_temp_two;
    regulator_value_changed_two =true;
  }
  else
  {
    regulator_value_changed_two =false;
  }


/*############### Uart Data ###########################*/
  
  if (Serial.available() > 0) 
  {   // is a character available
    
    serialReceived = Serial.readStringUntil('\n');

    if (serialReceived.substring(0, 7) == ("Status\r") ||serialReceived.substring(0, 7) == ("Status"))
    {
      dimmer_status = true;     
    }
    
    if (serialReceived.substring(0, 8) == "Dimmer1:")
    {
      Dimmer_value_temp_one = serialReceived;     
    }

    if (serialReceived.substring(0, 8) == "Dimmer2:")
    {
      Dimmer_value_temp_two = serialReceived;
    }

  }

/*################## Flag setting for Dimmable Triac one through uart ##################################*/

  if( Dimmer_value_temp_one!=Dimmer_value_one)
  {
    Dimmer_value_one=Dimmer_value_temp_one;
    dimmer_value_changed_one =true;
  }
  else 
  {
    dimmer_value_changed_one =false;
  }

/*################## Flag setting for Dimmable Triac two through uart ##################################*/

  if( Dimmer_value_temp_two!=Dimmer_value_two)
  {
    Dimmer_value_two=Dimmer_value_temp_two;
    dimmer_value_changed_two =true;
  }
  else 
  {
    dimmer_value_changed_two =false;
  }



  
/*####################### Dimmable Triac one ##################################*/
  
  if (Dimmer_value_one.substring(0, 8) == "Dimmer1:" && dimmer_value_changed_one == true )
  {
    dimming_one = Dimmer_width - Dimmer_value_one.substring(8, 10).toInt();
    delay(5);
    Serial.println("Uart value to "+Dimmer_value_one);
    dimvalue_one = Dimmer_value_one.substring(8, 10).toInt();
  }

  if (regulator_value_one.substring(0, 8) == "Dimmer1:" && regulator_value_changed_one == true )
  {
    dimming_one = Dimmer_width - regulator_value_one.substring(8, 10).toInt();
    delay(5);
    Serial.println("Regulator value to "+regulator_value_one);
    dimvalue_one = regulator_value_one.substring(8, 10).toInt();
  }

  
/*####################### Dimmable Triac two ##################################*/

    if (Dimmer_value_two.substring(0, 8) == "Dimmer2:" && dimmer_value_changed_two == true )
  {
    dimming_two = Dimmer_width - Dimmer_value_two.substring(8, 10).toInt();
    delay(5);
    Serial.println("Uart value to "+Dimmer_value_two);
    dimvalue_two = Dimmer_value_two.substring(8, 10).toInt();
  }

  if (regulator_value_two.substring(0, 8) == "Dimmer2:" && regulator_value_changed_two == true )
  {
    dimming_two = Dimmer_width - regulator_value_two.substring(8, 10).toInt();
    delay(5);
    Serial.println("Regulator value to "+regulator_value_two);
    dimvalue_two = regulator_value_two.substring(8, 10).toInt();
  }
  

/*#########################################################################*/


if (dimmer_status == true)
  {
   dim_status(); 
   dimmer_status = false;
  
  }
}


void dim_status() 
{
  Serial.println("Status:"+String(dimvalue_one)+","+String(dimvalue_two)+","+regulator_value_one.substring(8,10)+","+regulator_value_two.substring(8,10)); 
}
