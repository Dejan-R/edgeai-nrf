## EdgeAI_nRF: Real-Time Vibration Anomaly Detection


This project implements an **embedded Edge AI system** on **nRF5340DK + nRF7002EK** running **Zephyr RTOS**.  
A TinyML model trained with Edge Impulse detects vibration anomalies from an MPU6050 accelerometer and publishes fault information via **MQTT over Wi-Fi**

---

## Features
 - **Real-time deterministic vibration acquisition at 100 Hz (MPU6050 over I2C)**
 - **Embedded DSP and ML inference running directly on nRF5340 (Cortex-M33)**
 - **FFT-based feature extraction with unsupervised K-means anomaly detection**
 - **Fully autonomous edge classification without cloud dependency**
 - **Reliable MQTT telemetry over Wi-Fi (nRF7002)**
 - **Integrated hardware watchdog for industrial-grade robustness**
 - **Designed for predictive maintenance of industrial rotating equipment**

---

## Project Structure
```text

EDGEAI_NRF/
├── boards/                           # Development board configurations
│   ├── nrf5340dk_nrf5340_cpuapp_ns.conf
│   ├── nrf5340dk_nrf5340_cpuapp.overlay
│   └── nrf7002dk_nrf5340_cpuapp_ns.conf
├── src/                              # Main application source code
│   ├── main.cpp                      # Main code
│   ├── mpu6050/                      # Driver for MPU6050 sensor
│   │   └── mpu6050.h / .cpp          # Sensor (initialization, communication)
│   ├── mqtt/                         # MQTT communication
│   │   └── mqtt.h / .cpp
├── third_party/                       # Edge Impulse SDK and model
│   └── edge_impulse/
│       ├── edge-impulse-sdk/
│       ├── model-parameters/
│       ├── tflite-model/
│       ├── CMakeLists.txt
│       └── README.md
├── utils/                             # Helper CMake functions and build tools
│                                      
├── .gitignore                         # Ignore build and temporary files
├── CMakeLists.txt                     # Project CMake configuration
├── Kconfig                            # Zephyr OS configuration options
├── prj.conf                           # Main Zephyr configuration (I2C, Wi-Fi,…)
├── README.md                          # Project description and build/deploy instructions
└── sysbuild.conf                      # System build configuration (Wi-Fi)
```
> **Note:**  
> The project was inspired by Edge Impulse examples:  
> 1. [Nordic nRF52840/nRF5340 TinyML firmware](https://github.com/edgeimpulse/firmware-nordic-nrf52840dk-nrf5340dk) – used as a reference for model integration and MCU setup (adapted for GY-521 sensor).  
> 2. [Standalone inferencing example for Zephyr](https://github.com/edgeimpulse/example-standalone-inferencing-zephyr) – used as a base for project structure and Zephyr integration.  
> All hardware, sensor configuration, and firmware logic have been **customized** for this project.
> **SDK / Toolchain:** This project is built using **nRF Connect SDK and Zephyr 3.1.1 toolchain**.

## Edge Impulse SDK
The Edge Impulse SDK is required for this project. Please download it from:
https://mlstudio.nordicsemi.com/
Place it in the folder `third_party/edge_impulse/` before building.

---

## Overview

The embedded Edge node consists of:
- **MCU:** nRF5340DK
- **Wi-Fi:** nRF7002EK
- **Sensor:** GY-521 (MPU6050 accelerometer)
- **RTOS:** Zephyr
- **ML framework:** Edge Impulse
- **Communication:** MQTT

Vibration data is continuously collected, processed locally, and classified as *OK* or *FAULT*.  
When an anomaly is detected, the device publishes an MQTT message containing the anomaly score, threshold value, and classification result.

## Runtime Behavior
- Deterministic sampling triggered via Zephyr timer and semaphore
- Frame-based inference execution
- Consecutive anomaly validation to prevent false positives
- Watchdog-fed main loop for system supervision

## Notes

FIXED_THRESHOLD = 300.0f is an example value for demo purposes; it can be calibrated for specific motors or operating conditions.

Edge Impulse handles the DSP and ML inference; the system runs fully on-device, no cloud required.

The watchdog ensures system reliability even under long-term operation.

The system is intended for **predictive maintenance of industrial motors**  
(e.g., pumps, compressors, rotating machinery).

By learning normal operating vibration patterns, the model can detect deviations and **signal potential failures before they occur**, reducing unplanned downtime.

## Author / Contact

**Dejan Rakijasic**  
LinkedIn: [https://www.linkedin.com/in/dejan-rakijasic/](https://www.linkedin.com/in/dejan-rakijasic/)
