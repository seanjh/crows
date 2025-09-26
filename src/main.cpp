#include <Arduino.h>
#include <I2S.h>

const int I2S_LRC = 7;  // GPIO7 - Left/Right Clock
const int I2S_BCLK = 6; // GPIO6 - Bit Clock
const int I2S_DIN = 5;  // GPIO5 - Data Out

void setup() {
  Serial.begin(115200);
  Serial.println("Starting minimal I2S test...");

  // TODO(human) - Configure I2S with new API

  Serial.println("I2S initialized");
}

const int frequency = 440;    // 440Hz square wave
const int sampleRate = 16000; // 16kHz sample rate
const int amplitude = 8000;   // Amplitude value
unsigned int sample_count = 0;
const unsigned int samplesPerHalfCycle = sampleRate / frequency / 2;
bool isHigh = true;

void loop() {
  // Generate a single square wave sample
  if (sample_count % samplesPerHalfCycle == 0) {
    isHigh = !isHigh;
  }

  int16_t sample = isHigh ? amplitude : -amplitude;

  // TODO(human) - Write sample to I2S

  sample_count++;

  // Small delay to prevent overwhelming the I2S buffer
  delayMicroseconds(62); // ~16kHz sample rate (1000000/16000 = 62.5)
}
