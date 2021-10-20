This repository contains the FW for the DABon project

# Purpose of the project
The goal of this project is to have a portable DAB/FM radio and MP3 player.  
The core of the project is a Cortex-M4 STM32F407 which is connected with several peripherals:
* 128x64 OLED display based on SSD1306
* micro SD card holder
* Si4684 DAB/FM receievr
* W25Q16DV EEPROM
* SGTL5000 audio codec
* MCP73871 power-path and battery charger
* CC2564MODA Bluetooth module, for music streaming
* STC3115 gas-gauge, for accurate battery measurements

# Further details/notes
* There's no commercial RTOS in the STM32, but it is based on a proprietary non-preemptive scheduler that I developed
  * Being non-preemptive makes life a lot easier...
* All the drivers for STM32's internal peripherals and external devices (es: SGTL5000, ...) are developed by myself
* MP3 decoding is performed by "libmad"
* FAT32 support is provided by "FatFS" 
