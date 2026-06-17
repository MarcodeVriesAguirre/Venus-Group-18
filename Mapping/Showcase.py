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


def handle_payload(payload):
    parts = payload.split(",")

    if parts[0] == "Rock":
        x = int(parts[1]) #owo
        y = int(parts[2])
        rock_type = parts[3]
        set_tile(x, y, rock_type[0])

    elif parts[0] == "Mountain":
        x = int(parts[1])
        y = int(parts[2])
        set_tile(x, y, "M")

    elif parts[0] == "Cliff":
        x = int(parts[1])
        y = int(parts[2])
        set_tile(x, y, "C")

    draw_map()


print("Enter commands:")
print("  Rock,x,y,type")
print("  Mountain,x,y")
print("  Cliff,x,y")
print("  quit")

while True:
    command = input("> ").strip()

    if command.lower() == "quit":
        break

    try:
        handle_payload(command)
    except Exception as e:
        print("Invalid command:", e)