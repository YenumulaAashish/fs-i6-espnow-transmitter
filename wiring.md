# Wiring Guide

## Signal path

```text
FlySky FS-i6
    |
    | PPM OUT
    v
Signal conditioning
    |
    v
ESP32-S3 GPIO5
    |
    | ESP-NOW
    v
ESP-NOW receiver / Betaflight FC
```

## Recommended PPM input circuit

```text
FS-i6 PPM OUT
      |
     10k
      |
      +---- Base, NPN transistor (BC547 / 2N3904)

NPN Emitter ---------------------- GND

NPN Collector -------------------- ESP32-S3 GPIO5
      |
     4.7k
      |
ESP32-S3 3.3V

FS-i6 GND ------------------------ ESP32-S3 GND
```

## Important

- Do not connect FS-i6 battery/VCC directly to the ESP32-S3.
- FS-i6 and ESP32-S3 must share ground.
- Verify the trainer-port pinout for your exact cable before soldering.
- If you already have a verified clean 3.3 V PPM signal, you may connect it directly to GPIO5 with common ground.

## Channel mapping

| FS-i6 | Function | ESP-NOW |
|---|---|---|
| CH1 | Roll | CH1 |
| CH2 | Pitch | CH2 |
| CH3 | Throttle | CH3 |
| CH4 | Yaw | CH4 |
| CH5 | AUX1 / Arm | CH5 |
| CH6 | AUX2 | CH6 |
| — | Unused | CH7 |
| — | Unused | CH8 |

## RGB LED

| Color | Meaning |
|---|---|
| Yellow briefly | Boot |
| Red | PPM lost |
| Blue | PPM OK, waiting for ESP-NOW |
| Green | ESP-NOW peer found, Arm off |
| Purple | ESP-NOW peer found, Arm on |
