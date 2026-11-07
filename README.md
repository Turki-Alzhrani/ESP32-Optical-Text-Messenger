# Optical Text Messenger

Send text from a phone browser through an LED to an Arduino UNO with an OLED display.

## Components

- ESP32 DevKit
- Arduino UNO
- Blue LED
- 220 Ω resistor
- LDR (photoresistor)
- 10 kΩ potentiometer
- SSD1306 128×64 I²C OLED
- Arduino prototyping shield
- Breadboards, jumper wires and USB power

## Circuit

![Circuit wiring](images/wiring-diagram.png)

<img src="images/full-prototype.jpeg" alt="Complete circuit" width="650">

## Transmitter

<img src="images/esp32-transmitter.jpeg" alt="ESP32 transmitter" width="450">

## Receiver

<img src="images/arduino-receiver.jpeg" alt="Arduino UNO receiver" width="450">

<img src="images/optical-front-end.jpeg" alt="LDR, potentiometer and OLED receiver circuit" width="650">

## Demo

| Phone | OLED |
| :---: | :---: |
| <img src="images/phone-interface.png" alt="Phone text input" width="280"> | <img src="images/received-message.jpeg" alt="Received text on OLED" width="280"> |
