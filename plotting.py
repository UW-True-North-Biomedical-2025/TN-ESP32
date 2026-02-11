import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np
import asyncio
import struct
from collections import deque
from bleak import BleakClient, BleakScanner

# Configure BLE connection to ESP32
DEVICE_NAME = "ESP32S3-EMG"  # Change to match your ESP32's BLE name
SERVICE_UUID = "c8c4a1d0-9f8a-4d4d-8d79-bd3e9e4c11b0"
EMG_RAW_CHAR_UUID = "6e2c1a30-4f27-4ad0-8c40-7aa3e1f8f5b4"
EMG_ENVELOPE_UUID = "c1f5bbda-f9cc-4a3e-a4c8-9f6247b70c4e"

# BLE client
ble_client = None
sample_count = 0

# Data buffers - store last 100 data points
#creates sliding of the plot by only keeping last 100 points of data
max_points = 100
x_data = deque(maxlen=max_points)
y1_data = deque(maxlen=max_points)
y2_data = deque(maxlen=max_points)
y3_data = deque(maxlen=max_points)
y4_data = deque(maxlen=max_points)


def notification_handler_emg1(sender, data):
    global sample_count
    value = struct.unpack('<H', data)[0]
    x_data.append(sample_count)
    y1_data.append(value)
    sample_count += 1

def notification_handler_emg2(sender, data):

    value = struct.unpack('<H', data)[0]
    y2_data.append(value)

def notification_handler_emg3(sender, data):
    value = struct.unpack('<H', data)[0]
    y3_data.append(value)

def notification_handler_emg4(sender, data):
    value = struct.unpack('<H', data)[0]
    y4_data.append(value)

async def find_device():
    print(f"Scanning for '{DEVICE_NAME}'...")
    devices = await BleakScanner.discover(timeout=5.0)
    for device in devices:
        if device.name == DEVICE_NAME:
            print(f"Found: {device.name} at {device.address}")
            return device.address
    return None

async def connect_ble():
    global ble_client
    
    address = await find_device()
    if not address:
        print(f"Device '{DEVICE_NAME}' not found!")
        return False
    
    try:
        ble_client = BleakClient(address)
        await ble_client.connect()
        print(f"Connected to {DEVICE_NAME}")
        
        # Subscribe to BLE notifications
        await ble_client.start_notify(EMG_RAW_CHAR_UUID, notification_handler_emg1)
        await ble_client.start_notify(EMG_ENVELOPE_UUID, notification_handler_emg2)
        # Add more characteristics if you have EMG3/EMG4
        
        print("Receiving data via BLE...")
        return True
    except Exception as e:
        print(f"Connection failed: {e}")
        return False

async def disconnect_ble():
    """Disconnect from BLE device"""
    global ble_client
    if ble_client and ble_client.is_connected:
        await ble_client.disconnect()
        print("Disconnected from BLE device") 

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
    # Data comes via BLE notifications, just update the plot
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

async def main():
    """Main async function"""
    # Connect to BLE device
    if not await connect_ble():
        return
    
    # Start the animation
    ani = animation.FuncAnimation(
        fig, update, init_func=init, 
        interval=50, blit=True, cache_frame_data=False
    )

    plt.tight_layout()
    
    try:
        plt.show()
    except KeyboardInterrupt:
        print("\nStopping...")
    finally:
        await disconnect_ble()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nExiting...")