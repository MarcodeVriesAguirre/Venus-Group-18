import struct  

class MessageDecoder:
    def __init__(self):
        self.buffer = b""

    def feed(self, data):
        self.buffer += data

        while True:
            if len(self.buffer) < 4:
                return
            # Need 4 bytes for header

            msg_len = struct.unpack(">I", self.buffer[:4])[0]
            # Read 32-bit length

            if len(self.buffer) < 4 + msg_len:
                return
            # Wait for full payload

            payload = self.buffer[4:4 + msg_len]
            # Extract payload

            self.buffer = self.buffer[4 + msg_len:]
            # Remove processed message

            self.decode_payload(payload)
            # Decode payload

    def decode_payload(self, payload):
        bits = "".join(f"{byte:08b}" for byte in payload)
        print("Received bits:", bits)


decoder = MessageDecoder()

payload = bytes([0b11110011, 0b000001+11])
# Example payload

message = struct.pack(">I", len(payload)) + payload
# [4-byte length][payload]

print("Raw message:", message)

decoder.feed(message)