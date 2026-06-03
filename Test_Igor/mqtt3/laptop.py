import struct
import paho.mqtt.client as mqtt

HOST = "mqtt.ics.ele.tue.nl"

USER = "robot_18_1"
PASSWORD = "mv6p0Gzx"

TOPIC = "/pynqbridge/robot_18_1/send"

buffer = b""


def unpack_messages(buffer):
    messages = []

    while len(buffer) >= 4:

        msg_len = struct.unpack(">I", buffer[:4])[0]

        if len(buffer) < 4 + msg_len:
            break

        payload = buffer[4:4+msg_len]

        messages.append(payload)

        buffer = buffer[4+msg_len:]

    return messages, buffer


def on_connect(client, userdata, flags, reason_code, properties):

    print("Connected:", reason_code)

    if reason_code != 0:
        print("Connection failed")
        return

    client.subscribe(TOPIC)

    print("Listening on:")
    print(TOPIC)


def on_message(client, userdata, message):

    global buffer

    buffer += message.payload

    messages, buffer = unpack_messages(buffer)

    for payload in messages:

        text = payload.decode(errors="replace")

        print("\n========== MESSAGE ==========")
        print(text)

        data = {}

        fields = text.split(";")

        for field in fields:

            if "=" in field:
                key, value = field.split("=", 1)
                data[key] = value

        print("\nDecoded fields:")

        for key, value in data.items():
            print(f"{key}: {value}")

        print("=============================")


client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

client.username_pw_set(USER, PASSWORD)

client.on_connect = on_connect
client.on_message = on_message

client.connect(HOST,1883,60)

client.loop_forever()