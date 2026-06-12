import paho.mqtt.client as mqtt
import time

MQTT_BROKER = "mqtt.ics.ele.tue.nl"
USERNAME = "robot_18_1"
PASSWORD = "mv6p0Gzx"

SEND_TOPICS = [
  
    "/pynqbridge/robot_18_1/send",
    "/pynqbridge/18/send",
    "/pynqbridge/18_1/send",
    "/PYNQBRIDGE/18_1/SEND",
    "/PYNQBRIDGE/robot_18_1/SEND",
    "/PYNQBRIDGE/18/SEND",
]

RECV_TOPICS = [
    "/pynqbridge/18_1/recv",
    "/pynqbridge/robot_18_1/recv",
    "/pynqbridge/18/recv",
    "/PYNQBRIDGE/18_1/RECV",
    "/PYNQBRIDGE/robot_18_1/RECV",
    "/PYNQBRIDGE/18/RECV",
]


def on_connect(client, userdata, flags, reason_code, properties):
    print("Connected:", reason_code)

    for topic in SEND_TOPICS:
        result, mid = client.subscribe(topic)
        print("Subscribing to:", topic, "result:", result)


def on_message(client, userdata, message):
    print("\n========== MESSAGE FROM MQTT ==========")
    print("Topic  :", message.topic)
    print("Payload:", message.payload.decode(errors="replace"))
    print("=======================================")


client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.username_pw_set(USERNAME, PASSWORD)

client.on_connect = on_connect
client.on_message = on_message

print(f"Connecting to {MQTT_BROKER}...")
client.connect(MQTT_BROKER, 1883, 60)

client.loop_start()
time.sleep(2)

print("\nListening for PYNQ messages.")
print("Type a message and press Enter to send it to the robot.")
print("Press CTRL+C to stop.\n")

try:
    while True:
        text = input("ENTER COMMAND: ")

        for topic in RECV_TOPICS:
            info = client.publish(topic, text)
            info.wait_for_publish()
            print("Published to:", topic)

except KeyboardInterrupt:
    print("\nStopping...")

client.loop_stop()
client.disconnect()
