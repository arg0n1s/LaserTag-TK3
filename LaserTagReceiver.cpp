#include "LaserTagReceiver.h"

//const int LaserTagReceiver::SHOT_BUFFER_SIZE = 32;
//volatile unsigned long* LaserTagReceiver::shot_buffer = unsigned long[LaserTagReceiver::SHOT_BUFFER_SIZE];
volatile unsigned long LaserTagReceiver::shot_buffer[SHOT_BUFFER_SIZE];
const unsigned long LaserTagReceiver::PREAMBLE = 0xAA000000;
const unsigned long LaserTagReceiver::MASK = ~LaserTagReceiver::PREAMBLE;
const int LaserTagReceiver::RED_CHANNEL = 0;
const int LaserTagReceiver::BLUE_CHANNEL = 1;
volatile int LaserTagReceiver::buffer_idx = -1;


LaserTagReceiver::LaserTagReceiver() {
	irRecv = NULL;
	team_red_even = true;
	buffer_idx = -1;
	signalDuration = 125000;
}

LaserTagReceiver::LaserTagReceiver(const uint8_t IR_RECEIVER, const uint8_t SIGNAL_LED_R, const uint8_t SIGNAL_LED_B) : IR_RECEIVER(IR_RECEIVER), SIGNAL_LED_R(SIGNAL_LED_R), SIGNAL_LED_B(SIGNAL_LED_B){
 	team_red_even = true;
 	buffer_idx = -1;
 	signalDuration = 125000;

  	irRecv = new IRrecv(IR_RECEIVER);
  	irRecv->enableIRIn(); 
  	ledcSetup(RED_CHANNEL, 50, 8);
  	ledcSetup(BLUE_CHANNEL, 50, 8);
  	ledcAttachPin(SIGNAL_LED_R, RED_CHANNEL);
  	ledcAttachPin(SIGNAL_LED_B, BLUE_CHANNEL);
}

LaserTagReceiver::~LaserTagReceiver() {
	free(irRecv);
}

void LaserTagReceiver::doReceive() {
	pthread_t threads[2];
  	int t1 = 0;
  	int t2 = 1;
  	pthread_create(&threads[0], NULL, &LaserTagReceiver::doSenseShots, this);
  	pthread_create(&threads[1], NULL, &LaserTagReceiver::mainLoop, this);
}

void LaserTagReceiver::registerShotCallback(std::function<void(unsigned long)> callback) {
	this->callback = callback;
}

void LaserTagReceiver::setTeams(const bool team_red_even) {
	this->team_red_even = team_red_even;
}

void LaserTagReceiver::setSignalDuration(const unsigned int duration) {
	signalDuration = duration;
}

bool LaserTagReceiver::receiveUInt32(decode_results* result, unsigned long* value) {
	unsigned long preamble = (result->value) & 0xFF000000;
  	if(preamble != PREAMBLE) 
    	return false;
    
  	*value = (result->value) & MASK;
  	return true;
}

void *LaserTagReceiver::doSenseShots(void * context) {
	while(true) {
    	((LaserTagReceiver *)context)->senseShot();
    	delay(10);
  	}
}

void LaserTagReceiver::senseShot() {
	unsigned long value = 0;
  	decode_results res;
  
  	if(irRecv->decode(&res)) {
    	if(receiveUInt32(&res, &value)) {
      		Serial.println(value);
      		if(buffer_idx < SHOT_BUFFER_SIZE) {
        		buffer_idx++;
        		shot_buffer[buffer_idx] = value;
      		}
    	}
    	irRecv->resume();
  	}
}

bool LaserTagReceiver::popShotBuffer(unsigned long* value) {
	if(buffer_idx < 0) {
    	return false;
  	}

  	*value = shot_buffer[buffer_idx];
  	buffer_idx--;
  	return true;
}

void LaserTagReceiver::flashLED(unsigned int channel) {
	ledcWrite(channel,255);
  	ets_delay_us(signalDuration);
  	ledcWrite(channel,0);
  	ets_delay_us(signalDuration);
}

void LaserTagReceiver::signalHit(const unsigned long& value) {
	bool even = (value % 2) == 0;
	bool flashRed = true;
	if(even && !team_red_even) {
		flashRed = false;
	}
	if(!even && team_red_even) {
		flashRed = false;
	}
	if(flashRed) {
		flashLED(RED_CHANNEL);
    	flashLED(RED_CHANNEL);
	}else {
		flashLED(BLUE_CHANNEL);
    	flashLED(BLUE_CHANNEL);
	}
}

void *LaserTagReceiver::mainLoop(void * context) {
	while(true) {
    	unsigned long value = 0;
    	if(((LaserTagReceiver *)context)->popShotBuffer(&value)) {
    		((LaserTagReceiver *)context)->callback(value);
      		((LaserTagReceiver *)context)->signalHit(value);
    	}
  	}
}