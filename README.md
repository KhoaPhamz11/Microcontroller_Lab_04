# Microprocessors & Microcontrollers - Lab 4: Cooperative Scheduler

## Project Overview
This repository contains the firmware and simulation for **Lab 4: A Cooperative Scheduler** implemented on the **STM32F103C6** ARM Cortex-M3 microcontroller. 

The scheduler employs a **Delta-List** data structure to maintain sorted relative delays among pending tasks, achieving $O(1)$ time complexity for periodic timing updates inside the Timer ISR (`SCH_Update`).

## System Specifications
- **Target MCU:** STM32F103C6 (HSI 8 MHz)
- **Timer Configuration:** TIM2 Interrupt with Prescaler = 7999, Period = 9 (10 ms tick period)
- **Serial Interface:** USART1 (PA9-TX, PA10-RX) at 115200 bps
- **Outputs (Active-Low):**
  - PA0: `LED_RED` (Period: 500 ms)
  - PA1: `LED_BLUE` (Period: 1000 ms)
  - PA2: `LED_YELLOW` (Period: 1500 ms)
  - PA3: `LED_GREEN` (Period: 2000 ms)
  - PA4: `LED_PINK` (Period: 2500 ms)
- **Auxiliary Tasks:**
  - One-Shot Callback at $t = 300\text{ ms}$
  - Real-time tick tracking callback every $10\text{ ms}$

## Core Architecture
- `SCH_Update()`: Executed inside `TIM2` ISR; only decrements the head task's relative delay ($O(1)$).
- `SCH_Dispatch_Tasks()`: Dispatches ready tasks in the super loop (`while(1)`), pops executed tasks, and re-schedules periodic tasks before executing I/O intensive callbacks.

## Directory Layout
```text
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   └── scheduler.h
│   └── Src/
│       ├── main.c
│       └── scheduler.c
├── Lab4_scheduler.ioc
├── simulation/
│   └── Lab4_schematic.pdsprj
├── .gitignore
└── README.md