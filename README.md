# FreeRTOS Audio Player on STM32F405RGT6

This repository contains an embedded FreeRTOS-based application for the **STM32F405RGT6** microcontroller. The project demonstrates multitasking, peripheral control, SD card file access, touch-screen interaction, USB Mass Storage mode, internal temperature monitoring, and WAV audio playback using DAC + DMA.

## Project Overview

The firmware is developed with **STM32CubeIDE** and uses the STM32 HAL/CMSIS-RTOS2 environment. The application runs multiple FreeRTOS tasks to separate system initialization, user input handling, audio playback, display updates, and temperature measurement.

Main functions of the project:

- Play an 8-bit mono WAV file from an SD card.
- Display system status, music status, playback time, and internal MCU temperature on an LCD.
- Use a touch interface to control playback and USB mode.
- Use USB Mass Storage Class mode to allow a PC to copy `music.wav` to the storage device.
- Read the internal temperature sensor using ADC + DMA.
- Output audio through DAC Channel 1 using DMA and TIM6 trigger.

## Hardware Target

- **MCU:** STM32F405RGT6
- **Core:** ARM Cortex-M4
- **Development environment:** STM32CubeIDE
- **RTOS:** FreeRTOS / CMSIS-RTOS2
- **Storage:** SD card with FATFS
- **Display/Input:** LCD with touch controller
- **Audio output:** DAC Channel 1
- **USB:** USB Device Mass Storage Class

## Main Peripherals Used

| Peripheral | Purpose |
|---|---|
| ADC1 + DMA | Read VREFINT and internal temperature sensor |
| DAC + DMA | Output audio waveform |
| TIM6 | DAC trigger timer for audio sample rate |
| TIM3 | ADC trigger timer |
| SDIO | SD card interface |
| FATFS | File system access for `music.wav` |
| SPI1 | LCD and touch communication |
| USB OTG HS | USB Mass Storage mode |
| USART1 | Debug/communication interface |

## FreeRTOS Design

The application is divided into several FreeRTOS threads:

| Task | Priority | Description |
|---|---:|---|
| `defaultTask` | Normal | Initializes ADC DMA, timer, SD card mount, and initial SD/music status display |
| `controlTask` | Above Normal | Handles application commands from the queue, including play and USB mode control |
| `audioTask` | High | Services audio buffer refill, playback completion, and playback time display |
| `touchTask` | Normal | Reads touch input and sends commands to the control queue |
| `tempTask` | Low | Calculates and updates internal MCU temperature on the LCD |

Synchronization objects:

- `appCmdQueue`: message queue for user/application commands.
- `spi1Mutex`: protects LCD/touch access over SPI.
- `sdMutex`: protects SD/FATFS and audio file access.

## Audio File Requirement

The firmware expects the audio file to be named:

```text
music.wav
```

Required WAV format:

- PCM format
- Mono channel
- 8-bit unsigned samples
- 16 kHz sample rate

The firmware checks the WAV header before playback. If the file format is invalid, an error message is displayed on the LCD.

## User Interface

The LCD interface shows:

- Project/group title
- Internal temperature
- Play/Pause control area
- SD card status
- `music.wav` status
- Playback time in `MM:SS / MM:SS` format
- USB mode status

Touch control is used to send commands to the FreeRTOS control task through `appCmdQueue`.

## USB Mass Storage Mode

The project supports USB MSC mode so that a PC can access the storage device and copy a new `music.wav` file.

Typical flow:

1. Enter USB mode from the touch interface.
2. Connect to the PC.
3. Copy or replace `music.wav`.
4. Exit USB mode.
5. The firmware remounts the SD card and refreshes the music file status.

## Repository Structure

```text
.
├── Core/
│   ├── Inc/              # Application headers
│   └── Src/              # Main application source files
├── Drivers/              # CMSIS and STM32 HAL drivers
├── FATFS/                # FATFS middleware configuration
├── USB_DEVICE/           # USB device and MSC configuration
├── STM32F405RGTX_FLASH.ld
├── STM32F405RGTX_RAM.ld
├── project.ioc           # STM32CubeMX configuration file
└── README.md
```

## How to Build

1. Install **STM32CubeIDE**.
```bash
git clone https://github.com/lolxpraw/freertos-stm32f405.git
```

3. Open STM32CubeIDE.
4. Select **File → Import → Existing Projects into Workspace**.
5. Choose the cloned project folder.
6. Build the project using the Debug or Release configuration.
7. Flash the firmware to the STM32F405RGT6 board.

## How to Use

1. Format the SD card with a FAT-compatible file system.
2. Copy a valid `music.wav` file to the SD card.
3. Insert the SD card into the hardware system.
4. Flash and run the firmware.
5. Use the touch interface to start playback or enter USB mode.

## Learning Objectives

This project demonstrates:

- Real-time task scheduling with FreeRTOS.
- Inter-task communication using message queues.
- Resource protection using mutexes.
- FATFS-based file reading from SD card.
- Double-buffered audio playback using DAC + DMA.
- Timer-triggered ADC and DAC operation.
- Touch-screen based embedded user interface.
- USB Mass Storage integration on STM32.

## Demo

https://github.com/user-attachments/assets/ccf996e0-c88e-4a05-88fe-169f24f48223

## Notes

- The project is intended for STM32F405RGT6-based hardware.
- The audio playback implementation is designed for 8-bit mono 16 kHz WAV files.
- Hardware pin mapping and peripheral configuration can be reviewed or modified through `project.ioc` in STM32CubeIDE.

## Author

Developed as an embedded systems / real-time operating system project using STM32F4 and FreeRTOS.
