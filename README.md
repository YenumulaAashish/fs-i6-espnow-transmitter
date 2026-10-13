# FS-i6 PPM to ESP-NOW Transmitter (ESP32-S3)

A lightweight RC transmitter bridge that reads **PPM from the FlySky FS-i6 trainer port** and sends RC channels over **ESP-NOW** using an **ESP32-S3**.

## Features

- FlySky FS-i6 trainer-port PPM input
- ESP32-S3 DevKitC-1 support
- 6 PPM input channels
- 8 ESP-NOW output channels
- 50 Hz update rate
- 150 ms PPM-loss failsafe
- RGB status LED
- Serial channel monitoring at 115200 baud

## Signal flow

```text
FlySky FS-i6
    |
    | PPM OUT
    v
ESP32-S3 GPIO5
    |
    | ESP-NOW
    v
ESP-NOW receiver
    |
    v
Betaflight
```

## Hardware

- FlySky FS-i6
- ESP32-S3 DevKitC-1
- BC547 or 2N3904 NPN transistor
- 10 kΩ resistor
- 4.7 kΩ resistor
- USB data cable

See [wiring.md](wiring.md).

## Channel mapping

| Channel | Function |
|---|---|
| CH1 | Roll |
| CH2 | Pitch |
| CH3 | Throttle |
| CH4 | Yaw |
| CH5 | AUX1 / Arm |
| CH6 | AUX2 |
| CH7 | Unused |
| CH8 | Unused |

## RGB status

| Color | Status |
|---|---|
| Yellow briefly | Boot |
| Red | PPM lost |
| Blue | PPM OK, waiting for receiver |
| Green | ESP-NOW peer found, Arm off |
| Purple | ESP-NOW peer found, Arm on |

## Build

```powershell
py -m platformio run -e esp32s3
```

## Upload

```powershell
py -m platformio run -e esp32s3 -t upload
```

## Serial monitor

```powershell
py -m platformio device monitor -b 115200
```

Typical output:

```text
PPM OK   RC: 1500 1500 1000 1500 1000 1000 1000 1000 | ESP-NOW:CONNECTED | ARM:OFF
```

## Failsafe

If PPM is lost for more than 150 ms:

```text
Roll     = 1500
Pitch    = 1500
Throttle = 1000
Yaw      = 1500
AUX1     = 1000
AUX2     = 1000
CH7      = 1000
CH8      = 1000
```

The receiving flight controller should still have its own failsafe configured.

## Betaflight check

Before installing propellers, verify:

```text
Roll      ~1500 centered
Pitch     ~1500 centered
Throttle  ~1000 minimum
Yaw       ~1500 centered
AUX1      changes with Arm switch
AUX2      changes with assigned switch
```

## Project structure

```text
fs-i6-espnow-transmitter/
├── src/
│   └── tx.cpp
├── platformio.ini
├── wiring.md
├── README.md
├── LICENSE
└── .gitignore
```

## Safety

- Remove propellers during receiver, motor and failsafe testing.
- Verify channel directions and endpoints before flight.
- Do not apply 5 V to an ESP32-S3 GPIO.
- Verify trainer-port wiring before connecting the FS-i6.
