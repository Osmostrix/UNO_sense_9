/*
 * SN74HC165N_shift_reg
 *
 * Program to shift in the bit values from a SN74HC165N 8-bit
 * parallel-in/serial-out shift register.
 *
 * This sketch demonstrates reading in 16 digital states from a
 * pair of daisy-chained SN74HC165N shift registers while using
 * only 4 digital pins on the Arduino.
 *
 * You can daisy-chain these chips by connecting the serial-out
 * (Q7 pin) on one shift register to the serial-in (Ds pin) of
 * the other.
 * 
 * Of course you can daisy chain as many as you like while still
 * using only 4 Arduino pins (though you would have to process
 * them 4 at a time into separate unsigned long variables).
 * 
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "Arduino.h"
#include "Tlc5940.h"

/* How many shift register chips are daisy-chained.
*/
#define NUMBER_OF_SHIFT_CHIPS   4

/* Width of data (how many ext lines).
*/
#define DATA_WIDTH   NUMBER_OF_SHIFT_CHIPS * 8

/* Width of pulse to trigger the shift register to read and latch.
*/
#define PULSE_WIDTH_USEC   5

/* Optional delay between shift register reads.
*/
#define POLL_DELAY_MSEC   1

/* You will need to change the "int" to "long" If the
 * NUMBER_OF_SHIFT_CHIPS is higher than 2.
*/
#define BYTES_VAL_T unsigned long

long ploadPin        = 8;  // Connects to Parallel load pin the 165
long clockEnablePin  = 7;  // Connects to Clock Enable pin the 165
long dataPin         = 12; // Connects to the Q7 pin the 165
long clockPin        = 6; // Connects to the Clock pin the 165

BYTES_VAL_T pinValues;
BYTES_VAL_T oldPinValues;
BYTES_VAL_T l_num = 4294967296 ;// This is the value of 2^32 in BYTES_VAL_T. Need to adjust this according to Sensor count

static int chimera = 0;

int cupA[6] = {6, 8, 9, 11, 7, 12};

int relative[64] = {27, 38, 36, 26, 29, 40, 74, 76, 
                    10, 62, 54, 80, 88, 83, 79, 82, 
                    78, 45, 51, 32, 84, 35, 85, 87,
                    21, 48, 24, 14, 53, 56, 52, 61,
                    13, 19, 30, 17, 16, 15, 43, 70,
                    63, 66, 60, 75, 71, 68, 46, 49,
                    42, 39, 57, 64, 65, 77, 59, 67,
                    69, 72, 73, 50, 86, 89, 55, 58};

int cupB[6] = {0, 2, 1, 3, 5, 4};


int A[7][6] = {{4095, 4095, 4095, 4095, 4095, 4095},//0
               {0, 0, 0, 0, 0, 0},//1
               {4095, 0, 4095, 4095, 4095, 4095},//2
               {4095, 0, 4095, 0, 0, 4095},//3
               {4095, 4095, 4095, 0, 0, 0},//4
               {4095, 4095, 4095, 0, 0, 4095},//5
               {4095, 4095, 4095, 4095, 4095, 4095},//6 
              };
              
int B[7][6] = {{4095, 0, 0, 0, 0, 4095},//0
               {4095, 4095, 4095, 4095, 4095, 4095},//1
               {4095, 0, 4095, 0, 0, 4095},//2
               {4095, 0, 4095, 0, 0, 4095},//3
               {0, 0, 4095, 0, 0, 0},//4
               {4095, 0, 4095, 0, 0, 4095},//5
               {4095, 0, 4095, 0, 0, 4095},//6 
              };
               
int C[7][6] = {{4095, 4095, 4095, 4095, 4095, 4095},//0
               {0, 0, 0, 0, 0, 0},//1
               {4095, 4095, 4095, 0, 0, 4095},//2
               {4095, 4095, 4095, 4095, 4095, 4095},//3
               {4095, 4095, 4095, 4095, 4095, 4095},//4
               {4095, 0, 4095, 4095, 4095, 4095},//5
               {4095, 0, 4095, 4095, 4095, 4095},//6 
              };

/* This function is essentially a "shift-in" routine reading the
 * serial Data from the shift register chips and representing
 * the state of those pins in an unsigned integer (or long).
*/
BYTES_VAL_T read_shift_regs()
{
    long bitVal;
    int shift = 0;
    BYTES_VAL_T bytesVal = 0;

    /* Trigger a parallel Load to latch the state of the data lines,
    */
    digitalWrite(clockEnablePin, HIGH);
    digitalWrite(ploadPin, LOW);
    delayMicroseconds(PULSE_WIDTH_USEC);
    digitalWrite(ploadPin, HIGH);
    digitalWrite(clockEnablePin, LOW);

    /* Loop to read each bit value from the serial out line
     * of the SN74HC165N.
    */
    while(shift != NUMBER_OF_SHIFT_CHIPS)
    {
      for(int i = (DATA_WIDTH - (8*(1+shift))); i < (DATA_WIDTH - (8*shift)); i++)
      {
          bitVal = digitalRead(dataPin);
  
          /* Set the corresponding bit in bytesVal.
          */
          bytesVal |= (bitVal << ((DATA_WIDTH-1) - i));
  
          /* Pulse the Clock (rising edge shifts the next bit).
          */
          digitalWrite(clockPin, HIGH);
          delayMicroseconds(PULSE_WIDTH_USEC);
          digitalWrite(clockPin, LOW);
      }
      shift++;
    }
    
    return(bytesVal);
}

/* Dump the list of zones along with their current status.
*/
void display_pin_values()
{
    Serial.print("Pin States:\r\n");

    for(int i = 0; i < DATA_WIDTH; i++)
    {
        Serial.print("  Pin-");
        Serial.print(i);
        Serial.print(": ");

        if((pinValues >> i) & 1) {
            Serial.print("HIGH");
            
        }
        else {
            Serial.print("LOW");
            
        }
        Tlc.update();
        Serial.print((pinValues >> i));

        Serial.print("\r\n");
    }

    Serial.print("\r\n");
    
}

int counterA = 0, counterB = 0;

void ignite_pin_values()
{
    //Serial.print("Pin States:\r\n");
    int counterA = 0, counterB = 0;
    
    for(int i = 0; i < DATA_WIDTH; i++)
    {
        //Serial.print("  Pin-");
        //Serial.print(i);
        //Serial.print(": ");

        if((pinValues >> i) & 1) {
            //Serial.print("HIGH");
            if ((i <6)) {
              Tlc.set(i, 0);
            }
            else if ((i < 13) && (i!= 10)) {
              Tlc.set(i, 0);
              
            }

        }
        else {
            //Serial.print("LOW");
            if ((i < 6)){
              Tlc.set(i, 4095);
              counterB++;
              
            }
            else if ((i < 13) && (i!=10)) {
              Tlc.set(i, 4095);
              counterA++;              
            }
        }
        Tlc.update();
        //Serial.print((pinValues >> i));

        //Serial.print("\r\n");
    }

    //Serial.print("\r\n");

    enumerateN(1, counterA);
    delay(POLL_DELAY_MSEC);
    enumerateN(41, counterB);
    
}

void cup_light()
{
    //Serial.print("Pin States:\r\n");
    
    for(int i = 0; i < DATA_WIDTH; i++)
    {
        //Serial.print("  Pin-");
        //Serial.print(i);
        //Serial.print(": ");

        if((pinValues >> i) & 1) {
            //Serial.print("HIGH");
            if ((i <6)) {
              Tlc.set(i, 0);
            }
            else if ((i < 13) && (i!= 10)) {
              Tlc.set(i, 0);
              
            }

        }
        else {
            //Serial.print("LOW");
            if ((i < 6)){
              Tlc.set(i, 4095);
              
              
            }
            else if ((i < 13) && (i!=10)) {
              Tlc.set(i, 4095);
                          
            }

            
            if (i == 10) {
              chimera++;
              Serial.print(chimera);
            }
        }
        Tlc.update();
        //Serial.print((pinValues >> i));

        //Serial.print("\r\n");
    }

    //Serial.print("\r\n");
    
}

void enumerateN(int start, int pattern) {
     int holder = start;
     int i;
     for (i = 0; i < 6; i++) {
          Tlc.set(relative[holder+i], A[pattern][i]);
          Tlc.set(relative[holder+8+i], B[pattern][i]);
          Tlc.set(relative[holder+16+i], C[pattern][i]);
     }

     Tlc.update();
}

void starwars()
{
  int o = 6, m = 5, n =7, p = 6, q = 5, r = 7;
  int k = 57, a = 57, b = 56, c = 56, d =58, e = 58;
  int f = 5, g = 7, h =5, s = 6, t = 7, u = 5, v = 7, w = 6;
  int x = 56, y = 58, z =56, aa = 57, bb = 58, cc = 56, dd = 58, ee = 57;

  for(int i = 0; i < 16; i++)
  {
    if(i > 7)
    {
      Tlc.set(relative[h], 0); 
      Tlc.set(relative[t], 0); 
      Tlc.set(relative[bb], 0); 
      Tlc.set(relative[z], 0); 
        h+=8;
        t+= 8;  
        bb-=8;
        z-= 8;
    }
    
    if(i > 6 && i < 15)
    {
      Tlc.set(relative[u], 2047);
      Tlc.set(relative[v], 2047); 
      Tlc.set(relative[w], 0); 
      Tlc.set(relative[cc], 2047);
      Tlc.set(relative[dd], 2047); 
      Tlc.set(relative[ee], 0); 
        w+=8;
        u+= 8;  
        v+= 8; 
        cc-=8;
        dd-= 8;  
        ee-= 8; 
    }
    
    if(i > 5 && i < 14)
    {
      Tlc.set(relative[s], 2047);
      s+= 8;

      Tlc.set(relative[aa], 2047);
      aa-= 8;
    }
    
    if(i > 4 && i < 13)
    {
      Tlc.set(relative[f], 2047);
      Tlc.set(relative[g], 2047); 
      Tlc.set(relative[x], 2047);
      Tlc.set(relative[y], 2047); 
        f+= 8;  
        g+= 8; 
        x-= 8;  
        y-= 8; 
    }

    if( i > 2 && i < 11)
    {
      Tlc.set(relative[p], 0);
      Tlc.set(relative[q], 0);
      Tlc.set(relative[r], 0);
      Tlc.set(relative[a], 0);
      Tlc.set(relative[c], 0);
      Tlc.set(relative[e], 0); 
      r+= 8;  
      p+= 8;
      q+=8;   
      a-= 8;  
      c-= 8;
      e-=8;  
    }
    
    if(i > 1 && i < 10)
     {
        Tlc.set(relative[m], 2047);
        Tlc.set(relative[n], 2047); 
        Tlc.set(relative[b], 2047);
        Tlc.set(relative[d], 2047); 
        m+= 8;  
        n+= 8; 
        b-= 8;  
        d-= 8; 
     }

     if(i < 8)
     {
     Tlc.set(relative[o], 2047);
     Tlc.set(relative[k], 2047);

     o+= 8; 
     k-= 8;
     }

     Tlc.update();
     delay(200);
  }

  pinValues = read_shift_regs();
  if(pinValues != oldPinValues) {
      //Serial.print("*Pin value change detected*\r\n");
      cup_light();
      oldPinValues = pinValues;
  }
}

void wave()
{
  for(int i = 0; i < 11; i++)
    {
        if(i < 8)
        {
        setRow(i,4095);
        
        }

        if(i > 0 && i < 9)
        {
        setRow(i - 1,1000);
        }

        if(i > 1 && i < 10)
        {
          setRow(i - 2,100);

        }

        if(i > 2)
        {
          setRow(i - 3,0);

        }
      
        Tlc.update();
        delay(100);
    }

     pinValues = read_shift_regs();
    if(pinValues != oldPinValues) {
      //Serial.print("*Pin value change detected*\r\n");
      cup_light();
      oldPinValues = pinValues;
    }
     
}
void setRow(int row, int pwm)
{
  for (int i = 0; i < 8; i++) 
  {
       Tlc.set(relative[i + (row * 8)], pwm);
  }
}

void snake()
{

  pinValues = read_shift_regs();
  if(pinValues != oldPinValues) {
      //Serial.print("*Pin value change detected*\r\n");
      cup_light();
      oldPinValues = pinValues;
  }
  int i = 0, j = 0, h = 15, k = 1;
  int row = 0;
  
   for (row = 0; row < 8; row++)
    {
      if((row % 2) == 0)
      {
        for(i = (8 * row); i < (8*(row + 1)) ; i++)
        {
            Tlc.set(relative[i], 2047);
            
            if(row > 0)
            {
              Tlc.set(relative[i - k], 0);
              k = k + 2;
            }

            Tlc.update();
            delay (100);
        }

       k = 1;
      }     
      
      else if ((row % 2) == 1)
      {
        for(j =((8 * (row + 1)) - 1); j >= (8*row) ; j--)
        {
            Tlc.set(relative[j], 2047);
            
            if(row == 1)
              Tlc.set(relative[h-j], 0);
            else
            {
              Tlc.set(relative[j-h], 0);
              h= h- 2;
            }
            
            Tlc.update();
            pinValues = read_shift_regs();
            delay (100);
        }
        
            h= 15;
      }
    }

    for(i = 63; i > 55; i--)
    {
      Tlc.set(relative[i], 0);
      Tlc.update();
      pinValues = read_shift_regs();
      if(pinValues != oldPinValues) {
          //Serial.print("*Pin value change detected*\r\n");
          cup_light();
          oldPinValues = pinValues;
      }
      delay (100);
    }
}

int L[64] = {2047, 2047, 2047, 0, 0, 0, 0, 0,
             0, 0, 2047, 0, 0, 0, 0, 0,
             2047, 2047, 2047, 0, 0, 0, 0, 0,
             0, 0, 2047, 2047, 2047, 0, 0, 0,
             0, 0, 2047, 0, 2047, 0, 0, 0,
             0, 0, 2047, 0, 2047, 2047, 2047, 2047,
             0, 0, 0, 0, 2047, 0, 2047, 0,
             0, 0, 0, 0, 2047, 0, 0, 0};

void logo() {
 
    int i;

    for (i = 0; i < 64; i++) {
        Tlc.set(relative[i], L[i]);
    }
    Tlc.update();
    pinValues = read_shift_regs();
    if(pinValues != oldPinValues) {
      //Serial.print("*Pin value change detected*\r\n");
      cup_light();
      oldPinValues = pinValues;
    }
}

void setup()
{
  Tlc.init();
    Serial.begin(9600);

    /* Initialize our digital pins...
    */
    pinMode(ploadPin, OUTPUT);
    pinMode(clockEnablePin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, INPUT);

    digitalWrite(clockPin, LOW);
    digitalWrite(ploadPin, HIGH);

    /* Read in and display the pin states at startup.
    */
    
}

int pinExclusion[13] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 2048, 4096, 1024};

static int j = 0;
void loop()
{
  Tlc.set(relative[27], 4095);
  Tlc.update();

  int l = 0;

  while (l < 1) {
      pinValues = read_shift_regs();
      if (pinValues != oldPinValues) {
          Tlc.set(relative[27], 4095);
          Tlc.update();
          l++;
      }
      oldPinValues = pinValues;
  }

  
    /* Read the state of all zones.
    */
   // pinValues = read_shift_regs();
    
    
    /* If there was a chage in state, display which ones changed.
    */


    /*for (i = 0; i < 64; i++) {
        Tlc.set(relative[i], 2047); 
        Tlc.update();
        Serial.print(i);
        Serial.print("\n");
        delay(100);
    }*/

    int j = 0;


    Tlc.clear();
    while (j < 12) {
        pinValues = read_shift_regs();
       
        if(pinValues != oldPinValues)
        {
            Serial.print("*Pin value change detected*\r\n");
            ignite_pin_values();
            oldPinValues = pinValues;
    
            if (pinCheck()) {
                chimera++;
                Serial.print(chimera);
                Serial.print("\n");
              
            }               
        }

        if ((chimera%5) == 0) {
           ignite_pin_values();
        }

        if ((chimera%5) == 1) {
          snake();
          delay(2000);
        }

        if ((chimera%5) == 2) {
          wave();
          delay(2000);
        }

        if ((chimera%5) == 3) {
          Tlc.clear();
          starwars();
          delay(2000);
        }

        if ((chimera%5) == 4) {
          logo();
          delay(5000);
          Tlc.clear();
        }
    }

    delay(POLL_DELAY_MSEC);
}

boolean pinCheck() {
   
    for(int i = 0; i < DATA_WIDTH; i++)
    {

        if((pinValues >> i) & 1) {
            if ((i == 10)) {
              Serial.print("HIGH");
              Serial.print("\n");
              return false;
            }

        }
        else {
           if ((i == 10)) {
              Serial.print("LOW");
              Serial.print("\n");
              return true;
           }
        }
        
        
    }
    return false;
}

int power (int x, int y) {

  if (y > 0) {
       x = x*(power(x, y-1));
  }
  else {
    return 1;
  }
  
  return x;
}
