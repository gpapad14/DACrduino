/*
 Giorgos PAPADOPOULOS, 6 Feb 2020
 LPNHE, Sorbonne U, Jussieu, 75005, Paris 

 Code for an Arduino Mega2560 to control the EVAL-AD5791 20bit DAC board.
 
 Open the Serial Monitor, top right corner of this window if opened with the Arduino app.
 In the Serial Monitor set "Autoscroll" (optional), "Show timestamp" (optional), "No line ending" ( MANDATORY! ), "57600 baud" ( MANDATORY! ).
 After uploading the script the DAC output is set at a certain voltage. I originally chose DACin=524288 -> ~0V.
 To stop operating, just unplug the Arduino from the PC or power it off.
*/

#include <SPI.h>
#include <math.h>
#include <stdio.h>      /* printf */
#include <stdlib.h> /* system, NULL, EXIT_FAILURE */

// Instead of using the LDAC, CLR and RESET one can set the configuration only with MOSI/SDIN in the Software Control Register (page23)
// Also, if one wants fixed values like NOT(LDAC)=Low, NOT(CLR)=High, NOT(RESET)=High, one can play with the jumpers LK3,LK11,LK4 respectively.
//const int LDAC  = 42; // random choice of pin
//const int CLR   = 44; // random choice of pin
//const int RESET = 46; // random choice of pin
const int SYNC = 53; // Slave Select (SS) or Chip Select (CS) or SYNC. Not random choice, Arduino Mega2560 provides SS by default in pin 53
const int SDO  = 50; // SDO or MISO is provided by default in pin 50 or ICSP-1 for Arduino Mega2560.
// SDIN or MOSI is provided by default in pin 51 or ICSP-4 for Arduino Mega2560

// SCLK is provided by pin 52 or ICSP-3 for Arduino Mega2560

byte PrimByte, SecByte, ThirdByte; // Primary, secondary, trird bytes 8 bits each = 24 bits in total
double input;
String inputSTR;
float vrefN=-10.;
float vrefP=10.;
double DACoutput;
double  STARTCODE, STOPCODE, STEPCODE, DELAYTIME;
String inFolder="folder/";



void setup() {
  Serial.begin(57600);
  Serial.println(" --- Setting up DAC");
  //pinMode(LDAC, OUTPUT);
  //pinMode(CLR, OUTPUT);
  //pinMode(RESET, OUTPUT);
  pinMode(SYNC,OUTPUT);
  pinMode(SDO, INPUT);

  //digitalWrite(LDAC, LOW);
  //digitalWrite(CLR, HIGH);
  //digitalWrite(RESET, HIGH);
  digitalWrite(SDO, HIGH);
  digitalWrite(SYNC, HIGH);

  // initialize SPI.
  SPI.begin();
  SPI.beginTransaction( SPISettings(20000000,MSBFIRST,SPI_MODE2) );
  // instead beginTransaction one can use the following individual commands:
  //SPI.setBitOrder(MSBFIRST);
  //SPI.setDataMode(SPI_MODE1); // options SPI_MODE? where ?=0,1,2,3. The DAC can support all 4, for different purposes. 

  // Set DAC in control-register: let the DAC listen to you.
  digitalWrite(SYNC, LOW);
  SPI.transfer(B00100000); // the first 4 digits 0010 -> "Write to the control register" (page 19)
  SPI.transfer(B00000000);
  SPI.transfer(B00010010); // 6 last digits are SDODIS, BIN/2sC, DACTRI, OPGND, RBUF, (not important). Look page 22 to understand what is what.
  digitalWrite(SYNC, HIGH);
  
  Serial.print("Control register : ");
  Serial.print(B00100000, BIN);
  Serial.print(" ");
  Serial.print(B00000000, BIN);
  Serial.print(" ");
  Serial.println(B00010010, BIN);
    
  // We have finished with configuring the control register. Move on to something else.

  //SetVoltageOutput(524288); // DAC input range [0, 2^20-1]. 524288 corresponds to 0V. The +1 will be done in SetVoltageOutput().
  //SetVoltageOutput(500000);
  Serial.println(" --- Incert the DAC input (integer) code in the range [0 (refN), 2^20-1=1048575 (refP-1LSB)]."); // The 0 value is excluded because of a bug in the loop.


  //system("mkdir ZZfoldername");

}


void SetVoltageOutput(long programVal) {
  Serial.print("DACin : "); // DAC input code
  Serial.print(programVal); //  DAC input range [0, 2^20-1].
  DACoutput=(vrefP-vrefN)*(programVal)/(pow(2,20)-1) + vrefN;

  digitalWrite(SYNC, LOW); // Let the DAC listen to you.
  PrimByte  = programVal >> 16;
  //Serial.println(PrimByte, BIN);
  SecByte   = programVal >> 8;
  //Serial.println(SecByte, BIN);
  ThirdByte = programVal;  
  //Serial.println(ThirdByte, BIN);
  PrimByte  = B00010000 ^ PrimByte; // 4 first digits 0001 is DAC register address, "Write to the DAC register" (page 19)
  //Serial.println(PrimByte, BIN);

  // Send the 3 bytes.
  SPI.transfer(PrimByte);
  SPI.transfer(SecByte);
  SPI.transfer(ThirdByte);
  digitalWrite(SYNC, HIGH); // Make DAC stop listening 

  //Serial.print("  ("); 
  //Serial.print(PrimByte, BIN);
  //Serial.print(" ");
  //Serial.print(SecByte, BIN);
  //Serial.print(" ");
  //Serial.print(ThirdByte, BIN);
  //Serial.print(")  ");
  Serial.print(" , DACout : ");
  Serial.print(DACoutput, 4);
  Serial.println(" v");

  //system("mkdir ~/Desktop/foldername");
}


void loop() {
  if(Serial.available()>0) {
    inputSTR = Serial.readString();
    //inputSTR = Serial.readStringUntil('\n'); // Fixes the issue in case of "Newline" option is chosen at the Serial Monitor, but can also run normally alternatively, just shows a warning.
    //Serial.print("inputSTR = ");
    //Serial.println(inputSTR); 
    String word1="";
    word1+=inputSTR[0];
    word1+=inputSTR[1];
    word1+=inputSTR[2];
    word1+=inputSTR[3];
    word1+=inputSTR[4];
    //Serial.println(word1);  
    input=inputSTR.toInt();
    //Serial.println("input = ");
    //Serial.println(input);


    
    // --- SCAN action
    if(inputSTR == "SCAN") { // Just type " SCAN " with capital letter and "Send" in the Serial Monitor.
// !!! If something goes wrong with the loop, just push the reset button on the Arduino!  
      STARTCODE = 0;        // myst be in the valid range, eg 0 (min)
      STOPCODE  = 1048575;  // must be in the valid range, eg 1048575 (max)
      STEPCODE  = 40960;     // must be integer, at least 1, eg 1024 in full range ~1000 values (1024, 2048, 4096)
      DELAYTIME = 100;      // in ms.
      Serial.print(" --- SCAN from "+ String(STARTCODE,0) + " to " + String(STOPCODE,0) + " with step "+String(STEPCODE,0) );
      if(DELAYTIME>0) { Serial.println(" and delay " + String(DELAYTIME) +" ms"); }
      else { Serial.println(""); }
      int icounter=0;
      double j=STARTCODE;
      // As the while loop runs no messages can interrupt the Arduino. 
      // The first message (only) will be kept and will run as soon as the loop stops. 
      while(j<=STOPCODE) { 
        Serial.print("#"+ String(icounter)+" ");
        SetVoltageOutput(j);
        delay(DELAYTIME);
        j+=STEPCODE;
        icounter++;
      }
      Serial.println(" --- END SCAN : total number of values : " + String(icounter));
    }
    // --- RESET action
    else if(inputSTR=="RESET"){ // Just type " RESET " with capital letter and "Send" in the Serial Monitor.
      // RESET can be performed with the reset button on the Arduino as well.
      Serial.print(" === ");
      Serial.println(inputSTR);
      Serial.println("");
      setup();
    }
    
    else if(word1=="SCAN#") {
      Serial.println("-- We are in");
      String startCode="";
      String stopCode="";
      String stepCode="";
      String delayTime="";
      int found=0;
      int ichar=5;
      while(found<4 && ichar<inputSTR.length()) {
        String letter="";
        letter+=inputSTR[ichar];
        if(letter=="#"){
          ichar++;
          found++;
        }
        else {
          if(found==0){       startCode+=inputSTR[ichar]; }
          else if(found==1){  stopCode+=inputSTR[ichar];  }
          else if(found==2){  stepCode+=inputSTR[ichar];  }
          else if(found==3){  delayTime+=inputSTR[ichar]; }
          ichar++;
        }
      }
      //Serial.println("Start: " +startCode+", Stop: "+stopCode+", Step: "+stepCode+", Delay: "+delayTime);
      STARTCODE = startCode.toInt();        // myst be in the valid range, eg 0 (min)
      STOPCODE  = stopCode.toInt();  // must be in the valid range, eg 1048575 (max)
      STEPCODE  = stepCode.toInt();     // must be integer, at least 1, eg 1024 in full range ~1000 values (1024, 2048, 4096)
      DELAYTIME = delayTime.toInt();      // in ms.
      
      if(STEPCODE==0){ STEPCODE=40960; }
      //if(DELAYTIME==0){ DELAYTIME=50; }
      if(
        STARTCODE<0 || STARTCODE>1048575 ||
        STOPCODE<0 || STOPCODE>1048575 ||
        STEPCODE<-1048575 || STEPCODE>1048575 ||
        DELAYTIME<0 || DELAYTIME>60000) {
          Serial.println("- FATAL ERROR!!! 101");
        }
      else{
        Serial.print(" --- SCAN from "+ String(STARTCODE,0) + " to " + String(STOPCODE,0) + " with step "+String(STEPCODE,0) );
        if(DELAYTIME>0) { Serial.println(" and delay " + String(DELAYTIME) +" ms"); }
        else { Serial.println(""); }
        int icounter=0;
        double j=STARTCODE;
        // As the while loop runs no messages can interrupt the Arduino. 
        // The first message (only) will be kept and will run as soon as the loop stops. 
        if(STARTCODE<STOPCODE && STEPCODE<0){
          Serial.print("Warning! For a rising SCAN give a positive STEPCODE.");
        }
        else if(STARTCODE<STOPCODE && STEPCODE>0){
          while(j<=STOPCODE && j>=0 && j<1048575 ) { 
            Serial.print("#"+ String(icounter)+" ");
            SetVoltageOutput(j);
            delay(DELAYTIME);
            j+=STEPCODE;
            icounter++;
          }
        }
        else if(STOPCODE<STARTCODE && STEPCODE>0){
          Serial.print("Warning! For a falling SCAN give a negative STEPCODE.");
        }
        else if(STOPCODE<STARTCODE  && STEPCODE<0){
          while(j>=STOPCODE && j>=0 && j<1048575 ) { 
            Serial.print("#"+ String(icounter)+" ");
            SetVoltageOutput(j);
            delay(DELAYTIME);
            j+=STEPCODE;
            icounter++;
          }
        }
        
        Serial.println(" --- END SCAN : total number of values : " + String(icounter));
      }
      
    }
    // SCAN#500000#550000#5000#100#
    
    else if(inputSTR == "DACregout") {      
      digitalWrite(SYNC, LOW);
      SPI.transfer(B10010000); // the first 4 digits 0010 -> "Write to the control register" (page 19)
      SPI.transfer(B0000000);
      SPI.transfer(B00000000); // 6 last digits are SDODIS, BIN/2sC, DACTRI, OPGND, RBUF, (not important). Look page 22 to understand what is what.
      digitalWrite(SYNC, HIGH);
    }

    
    // New line will give inputSTR.toInt(\"\n")=0. Stop these cases. 
    else if(input==0 && inputSTR!="0") {
      Serial.print(inputSTR);
      Serial.println("UNACCEPTED input. Please incert an interger number in the range 0 to 1048575.");
    }
    // Stop non-integer inputs.
    else if( ((inputSTR.toFloat()-input)!=0) ) {
      Serial.print(inputSTR.toFloat());
      Serial.println(" : UNACCEPTED input. Please incert an interger number in the range 0 to 1048575.");
    }
    // Stop inserted values out of the input range.
    else if( input<0 || input>1048575 ) {
      Serial.print(input);
      Serial.println(" : UNACCEPTED input. Please incert an interger number in the range 0 to 1048575.");
    } 
    // The 0 value is the first output value and corresponds to the negative reference voltage.
    else if(input>=0 && input<=1048575) {
      SetVoltageOutput(input);
    }


  }
}
