import paho.mqtt.client as mqtt
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import json
import threading

# =====================
# CONFIGURATION
# =====================
MQTT_BROKER = "mqtt.ics.ele.tue.nl"
USERNAME = "robot_41_1"
PASSWORD = "t7gIhbJF"

TOPICS = [
    "/pynqbridge/41/send",  # Robot 1
    "/pynqbridge/42/send",  # Robot 2
]

# =====================
# MAP DATA
# =====================
map_data = {
    "robot1": {"path": [], "pos": None},
    "robot2": {"path": [], "pos": None},
    "cubes": [],
    "obstacles": [],
    "tape": [],
}
map_lock = threading.Lock()

# =====================
# MESSAGE PARSER
# =====================
def parse_message(robot_id, message):
    """
    Parses incoming robot messages and updates the map.
    Easy to extend: just add more elif blocks for new message types.

    Expected formats:
        POS 0.5 0.3 90       -> robot position and heading
        CUBE 1.2 0.5         -> cube found at x y
        OBSTACLE 0.8 1.3     -> obstacle at x y
        TAPE 0.2 0.1         -> black tape at x y
    """
    parts = message.strip().split()
    if len(parts) < 3:
        print(f"[WARN] Could not parse: {message}")
        return

    msg_type = parts[0].upper()
    
    try:
        x, y = float(parts[1]), float(parts[2])
    except ValueError:
        print(f"[WARN] Invalid coordinates in: {message}")
        return

    with map_lock:
        if msg_type == "POS":
            map_data[robot_id]["pos"] = (x, y)
            map_data[robot_id]["path"].append((x, y))

        elif msg_type == "CUBE":
            if (x, y) not in map_data["cubes"]:
                map_data["cubes"].append((x, y))
                print(f"[MAP] Cube found at ({x}, {y}) by {robot_id}")

        elif msg_type == "OBSTACLE":
            if (x, y) not in map_data["obstacles"]:
                map_data["obstacles"].append((x, y))
                print(f"[MAP] Obstacle at ({x}, {y}) by {robot_id}")

        elif msg_type == "TAPE":
            if (x, y) not in map_data["tape"]:
                map_data["tape"].append((x, y))
                print(f"[MAP] Tape at ({x}, {y}) by {robot_id}")

        # ---- ADD NEW MESSAGE TYPES HERE ----
        # elif msg_type == "ROCK":
        #     map_data["rocks"].append((x, y))
        # ------------------------------------

        else:
            print(f"[WARN] Unknown message type: {msg_type}")

# =====================
# MQTT CALLBACKS
# =====================
def on_connect(client, userdata, flags, reason_code, properties):
    print(f"Connected with code {reason_code}")
    for topic in TOPICS:
        client.subscribe(topic)
        print(f"Subscribed to {topic}")

def on_message(client, userdata, msg):
    payload = msg.payload.decode("utf-8").strip()
    print(f"[MQTT] {msg.topic}: {payload}")

    # Map topic to robot id
    if "41" in msg.topic:
        robot_id = "robot1"
    elif "42" in msg.topic:
        robot_id = "robot2"
    else:
        print(f"[WARN] Unknown topic: {msg.topic}")
        return

    parse_message(robot_id, payload)

# =====================
# MQTT SETUP
# =====================
def start_mqtt():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.username_pw_set(USERNAME, PASSWORD)
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(MQTT_BROKER, 1883, 60)
    client.loop_forever()

# =====================
# MAP DISPLAY
# =====================
def draw_map():
    plt.ion()
    fig, ax = plt.subplots(figsize=(8, 8))

    while True:
        ax.clear()
        ax.set_title("Robot Map")
        ax.set_xlabel("X (m)")
        ax.set_ylabel("Y (m)")
        ax.set_aspect("equal")
        ax.grid(True)

        with map_lock:
            # Draw robot 1 path and position
            if map_data["robot1"]["path"]:
                px, py = zip(*map_data["robot1"]["path"])
                ax.plot(px, py, "g-", linewidth=1, alpha=0.5)
            if map_data["robot1"]["pos"]:
                ax.plot(*map_data["robot1"]["pos"], "go", markersize=10, label="Robot 1")

            # Draw robot 2 path and position
            if map_data["robot2"]["path"]:
                px, py = zip(*map_data["robot2"]["path"])
                ax.plot(px, py, "b-", linewidth=1, alpha=0.5)
            if map_data["robot2"]["pos"]:
                ax.plot(*map_data["robot2"]["pos"], "bo", markersize=10, label="Robot 2")

            # Draw cubes
            for x, y in map_data["cubes"]:
                ax.plot(x, y, "bs", markersize=10)
                ax.annotate("cube", (x, y), textcoords="offset points", xytext=(5, 5), fontsize=7)

            # Draw obstacles
            for x, y in map_data["obstacles"]:
                ax.plot(x, y, "r^", markersize=10)
                ax.annotate("obstacle", (x, y), textcoords="offset points", xytext=(5, 5), fontsize=7)

            # Draw tape
            for x, y in map_data["tape"]:
                ax.plot(x, y, "ks", markersize=8)
                ax.annotate("tape", (x, y), textcoords="offset points", xytext=(5, 5), fontsize=7)

        # Legend
        legend_elements = [
            mpatches.Patch(color="green", label="Robot 1"),
            mpatches.Patch(color="blue", label="Robot 2"),
            mpatches.Patch(color="blue", label="Cube"),
            mpatches.Patch(color="red", label="Obstacle"),
            mpatches.Patch(color="black", label="Tape"),
        ]
        ax.legend(handles=legend_elements, loc="upper left")

        plt.pause(0.5)  # Refresh every 0.5 seconds

# =====================
# MAIN
# =====================
if __name__ == "__main__":
    # Start MQTT in background thread
    mqtt_thread = threading.Thread(target=start_mqtt, daemon=True)
    mqtt_thread.start()

    # Start map display on main thread
    draw_map()