#include <Wire.h>
#include <Keypad.h>
#include <Tone.h>
#include <SoftwareSerial.h>

/** 
 * A blue box is an electronic device that generates the same tones 
 * employed by a telephone operator's dialing console to switch long-distance 
 * calls. The blue box allowed users to route their own calls by emulating 
 * the in-band signaling mechanism that then controlled switching in long 
 * distance dialing systems. The most typical use of a blue box was to place 
 * free telephone calls.  
 *
 * Extrapolated from: Unattributed 
 *
 * Author: Doug Bell (douglas.bell@gmail.com)
 *
 * 3rd Party Libaries
 *
 * Keypad: http://playground.arduino.cc/Code/Keypad
 * Tone: https://github.com/douglasbell/BlueBox/tree/master/Tone
 *
 */
 
// Attach the serial display's RX line to digital pin 12
SoftwareSerial lcd(3,13); // pin 3 = RX (unused), pin 13 = TX

// MF 0,1,2,3,4,5,6,7,8,9,kp,st,2400+2600,kp2,st2,ss4,supervisor
int bb[16][2] = { 
  {1300,1500},{700,900},{700,1100},      // 0,1,2
  {900,1100},{700,1300},{900,1300},      // 3,4,5
  {1100,1300},{700,1500},{900,1500},     // 6,7,8
  {1100,1500},{1100,1700},{1500,1700},   // 9,kp,st
  {2600,2400},{1300,1700},{900,1700},    // 2400+2600,kp2,st2
  {2400,2040}                            // ss4, supervisory
};

// DTMF Frequencies in Hz
int dtmfFreq[8] = {
  697,770,852,941,1209,1336,1477,1633  
};

// DTMF tones by button
int dtmf[16][2] = {
  {0,4},{0,5},{0,6},{0,7}, // 1,2,3,A
  {1,4},{1,5},{1,6},{1,7}, // 4,5,6,B
  {2,4},{2,5},{2,6},{2,7}, // 7,8,9,C
  {3,4},{3,5},{3,6},{3,7}, // *,0,#,D
};

// SS4 tones by button
int ss4[][4] = { 
  {0,1,0,1},{1,1,1,0},{1,1,0,1}, // 0,1,2
  {1,1,0,0},{1,0,1,1},{1,0,1,0}, // 3,4,5
  {1,0,0,1},{1,0,0,0},{0,1,1,1}, // 6,7,8
  {0,1,1,0},{0,0,0,0},{0,1,0,0}, // 9,KP,OP11
  {0,0,1,1},                     // OP12
};

// Auto dial hold digits
uint8_t speedDial[][3] = {
  {1,2,1},{1,0,1},{1,2,1}, // 0,1,2
  {1,3,1},{1,4,1},{1,0,5}, // 3,4,5
  {6,6,6},{1,0,7},{1,8,1}, // 6,7,8
  {1,0,9}
};

// 75 ms for MF tones, 120 for KP/ST
uint8_t bbdur[2] = {60,100}; 

// tones for 0,1 respectively
int ss4Tone[2] = {2040,2400}; 

// Key layout array
char keys[4][4] = {
  {'1','2','3','a'},
  {'4','5','6','b'},
  {'7','8','9','c'},
  {'#','0','*','d'}
};

// Connect to the row pinouts of the keypad
byte rowPins[4] = {5,4,3,2}; 

// Connect to the column pinouts of the keypad
byte colPins[4] = {9,8,7,6}; 

// The Array of Tone objects
Tone freq[2]; 

// Initialize the keypad with the mapped keys and pins
Keypad keypad = Keypad(makeKeymap(keys),rowPins,colPins,4,4);

// recording on/off
boolean rec = 0; 

// stored digits?
boolean stored = 0; 

// are any stored digits playing
boolean autoDial = 0; 

// 0 for multifrequency, 1 for intern, 2 for SS4, 3 for DP, 4 for DTMF
int mode = 0; 

// MF2 mode
int mf2 = 0; 

// int store
int store[24] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

// count of button presses for lcd display
int presses = 0; 

// Do we have a notification on screen
boolean notify = false;

/**
 * Program setup 
 */
void setup(void){ 
  freq[0].begin(10); // Initialize our first tone generator
  freq[1].begin(12); // Initialize our second tone generator
  keypad.setHoldTime(1500); // hold for two seconds to change state to HOLD
  pinMode(10, INPUT); // 2600 button
  // pinMode(13, OUTPUT); // LED for recording
  keypad.addEventListener(processButton);
  notificationTone(); // boot successful
  
  // Set up LCD
  lcd.begin(9600);
  delay(500); // wait for display to boot
  resetLcd();
  lcd.write(254); // cursor to beginning of first line
  lcd.write(128);
  lcd.write("BlueBox v1.0    ");
  lcd.write("Ready to Dial   ");

  Serial.begin(9600);
}

/**
 * Main program loop 
 *
 * Get the button pressed and see if we need to reset or update
 * the LCD
 */
void loop(void){ 
  char button = keypad.getKey(); // check for button press
  if (button != NULL) {
    Serial.print("Button: ");
    Serial.println(button);
    if (notify || presses == 0 || presses >= 32) {
      resetLcd();
      presses = 0; // Reset count
    }
    presses++;
    if (isDigit(button)) {
      lcd.print(button);
    } else {
      notify = true;
      resetLcd();
      if (button == 'a') {
        lcd.print("Hold 2 Seconds  ");
        lcd.print("to Change Modes");
      } else if (button == 'b') {
        lcd.print("MF2 Mode        ");
      } else if (button == 'c') {
        lcd.print("Open Funcation  "); 
      } else if (button == 'd') {
        lcd.print("2600Hz          ");   
      } // else WTF? 
    }
  }
  //if(digitalRead(10)==HIGH){ // play 2600Hz if top button pressed
  if (button == 'd') {
    supervisor(); // supervisory signalling
  }
  return; 
}

void resetLcd() {
   lcd.write(254); // cursor to beginning of first line
   lcd.write(128);
   lcd.write("                ");
   lcd.write("                ");
   lcd.write(254); // cursor to beginning of first line
   lcd.write(128);
}
/** 
 * Supervisory Button (TOP) 
 */
void supervisor(void){
  if(mode==1){ // international seizure of trunk
    mf(12);
    delay(1337); 
    sf(2400,750);
  }else if(mode==2){ // SS4 Supervisory Signal
    mf(15);
    delay(150);
    sf(2040,350);
  }else{ // non international
    sf(2600,750);
  }
  return;
}

/** 
 * Process button presses
 */
void processButton(KeypadEvent b){
  b -= 48;
  switch (keypad.getState()){
    case RELEASED: // drop right away
      return;
    case PRESSED: // momentary
      if(mode==2){ // Signal Switching 4
        ss4Signal(b);
        break;
      }else if(mode==3){
        pulse(b); // pulse it
        return; 
      }
      
      if(mode==4&&(b<10&&b>=0||b==-13||b==-6||(b>=49&&b<=52))){ // MF tone
        mf(b); 
      }
      
      if(b<10&&b>=0||b==-13||b==-6){ // MF tone
        mf(b); 
      }else if(b==52){ // D
        if (stored) playStored(); // don't copy function onto stack if not needed 
        return;
      }else if(mode==1){ // international kp2/st2
        if(b>=49&&b<=51){
          mf(b);
          return;
        }
      }else if(mode==0&&(b<=51&&b>=49)){ // A,B,C redbox
        redBox(b); // pass it to RedBox()
        return;
      }
      break;
    case HOLD: // HELD (special functions)
      if(b==50&&mode==3){ // HOLD B for MF2 in PD Mode
        (mf2)? mf2 = 0 : mf2 = 1; // turn off if on, or on if off
        freq[0].play(440,70);
        delay(140);
        freq[0].play(440,70);
        delay(140);
      }
      
      if(b<10&&b>=0||b==-13||b==-6){
        playSpeedDial(b); 
      }else if(b==51){ // C takes care of recording now
        if(rec){ // we are done recording:
          // digitalWrite(13, LOW); // turn off LED
          rec = 0;
          stored=1; // we have digits stored
          recordNotification();
        }else{ // we start recording
          //digitalWrite(13, HIGH); // light up LED
          rec = 1;
          for(int i=0;i<=23;i++){ // reset array
            store[i] = -1;
          }
          recordNotification();
        } // END recording code
      }else if(b==49){ // ('A' HELD) switching any mode "on" changes to mode, all "off" is domestic
       resetLcd();
       if(mode==0){ // mf to international
          mode=1;
           lcd.print("International");   
        }else if(mode==1){ // international to ss4 mode
          mode=2;
          lcd.print("SS4");
        }else if(mode==2){ // ss4 mode to pulse mode
          mode=3;
          lcd.print("Pulse");
        }else if(mode==3){ // pulse mode to DTMF
          mode=4;
          lcd.print("DTMF");
        }else if(mode==4){ // DTMF to domestic
          mode=0;
          lcd.print("Domestic");
        }
        modeNotification();
        return;
      }       
      break;
  }
  return;
}

/**
 * Pulse mode
 */
void pulse(int signal){
  int pulse = 2600;
  
  if(mf2) { // MF2 mode (AC11)
    pulse = 2280; 
  } 
  
  if(signal==49) {
    freq[0].play(300,200);
  }
  
  if(signal>9) { 
    return; 
  } 
  
  if(signal==-13||signal==-6){
    return;
  }else{
    
    if(signal==0){ 
      signal = 10; 
    }
    
    for(int i=0;i<signal;i++){
      digitalWrite(13, HIGH); // pulsing LED
      freq[0].play(pulse,66);
      delay(66);
      digitalWrite(13, LOW);
      delay(34);
    }
    delay(500); // no new digit accepted until after this time
  }
  
  return;
}

/** 
 * Play the stored tones
 */
void playStored(void){
  if(stored){
    autoDial = 1;
    for(int i=0;i<=23;i++){
      if(store[i]==-1){
        return;
      }else{
        mf(store[i]);  
      }    
    }  
  }else{
    return;
  }
  
  autoDial = 0; // turn off playing
  
  return;
}

/**
 * Record Notification tone
 */
void recordNotification(void){
  if(rec){
    sf(1700,66);
    delay(66);
    sf(2200,500);
    delay(500);
  }else{
    sf(2200,66);
    delay(66);
    sf(1700,500);
    delay(500);  
  }
  return;
}

/**
 * Mode notification
 */
void modeNotification(){
  int count = 1;
  int dur = 70;
  int frequency = 440;

  if(mode==1){ 
    count = 2; 
  }
  
  if(mode==2){ 
    count = 3; 
  }
  
  if(mode==3){ 
    count = 4; 
  }
  
  if(mode==4){ 
    count = 5; 
  }
 
  for(int i = 0;i<count;i++){
    freq[0].play(frequency,dur);
    delay(dur*2);
  } 
}

/**
 * Notification Tone
 */
void notificationTone(void){
  Serial.println(mode);
  for(int i=0;i<=2;i++){
    freq[0].play(2600,33);
    delay(66);
  }
  delay(500);
  return;
}

/**
 * SS4 signalling
 */
void ss4Signal(int signal){
  if(signal==49){ // A  
    mf(15);
    delay(250);
    sf(2400,350);
    return;
  }
  
  if(signal==50){// B
    signal==11;
  }
  
  if(signal==51){ // C
    signal==12;
  }
  
  if(signal==52){ // D
    sf(2600,750);
    return;
  }
  
  if(signal==-13){ 
    signal = 10; 
  }
  
  if(signal==-6){ 
    supervisor();
    return; 
  }
  
  for(int i=0;i<=3;i++){
   (ss4[signal][i]) ? freq[0].play(ss4Tone[1],35) : freq[0].play(ss4Tone[0],35);
   delay(70);
  }
  
  return;
}

/**
 * Play an multifrequency tone
 */
void mf(int digit){ 
  Serial.print("Multifrequency: ");
  if(rec && ((digit>=0&&digit<=9)||digit==-13||digit==-6)){ // Store the digit IFF recording
    for(int i=0;i<=23;i++){ // ONLY record KP,ST,0-9
      if(store[i]==-1){
        store[i]=digit;
        break;
      }
    }
  } 
  int duration = bbdur[0]; // OKAY for DTMF too
  if(mode==1){ 
    if(digit==49){ // international mode A
      digit=13;
    }
    if(digit==50){ // international mode B
      digit=14;
    }
    if(digit==51){
     sf(2600,1000);
     return; 
    }
  }else if(mode==4){ // DTMF
    Serial.println(digit);
    if((digit>=1&&digit<=3)) { // 1,2,3
      digit-=1;
    }else if(digit>=7&&digit<=9){
      digit+=1;
    }else if(digit==49){ 
      digit = 3; // A
    }else if(digit==50){ 
      digit = 7; // B
    }else if(digit==51){ 
      digit = 11;  // C
    }else if(digit==52){ 
      digit = 15;  // D
    }else if(digit==-13){ 
      digit = 12;  // *
    }else if(digit==-6){ 
      digit = 14;  // #
    }else if(digit==0){ 
      digit = 13; // 0
    }else if(digit==13){ 
      duration=150; 
    }
    
    Serial.println("and: ");
    Serial.println(digit);
    
    freq[0].play(dtmfFreq[dtmf[digit][0]],duration); 
    freq[1].play(dtmfFreq[dtmf[digit][1]],duration);
    
    return;
  }
  
  if(digit<0){
    duration = bbdur[1];
    
    if(digit==-13){ 
      digit=10; 
    }
    
    if(digit==-6){ 
      digit=11; 
    }
  }
  
  if(digit==-1){ 
    return; 
  } // -1 in storage?
  
  if(digit==12){ // 85ms for international trunk seizing
    duration = 200;
  }
  
  if (mode==3) { // SS4 Supervisory MF (150ms)
    duration = 150; 
  }
  
  freq[0].play(bb[digit][0],duration);
  freq[1].play(bb[digit][1],duration);
  (autoDial) ? delay(duration + 60) : delay(duration); // ? expression? statement?
  
  if(rec){ 
    delay(25); 
    sf(2600,33); 
  }// chirp to signify recording
  
  return; 
}

/**
 * Play a single frequency
 */
void sf(int frequency,int duration){ 
  freq[0].play(frequency,duration);
  return;
}

/**
 * Play red box tones from the given coin value
 */
void redBox(int coin){ 
  int iter;
  int delayMs = 66;
  int rb[2] = {1700,2200};
  
  switch(coin){
    case 49:
      iter = 5;
      delayMs = 33;
      break;
    case 50:
      iter = 2;
      break;
    case 51:
      iter = 1;
      break;
  } 
  
  for(int i=0;i<iter;i++){ 
    freq[0].play(rb[0],delayMs);
    freq[1].play(rb[1],delayMs);
    delay(delayMs * 2); // pause for coin and between
  }
}

/**
 * Play speed dials
 */
void playSpeedDial(int sd){ // speed dial
  if(rec) { // We are recording
    return; 
  }
  
  autoDial = 1; // turn on for pauses
  sf(2600,750); // play 2600 1 sec
  delay(2000); // pause
  mf(-13); // KP
  
  for(int i=0;i<=2;i++){
      mf(speedDial[sd][i]);
  }
  
  mf(-6); // ST
  autoDial = 0; // turn off pauses
  
  return;
}
