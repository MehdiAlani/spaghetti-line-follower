# 🏁 Spaghetti — Line Following Robot (ESP32-S3)

Autonomous line-following robot developed for robotics competitions using ESP32-S3, RTOS, and a custom embedded control system.

---

## 📸 Robot

![Spaghetti](spaghetti.jpg)

---

## 🚀 Overview

This is my third line-following robot and the final version of a 3-stage evolution built during my first year in robotics competitions.

The goal was to improve:
- stability at high speed  
- sensor accuracy  
- control system reliability  
- real-time debugging  

---

## ⚙️ System

- ESP32-S3 main controller  
- RTOS-based firmware  
- MPU6050 gyro sensor  
- Magnetic wheel encoders  
- 9× TCRT5000 IR sensor array  
- CD74HC4067 analog multiplexer  
- SSD1306 OLED display  
- WebSocket debugging system  

---

## 🧠 Control

- PID line following  
- encoder feedback control  
- IMU heading correction  
- real-time tuning during runs  

---

## 📷 Sensor Array

![IR Sensor](ir_sensor.jpg)

Custom 9-sensor IR array using TCRT5000 with analog multiplexing.

---



## GPIO Mapping

| Function | GPIO |
|----------|------|
| Right PWM | 40 |
| Right Forward | 42 |
| Right Backward | 2 |
| Left PWM | 39 |
| Left Forward | 41 |
| Left Backward | 1 |
| Right Encoder A | 21 |
| Right Encoder B | 47 |
| Left Encoder A | 46 |
| Left Encoder B | 45 |
| MUX SIG | 14 |
| MUX S0 | 19 |
| MUX S1 | 20 |
| MUX S2 | 3 |
| MUX S3 | 48 |
| SDA0 | 4 |
| SCL0 | 5 |
| SDA1 | 17 |
| SCL1 | 16 |