import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np
import serial
import time

# Configure serial connection to ESP32
# TODO: Change port to match your ESP32 (use: ls /dev/cu.*)
ser = serial.Serial('/dev/cu.usbserial-0001', 115200, timeout=1)
time.sleep(2)

# Your code here
# In Python - reads the same serial output
line = ser.readline().decode('utf-8').strip()

def read_emg_data():
# Read from serial
    if ser.in_waiting > 0:
        line = ser.readline().decode('utf-8').strip()
        if line.startswith("EMG:"):
            parts = line.split(',')
            x = float(parts[0].split(':')[1].strip())  # x_value from main.cpp
            y1 = float(parts[1].split(':')[1].strip())
            y2 = float(parts[2].split(':')[1].strip())
            y3 = float(parts[3].split(':')[1].strip())
            y4 = float(parts[4].split(':')[1].strip())
            return x, y1, y2, y3, y4
    return None 

data = read_emg_data()
if data:
    x, y1, y2, y3, y4 = data