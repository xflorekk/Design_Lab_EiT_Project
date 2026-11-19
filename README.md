# Design Lab - EiT Project

## Description
This repository contains the source code, documentation, and hardware/software design files for a student engineering project developed as part of the **Design Laboratory** course (Electronics and Telecommunications / EiT). 

The main goal of the project is to **understand and create microcontroller program and electronic environment**..

## Authors
- Daniel Florek
- Jakub Wąsowicz
- Filip Dudek

## Block diagaram

![image](assets\KiCad_1.3.png)

## Pinout tables
| ATmega 328PB Pin | 7seg Pin |
| :---: | :---: |
|VCC|VCC|
|PC0|D1|
|PC1|D2|
|PC2|D3|
|PC3|D4|
|PD2|A|
|PD3|B|
|PD4|C|
|PD5|D|
|PD6|E|
|PD7|F|
|PB0|G|
|PB1|DP|

| ATmega 328PB Pin | Keyboard Pin |
| :---: | :---: |
|PE0|C1|
|PE1|C1|
|PE2|C1|
|PE3|C1|
|GND|R1|

| ATmega 328PB Pin | USB-UART Pin |
| :---: | :---: |
|VCC|VCCIO|
|GND|GND|
|PD0|TXD|
|PD1|RXD|

| ATmega 328PB Pin | BMP280 Pin |
| :---: | :---: |
|3V3|VCC|
|GND|GND|
|PC5|SCL|
|PC4|SDA|
|3V3|CSB|
|GND|SDO|

| ATmega 328PB Pin | SD card reader Pin |
| :---: | :---: |
|GND|GND|
|VCC|+5|
|PB2|CS|
|PB3|MOSI|
|PB4|MISO|
|PB5|SCK|
|GND|GND|