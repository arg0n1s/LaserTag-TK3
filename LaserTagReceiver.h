#include <IRremote.h>
#include <pthread.h>
#include <functional>

#define SHOT_BUFFER_SIZE 32

class LaserTagReceiver {
	static const unsigned long PREAMBLE;
	static const unsigned long MASK;
	static const int RED_CHANNEL;
	static const int BLUE_CHANNEL;

	uint8_t IR_RECEIVER, SIGNAL_LED_R, SIGNAL_LED_B;
	bool team_red_even;
	unsigned int signalDuration;
	std::function<void(unsigned long)> callback;

	IRrecv *irRecv;
	static volatile unsigned long shot_buffer[SHOT_BUFFER_SIZE];
	static volatile int buffer_idx;

	public:

	LaserTagReceiver();

	LaserTagReceiver(const uint8_t IR_RECEIVER, const uint8_t SIGNAL_LED_R, const uint8_t SIGNAL_LED_B);

	~LaserTagReceiver();

	void doReceive();

	void registerShotCallback(std::function<void(unsigned long)> callback);

	void setTeams(const bool team_red_even);

	void setSignalDuration(const unsigned int duration);

	private:

	bool receiveUInt32(decode_results* result, unsigned long* value);

	static void *doSenseShots(void * context);

	void senseShot();

	bool popShotBuffer(unsigned long* value);

	void flashLED(unsigned int channel);

	void signalHit(const unsigned long& value);

	static void *mainLoop(void * context);

};