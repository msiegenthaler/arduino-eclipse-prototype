#include "WProgram.h"

extern "C" void __cxa_pure_virtual(void);
void __cxa_pure_virtual(void) {}


// prototypes
void blink(int n, int d);

// variables
int ledPin = 13; // LED connected to digital pin 13

void setup() {
	Serial.begin(9600); // opens serial port, sets data rate to 9600 bps
	pinMode(ledPin, OUTPUT); // sets the digital pin as output
	Serial.println("------------------");
	Serial.println("   hello world");
	Serial.println("------------------");
}

void loop() {
	blink(3, 50); // blink led (n times, ms interval)
	delay(100); // wait 1 sec
}


void blink(int n, int d) {
	for (int i = 0; i < n; i++) {
		digitalWrite(ledPin, HIGH);
		delay(d / 2);
		digitalWrite(ledPin, LOW);
		delay(d / 2);
	}
}

int main(void) {
	init();
	setup();
	for (;;) loop();
	return 0;
}
