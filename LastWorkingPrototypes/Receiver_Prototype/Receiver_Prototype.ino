#include <IRremote.h>
#include <pthread.h>

static const uint8_t IR_RECEIVER = 4;
static const uint8_t SIGNAL_LED_R = 25;
static const uint8_t SIGNAL_LED_B = 26;
static const unsigned long PREAMBLE = 0xAA000000;
static const unsigned long MASK = ~PREAMBLE;
static const int SHOT_BUFFER_SIZE = 32;

IRrecv irRecv(IR_RECEIVER);
volatile unsigned long shot_buffer[SHOT_BUFFER_SIZE];
volatile int buffer_idx = -1;

bool receiveUInt32(decode_results* result, unsigned long* value){
  unsigned long preamble = (result->value) & 0xFF000000;
  if(preamble != PREAMBLE) 
    return false;
    
  *value = (result->value) & MASK;
  return true;
}

void *doSenseShots(void *threadid) {
  while(true) {
    senseShot();
    delay(10);
  }
}

void senseShot(){
  unsigned long value = 0;
  decode_results res;
  
  if(irRecv.decode(&res)) {
    if(receiveUInt32(&res, &value)) {
      Serial.println(value);
      if(buffer_idx < SHOT_BUFFER_SIZE) {
        buffer_idx++;
        shot_buffer[buffer_idx] = value;
      }
    }
    irRecv.resume();
  }
}

boolean popShotBuffer(unsigned long* value){
  if(buffer_idx < 0) {
    return false;
  }
  *value = shot_buffer[buffer_idx];
  buffer_idx--;
  return true;
  
}

void flashLED(unsigned int channel, unsigned int duration) {
  ledcWrite(channel,255);
  ets_delay_us(duration);
  ledcWrite(channel,0);
  ets_delay_us(duration);
}

void signalHit(const unsigned long& value) {
  if(value == 42) {
    flashLED(0, 125000);
    flashLED(0, 125000);
  }
  if(value == 33) {
    flashLED(1, 125000);
    flashLED(1, 125000);
  }
  
}


void *mainLoop(void *threadid) {
  while(true) {
    unsigned long value = 0;
    if(popShotBuffer(&value)) {
      Serial.print("Gun ID: ");
      Serial.println(value);
      signalHit(value);
    }
  }
   
}

void setup() {
  Serial.begin(9600);
  irRecv.enableIRIn(); 
  ledcSetup(0, 50, 8);
  ledcSetup(1, 50, 8);
  ledcAttachPin(SIGNAL_LED_R, 0);
  ledcAttachPin(SIGNAL_LED_B, 1);
  pthread_t threads[2];
  int t1 = 0;
  int t2 = 0;
  pthread_create(&threads[0], NULL, doSenseShots, (void *)t1);
  pthread_create(&threads[1], NULL, mainLoop, (void *)t2);
}

void loop() {
}
