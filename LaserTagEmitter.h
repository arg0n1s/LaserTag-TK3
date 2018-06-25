#include <IRremote.h>

class LaserTagEmitter {

	IRsend irSend;
	uint8_t ir_pin, buzzer_pin, trigger_pin;
	unsigned long shot;
	static volatile int interruptCounter;
	static portMUX_TYPE mux;

	public:
		static const unsigned long PREAMBLE;

		LaserTagEmitter();
		LaserTagEmitter(const uint8_t ir_pin, const uint8_t buzzer_pin, const uint8_t trigger_pin);

		void init(const uint16_t gun_id);
		void pollTrigger();

	private:
		static void buttonISR();
		void sendUInt32(unsigned long value);
		void playSound(unsigned int frequency, unsigned int duration) const;
		void sweepSound() const;

};