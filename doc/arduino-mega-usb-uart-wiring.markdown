# Arduino Mega USB to UART converter wiring

## Introduction

This wiring schema uses only Tx from Arduino and is suitable to be used as standard error console.

## Wiring illustration

![arduino-mega-usb-uart-wiring.png](arduino-mega-usb-uart-wiring.png)

## Wiring table

| Signal | ATMega2560 port and pin | Arduino Mega 2560 pin | USB to UART converter pin |
| --- | --- | --- | --- |
| Ground (GND) | GND | GND | GND |
| Transmit data  from Arduino (TxD) | PORTD pin 3 (TXD1/INT3) | Digital pin 18 (TX1) | RxD |

