import time
import paho.mqtt.client as mqtt
from common import pack_message

HOST = "mqtt.ics.ele.tue.nl"
USER = "robot_18_1"
PASSWORD = "mv6p0Gzx"

TOPIC = "/pynqbridge/robot_18_1/send"

payload = (
    "ROBOT=A;"
    "TYPE=ROCK;"
    "COLOR=RED;"
    "SIZE=SMALL;"
    "TEMP=25.4"
).encode()

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.username_pw_set(USER, PASSWORD)

client.connect(HOST, 1883, 60)
client.loop_start()

time.sleep(1)

info = client.publish(TOPIC, pack_message(payload), qos=1)
info.wait_for_publish()

print("Publish rc:", info.rc)
print("Sent to:", TOPIC)

time.sleep(2)

client.loop_stop()
client.disconnect()