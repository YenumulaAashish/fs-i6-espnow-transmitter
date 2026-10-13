#include <Arduino.h>
#include "esp32-hal-rgb-led.h"
#include "EspNowRcLink/Transmitter.h"

// FS-i6 PPM -> ESP32-S3 -> ESP-NOW

const int PPM_PIN = 5;

#ifdef RGB_BUILTIN
const int RGB_LED_PIN = RGB_BUILTIN;
#else
const int RGB_LED_PIN = 48;
#endif

const uint8_t LED_BRIGHTNESS = 35;

const uint8_t PPM_CHANNEL_COUNT = 6;
const uint8_t OUTPUT_CHANNEL_COUNT = 8;

const int RC_MIN = 1000;
const int RC_CENTER = 1500;
const int RC_MAX = 2000;
const int AUX_ARM_THRESHOLD = 1500;

const uint16_t PPM_MIN_VALID_US = 800;
const uint16_t PPM_MAX_VALID_US = 2200;
const uint16_t PPM_SYNC_GAP_US = 3000;

const uint32_t PPM_FAILSAFE_US = 150000;
const uint32_t SEND_INTERVAL_MS = 20;

EspNowRcLink::Transmitter tx;

volatile uint16_t ppmRaw[PPM_CHANNEL_COUNT] = {
  1500, 1500, 1000, 1500, 1000, 1000
};

volatile uint8_t ppmIndex = 0;
volatile uint32_t lastPpmEdgeUs = 0;
volatile uint32_t lastValidFrameUs = 0;

enum LedState {
  LED_STATE_UNKNOWN,
  LED_STATE_PPM_LOST,
  LED_STATE_WAITING,
  LED_STATE_CONNECTED,
  LED_STATE_ARMED
};

LedState currentLedState = LED_STATE_UNKNOWN;

static void setRgb(uint8_t red, uint8_t green, uint8_t blue)
{
  neopixelWrite(RGB_LED_PIN, red, green, blue);
}

static void updateStatusLed(bool ppmOK, bool espNowConnected, bool armed)
{
  LedState newState;

  if (!ppmOK) newState = LED_STATE_PPM_LOST;
  else if (!espNowConnected) newState = LED_STATE_WAITING;
  else if (armed) newState = LED_STATE_ARMED;
  else newState = LED_STATE_CONNECTED;

  if (newState == currentLedState) return;

  currentLedState = newState;

  switch (currentLedState) {
    case LED_STATE_PPM_LOST:
      setRgb(LED_BRIGHTNESS, 0, 0);
      break;
    case LED_STATE_WAITING:
      setRgb(0, 0, LED_BRIGHTNESS);
      break;
    case LED_STATE_CONNECTED:
      setRgb(0, LED_BRIGHTNESS, 0);
      break;
    case LED_STATE_ARMED:
      setRgb(LED_BRIGHTNESS, 0, LED_BRIGHTNESS);
      break;
    default:
      setRgb(0, 0, 0);
      break;
  }
}

void IRAM_ATTR ppmISR()
{
  uint32_t now = micros();
  uint32_t interval = now - lastPpmEdgeUs;
  lastPpmEdgeUs = now;

  if (interval > PPM_SYNC_GAP_US) {
    ppmIndex = 0;
    return;
  }

  if (interval >= PPM_MIN_VALID_US &&
      interval <= PPM_MAX_VALID_US &&
      ppmIndex < PPM_CHANNEL_COUNT) {

    ppmRaw[ppmIndex] = static_cast<uint16_t>(interval);
    ppmIndex++;

    if (ppmIndex == PPM_CHANNEL_COUNT) {
      lastValidFrameUs = now;
    }
  }
}

static bool isTransmitterConnected()
{
  WifiEspNowPeerInfo peers[1];
  return WifiEspNow.listPeers(peers, 1) > 0;
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("======================================");
  Serial.println(" FS-i6 PPM -> ESP32-S3 -> ESP-NOW");
  Serial.println("======================================");

  pinMode(PPM_PIN, INPUT);

  attachInterrupt(
    digitalPinToInterrupt(PPM_PIN),
    ppmISR,
    RISING
  );

  setRgb(0, 0, 0);
  delay(100);
  setRgb(LED_BRIGHTNESS, LED_BRIGHTNESS, 0);

  tx.begin(true);

  delay(300);
  currentLedState = LED_STATE_UNKNOWN;

  Serial.println("ESP-NOW transmitter started");
  Serial.println("PPM input = GPIO5");
  Serial.print("RGB LED pin = ");
  Serial.println(RGB_LED_PIN);
}

void loop()
{
  static uint32_t lastSendMs = 0;

  tx.update();

  uint32_t now = millis();

  if (now - lastSendMs < SEND_INTERVAL_MS) return;
  lastSendMs = now;

  uint16_t ppm[PPM_CHANNEL_COUNT];
  uint32_t frameTime;

  noInterrupts();
  for (uint8_t i = 0; i < PPM_CHANNEL_COUNT; i++) {
    ppm[i] = ppmRaw[i];
  }
  frameTime = lastValidFrameUs;
  interrupts();

  bool ppmOK = false;

  if (frameTime != 0) {
    uint32_t age = micros() - frameTime;
    ppmOK = age < PPM_FAILSAFE_US;
  }

  int channels[OUTPUT_CHANNEL_COUNT];

  if (ppmOK) {
    channels[0] = constrain((int)ppm[0], RC_MIN, RC_MAX);
    channels[1] = constrain((int)ppm[1], RC_MIN, RC_MAX);
    channels[2] = constrain((int)ppm[2], RC_MIN, RC_MAX);
    channels[3] = constrain((int)ppm[3], RC_MIN, RC_MAX);
    channels[4] = constrain((int)ppm[4], RC_MIN, RC_MAX);
    channels[5] = constrain((int)ppm[5], RC_MIN, RC_MAX);
    channels[6] = RC_MIN;
    channels[7] = RC_MIN;
  } else {
    channels[0] = RC_CENTER;
    channels[1] = RC_CENTER;
    channels[2] = RC_MIN;
    channels[3] = RC_CENTER;
    channels[4] = RC_MIN;
    channels[5] = RC_MIN;
    channels[6] = RC_MIN;
    channels[7] = RC_MIN;
  }

  for (uint8_t i = 0; i < OUTPUT_CHANNEL_COUNT; i++) {
    tx.setChannel(i, channels[i]);
  }

  tx.commit();

  bool espNowConnected = isTransmitterConnected();
  bool armed = ppmOK && channels[4] > AUX_ARM_THRESHOLD;

  updateStatusLed(ppmOK, espNowConnected, armed);

  Serial.print(ppmOK ? "PPM OK   RC:" : "PPM LOST RC:");

  for (uint8_t i = 0; i < OUTPUT_CHANNEL_COUNT; i++) {
    Serial.print(' ');
    Serial.print(channels[i]);
  }

  Serial.print(" | ESP-NOW:");
  Serial.print(espNowConnected ? "CONNECTED" : "WAITING");

  Serial.print(" | ARM:");
  Serial.print(armed ? "ON" : "OFF");

  Serial.println();
}
