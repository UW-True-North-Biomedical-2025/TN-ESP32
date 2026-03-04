import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np
import asyncio
import struct
import threading
from collections import deque
from bleak import BleakClient, BleakScanner

# Configure BLE connection to ESP32
DEVICE_NAME = "MyESP32"  # Change to match your ESP32's BLE name
SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
EMG1_CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"
EMG2_CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a9"
EMG3_CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26aa"
EMG4_CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26ab"

# BLE client
ble_client = None
sample_count = 0
connection_success = False
connection_error = None

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
    
    print(f"\nFound {len(devices)} BLE devices:")
    for device in devices:
        print(f"  - {device.name or 'Unknown'} ({device.address})")
    print()
    
    for device in devices:
        if device.name == DEVICE_NAME:
            print(f"✓ Target device found: {device.name} at {device.address}")
            return device.address
    return None

async def connect_ble():
    global ble_client
    
    address = await find_device()
    if not address:
        print(f"Device '{DEVICE_NAME}' not found!")
        return False
    
    try:
        ble_client = BleakClient(address, timeout=15.0)
        print("Attempting connection...")
        await ble_client.connect()
        print(f"✓ Connected to {DEVICE_NAME}")
        
        # Small delay to ensure connection is stable
        await asyncio.sleep(1)
        
        if not ble_client.is_connected:
            print("Connection dropped after connect!")
            return False
        
        print("Subscribing to characteristics...")
        
        # Subscribe to BLE notifications for all 4 EMG channels
        await ble_client.start_notify(EMG1_CHAR_UUID, notification_handler_emg1)
        print("  ✓ EMG1 subscribed")
        
        await ble_client.start_notify(EMG2_CHAR_UUID, notification_handler_emg2)
        print("  ✓ EMG2 subscribed")
        
        await ble_client.start_notify(EMG3_CHAR_UUID, notification_handler_emg3)
        print("  ✓ EMG3 subscribed")
        
        await ble_client.start_notify(EMG4_CHAR_UUID, notification_handler_emg4)
        print("  ✓ EMG4 subscribed")
        
        print("✓ Receiving data via BLE...")
        return True
    except Exception as e:
        print(f"Connection failed: {e}")
        import traceback
        traceback.print_exc()
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

def on_close(event):
    """Handle window close event"""
    print("\nClosing plot window...")
    global ble_client
    if ble_client and ble_client.is_connected:
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        loop.run_until_complete(disconnect_ble())
        loop.close()

def run_ble_in_thread():
    """Run BLE connection in background thread"""
    global connection_success, connection_error
    
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    
    async def ble_task():
        global connection_success, connection_error
        try:
            if await connect_ble():
                connection_success = True
                # Keep the connection alive
                while ble_client and ble_client.is_connected:
                    await asyncio.sleep(1)
            else:
                connection_error = "Connection failed"
        except Exception as e:
            connection_error = str(e)
    
    loop.run_until_complete(ble_task())
    loop.close()

def main():
    """Main function"""
    global connection_success, connection_error
    
    print("Connecting to BLE device...")
    
    # Start BLE in background thread
    ble_thread = threading.Thread(target=run_ble_in_thread, daemon=True)
    ble_thread.start()
    
    # Wait for connection (up to 10 seconds)
    import time
    for i in range(20):  # 20 x 0.5s = 10 seconds
        if connection_success:
            break
        if connection_error:
            print(f"Failed to connect: {connection_error}")
            return
        time.sleep(0.5)
    
    if not connection_success:
        print("Failed to connect. Exiting.")
        return
    
    print("Starting plot...")
    # Set up window close handler
    fig.canvas.mpl_connect('close_event', on_close)
    
    # Start the animation
    ani = animation.FuncAnimation(
        fig, update, init_func=init, 
        interval=50, blit=True, cache_frame_data=False
    )

    plt.tight_layout()
    plt.show(block=True)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nExiting...")
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()