
/*Code for Wifi two triac 2 amps mini board
The Board has two Triacs both dimmable using this code

This code is for Atmega328p 
Firmware Version: 0.3
Hardware Version: 0.1

Code Edited By :Naren N Nayak
Date: 07/12/2017
Last Edited By:Naren N Nayak
Date: 24/10/2017

*/ 


#include <TimerOne.h>

//Relay no.
#define DIMMABLE_TRIAC_2     8 //Gpio 8             
#define DIMMABLE_TRIAC_1     9 //Gpio 9 

/*Dual colour LED*/ 
#define DLED_RED   3
#define DLED_GREEN 4

//manual switch
#define SWITCH_INPIN1 A5 //switch 1 
#define SWITCH_INPIN2 A4 //switch 2

/* ZCD */

#define ZCD_INT 0  //Arduino GPIO2 
#define Dimmer_width 115

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
int freqStep = 75;//75*5 as prescalar is 16 for 80MHZ
volatile int dim_value_one = 0;
volatile int dim_value_two = 0;
int dimming_one = 115;
int dimming_two = 115;
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


void setup() {

  Serial.begin(115200);
  Serial.println("WiFi-2A-Dimmer");
  pinMode(DIMMABLE_TRIAC_2, OUTPUT); //relay1 output
  pinMode(DIMMABLE_TRIAC_1, OUTPUT); //Dimmer output
  
  pinMode(DLED_RED, OUTPUT);
  pinMode(DLED_GREEN, OUTPUT);

  attachInterrupt(ZCD_INT, zero_cross_detect, CHANGE);    // Attach an Interupt to Pin 2 (interupt 0) for Zero Cross Detection
  Timer1.initialize(freqStep);                      // Initialize TimerOne library for the freq we need
  Timer1.attachInterrupt(dim_check, freqStep);
  
  
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

/* Multi by 2 so that 2.5V level gets to 5V 10K 10K divider 
   Div by 11 so that value doesnt exceed 99 asdimmer range is 0-99
   Div and mul by 10 gives better variation in Pot value */
     
   
   int_regulator_temp_one= ((round(((((analogRead(SWITCH_INPIN2))*2))/11)/10))*10); 
   int_regulator_one+=int_regulator_temp_one;

   int_regulator_temp_two= ((round(((((analogRead(SWITCH_INPIN1))*2))/11)/10))*10); 
   int_regulator_two+=int_regulator_temp_two;
   
   i++;
   
   if(i>=100)
   {
    int_regulator_one=(round((int_regulator_one/100)*10)/10);
    regulator_value_temp_one="Dimmer1:"+String((int_regulator_one));

    int_regulator_two=(round((int_regulator_two/100)*10)/10);
    regulator_value_temp_two="Dimmer2:"+String((int_regulator_two));
    
    i=0;
    //Serial.println("Regulator value to "+regulator_value);
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
    Serial.println(serialReceived);
    
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
  }

  if (regulator_value_one.substring(0, 8) == "Dimmer1:" && regulator_value_changed_one == true )
  {
    dimming_one = Dimmer_width - regulator_value_one.substring(8, 10).toInt();
    delay(5);
    Serial.println("Regulator value to "+regulator_value_one);
  }

  
/*####################### Dimmable Triac two ##################################*/

    if (Dimmer_value_two.substring(0, 8) == "Dimmer2:" && dimmer_value_changed_two == true )
  {
    dimming_two = Dimmer_width - Dimmer_value_two.substring(8, 10).toInt();
    delay(5);
    Serial.println("Uart value to "+Dimmer_value_two);
  }

  if (regulator_value_two.substring(0, 8) == "Dimmer2:" && regulator_value_changed_two == true )
  {
    dimming_two = Dimmer_width - regulator_value_two.substring(8, 10).toInt();
    delay(5);
    Serial.println("Regulator value to "+regulator_value_two);
  }
  
}


