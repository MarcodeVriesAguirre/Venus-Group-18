import paho.mqtt.client as mqtt

HOST = "mqtt.ics.ele.tue.nl"
PORT = 1883

USER = "robot_5_1"
PASSWORD = "2hHjtag4"

MODULE = "robot_5_1"

SEND_TOPIC = f"/PYNQBRIDGE/{MODULE}/SEND"

RECV_TOPIC = f"/PYNQBRIDGE/{MODULE}/RECV"


def parse_fields(text):
    data = {}
    fields = text.split(";")
    for field in fields:
        if "=" in field:
            key, value = field.split("=", 1)
            data[key.strip()] = value.strip()
    return data


def on_connect(client, userdata, flags, reason_code, properties):
    print("Connected:", reason_code)

    if reason_code.value != 0:
        print("Connection failed:", reason_code)
        return

    client.subscribe(SEND_TOPIC)
    print("Listening on:", SEND_TOPIC)


def on_message(client, userdata, message):
    print("\n========== MQTT MESSAGE ==========")
    print("Topic  :", message.topic)
    print("Raw    :", message.payload)

    text = message.payload.decode(errors="replace")
    print("Text   :", text)

    data = parse_fields(text)

    if data:
        print("Fields :")
        for key, value in data.items():
            print(f"  {key}: {value}")
    else:
        print("No KEY=VALUE fields found.")

    print("==================================")


client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.username_pw_set(USER, PASSWORD)
client.on_connect = on_connect
client.on_message = on_message

client.connect(HOST, PORT, 60)
client.loop_forever()
