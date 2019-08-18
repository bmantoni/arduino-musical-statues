#include <Adafruit_NeoPixel.h>

#include "SoftwareSerial.h"
SoftwareSerial softSerial(10, 11);
# define Start_Byte 0x7E
# define Version_Byte 0xFF
# define Command_Length 0x06
# define End_Byte 0xEF
# define Acknowledge 0x00 //Returns info with command 0x41 [0x01: info, 0x00: no info]

# define ACTIVATED LOW

# define MIN_WAIT 5
# define MAX_WAIT 10
# define BUTTON_PLAY 3
# define DEFAULT_VOLUME 30

# define LED_PIN 5
# define LED_COUNT 25

//int BUTTON_PLAY = 3;

boolean isPlaying = false;

Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup () {
  Serial.begin(9600);
  softSerial.begin(9600);

  Serial.println();
  Serial.println(F("Starting up..."));

  randomSeed(analogRead(0));
  pinMode(BUTTON_PLAY, INPUT);
  digitalWrite(BUTTON_PLAY, HIGH);

  delay(1000);
  playFirst();
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

void startLEDs() {
  leds.begin();  // Call this to start up the LED strip.
  clearLEDs();   // This function, defined below, turns all LEDs off...
  leds.show();   // ...but the LEDs don't actually update until you call this.
}

void clearLEDs() {
  setAllLeds(0, 0, 0);
}

void setAllLeds(int r, int g, int b) {
  for (int i=0; i<LED_COUNT; i++)
  {
    leds.setPixelColor(i, r, g, b);
  }
  leds.show();
}

void freeze() {
  doFreezeLights();
  pause();
  isPlaying = false;
}

void doFreezeLights() {
  setAllLeds(255, 0, 0);
}

void doUnFreezeLights() {
  setAllLeds(0, 255, 0);
}

void waitToFreeze() {
  long stopInSeconds = random(MIN_WAIT, MAX_WAIT);
  Serial.println((String)"Stopping in " + stopInSeconds + " seconds...");
  delay(1000 * stopInSeconds);
}

void waitToStart() {
  Serial.println("checking if play button pressed");
  if (digitalRead(BUTTON_PLAY) == ACTIVATED) {
    Serial.println("Play button pressed");
    unfreeze();
  }
}

void unfreeze() {
  if (isPlaying) {
    doUnFreezeLights();
    pause();
    isPlaying = false;
  } else {
    isPlaying = true;
    play();
  }
}

void playFirst()
{
  execute_CMD(0x3F, 0, 0);
  delay(500);
  setVolume(DEFAULT_VOLUME);
  delay(500);
  execute_CMD(0x11,0,1); 
  delay(500);
}

void pause()
{
  execute_CMD(0x0E,0,0);
  delay(500);
}

void play()
{
  execute_CMD(0x0D,0,1); 
  delay(500);
}

void playNext()
{
  execute_CMD(0x01,0,1);
  delay(500);
}

void playPrevious()
{
  execute_CMD(0x02,0,1);
  delay(500);
}

void setVolume(int volume)
{
  execute_CMD(0x06, 0, volume); // Set the volume (0x00~0x30)
  delay(2000);
}

void execute_CMD(byte CMD, byte Par1, byte Par2)
{
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
