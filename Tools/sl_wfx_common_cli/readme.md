# SL WFX Common CLI

## Overview

SL WFX Common CLI is a library with the purpose to provide a Command Line Interface (CLI), common to different OS and targets, to allow interaction with part of the Wi-Fi FMAC driver and an IP stack.

|                  | Resource supported                                                             |
|------------------|--------------------------------------------------------------------------------|
| OS               | MicriumOS<br>FreeRTOS                                                          |
| Target           | EFM32GG11 (SLSTK3701A)<br>WGM160P<br>STM32F4 (Nucleo-F429ZI)                   |
| IP stack         | LwIP                                                                           |

## Architecture Description

The SL WFX Common CLI is composed of three layers listed below from top to bottom:

* **Module layer**: this layer is composed of software modules and each module regroups commands related to a feature. This allows to control with a fine granulartiy which modules to add to an application.
* **Shell Generic layer**: this layer provides generic shell functions, allowing the modules to register commands, interact with the input/output and wait for events with an abstraction of the OS and the target on which the application is running.
* **Shell Specific layer**: this layer implements the functions required by the Shell Generic layer with the specificies related to the OS and the target on which the application is running.

The goal of this architecture is the ease the addition of new modules and new supported resources by having a low dependency between the layers.
