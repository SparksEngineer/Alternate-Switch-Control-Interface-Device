///////////////////////////////////////////////////////////////////////
//  This code is to make the servo move from 45 to 90 degrees and     //
//  90 to 45 degrees with a press of a button. Also this code, once   //
//  the button is pressed will not move until pressed again.         //
//////////////////////////////////////////////////////////////////////

#include<Servo.h>

// defining the constant pins for the microcontroller 
#define ServoPin 9
#define button_pin 2

// This will be the the variable of the code
int button_pressed = LOW; // This is when the button is pressed is down
int Servo_angle = 0; // Setting the angle of the servo
Servo servo; // Making the Servo library into the variable servo


void setup() {
  // Setting up the code
  Serial.begin(115200);
  servo.attach(ServoPin); //This will attach the servo to the designded pin
  servo.write(45);
  pinMode(button_pin,INPUT_PULLUP); // This will help make the pin work using an internal resistance
  Serial.println("Servo on");
  
}

void loop() {
  // Making the loop for the servo movement form 45 to 90 degrees 
  
  if (digitalRead(button_pin) == button_pressed ){
        Servo_angle =90;
  }
  else {
       Servo_angle =45;
     
    }
      servo.write(Servo_angle);
  }
