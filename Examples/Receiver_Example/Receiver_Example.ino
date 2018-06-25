#include <LaserTagReceiver.h>

LaserTagReceiver vest;

void shotCallback(unsigned long value) {
  //do something with this value, e.g. serial print, MQTT publish, whatever..
}

void setup() {
  vest = LaserTagReceiver(4, 25, 26);
  vest.registerShotCallback(&shotCallback);
  vest.doReceive();
}

void loop() {
  // loop is unused -> doReceive() runs two continuous threads 
}

