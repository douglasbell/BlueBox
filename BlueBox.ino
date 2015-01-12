//#include <Tone.h>
#include <Keypad.h>

#define TONE_PIN   12

// Define the keymap for our 4x4 keypad
const char keys[4][4] = {
  {'1','2','3','a'},
  {'4','5','6','b'},
  {'7','8','9','c'},
  {'#','0','*','d'}
};

// Connect to the row pinouts of the keypad
byte rowPins[4] = {5,4,3,2}; 

// Connect to the column pinouts of the keypad
byte colPins[4] = {9,8,7,6}; 

// Initialize the keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins ,colPins, 4, 4);


// DTMF Frequencies in Hz
int dtmfFreq[8] = {
  697,770,852,941,1209,1336,1477,1633  
};

// DTMF Tones
int dtmf[16][2] = {
  {0,4},{0,5},{0,6},{0,7}, // 1,2,3,A
  {1,4},{1,5},{1,6},{1,7}, // 4,5,6,B
  {2,4},{2,5},{2,6},{2,7}, // 7,8,9,C
  {3,4},{3,5},{3,6},{3,7}, // *,0,#,D
};

// The Frequency to play
//Tone freq;


/** 
 * Setup the program 
 */
void setup() {
  //freq.begin(TONE_PIN);
  Serial.begin(9600);
}

/**
 * The program loop
 */
void loop() {
  char button = keypad.getKey(); // check for button press
 
  if (button != NULL) {
    Serial.print("Button: ");
    Serial.println(button);
    if (button == 'a'){
      // freq.play(2600);
      tone(12, 2600, 500);
    } else if (isdigit(button) || button == '#' || button == '*') {
      tone(12, dtmfFreq[dtmf[button][0]], 500);
    } 
    //tone(13, 2600, 500);
   // delay(500);
   // noTone(13);
  }
 
}
