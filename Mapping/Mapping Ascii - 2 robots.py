import paho.mqtt.client as mqtt

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


def set_tile(x, y, value):
    if 0 <= x < WIDTH and 0 <= y < HEIGHT:
        game_map[y][x] = value


def draw_map():
    for row in game_map:
        print(" ".join(row))
    print()


def handle_payload(payload):
    parts = payload.decode(errors="replace").split(",")

    if parts[0] == "Rock":
        x = int(parts[1])
        y = int(parts[2])
        rock_type = parts[3]
        set_tile(x, y, rock_type[0])

    elif parts[0] == "Mountain":
        x = int(parts[1])
        y = int(parts[2])
        set_tile(x, y, "M")

    elif parts[0] == "Cliff":
        x = int(parts[1])
        y = int(parts[2])
        set_tile(x, y, "C")

    draw_map()


def make_on_connect(topics):
    def on_connect(client, userdata, flags, reason_code, properties):
        for topic in topics:
            client.subscribe(topic)
            print(f"Subscribed to {topic}")
    return on_connect


def on_message(client, userdata, message):
    print(f"[{message.topic}] {message.payload.decode(errors='replace')}")
    handle_payload(message.payload)


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

# Connect both
client1.connect(BROKER1, 1883, 60)
client2.connect(BROKER2, 1883, 60)

# Start both loops
client1.loop_start()
client2.loop_start()

print("Listening on both MQTT channels...")

try:
    while True:
        pass
except KeyboardInterrupt:
    client1.loop_stop()
    client2.loop_stop()
    client1.disconnect()
    client2.disconnect()