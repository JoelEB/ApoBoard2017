software
========

This Repo contains source code for the DIY Interactive Electronics Apogaea PCB.

The system is based on an Atmega328p microcontroller (the same device in the Arduino Duemilanove), and can be programmed with an FTDI serial adapter and the Arduino IDE.

The software reads analog signals from the on-board microphone amplifier and performs an FHT (Fast Hartley Transform), which outputs an array of frequency values. We use this array to decide how to control the LEDs, and repeat the process. The Software also support multiple modes, which the user can switch between with the on-board momentary tactile button. This software is alpha-quality, and may or may not work as expected!

All of this code is developed for free, for Apogaea, and for fun. It is publicly available under the GNU General Public License version 2 (the "GPL License"). For more information, please see: http://www.gnu.org/licenses/gpl-2.0.html

Copyright (c) John English, 2013
Licensing: GPL v2
