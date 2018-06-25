#include <IRremote.h>
static const uint8_t IR_LED = 25;
static const uint8_t BUZZER = 26;
static const uint8_t TRIGGER = 27;

static const unsigned long GUN_ID = 42;
static const unsigned long PREAMBLE = 0xAA000000;
static const unsigned long SHOT = PREAMBLE | GUN_ID;

IRsend irSend(IR_LED);

volatile byte triggered = 0;
volatile int interruptCounter = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void sendUInt32(unsigned long value) {
  irSend.sendNEC(value, 32);
}

void playSound(unsigned int frequency, unsigned int duration) {
  ledcWriteTone(0, frequency);
  ledcWrite(0,255);
  ets_delay_us(duration);
  ledcWrite(0,0);
}

void sweepSound(){
  for(unsigned int i = 8800; i>440; i-=440){
    playSound(i, 5000);
  }
}

void buttonISR(){
  portENTER_CRITICAL_ISR(&mux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&mux);
}

void doShoot(){
  if(interruptCounter>0) {
    portENTER_CRITICAL(&mux);
    if(interruptCounter%2 == 0) {
      sendUInt32(SHOT);
      sweepSound();
      interruptCounter = 0;
    }   
    portEXIT_CRITICAL(&mux);
  }
}

void setup() {
  Serial.begin(9600);
  ledcSetup(0, 440, 8);
  ledcAttachPin(BUZZER, 0);
  pinMode(TRIGGER, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(TRIGGER), buttonISR, HIGH);

}

void loop() {
  doShoot();
}

