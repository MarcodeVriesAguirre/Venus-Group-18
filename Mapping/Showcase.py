WIDTH = 20
HEIGHT = 20

game_map = [["." for _ in range(WIDTH)] for _ in range(HEIGHT)]

# Border of cliffs
for x in range(WIDTH):
    game_map[0][x] = "C"
    game_map[HEIGHT - 1][x] = "C"

for y in range(HEIGHT):
    game_map[y][0] = "C"
    game_map[y][WIDTH - 1] = "C"

# Mountain square in the center (6x6)
for y in range(7, 11):
    for x in range(7, 11):
        game_map[y][x] = "M"

for y in range(12, 16):
    for x in range(12, 15):
        game_map[y][x] = "C"

# One of each resource/entity
game_map[2][6] = "r"
game_map[6][3] = "g"
game_map[3][16] = "b"
game_map[14][17] = "w"
game_map[8][14] = "e"

game_map[15][2] = "R"
game_map[17][5] = "G"
game_map[12][8] = "B"


def set_tile(x, y, value):
    if 0 <= x < WIDTH and 0 <= y < HEIGHT:
        game_map[y][x] = value


def draw_map():
    for row in game_map:
        print(" ".join(row))
    print()


draw_map()