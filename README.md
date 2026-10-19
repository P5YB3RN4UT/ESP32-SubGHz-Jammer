<p align="center">
 <img src="https://raw.githubusercontent.com/P5YB3RN4UT/ESP32-SubGHz-Jammer/master/img/subghz.png">
</p>

<h3 align="center">ESP32 Sub-GHz Jammer</h3>

---


## Summary
This project involves creating a portable device that uses an ESP32 microcontroller to function as a Sub-GHz Jammer.


## Disclaimer 
This tool is strictly for educational purposes only. The author does not take responsibility for any illegal activity undertaken from the misuse of this software. Jamming Sub-GHz frequencies is a criminal offense.


## Introduction
This project includes source code for a Sub-GHz Jammer with the following requirements:

- <b>Frequency Jamming</b>

  <i>Be able to jam the frequencies 315 MHz, 433 MHz, 868 MHz and 915 MHz.</i>

- <b>WiFi Access Point</b>

  <i>The ESP32 acts as an open or password-protected access point.</i>

- <b>Frequency Selection</b>

  <i>Be able to switch between frequencies via the Web UI and provide visual feedback via the RGB LED.</i>

- <b>Stealth Mode</b>

  <i>Remotely control the RGB LED feedback to be active or inactive via the Web UI.</i>


## Hardware

Required:
1. ESP32-WROOM-32
2. Programming Cable
3. 18650 Battery
4. 18650 Battery Holder
5. CC1101 Module
6. TC4056/TP4056 Charging Module
7. On/Off Switch
8. Tactile Push Button
9. RGB LED
10. 3 x 220Ω Resistors
11. Project Box

Additional:
1. Perfboard
2. Pin Headers


## Pinout

| 18650 Battery | TC/TP4056 Charging Module | On/Off Switch | ESP32-WROOM-32 | Push Button | RESISTOR | CC1101 Module | RGB LED |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| + | B + |  |  |  |  |  |  |
| -	| B - |  |  |  |  |  |  |
| 	| OUT + | °/° | 5V |  |  |  |  |
| 	| OUT - |  | GND |  |  |  |  |
| 	|  |  | GPIO 33 | PIN 1 |  |  |  |
| 	|  |  | GND | PIN 2 |  |  |  |
| 	|  |  | GND |  |  |  | CATHODE |
| 	|  |  | GPIO 04 |   | 220Ω |  | RED |
| 	|  |  | GPIO 16 |   | 220Ω |  | GREEN |
| 	|  |  | GPIO 17 |   | 220Ω |  | BLUE |
| 	|  |  | 3V3 |   |  | VCC |  |
| 	|  |  | GND |   |  | GND |  |
| 	|  |  | GPIO 18 |  |  | SCK |  |
| 	|  |  | GPIO 19 |  |  | MISO |  |
| 	|  |  | GPIO 23 |  |  | MOSI |  |
| 	|  |  | GPIO 05 |  |  | CSN |  |
| 	|  |  | GPIO 25 |  |  | GDO0 |  |
