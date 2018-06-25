#include <LaserTagEmitter.h>

LaserTagEmitter gun;

void setup() {
  gun = LaserTagEmitter(25, 26, 27);
  gun.init(33);
}

void loop() {
  // this checks for the trigger flag set by the trigger interrupt service routine
  // ToDo: put this in a separate thread
  gun.pollTrigger();
}
