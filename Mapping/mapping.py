import paho.mqtt.client as mqtt
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np
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
    "cubes":     [],  # {"x","y","color","size"}
    "obstacles": [],  # {"x","y","w","h"}
    "tape":      [],  # {"x","y","len","angle"}
}
map_lock = threading.Lock()

# =====================
# MESSAGE PARSER
# =====================
def parse_message(robot_id, message):
    """
    Parses incoming robot messages and updates the map.

    Expected formats:
        POS 0.5 0.3          -> robot position (meters)
        CUBE 1.2 0.5 red 0.15        -> x y color size(m)
        OBSTACLE 0.8 1.3 0.25 0.20   -> x y width(m) height(m)
        TAPE 0.2 0.1 0.4 1.57        -> x y length(m) angle(rad)
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
            color = parts[3] if len(parts) > 3 else "orange"
            size  = float(parts[4]) if len(parts) > 4 else 0.12
            if not any(abs(c["x"]-x)<0.05 and abs(c["y"]-y)<0.05 for c in map_data["cubes"]):
                map_data["cubes"].append({"x": x, "y": y, "color": color, "size": size})
                print(f"[MAP] Cube ({color}, {size}m) at ({x}, {y}) by {robot_id}")

        elif msg_type == "OBSTACLE":
            w = float(parts[3]) if len(parts) > 3 else 0.20
            h = float(parts[4]) if len(parts) > 4 else 0.20
            if not any(abs(o["x"]-x)<0.05 and abs(o["y"]-y)<0.05 for o in map_data["obstacles"]):
                map_data["obstacles"].append({"x": x, "y": y, "w": w, "h": h})
                print(f"[MAP] Obstacle {w}x{h}m at ({x}, {y}) by {robot_id}")

        elif msg_type == "TAPE":
            length = float(parts[3]) if len(parts) > 3 else 0.30
            angle  = float(parts[4]) if len(parts) > 4 else 0.0
            if not any(abs(t["x"]-x)<0.05 and abs(t["y"]-y)<0.05 for t in map_data["tape"]):
                map_data["tape"].append({"x": x, "y": y, "len": length, "angle": angle})
                print(f"[MAP] Tape len={length}m angle={angle}rad at ({x}, {y}) by {robot_id}")

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

            # --- Robot paths and positions ---
            if map_data["robot1"]["path"]:
                px, py = zip(*map_data["robot1"]["path"])
                ax.plot(px, py, "g-", linewidth=1, alpha=0.5)
            if map_data["robot1"]["pos"]:
                ax.plot(*map_data["robot1"]["pos"], "go", markersize=10, label="Robot 1")

            if map_data["robot2"]["path"]:
                px, py = zip(*map_data["robot2"]["path"])
                ax.plot(px, py, "b-", linewidth=1, alpha=0.5)
            if map_data["robot2"]["pos"]:
                ax.plot(*map_data["robot2"]["pos"], "bo", markersize=10, label="Robot 2")

            # --- Cubes: real size and color from message ---
            for c in map_data["cubes"]:
                half = c["size"] / 2
                rect = mpatches.Rectangle(
                    (c["x"] - half, c["y"] - half),
                    c["size"], c["size"],
                    linewidth=1.5,
                    edgecolor="black",
                    facecolor=c["color"],
                    zorder=3
                )
                ax.add_patch(rect)
                ax.annotate(
                    f'{c["color"]} {int(c["size"]*100)}cm',
                    (c["x"], c["y"] + half),
                    textcoords="offset points",
                    xytext=(0, 4),
                    fontsize=7,
                    ha="center"
                )

            # --- Obstacles: real width x height from message ---
            for o in map_data["obstacles"]:
                rect = mpatches.Rectangle(
                    (o["x"] - o["w"]/2, o["y"] - o["h"]/2),
                    o["w"], o["h"],
                    linewidth=1.5,
                    edgecolor="#7b241c",
                    facecolor="#c0392b",
                    alpha=0.75,
                    zorder=3
                )
                ax.add_patch(rect)
                ax.annotate(
                    f'{o["w"]}x{o["h"]}m',
                    (o["x"], o["y"] + o["h"]/2),
                    textcoords="offset points",
                    xytext=(0, 4),
                    fontsize=7,
                    ha="center"
                )

            # --- Tape: real length and angle from message ---
            for t in map_data["tape"]:
                half = t["len"] / 2
                dx = np.cos(t["angle"]) * half
                dy = np.sin(t["angle"]) * half
                ax.plot(
                    [t["x"] - dx, t["x"] + dx],
                    [t["y"] - dy, t["y"] + dy],
                    color="black",
                    linewidth=4,
                    solid_capstyle="round",
                    zorder=3
                )
                ax.annotate(
                    f'tape {t["len"]}m',
                    (t["x"], t["y"]),
                    textcoords="offset points",
                    xytext=(5, 5),
                    fontsize=7
                )

        # Legend
        legend_elements = [
            mpatches.Patch(color="green",  label="Robot 1"),
            mpatches.Patch(color="blue",   label="Robot 2"),
            mpatches.Patch(color="orange", label="Cube (color from msg)"),
            mpatches.Patch(color="#c0392b",label="Obstacle"),
            mpatches.Patch(color="black",  label="Tape"),
        ]
        ax.legend(handles=legend_elements, loc="upper left")

        plt.pause(0.5)

# =====================
# MAIN
# =====================
if __name__ == "__main__":
    mqtt_thread = threading.Thread(target=start_mqtt, daemon=True)
    mqtt_thread.start()

    draw_map()