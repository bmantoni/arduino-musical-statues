#include <Adafruit_NeoPixel.h>
#include <math.h>
#include "SoftwareSerial.h"

SoftwareSerial softSerial(10, 11);
# define Start_Byte 0x7E
# define Version_Byte 0xFF
# define Command_Length 0x06
# define End_Byte 0xEF
# define Acknowledge 0x00 //Returns info with command 0x41 [0x01: info, 0x00: no info]

# define ACTIVATED LOW

// Music /////////////////
# define MIN_WAIT 5
# define MAX_WAIT 10
# define BUTTON_PLAY 3
# define DEFAULT_VOLUME 15
# define NUM_SONGS 8
# define STOP_SONG 1
# define GO_SONG 2

// Lights ////////////////
# define LED_PIN 5
# define LED_COUNT 25

// Motion ////////////////
# define WATCH_TIME 5
# define PIR_PIN 2
int pirState = LOW;
int motionDetected = 0;
int watchingPhaseDone = 0;

boolean isPlaying = false;
int currentSong = 0;
Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup () {
  Serial.begin(9600);
  softSerial.begin(9600);

  Serial.println();
  Serial.println(F("Starting up..."));

  randomSeed(analogRead(0));
  pinMode(BUTTON_PLAY, INPUT);
  digitalWrite(BUTTON_PLAY, HIGH);

  pinMode(PIR_PIN, INPUT);

  delay(1000);
  initMusic();
  playRandomSong();
  //playStopSound();
  isPlaying = true;

  startLEDs();
}

void loop () { 
  if (isPlaying) {
    waitToFreeze();
    freeze();
  }
  waitToStart();
}

void checkForMotion(){
  //Serial.println("Checking for motion...");
  int val = digitalRead(PIR_PIN);
  if (val == HIGH) {
    if (pirState == LOW) {
      // we have just turned on
      Serial.println("Motion detected!");
      pirState = HIGH;
      motionDetected = 1;
    }
  } else {
    if (pirState == HIGH){
      // we have just turned of
      Serial.println("Motion ended!");
      pirState = LOW;
    }
  }
}

void startLEDs() {
  leds.begin();  // Call this to start up the LED strip.
  clearLEDs();   // This function, defined below, turns all LEDs off...
  leds.show();   // ...but the LEDs don't actually update until you call this.
}

void clearLEDs() {
  setAllLeds(0, 0, 0);
}

int showOdd = 0;

void setAlternatingLeds(int r, int g, int b) {
  for (int i = 0; i < LED_COUNT; i++) {
    if ((i % 2) == 0) {
      if (showOdd == 0) {
        leds.setPixelColor(i, r, g, b);
      } else { 
        leds.setPixelColor(i, 0, 0, 0);
      }
    } else {
      if (showOdd == 1) {
        leds.setPixelColor(i, r, g, b);
      } else { 
        leds.setPixelColor(i, 0, 0, 0);
      }
    }
  }
  leds.show();
  Serial.println(showOdd);
  showOdd = ( showOdd == 0 ) ? 1 : 0;
}

void setAllLeds(int r, int g, int b) {
  for (int i=0; i<LED_COUNT; i++) {
    leds.setPixelColor(i, r, g, b);
  }
  leds.show();
}

void freeze() {
  doFreezeLights();
  playSong(STOP_SONG);
  pause();
  isPlaying = false;
}

void playSong(int num) {
  execute_CMD(0x03, 0, num);
  play();
  delay(500);  
}

void doFreezeLights() {
  setAllLeds(255, 0, 0);
}

void doUnFreezeLights() {
  //setAllLeds(0, 255, 0);
  clearLEDs();
}

void waitToFreeze() {
  long stopInSeconds = random(MIN_WAIT, MAX_WAIT);
  Serial.println((String)"Stopping in " + stopInSeconds + " seconds...");
  delay(1000 * stopInSeconds);
}

void waitToStart() {
  // first, wait a random amount of time T with red light, checking for motion
  if (watchingPhaseDone == 0) {
    waitForImWatchingYouPhase();
    watchingPhaseDone = 1;
  }
  // after either nobody moves after T, or someone moves, continue
  clearLEDs();
  //Serial.println("checking if play button pressed");
  if (digitalRead(BUTTON_PLAY) == ACTIVATED) {
    Serial.println("Play button pressed");
    unfreeze();
    watchingPhaseDone = 0;
  }
}

// return true if someone moved? false if not
void waitForImWatchingYouPhase() {
  clearLEDs();
  uint32_t waitTimeMillis = WATCH_TIME * 1000L;
  for (uint32_t tStart = millis(); (millis()-tStart) < waitTimeMillis;) {
    checkForMotion();
    if (motionDetected == 1) {
      blinkXTimes(3, 255, 0, 0);
      motionDetected = 0;
      return;
    }
    // do watching animation
    if ( floor(log10(millis() - tStart)) == 3 ) {
      setAlternatingLeds(255, 255, 0);
    }
  }
  // nobody moved!
  blinkXTimes(3, 0, 255, 0);
}

void blinkXTimes(int count, int r, int g, int b) {
  for (int i = 0; i < count; ++i) {
    delay(500);
    setAllLeds(r, g, b);
    delay(500);
    setAllLeds(0, 0, 0);
  }
}

void unfreeze() {
  if (isPlaying) {
    pause();
    isPlaying = false;
  } else {
    playSong(GO_SONG);
    doUnFreezeLights();
    isPlaying = true;
    //play();
    resumeCurrentSong();
    //play();
    delay(500);
  }
}

void resumeCurrentSong() {
  playSong(currentSong);
}

void playRandomSong() {
  long skipSongs = random(2, NUM_SONGS - 1); // first 2 songs are special sounds
  currentSong = skipSongs + 1;
  Serial.println((String)"Setting currentSong to " + currentSong);
  for (int i = 0; i < skipSongs; i++) {
    playNext();
  }
}

void initMusic() {
  //execute_CMD(0x3F, 0, 0);
  //delay(500);
  setVolume(DEFAULT_VOLUME);
  delay(500);
  execute_CMD(0x11,0,0); 
  delay(500);
}

void pause() {
  execute_CMD(0x0E,0,0);
  delay(500);
}

void play() {
  execute_CMD(0x0D,0,1); 
  delay(500);
}

void playNext() {
  execute_CMD(0x01,0,1);
  delay(500);
}

void playPrevious() {
  execute_CMD(0x02,0,1);
  delay(500);
}

void setVolume(int volume) {
  execute_CMD(0x06, 0, volume); // Set the volume (0x00~0x30)
  delay(2000);
}

void execute_CMD(byte CMD, byte Par1, byte Par2) {
  // Calculate the checksum (2 bytes)
  word checksum = -(Version_Byte + Command_Length + CMD + Acknowledge + Par1 + Par2);
  // Build the command line
  byte Command_line[10] = { Start_Byte, Version_Byte, Command_Length, CMD, Acknowledge,
  Par1, Par2, highByte(checksum), lowByte(checksum), End_Byte};
  //Send the command line to the module
  for (byte k=0; k<10; k++)
  {
    softSerial.write( Command_line[k]);
  }
}
