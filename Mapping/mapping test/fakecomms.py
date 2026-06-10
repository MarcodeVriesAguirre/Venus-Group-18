import paho.mqtt.client as mqtt
import socket
import time

MQTT_BROKER = "mqtt.ics.ele.tue.nl"
USERNAME = "robot_18_1"
PASSWORD = "mv6p0Gzx"

RECV_TOPICS = [
    "/pynqbridge/18_1/recv",
    "/pynqbridge/robot_18_1/recv",
    "/pynqbridge/18/recv",
]

TCP_HOST = "127.0.0.1"
TCP_PORT = 5555

# ── WORLD STATE (PYTHON NOW OWNS THIS) ─────────────
robots = {}
objects = []
cliffs = []

sock = None


# ─────────────────────────────────────────────────────
# TCP
# ─────────────────────────────────────────────────────

def tcp_connect():
    global sock
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((TCP_HOST, TCP_PORT))
    print("[TCP] connected to C renderer")


def send(msg):
    if sock:
        sock.sendall((msg + "\n").encode())


# ─────────────────────────────────────────────────────
# MQTT PARSER → WORLD UPDATE
# ─────────────────────────────────────────────────────

def parse(msg: str):
    parts = msg.split(",")

    if parts[0] == "ROBOT":
        rid = int(parts[1])
        x, y, h = float(parts[2]), float(parts[3]), float(parts[4])
        robots[rid] = (x, y, h)

    elif parts[0] == "OBJECT":
        objects.append((parts[1], float(parts[2]), float(parts[3])))

    elif parts[0] == "CLIFF":
        cliffs.append((float(parts[1]), float(parts[2])))


# ─────────────────────────────────────────────────────
# SEND FULL FRAME TO C
# ─────────────────────────────────────────────────────

def flush_frame():
    send("FRAME_START")

    for rid, (x, y, h) in robots.items():
        send(f"R|{rid}|{x}|{y}|{h}")

    for t, x, y in objects:
        send(f"O|{t}|{x}|{y}")

    for x, y in cliffs:
        send(f"C|{x}|{y}")

    send("FRAME_END")


# ─────────────────────────────────────────────────────
# MQTT
# ─────────────────────────────────────────────────────

def on_connect(client, userdata, flags, reason_code, properties):
    print("MQTT connected:", reason_code)
    for t in RECV_TOPICS:
        client.subscribe(t)


def on_message(client, userdata, message):
    raw = message.payload.decode(errors="replace").strip()
    parse(raw)


client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.username_pw_set(USERNAME, PASSWORD)
client.on_connect = on_connect
client.on_message = on_message

print("Connecting MQTT...")
client.connect(MQTT_BROKER, 1883, 60)

tcp_connect()

client.loop_start()

# ── MAIN LOOP: send frames at 30 FPS ───────────────
import time

try:
    while True:
        flush_frame()
        time.sleep(1/30)

except KeyboardInterrupt:
    pass

client.loop_stop()
client.disconnect()
sock.close()