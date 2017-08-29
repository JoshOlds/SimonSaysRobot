/* SimonSays.ino
*  Arduino project by Joshua Olds & Jake Overall (Team Jankiworks)
*  This file is the main Arduino project file for the Arbotix-Dynamixel servo based Robot Arm Simon Says game!
*  Contained within is the complete game logic, sound effects, and arm servo control functions
*  This project was created as a demo for an upcoming BCW Robotics Course
*/


#include <ax12.h>
#include <BioloidController.h>
#include "poses.h"

//Define pins
#define YEL_BTN 19
#define WHT_BTN 16
#define RED_BTN 18
#define GRN_BTN 17

#define YEL_LED 12
#define WHT_LED 13
#define RED_LED 15
#define GRN_LED 14

#define SPEAKER_PIN 3
#define YEL_SOUND 500
#define WHT_SOUND 1000
#define RED_SOUND 1500
#define GRN_SOUND 2000

//Version info
const String VERSION = "Simon Says Bot V 1.0";

//Setup objects and global variables
BioloidController bioloid = BioloidController(1000000);

//Game State Variables
#define WAITING_STATE 0
#define ARM_PLAYBACK_STATE 1
#define ARM_LOSE_STATE 2
#define PLAYER_PLAYBACK_STATE 3
#define ARM_WIN_STATE 4

int GAME_STATE = WAITING_STATE;
int SIMON_ARRAY[20];
int SIMON_ARRAY_RECORD_INDEX = 0;
bool WAITING_FOR_PLAYER = false;
int MOCK_CHANCE = 10; //Chance to mock player (1 in xx)
long SERIAL_LAST_TIME = 0;
long SERIAL_TIMEOUT = 20;
char data = 0;
void setup(){
  
  //Setup Button Pins
  pinMode(YEL_BTN, INPUT);
  pinMode(WHT_BTN, INPUT);
  pinMode(RED_BTN, INPUT);
  pinMode(GRN_BTN, INPUT);
  
  //Setup LED Pins
  pinMode(YEL_LED, OUTPUT);
  pinMode(WHT_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GRN_LED, OUTPUT);
  
  //Setup Speaker
  pinMode(SPEAKER_PIN, OUTPUT);
  
  //Various other pins
  pinMode(0,OUTPUT);  
  
  //Seed the random generator with noise
  randomSeed(analogRead(0));
   
  //Confirm setup complete
  Serial.begin(1000000);
  delay (500);   
  //Serial.println(VERSION);
//  Serial.println(F("Setup Complete!"));
  
}

//Main game loop
void loop(){
  
  sendPositions();
  
  //State machine
  switch(GAME_STATE)
  {
    
    //Waiting for player input
    case ( WAITING_STATE ):
    {
     if( !WAITING_FOR_PLAYER )
     {
     //  Serial.println(F("Waiting for player input..."));
       WAITING_FOR_PLAYER = true;
       while( Serial.available() )
       {
         char temp = Serial.read();
       }
     }
     int currentButton = pollButtons();
     if(currentButton == 0){ currentButton = pollButtonsSerial(); }
     if(currentButton != 0)
     {
       SIMON_ARRAY[SIMON_ARRAY_RECORD_INDEX] = currentButton;
       SIMON_ARRAY_RECORD_INDEX++;
       GAME_STATE = ARM_PLAYBACK_STATE;
       WAITING_FOR_PLAYER = false;
       // PLAY TRANSITION SOUND TODO
     }
     break;
    }
     
     //Arm plays back button presses
     case ( ARM_PLAYBACK_STATE ):
     {
       for(int i = 0; i < SIMON_ARRAY_RECORD_INDEX; i++)
       {
         //Fake green press once in a while
         long randNumber = random(0, MOCK_CHANCE);
         if(randNumber == 0){armMockGreen();}
         bool buttonPressed = armPressButton(SIMON_ARRAY[i]);
         //The arm failed to press button
         if(!buttonPressed)
         {
           //Serial.println(F("Player Wins!! Arm failed to press button!"));
           GAME_STATE = ARM_LOSE_STATE;
           SIMON_ARRAY_RECORD_INDEX = 0;
           armMoveToPose(HOME, 1000, 0);
           return;
         }
       }
       GAME_STATE = PLAYER_PLAYBACK_STATE;
       // PLAY TRANSITION SOUND TODO
       break;
     } 
     
     //Player's turn to playback then transition to waiting state
     case ( PLAYER_PLAYBACK_STATE ):
     {
       for(int i = 0; i < SIMON_ARRAY_RECORD_INDEX; i++)
       {
         bool buttonPressed = waitForButton(SIMON_ARRAY[i]);
         //Player pressed wrong button
         if(!buttonPressed)
         {
           //Serial.println(F("Robot Wins!! Player pressed wrong button!"));
           GAME_STATE = ARM_WIN_STATE;
           SIMON_ARRAY_RECORD_INDEX = 0;
           return;
         }
       }
       GAME_STATE = WAITING_STATE;
       break;
     }
     
     case ( ARM_WIN_STATE ):
     {
       playPlayerLoseSong();
       armTakeABow();
       GAME_STATE = WAITING_STATE;
       break;
     }
     
     case ( ARM_LOSE_STATE ):
     {
       playPlayerWinSong();
       armHangInShame();
       GAME_STATE = WAITING_STATE;
       break;
     }
     
   }
}


///Tests the LEDs and pinouts by pulsing each led on and off for 500ms. Serial prints which pin is pulsing
void pulseLED(int pinNumber, int duration)
{
  digitalWrite(pinNumber, HIGH);
  playLEDSound(pinNumber, duration);
  delay(duration);
  digitalWrite(pinNumber, LOW);
}

///Plays sound at specified frequency (0-255)
void playSound(int freq, int duration)
{
 tone(SPEAKER_PIN, freq, duration);
}

///Helper function to translate LED pin to sound
void playLEDSound(int ledPin, int duration)
{
  switch (ledPin)
  {
    case ( YEL_LED ):
      playSound(YEL_SOUND, duration);
      break;
     case ( WHT_LED ):
      playSound(WHT_SOUND, duration);
      break;
     case ( RED_LED ):
      playSound(RED_SOUND, duration);
      break;
     case ( GRN_LED ):
      playSound(GRN_SOUND, duration);
      break;
  }
}

///Helper function. Translate a button number to LED number. Turns LED on or off
void changeButtonLED(int buttonNumber, bool onOff)
{
  switch ( buttonNumber ){
   case (YEL_BTN):
    digitalWrite(YEL_LED, onOff); 
    break;
   case (WHT_BTN):
    digitalWrite(WHT_LED, onOff); 
    break;
   case (RED_BTN):
    digitalWrite(RED_LED, onOff); 
    break;
   case (GRN_BTN):
    digitalWrite(GRN_LED, onOff); 
    break;
  }
}

///Tests the buttons by printing which buttons are currently pressed. Buttons are pulled-up, which is why we check for !NOT
void testButtons()
{
 //if(!digitalRead(YEL_BTN)){Serial.println(F("Yellow Button Pressed!"));} 
 //if(!digitalRead(WHT_BTN)){Serial.println(F("White Button Pressed!"));}
 //if(!digitalRead(RED_BTN)){Serial.println(F("Red Button Pressed!"));}
 //if(!digitalRead(GRN_BTN)){Serial.println(F("Green Button Pressed!"));}
}

///Polls all of the game buttons. Returns 0 if none pressed, or the button pin# if one is pressed
int pollButtons()
{
 if(!digitalRead(YEL_BTN)){
   Serial.println("B1               ");
   pulseLED(YEL_LED, 500);
   return YEL_BTN;
 } 
 if(!digitalRead(WHT_BTN)){
   Serial.println("B2               ");
   pulseLED(WHT_LED, 500);
   return WHT_BTN;
 }
 if(!digitalRead(RED_BTN)){
   Serial.println("B3               ");
   pulseLED(RED_LED, 500);
   return RED_BTN;
 }
 if(!digitalRead(GRN_BTN)){
   Serial.println("B4               ");
   pulseLED(GRN_LED, 500);
   return GRN_BTN;
 }
 return 0;
}

int pollButtonsSerial()
{
  char data = 0;
  if(Serial.available())
  {
    data  = Serial.read();
  }
  switch(data)
  {
    case '0':
    return 0;
    break;
  case'1':
   pulseLED(YEL_LED, 500);
   return YEL_BTN;
   break;
  case'2':
   pulseLED(WHT_LED, 500);
   return WHT_BTN;
   break;
 case '3':
   pulseLED(RED_LED, 500);
   return RED_BTN;
   break;
  case '4': 
   pulseLED(GRN_LED, 500);
   return GRN_BTN;
   break;
 }
 return 0;
}

///Helper function to move arm to pose stored in FLASH memory. Returns true if expected button was pressed. use 0 if no expected button
bool armMoveToPose(prog_uint16_t* pose, int interpolationTime, int expectedButton)
{
  bool buttonPressed = false;
  delay(100);                       // recommended pause
  bioloid.loadPose(pose);           // load the pose from FLASH, into the nextPose buffer
  bioloid.readPose();               // read in current servo positions to the curPose buffer
  bioloid.interpolateSetup(interpolationTime);   // setup for interpolation from current->next over 1/2 a second
  while(bioloid.interpolating > 0){ // do this while we have not reached our new pose
    
    if(expectedButton != 0)
    { //Check out button as we interpolate
      if(!digitalRead(expectedButton)){buttonPressed = true;}
    }
    bioloid.interpolateStep();      // move servos, if necessary.
    delay(3); 
    sendPositions();
    
  }
  return buttonPressed;
}

void sendPositions()
{
  long now = millis();
  if(now - SERIAL_LAST_TIME > SERIAL_TIMEOUT)
  {
    SERIAL_LAST_TIME = now;
    int pos1 = ax12GetRegister(1, 36, 2); 
    int pos2 = ax12GetRegister(2, 36, 2);
    int pos3 = ax12GetRegister(3, 36, 2);
    int pos4 = ax12GetRegister(4, 36, 2);
    Serial.print("A,");
    Serial.print(pos1);
    Serial.print(",");
    Serial.print(pos2);
    Serial.print(",");
    Serial.print(pos3);
    Serial.print(",");
    Serial.println(pos4);
  }
}
    

/// Press the yellow button! Return if the button was actually pressed
bool armPressYellow(int pauseTime)
{
  //Serial.println("Pressing Yellow Button!");
  armMoveToPose(HOME, 500, 0);
  armMoveToPose(YEL_PERCH, 500, 0);
  delay(pauseTime);
  bool buttonPressed = armMoveToPose(YEL_PRESS, 250, YEL_BTN);
  if(buttonPressed){
    playSound(YEL_SOUND, 500);
    Serial.println("B1");
  }
  armMoveToPose(YEL_PERCH, 500, 0);
  armMoveToPose(HOME, 500, 0);
  //if(buttonPressed){Serial.println(F("Arm Pressed Yellow Button Successfully!"));}
  //else{Serial.println(F("Arm Failed to press Yellow Button!"));}
  return buttonPressed;
}

/// Press the white button! Return if the button was actually pressed
bool armPressWhite(int pauseTime)
{
  //Serial.println("Pressing White Button!");
  armMoveToPose(HOME, 500, 0);
  armMoveToPose(WHT_PERCH, 500, 0);
  delay(pauseTime);
  bool buttonPressed = armMoveToPose(WHT_PRESS, 250, WHT_BTN);
  if(buttonPressed){
    playSound(WHT_SOUND, 500);
    Serial.println("B2");
  }
  armMoveToPose(WHT_PERCH, 500, 0);
  armMoveToPose(HOME, 500, 0);
  //if(buttonPressed){Serial.println(F("Arm Pressed White Button Successfully!"));}
  //else{Serial.println(F("Arm Failed to press White Button!"));}
  return buttonPressed;
}

/// Press the red button! Return if the button was actually pressed
bool armPressRed(int pauseTime)
{
  //Serial.println("Pressing Red Button!");
  armMoveToPose(HOME, 500, 0);
  armMoveToPose(RED_PERCH, 750, 0);
  delay(pauseTime);
  bool buttonPressed = armMoveToPose(RED_PRESS, 500, RED_BTN);
  if(buttonPressed){
    playSound(RED_SOUND, 500);
    Serial.println("B3");
  }
  armMoveToPose(RED_PERCH, 500, 0);
  armMoveToPose(HOME, 500, 0);
  //if(buttonPressed){Serial.println(F("Arm Pressed Red Button Successfully!"));}
  //else{Serial.println(F("Arm Failed to press Red Button!"));}
  return buttonPressed;
}

/// Press the green button! Return if the button was actually pressed
bool armPressGreen(int pauseTime)
{
  //Serial.println("Pressing Green Button!");
  armMoveToPose(HOME, 500, 0);
  armMoveToPose(GRN_PERCH, 500, 0);
  delay(pauseTime);
  bool buttonPressed = armMoveToPose(GRN_PRESS, 250, GRN_BTN);
  if(buttonPressed){
    playSound(GRN_SOUND, 500);
    Serial.println("B4");
  }
  armMoveToPose(GRN_PERCH, 500, 0);
  armMoveToPose(HOME, 500, 0);
  //if(buttonPressed){Serial.println(F("Arm Pressed Green Button Successfully!"));}
  //else{Serial.println(F("Arm Failed to press Green Button!"));}
  return buttonPressed;
}

///Calls all button press functions to test arm paths
void testButtonPressPositions()
{
  armPressYellow(200);
  armPressWhite(200);
  armPressRed(200);
  armPressGreen(200);
  delay(1000);
}

///Moves arm to press correct button, based on button ID
bool armPressButton(int buttonID)
{
  bool buttonPressed = false;
  switch (buttonID)
  {
    case (YEL_BTN):
      buttonPressed = armPressYellow(200);
      break;
    case (WHT_BTN):
      buttonPressed = armPressWhite(200);
      break;
    case (RED_BTN):
      buttonPressed = armPressRed(200);
      break;
    case (GRN_BTN):
      buttonPressed = armPressGreen(200);
      break;
  }
  return buttonPressed;
}

///Waits until a button is pressed. Returns true if correct button was pressed
bool waitForButton(int buttonID)
{
  int pressedButton = 0;
  while(pressedButton == 0)
  { //Wait for button press
   sendPositions();
   pressedButton = pollButtons(); 
   if(pressedButton == 0){ pressedButton = pollButtonsSerial(); }
  }
  // PLAY SOUND TODO
  changeButtonLED(pressedButton, HIGH);
  delay(500);
  // STOP SOUND TODO
  changeButtonLED(pressedButton, LOW);
  // The right button?
  if(pressedButton == buttonID){
    //Serial.println(F("Player pressed correct button!"));
    return true;
  }
  //Serial.println(F("Player pressed wrong button!"));
  return false;
}

///Drives the arm to take a bow!
void armTakeABow()
{
  armMoveToPose(ARM_WIN_1, 1000, 0);
  armMoveToPose(ARM_WIN_2, 1000, 0);
  armMoveToPose(ARM_WIN_3, 1000, 0);
  armMoveToPose(ARM_WIN_4, 1000, 0);
  armMoveToPose(ARM_WIN_5, 1000, 0);
  armMoveToPose(ARM_WIN_6, 1000, 0);
  armMoveToPose(ARM_WIN_7, 1000, 0);
  armMoveToPose(HOME, 1000, 0);
}

///Arm hangs in shame
void armHangInShame()
{
  armMoveToPose(ARM_LOSE_1, 1000, 0);
  armMoveToPose(ARM_LOSE_2, 1000, 0);
  armMoveToPose(ARM_LOSE_3, 1000, 0);
  armMoveToPose(ARM_LOSE_4, 1000, 0);
  armMoveToPose(ARM_LOSE_5, 1000, 0);
  armMoveToPose(ARM_LOSE_6, 1000, 0);
  armMoveToPose(HOME, 1000, 0);
}

///Mock pressing the green button
void armMockGreen()
{
  armMoveToPose(GRN_MOCK_1, 500, 0);
  delay(1000);
  armMoveToPose(GRN_MOCK_2, 250, 0);
  armMoveToPose(GRN_MOCK_3, 250, 0);
  armMoveToPose(GRN_MOCK_4, 250, 0);
  armMoveToPose(GRN_MOCK_5, 250, 0);
  armMoveToPose(HOME, 1000, 0);
}

///Plays the player lose song
void playPlayerLoseSong()
{
  Serial.println("L");
  tone(SPEAKER_PIN, 250);
  delay(300);
  tone(SPEAKER_PIN, 200);
  delay(300);
  tone(SPEAKER_PIN, 150);
  delay(300);
  tone(SPEAKER_PIN, 100);
  delay(300);
  for(int i = 0; i < 6; i++)
  {
    tone(SPEAKER_PIN, 80);
    delay(100);
    tone(SPEAKER_PIN, 100);
    delay(100);
  }
  noTone(SPEAKER_PIN);
}

///Plays the player win song!
void playPlayerWinSong()
{
  Serial.println("W");
  tone(SPEAKER_PIN, 400);
  delay(300);
  tone(SPEAKER_PIN, 500);
  delay(300);
  tone(SPEAKER_PIN, 600);
  delay(300);
  tone(SPEAKER_PIN, 700);
  for(int i = 0; i < 6; i++)
  {
    tone(SPEAKER_PIN, 800);
    delay(100);
    tone(SPEAKER_PIN, 900);
    delay(100);
  }
  noTone(SPEAKER_PIN);
  delay(300);
}



