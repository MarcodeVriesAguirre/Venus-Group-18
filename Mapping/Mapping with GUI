import threading
import paho.mqtt.client as mqtt
import PySimpleGUI as sg

# Connection 1
BROKER1 = "mqtt.ics.ele.tue.nl"
USER1 = "robot_5_1"
PASS1 = "2hHjtag4"
RECV1 = ["/pynqbridge/5/send"]

# Connection 2
BROKER2 = "mqtt.ics.ele.tue.nl"
USER2 = "robot_18_1"
PASS2 = "mv6p0Gzx"
RECV2 = ["/pynqbridge/18/send"]

WIDTH = 50
HEIGHT = 50

game_map = [["." for _ in range(WIDTH)] for _ in range(HEIGHT)]

# Colors for map objects
COLORS = {
    ".": "white",
    "M": "brown",
    "C": "gray",
    "G": "gold",
    "S": "silver",
    "D": "darkgreen",
    "I": "lightblue",
}

window = None


def tile_color(value):
    return COLORS.get(value, "orange")


def set_tile(x, y, value):
    if 0 <= x < WIDTH and 0 <= y < HEIGHT:
        game_map[y][x] = value

        # Update GUI cell
        if window:
            window[f"CELL_{x}_{y}"].update(
                background_color=tile_color(value),
                text=value
            )


def handle_payload(payload):
    parts = payload.decode(errors="replace").split(",")

    try:
        if parts[0] == "Rock":
            x = int(parts[1])
            y = int(parts[2])
            rock_type = parts[3]

            symbol = rock_type[0].upper()
            set_tile(x, y, symbol)

        elif parts[0] == "Mountain":
            x = int(parts[1])
            y = int(parts[2])
            set_tile(x, y, "M")

        elif parts[0] == "Cliff":
            x = int(parts[1])
            y = int(parts[2])
            set_tile(x, y, "C")

    except Exception as e:
        print("Payload error:", e)


def make_on_connect(topics):
    def on_connect(client, userdata, flags, reason_code, properties):
        print("Connected:", client)
        for topic in topics:
            client.subscribe(topic)
            print("Subscribed to", topic)

    return on_connect


def on_message(client, userdata, message):
    print(f"[{message.topic}] {message.payload.decode(errors='replace')}")
    handle_payload(message.payload)


def mqtt_thread():
    # Client 1
    client1 = mqtt.Client(
        mqtt.CallbackAPIVersion.VERSION2,
        client_id="map_client_5"
    )
    client1.username_pw_set(USER1, PASS1)
    client1.on_connect = make_on_connect(RECV1)
    client1.on_message = on_message

    # Client 2
    client2 = mqtt.Client(
        mqtt.CallbackAPIVersion.VERSION2,
        client_id="map_client_18"
    )
    client2.username_pw_set(USER2, PASS2)
    client2.on_connect = make_on_connect(RECV2)
    client2.on_message = on_message

    client1.connect(BROKER1, 1883, 60)
    client2.connect(BROKER2, 1883, 60)

    client1.loop_start()
    client2.loop_start()

    while True:
        pass


# ---------------- GUI ----------------

cell_size = 2

layout = []

for y in range(HEIGHT):
    row = []
    for x in range(WIDTH):
        row.append(
            sg.Text(
                ".",
                size=(cell_size, 1),
                justification="center",
                background_color="white",
                relief="solid",
                key=f"CELL_{x}_{y}"
            )
        )
    layout.append(row)

window = sg.Window(
    "Robot Map",
    layout,
    finalize=True,
    resizable=True
)

threading.Thread(target=mqtt_thread, daemon=True).start()

while True:
    event, values = window.read(timeout=100)

    if event == sg.WIN_CLOSED:
        break

window.close()