import struct
import paho.mqtt.client as mqtt

TOPIC = "venus/test"


class MessageDecoder:
    def __init__(self):
        self.buffer = b""

    def feed(self, data):  # Add new incoming data to buffer
        self.buffer += data

        while True:  
            if len(self.buffer) < 4:
                return # Need at least 4 bytes for header

            msg_len = struct.unpack(">I", self.buffer[:4])[0] 
            # ^ Read first 4 bytes as unsigned 32-bit integer
            print("Payload length:", msg_len)

            if len(self.buffer) < 4 + msg_len:
                return
            # ^ Wait until full payload arrives

            payload = self.buffer[4:4 + msg_len]
            # Extract payload

            print("Payload bytes:", payload)

            self.buffer = self.buffer[4 + msg_len:]
            # Remove processed message from buffer

            self.decode_payload(payload)
            # Decode payload

    def decode_payload(self, payload):
        bits = "".join(f"{byte:08b}" for byte in payload)
        # Convert bytes into readable bits

        print("Received bits:", bits)


decoder = MessageDecoder()


def on_connect(client, userdata, flags, reason_code, properties):
    print("Connected with reason:", reason_code)

    client.subscribe(TOPIC)

    print("Subscribed to:", TOPIC)


def on_message(client, userdata, message):
    print("\nMQTT message received")

    print("Raw message bytes:", message.payload)

    decoder.feed(message.payload)


client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

client.on_connect = on_connect
client.on_message = on_message

client.connect("broker.hivemq.com", 1883, 60)

print("Waiting for messages...")

client.loop_forever()