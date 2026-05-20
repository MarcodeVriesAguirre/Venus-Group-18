import struct
import time
import paho.mqtt.client as mqtt  #stuff needed for mqtt

TOPIC = "venus/test"

def on_connect(client, userdata, flags, reason_code, properties):
    print("Connected with reason:", reason_code)  #if connected
    payload = bytes([0b10101010, 0b11110000])  #example payload
    message = struct.pack(">I", len(payload)) + payload
    client.publish(TOPIC, message)
    print("Message sent")
    client.disconnect()
# ^ stuff here send the message and says if it has been sent correctly

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)  # Creates the MQTT client object.
client.on_connect = on_connect  #when client is on, run the on connect funct

client.connect("broker.hivemq.com", 1883, 60)  # Connects to the public MQTT broker.
client.loop_forever()