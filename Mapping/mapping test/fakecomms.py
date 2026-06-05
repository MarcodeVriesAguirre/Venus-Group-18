import paho.mqtt.client as mqtt
import socket
import time
import threading
 
# ── MQTT config ──────────────────────────────────────────────────────────────
MQTT_BROKER = "mqtt.ics.ele.tue.nl"
USERNAME    = "robot_5_1"
PASSWORD    = "2hHjtag4"
 
# Topics to subscribe (robot → laptop)
RECV_TOPICS = [
    "/pynqbridge/5_1/recv",
    "/pynqbridge/robot_5_1/recv",
    "/pynqbridge/5/recv",
]
 
# Topic to publish commands (laptop → robot)
SEND_TOPIC = "/pynqbridge/robot_5_1/send"
 
# ── TCP server config (connects to C/raylib app) ──────────────────────────────
TCP_HOST = "127.0.0.1"
TCP_PORT = 5555
 
# ── Globals ───────────────────────────────────────────────────────────────────
tcp_conn   = None   # active TCP connection to C client
conn_lock  = threading.Lock()
 
 
# ─────────────────────────────────────────────────────────────────────────────
#  TCP helpers
# ─────────────────────────────────────────────────────────────────────────────
 
def send_to_c(message: str):
    """Send a newline-terminated message to the C raylib client."""
    global tcp_conn
    with conn_lock:
        if tcp_conn is None:
            return
        try:
            tcp_conn.sendall((message.strip() + "\n").encode())
        except OSError:
            print("[TCP] Client disconnected.")
            tcp_conn = None
 
 
def tcp_accept_loop(server_sock: socket.socket):
    """Background thread: wait for the C app to connect (reconnect-friendly)."""
    global tcp_conn
    print(f"[TCP] Waiting for C client on {TCP_HOST}:{TCP_PORT} ...")
    while True:
        try:
            conn, addr = server_sock.accept()
            with conn_lock:
                if tcp_conn:
                    tcp_conn.close()
                tcp_conn = conn
            print(f"[TCP] C client connected from {addr}")
        except OSError:
            break   # server socket was closed
 
 
# ─────────────────────────────────────────────────────────────────────────────
#  MQTT callbacks
# ─────────────────────────────────────────────────────────────────────────────
 
def on_connect(client, userdata, flags, reason_code, properties):
    print(f"[MQTT] Connected: {reason_code}")
    for topic in RECV_TOPICS:
        result, _ = client.subscribe(topic)
        print(f"[MQTT] Subscribed to '{topic}' → {result}")
 
 
def on_message(client, userdata, message):
    """
    Called whenever a message arrives from the robot.
 
    Expected payload formats (sent by the robot firmware):
        ROBOT,<id>,<x>,<y>,<heading_deg>
        OBJECT,<type>,<x>,<y>
        CLIFF,<x>,<y>
 
    Any other text is forwarded raw so you can extend later.
    """
    raw = message.payload.decode(errors="replace").strip()
    print(f"[MQTT] {message.topic} → {raw!r}")
 
    # Forward every robot message straight to the C app
    send_to_c(raw)
 
 
# ─────────────────────────────────────────────────────────────────────────────
#  Main
# ─────────────────────────────────────────────────────────────────────────────
 
def main():
    # ── Start TCP server ──────────────────────────────────────────────────────
    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_sock.bind((TCP_HOST, TCP_PORT))
    server_sock.listen(1)
 
    accept_thread = threading.Thread(target=tcp_accept_loop,
                                     args=(server_sock,), daemon=True)
    accept_thread.start()
 
    # ── Connect to MQTT broker ────────────────────────────────────────────────
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.username_pw_set(USERNAME, PASSWORD)
    client.on_connect = on_connect
    client.on_message = on_message
 
    print(f"[MQTT] Connecting to {MQTT_BROKER} ...")
    client.connect(MQTT_BROKER, 1883, 60)
    client.loop_start()
 
    # ── Command console (laptop → robot) ──────────────────────────────────────
    print("\nType a command and press Enter to send it to the robot.")
    print("Press CTRL+C to quit.\n")
 
    try:
        while True:
            text = input("COMMAND > ").strip()
            if not text:
                continue
            info = client.publish(SEND_TOPIC, text)
            info.wait_for_publish()
            print(f"[MQTT] Published to '{SEND_TOPIC}': {text!r}")
 
    except KeyboardInterrupt:
        print("\n[Main] Shutting down...")
 
    finally:
        client.loop_stop()
        client.disconnect()
        server_sock.close()
        print("[Main] Done.")
 
 
if __name__ == "__main__":
    main()