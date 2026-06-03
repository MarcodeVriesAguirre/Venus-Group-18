import paho.mqtt.client as mqtt
from common import unpack_messages

HOST = "mqtt.ics.ele.tue.nl"
USER = "robot_18_1"
PASSWORD = "mv6p0Gzx"

TOPIC = "/pynqbridge/robot_18_1/send"

def on_connect(client, userdata, flags, reason_code, properties):
    print("Connected:", reason_code)

    if reason_code != 0:
        print("Connection failed")
        return

    result, mid = client.subscribe(TOPIC)
    print("Subscribe result:", result, "topic:", TOPIC)

def on_subscribe(client, userdata, mid, reason_codes, properties):
    print("Subscription confirmed:", reason_codes)


buffer = b""

def on_message(client, userdata, message):
    global buffer

    buffer += message.payload

    messages, buffer = unpack_messages(buffer)

    for payload in messages:
        text = payload.decode()

        print("\nDecoded message:")
        print(text)

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.username_pw_set(USER, PASSWORD)

client.on_connect = on_connect
client.on_subscribe = on_subscribe
client.on_message = on_message

client.connect(HOST, 1883, 60)
client.loop_forever()