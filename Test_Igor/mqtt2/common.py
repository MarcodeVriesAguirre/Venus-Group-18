import struct

def pack_message(payload: bytes) -> bytes:
    return struct.pack(">I", len(payload)) + payload

def unpack_messages(buffer: bytes):
    messages = []

    while len(buffer) >= 4:
        msg_len = struct.unpack(">I", buffer[:4])[0]

        if len(buffer) < 4 + msg_len:
            break

        payload = buffer[4:4 + msg_len]
        messages.append(payload)
        buffer = buffer[4 + msg_len:]

    return messages, buffer