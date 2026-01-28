import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np
import serial
import time
from collections import deque

# Configure serial connection to ESP32
ser = serial.Serial('/dev/cu.usbserial-120', 115200, timeout=1)
time.sleep(2)

# Data buffers - store last 100 data points
max_points = 100
x_data = deque(maxlen=max_points)
y1_data = deque(maxlen=max_points)
y2_data = deque(maxlen=max_points)
y3_data = deque(maxlen=max_points)
y4_data = deque(maxlen=max_points)


def read_emg_data():
    if ser.in_waiting > 0:
        line = ser.readline().decode('utf-8').strip()
        if line.startswith("EMG:"):
            parts = line.split(',')
            x = float(parts[0].split(':')[1].strip()) 
            y1 = float(parts[1].split(':')[1].strip())
            y2 = float(parts[2].split(':')[1].strip())
            y3 = float(parts[3].split(':')[1].strip())
            y4 = float(parts[4].split(':')[1].strip())
            return x, y1, y2, y3, y4
    return None 

# Set up the figure and axis
fig, ax = plt.subplots(figsize=(10, 6))
line1, = ax.plot([], [], label='EMG1', linewidth=1.5, color="blue")
line2, = ax.plot([], [], label='EMG2', linewidth=1.5, color="red")
line3, = ax.plot([], [], label='EMG3', linewidth=1.5, color="yellow")
line4, = ax.plot([], [], label='EMG4', linewidth=1.5, color="green")

ax.set_xlabel('Time')
ax.set_ylabel('EMG Value')
ax.set_title('Real-time EMG Data')
ax.legend(loc='upper right')
ax.grid(True, alpha=0.3)

def init():
    ax.set_xlim(0, max_points)
    ax.set_ylim(0, 4096)
    return line1, line2, line3, line4


def update(frame):
    data = read_emg_data()
    if data:
        x, y1, y2, y3, y4 = data
        x_data.append(x)
        y1_data.append(y1)
        y2_data.append(y2)
        y3_data.append(y3)
        y4_data.append(y4)
    
    # Update lines with buffered data
    if len(x_data) > 0:
        line1.set_data(list(x_data), list(y1_data))
        line2.set_data(list(x_data), list(y2_data))
        line3.set_data(list(x_data), list(y3_data))
        line4.set_data(list(x_data), list(y4_data))
        
        # Auto-scale axes
        if len(x_data) > 1:
            ax.set_xlim(min(x_data), max(x_data))
            all_y = list(y1_data) + list(y2_data) + list(y3_data) + list(y4_data)
            if all_y:
                y_min, y_max = min(all_y), max(all_y)
                margin = (y_max - y_min) * 0.1 if y_max != y_min else 100
                ax.set_ylim(y_min - margin, y_max + margin)

    return line1, line2, line3, line4

# Start the animation
ani = animation.FuncAnimation(
    fig, update, init_func=init, 
    interval=50, blit=True, cache_frame_data=False
)

plt.tight_layout()
plt.show()