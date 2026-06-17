import paho.mqtt.client as mqtt

BROKER = "mqtt.ics.ele.tue.nl"
USER = "robot_5_1"
PASS = "2hHjtag4"

RECV = ["/pynqbridge/5/send"]

def on_connect(client, userdata, flags, reason_code, properties):
    for topic in RECV:
        client.subscribe(topic)

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

def on_message(client, userdata, message):
    parts = message.payload.decode(errors="replace").split(",")

    if parts[0] == "Rock":
        x = int(parts[1])
        y = int(parts[2])
        rock_type = parts[3]

        if 0 <= x < WIDTH and 0 <= y < HEIGHT:
            set_tile(x, y, rock_type[0])

        draw_map()

    if parts[0] == "Mountain":
        x = int(parts[1])
        y = int(parts[2])

        if 0 <= x < WIDTH and 0 <= y < HEIGHT:
            set_tile(x, y, "M")

        draw_map()

    if parts[0] == "Cliff":
        x = int(parts[1])
        y = int(parts[2])

        if 0 <= x < WIDTH and 0 <= y < HEIGHT:
            set_tile(x, y, "C")

        draw_map()



client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.username_pw_set(USER, PASS)

client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, 1883, 60)
client.loop_forever()