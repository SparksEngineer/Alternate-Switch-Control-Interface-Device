/*********************************************************************
  This program was created from an example for the nRF51822 based
  Adafruit Bluefruit LE modules. Like the Feather 32u4 Bluefruit LE,
  and the Feather M0 Bluefruit LE.

  Check out our instructable to understand the code more
  and learn about the circuit and components used for this project

  MIT license, check LICENSE for more information
  All text above, and the splash screen below must be included in
  any redistribution
*********************************************************************/
#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"
#include "Keyboard.h"

#if SOFTWARE_SERIAL_AVAILABLE
#include <SoftwareSerial.h>
#endif
/*=========================================================================
    APPLICATION SETTINGS

      FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
     
                                Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                                running this at least once is a good idea.

    MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
    -----------------------------------------------------------------------*/
#define FACTORYRESET_ENABLE         0
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
//    #define MODE_LED_BEHAVIOUR          "MODE"//used for BLE UART communication
/*=========================================================================*/
//Setting up what kind of BLE hardware the Micontroler has
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);


//Constants that won't change. They're used here to set pin numbers:
/**In the the Feather32u4 A1=D19, and in the Feather M0 A1=D15**/
const int GreenLED = 15;//GREEN LED connected to pin A1
const int RedLED = 16;//RED LED connected to pin A2
const int VBATPIN = 9;//Vatpin is A7 or D9 for Feather 32u4

const int SWITCH1 = 12; // Mode 1 switch (Left most setting)
const int SWITCH2 = 11; //Mode 2 switch (Right most setting)
const int BUTTON1 = 10;// Digital pin 10
const int BUTTON2 = 17;//A3 is Digital pin 21 for Feather 32u4
//For the Feather M0 the A3 is Digital pin 17
//Setting wait time that will make sure button signal is good
const int NoBounceDelayTime = 200;//Quick delay in microseconds

/**** Variables that must be intitiated but will change values ****/
//For saving Status of mode switch:
int SwitchState1 = 0; // variable for reading the Mode Switch status
int SwitchState2 = 0; // variable for reading the Mode Switch status
int CurrentSwitch1State = 0;
int CurrentSwitch2State = 0;
int LastSwitch1State = 0;
int LastSwitch2State = 0;

//Variables to save time in milliseconds (used for debuuging):
unsigned long Button1TotalTime = 0;//Will save total pressed time of Button1
unsigned long Button1PressedTime1 = 0;//Will save first time Button1 is pressed
unsigned long Button1PressedTime2 = 0;//Will save last time Button1 is pressed

unsigned long Button2TotalTime = 0;//Will save saving total pressed time of Button2
unsigned long Button2PressedTime1 = 0 ;//Will save first time Button1 is pressed
unsigned long Button2PressedTime2 = 0;//Will save last time Button1 is pressed

void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}
//code lines that are run only once are put here
void setup() {
  //Initializing or setting Up Communication for MCU;
  //  while (!Serial);
  Serial.begin(115200);//Initialize Serial Communication at a 115200 Baud Rate
  Keyboard.begin();// initialize keyboard communication: (aka set control over keyboard)

  Serial.println(F("Adafruit Bluefruit HID Keyboard Example"));
  Serial.println(F("---------------------------------------"));

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ) {
      error(F("Couldn't factory reset"));
    }
  }
  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();
  /* Change the device name to make it easier to find */
  Serial.println(F("Setting device name to 'Bluefruit Keyboard': "));
  if (! ble.sendCommandCheckOK(F( "AT+GAPDEVNAME=Bluefruit Keyboard" )) ) {
    error(F("Could not set device name?"));
  }

  /* Enable HID Service */
  Serial.println(F("Enable HID Service (including Keyboard): "));
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    if ( !ble.sendCommandCheckOK(F( "AT+BleHIDEn=On" ))) {
      error(F("Could not enable Keyboard"));
    }
  } else
  {
    if (! ble.sendCommandCheckOK(F( "AT+BleKeyboardEn=On"  ))) {
      error(F("Could not enable Keyboard"));
    }
  }
  /* Add or remove service requires a reset */
  Serial.println(F("Performing a SW reset (service changes require a reset): "));
  if (! ble.reset() ) {
    error(F("Couldn't reset??"));
  }
  // Initializing the button pin as an input with Pullup resistor (HIGH when not pressed logic):
  pinMode(VBATPIN, INPUT);
  pinMode(BUTTON1, INPUT_PULLUP);//"HIGH"(ON) until pressed, then "LOW"(OFF)
  pinMode(BUTTON2, INPUT_PULLUP);//"HIGH"(ON) until pressed, then "LOW"(OFF)

  // Initialize the SWITCH  pin as an input:
  pinMode(SWITCH1, INPUT_PULLDOWN);//"LOW"(OFF) until "pressed", then goes to "HIGH"(ON)
  pinMode(SWITCH2, INPUT_PULLDOWN);//"LOW"(OFF) until "pressed", then goes to "HIGH"(ON)
  pinMode(GreenLED, OUTPUT);//RED LED
  pinMode(RedLED, OUTPUT);//GREEN LED
}
/*********************************************************************
  Mode 1 = Switch is to the left. As such BLE keyboard mode
  Mode 2 = Switch is to the right. As such USB Keyboard mode
  Mode 0 = Switch is Centered. As such the BATTery voltage is sent to device as text
*********************************************************************/
void loop() {
  // Read the of the Switch value, and reset LastSwitch1State to LOW:
  LastSwitch1State = 0;
  LastSwitch2State = 0;
  CurrentSwitch1State = digitalRead(SWITCH1); //save the state of switch 1st pin (the most left)
  CurrentSwitch2State = digitalRead(SWITCH2); //save the state of switch last pin (the most right)
  /***********************************************************************/
  // Note to self for Button/Switch States:
  //     Buttons are "HIGH" until pressed, then they got to LOW
  //     Switch pin is "LOW" until moved/activated, then they got to HIGH
  /***********************************************************************/

  //************************** Mode 1, Bluetooth Keyboard mode **************************/
  while (digitalRead(SWITCH1) == HIGH) { //While Multi Mode Switch is to the Left (pin 12 is ON) stay in "Mode 1"
    CurrentSwitch1State = digitalRead(SWITCH1);
    CurrentSwitch2State = digitalRead(SWITCH2);

    /***If statement to send Serial msg only once to Serial monitor***/
    if ((CurrentSwitch1State == 1) && (LastSwitch1State == 0))
    {
      Serial.println("Mode 1, BLE mode");//Send Serial.print line once
      LastSwitch1State = CurrentSwitch1State;//save that the last state is now 1 so we don't go into this loop
      delay(100);//very small wait
    }

    /******** Start the actual code for Mode 1 *********/
    //------Section for when Button 1 is pressed------
    if (digitalRead(BUTTON1) == LOW) {//if button1 is pressed
      delay(NoBounceDelayTime);//Quick delay to make sure keypress is not sent twice
      digitalWrite(GreenLED, HIGH);//Turn ON Green LED

      Serial.println("Button 1 was pressed");
      //      //Save at what time the button was 1st pressed
      //      Button1PressedTime1 = millis();//save at what time the button was 1st pressed

      /****Send "w" keypress without releasing the key yet****/
      ble.println("AT+BLEKEYBOARDCODE=00-00-1A");//Sending code for "w" key which is 0x1A in HEX
      //The "Long Press" feature is included with this while loop
      while (digitalRead(BUTTON1) == LOW) { //while button is still pressed wait
        Serial.println("Button 1 looong press");
        //        //Save at what time the button was last pressed
        //        Button1PressedTime2 =  millis();
      }
      /*The following lines are to send debugging msgs to the serial monitor only*/
      //      if (Button1PressedTime2 > Button1PressedTime1) { //only if time 2 is larger than time1
      //        Button1TotalTime = Button1PressedTime2 - Button1PressedTime1;//calculate total spent time
      //        Serial.print("Button1 Total Time pressed: ");
      //        Serial.println(Button1TotalTime);
      //      }
      //      Button1TotalTime = 0;//reset the time counted

      /**** Release all pressed keys ****/
      ble.println("AT+BLEKEYBOARDCODE=00-00");//Indicating that to release all the keys
      delay(NoBounceDelayTime);//Small wait to make sure "switch bouncing" effect is stopped
    }//End loop for pressing Button 1

    //------Section for when Button 2 is pressed------
    if (digitalRead(BUTTON2) == LOW) {//if button2 is pressed
      delay(NoBounceDelayTime);//Small wait to make sure "switch bouncing" effect is stopped
      digitalWrite(GreenLED, HIGH);//Turn ON Green LED
      Serial.println("Button 2 was pressed");

      //      //Save at what time the button was 1st pressed
      //      Button2PressedTime1 = millis();//save at what time the button was 1st pressed

      //****Send "w" keypress without releasing****/
      ble.println("AT+BLEKEYBOARDCODE=00-00-2C");//Sending code for "w" key which is 0x1A in HEX

      //The "Long Press" feature is included with this while loop
      while (digitalRead(BUTTON2) == LOW)//while button is still pressed
      {
        Serial.println("Button 2 looong press");
        //        Button2PressedTime2 =  millis();//save at what time the button was pressed
      }
      /*The following lines are to send debugging msgs to the serial monitor only*/
//      Button2TotalTime = Button2PressedTime2 - Button2PressedTime1;//calculate total spent time
//      if (Button2PressedTime2 > Button2PressedTime1) { //only if time 2 is larger than time1
//        Serial.print("Button2 Total Time pressed: ");
//        Serial.println(Button2TotalTime);
//      }
//      Button2TotalTime = 0;//reset the time counted
//      //**** Release all pressed keys ****/
      ble.println("AT+BLEKEYBOARDCODE=00-00");//Indicating that to release all the keys
      delay(NoBounceDelayTime);//Small wait to make sure "switch bouncing" effect is stopped
    }//End loop for pressing Button 2
    
    else { //if a button is not pressed keep Green LED off
      digitalWrite(GreenLED, LOW);//Green LED off
    }

    float measuredvbat = analogRead(9);
    measuredvbat *= 2;    // we divided by 2, so multiply back
    measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
    measuredvbat /= 1024; // convert to voltage
    Serial.print("VBat: " );
    Serial.println(measuredvbat);

    //when voltage value is LOW
    if (measuredvbat < 3.4) {
      digitalWrite(RedLED, HIGH);//Turn ON Red LED
    }
    //when voltage value is High
    if (measuredvbat > 3.6) {
      digitalWrite(RedLED, LOW);//Turn Off Red LED
    }
    
  }//end of Mode 1 (when SwitchState1 is HIGH/ON)

  /**************************************Mode 2, USB Keyboard mode**************************************/
  while (digitalRead(SWITCH2) == HIGH) { //While Multi Mode Switch is to the right (pin 11 is ON) stay in "Mode 2"
    CurrentSwitch1State = digitalRead(SWITCH1);
    CurrentSwitch2State = digitalRead(SWITCH2);

    /***If statement to send Serial msg only once to Serial monitor***/
    if ((CurrentSwitch2State == 1) && (LastSwitch2State == 0))//If loop to send Serial.print once
    {
      Serial.println("----Mode 2, USB mode ON----");//when done print this msg once
      //save that the last state is 1 so we don't go into this loop
      LastSwitch2State = CurrentSwitch2State;
      delay(100);//small wait
    }

    /***** Start the actual code for Mode 2 ******/
    //**Section for when Button 1 is pressed, Short Press and Long Press included with while loop****/
    if (digitalRead(BUTTON1) == LOW) {//if button1 is pressed
      delay(NoBounceDelayTime);//Small wait to make sure "switch bouncing" effect is stopped
      digitalWrite(GreenLED, HIGH);//Turn ON LED
      Serial.println("Button 1 was pressed");

      //Save at what time the button was 1st pressed
      Button1PressedTime1 = millis();//save at what time the button was 1st pressed

      //****Send "w" keypress without releasing****/
      Keyboard.press(119);//Sending code for "w" key which is 119 in ASCII

      while (digitalRead(BUTTON1) == LOW)//while button is still pressed
      {
        Serial.println("Button 1 looong press");
        //        Button1PressedTime2 = millis();//save at what time the button was pressed
      }
      /*The following lines are to send debugging msgs to the serial monitor only*/
      //      if (Button1PressedTime2 > Button1PressedTime1) { //only if time 2 is larger than time1
      //        Button1TotalTime = Button1PressedTime2 - Button1PressedTime1;//calculate total spent time
      //        Serial.print("Button1 Total Time pressed: ");
      //        Serial.println(Button1TotalTime);
      //        Button1TotalTime = 0;//reset the time counted
      //      }
      //**** Release all pressed keys ****/
      Keyboard.releaseAll();//Lets go of all keys currently pressed
      delay(NoBounceDelayTime);//Small wait to make sure "switch bouncing" effect is stopped
    }//End loop for pressing Button 1

    /******Section for when Button 2 is pressed, Short Press and Long Press included with while loop****/
    if (digitalRead(BUTTON2) == LOW) {//if Button2 is pressed
      delay(NoBounceDelayTime);//Small wait to make sure "switch bouncing" effect is stopped
      digitalWrite(GreenLED, HIGH);//Turn ON LED

      Serial.println("Button 2 was pressed");
      //      //Save at what time the button was 1st pressed
      //      Button2PressedTime1 = millis();

      //****Send "spacebar" keypress without releasing****/
      Keyboard.press(32);//Sending code for "spacebar" key which is 32 in ASCII

      /**Serial Print lines are for debugging only, they may be commented out*/
      while (digitalRead(BUTTON2) == LOW) {//Wait in loop while button is still pressed
        Serial.println("Button 2 looong press");
        //        Button2PressedTime2 = millis();//save at what time the button was pressed
      }
    }//End loop for pressing button 2
    else {
      //if no button is pressed keep LED off
      digitalWrite(GreenLED, LOW);
    }
    /*The following lines are to send debugging msgs to the serial monitor only*/
    //            if (Button2PressedTime2 > Button2PressedTime1) { //only if time 2 is larger than time1
    //              Button2TotalTime = Button2PressedTime2 - Button2PressedTime1;//calculate total spent time
    //              Serial.print("Button2 Total Time pressed: ");
    //              Serial.println(Button2TotalTime);
    //              Button2TotalTime = 0;//reset the time counted
    //            }
    /**** Release all pressed keys ****/
    Keyboard.releaseAll();//Lets go of all keys currently pressed
  }//End of Mode 2
  /**************************************Mode 0, Only Information Mode**************************************/
  //While Multi Mode Switch is Centered (OFF) stay in "Mode 0"
  while ((digitalRead(SWITCH1) == LOW) && (digitalRead(SWITCH2) == LOW)) {
    CurrentSwitch1State = digitalRead(SWITCH1);
    CurrentSwitch2State = digitalRead(SWITCH2);

    if (((CurrentSwitch1State == 0) && (LastSwitch1State == 0)) || ((CurrentSwitch2State == 0) && (LastSwitch2State == 0))) {
      Serial.println("----Mode 0, Battery Mode ON----");//when done print this msg
      LastSwitch1State = 1;//save that the last state is now 1, so we don't go into this loop again ever again
      LastSwitch2State = 1;//save that the last state is now 1, so we don't go into this loop again ever again
    }
    float measuredvbat = analogRead(VBATPIN);
    measuredvbat *= 2;//Value was divided by 2 in MCU, so multiply back
    measuredvbat *= 3.3;// Multiply by 3.3V, our reference voltage
    measuredvbat /= 1024;// Convert Analog Input to a voltage
    Serial.print("VBat: " );
    Serial.println(measuredvbat);

        //when voltage value is LOW
        if (measuredvbat < 3.4) {
          digitalWrite(GreenLED, LOW);//Turn Green LED Off
          digitalWrite(RedLED, HIGH);//Turn red LED On
        }
        //when voltage value is HIGH
        if (measuredvbat > 3.6) {
          digitalWrite(GreenLED, HIGH);//Turn Green LED On
          digitalWrite(RedLED, LOW);//Turn red LED Off
        }
  }//end of Mode 0
}//END OF VOID LOOP
