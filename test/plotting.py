import asyncio
import threading
from collections import deque

import matplotlib.animation as animation
import matplotlib.pyplot as plt
from bleak import BleakClient, BleakScanner

# Configure BLE connection to ESP32
DEVICE_NAME = "TN-XIAO-ESP32C3"
SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
TELEMETRY_CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"

# BLE client
ble_client = None
connection_success = False
connection_error = None

# Data buffers - store last 200 data points
max_points = 200
time_data = deque(maxlen=max_points)
emg1_data = deque(maxlen=max_points)
emg2_data = deque(maxlen=max_points)
angle_data = deque(maxlen=max_points)


def parse_payload(data):
    text = data.decode("utf-8").strip()
    fields = [field.strip() for field in text.split(",")]
    if len(fields) != 4:
        raise ValueError(f"Unexpected payload: {text}")

    return (
        float(fields[0]),
        float(fields[1]),
        float(fields[2]),
        float(fields[3]),
    )


def notification_handler(sender, data):
    del sender

    try:
        timestamp_ms, emg1, emg2, imu_angle = parse_payload(data)
    except ValueError as exc:
        print(f"Skipping malformed packet: {exc}")
        return

    time_data.append(timestamp_ms / 1000.0)
    emg1_data.append(emg1)
    emg2_data.append(emg2)
    angle_data.append(imu_angle)


async def find_device():
    print(f"Scanning for '{DEVICE_NAME}'...")
    devices = await BleakScanner.discover(timeout=5.0)

    print(f"\nFound {len(devices)} BLE devices:")
    for device in devices:
        print(f"  - {device.name or 'Unknown'} ({device.address})")
    print()

    for device in devices:
        if device.name == DEVICE_NAME:
            print(f"Target device found: {device.name} at {device.address}")
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
        print(f"Connected to {DEVICE_NAME}")

        await asyncio.sleep(1)

        if not ble_client.is_connected:
            print("Connection dropped after connect!")
            return False

        print("Subscribing to telemetry characteristic...")
        await ble_client.start_notify(TELEMETRY_CHAR_UUID, notification_handler)

        print("Receiving CSV telemetry via BLE...")
        return True
    except Exception as exc:
        print(f"Connection failed: {exc}")
        return False


async def disconnect_ble():
    global ble_client
    if ble_client and ble_client.is_connected:
        await ble_client.disconnect()
        print("Disconnected from BLE device")


fig, (ax_emg, ax_angle) = plt.subplots(2, 1, figsize=(10, 8), sharex=True)
line_emg1, = ax_emg.plot([], [], label="EMG 1", linewidth=1.5, color="blue")
line_emg2, = ax_emg.plot([], [], label="EMG 2", linewidth=1.5, color="red")
line_angle, = ax_angle.plot([], [], label="IMU Angle", linewidth=1.5, color="green")

ax_emg.set_ylabel("EMG Value")
ax_emg.set_title("TN ESP32 Telemetry")
ax_emg.legend(loc="upper right")
ax_emg.grid(True, alpha=0.3)

ax_angle.set_xlabel("Time (s)")
ax_angle.set_ylabel("Angle (deg)")
ax_angle.legend(loc="upper right")
ax_angle.grid(True, alpha=0.3)


def init():
    ax_emg.set_xlim(0, 10)
    ax_emg.set_ylim(0, 1.5)
    ax_angle.set_ylim(-90, 90)
    return line_emg1, line_emg2, line_angle


def update(frame):
    del frame

    if len(time_data) > 0:
        x_values = list(time_data)
        line_emg1.set_data(x_values, list(emg1_data))
        line_emg2.set_data(x_values, list(emg2_data))
        line_angle.set_data(x_values, list(angle_data))

        if len(x_values) > 1:
            ax_emg.set_xlim(x_values[0], x_values[-1])

            emg_values = list(emg1_data) + list(emg2_data)
            if emg_values:
                y_min = min(emg_values)
                y_max = max(emg_values)
                margin = (y_max - y_min) * 0.1 if y_max != y_min else 0.1
                ax_emg.set_ylim(y_min - margin, y_max + margin)

            angle_values = list(angle_data)
            if angle_values:
                y_min = min(angle_values)
                y_max = max(angle_values)
                margin = (y_max - y_min) * 0.1 if y_max != y_min else 5.0
                ax_angle.set_ylim(y_min - margin, y_max + margin)

    return line_emg1, line_emg2, line_angle


def on_close(event):
    del event
    print("\nClosing plot window...")

    global ble_client
    if ble_client and ble_client.is_connected:
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        loop.run_until_complete(disconnect_ble())
        loop.close()


def run_ble_in_thread():
    global connection_success, connection_error

    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)

    async def ble_task():
        global connection_success, connection_error

        try:
            if await connect_ble():
                connection_success = True
                while ble_client and ble_client.is_connected:
                    await asyncio.sleep(1)
            else:
                connection_error = "Connection failed"
        except Exception as exc:
            connection_error = str(exc)

    loop.run_until_complete(ble_task())
    loop.close()


def main():
    global connection_success, connection_error

    print("Connecting to BLE device...")

    ble_thread = threading.Thread(target=run_ble_in_thread, daemon=True)
    ble_thread.start()

    import time

    for _ in range(20):
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
    fig.canvas.mpl_connect("close_event", on_close)
    animation.FuncAnimation(fig, update, init_func=init, interval=100, blit=False)
    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    main()
