#include "LaserTagEmitter.h"

const unsigned long LaserTagEmitter::PREAMBLE = 0xAA000000;
portMUX_TYPE LaserTagEmitter::mux = portMUX_INITIALIZER_UNLOCKED;
volatile int LaserTagEmitter::interruptCounter = 0;

LaserTagEmitter::LaserTagEmitter() {
	interruptCounter = 0;
	mux = portMUX_INITIALIZER_UNLOCKED;
}

LaserTagEmitter::LaserTagEmitter(const uint8_t ir_pin, const uint8_t buzzer_pin, const uint8_t trigger_pin) : ir_pin(ir_pin), buzzer_pin(buzzer_pin), trigger_pin(trigger_pin){
	interruptCounter = 0;
	mux = portMUX_INITIALIZER_UNLOCKED;

	irSend = IRsend(ir_pin);
	ledcSetup(0, 440, 8);
	ledcAttachPin(buzzer_pin, 0);
	pinMode(trigger_pin, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(trigger_pin), LaserTagEmitter::buttonISR, HIGH);
	shot = PREAMBLE;
}

void LaserTagEmitter::init(const uint16_t gun_id) {
	interruptCounter = 0;
	mux = portMUX_INITIALIZER_UNLOCKED;
	
	shot |= gun_id;
}

void LaserTagEmitter::pollTrigger() {
	if(interruptCounter>0) {
		portENTER_CRITICAL(&mux);
		if(interruptCounter%2 == 0) {
				sendUInt32(shot);
				sweepSound();
				interruptCounter = 0;
		}   
		portEXIT_CRITICAL(&mux);
	}
}

void LaserTagEmitter::buttonISR(){
  portENTER_CRITICAL_ISR(&mux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&mux);
}

void LaserTagEmitter::sendUInt32(unsigned long value) {
		irSend.sendNEC(value, 32);
}


void LaserTagEmitter::playSound(unsigned int frequency, unsigned int duration) const {
  ledcWriteTone(0, frequency);
  ledcWrite(0,255);
  ets_delay_us(duration);
  ledcWrite(0,0);
}

void LaserTagEmitter::sweepSound() const {
  for(unsigned int i = 8800; i>440; i-=440){
    playSound(i, 5000);
  }
}