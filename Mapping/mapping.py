import pygame
import paho.mqtt.client as mqtt

# ─────────────────────────────────────────────
# MQTT CONFIG
# ─────────────────────────────────────────────

MQTT_BROKER = "mqtt.ics.ele.tue.nl"
USERNAME = "robot_18_1"
PASSWORD = "mv6p0Gzx"

TOPICS = [
    "/pynqbridge/18_1/recv",
    "/pynqbridge/robot_18_1/recv",
    "/pynqbridge/18/recv",
]

# ─────────────────────────────────────────────
# WORLD STATE (FULL SIMULATION)
# ─────────────────────────────────────────────

robots = {}     # id -> (x,y,h)
objects = []    # (type,x,y)
cliffs = []     # (x,y)

# ─────────────────────────────────────────────
# MQTT CALLBACK
# ─────────────────────────────────────────────

def on_message(client, userdata, msg):
    raw = msg.payload.decode().strip()
    p = raw.split(",")

    if p[0] == "ROBOT":
        robots[int(p[1])] = (float(p[2]), float(p[3]), float(p[4]))

    elif p[0] == "OBJECT":
        objects.append((p[1], float(p[2]), float(p[3])))

    elif p[0] == "CLIFF":
        cliffs.append((float(p[1]), float(p[2])))


# ─────────────────────────────────────────────
# INIT MQTT
# ─────────────────────────────────────────────

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.username_pw_set(USERNAME, PASSWORD)
client.on_message = on_message

client.connect(MQTT_BROKER, 1883, 60)

for t in TOPICS:
    client.subscribe(t)

client.loop_start()

# ─────────────────────────────────────────────
# INIT PYGAME (REPLACES C RAYLIB)
# ─────────────────────────────────────────────

pygame.init()
screen = pygame.display.set_mode((1000, 700))
clock = pygame.time.Clock()

font = pygame.font.SysFont("Arial", 14)

camera_x, camera_y = 0, 0
zoom = 1.0


# ─────────────────────────────────────────────
# DRAW HELPERS
# ─────────────────────────────────────────────

def world_to_screen(x, y):
    return int((x - camera_x) * zoom + 500), int((y - camera_y) * zoom + 350)


def draw_robot(x, y, h, rid):
    sx, sy = world_to_screen(x, y)

    pygame.draw.rect(screen, (200, 50, 50),
                     pygame.Rect(sx-20, sy-20, 40, 40))

    # direction line
    dx = sx + int(20 * pygame.math.Vector2(1, 0).rotate(-h).x)
    dy = sy + int(20 * pygame.math.Vector2(1, 0).rotate(-h).y)

    pygame.draw.circle(screen, (255, 255, 255), (dx, dy), 5)

    label = font.render(f"R{rid}", True, (255,255,255))
    screen.blit(label, (sx-10, sy-35))


def draw_objects():
    for t, x, y in objects:
        sx, sy = world_to_screen(x, y)
        color = (255, 255, 0) if t == "rock" else (200, 0, 200)
        pygame.draw.circle(screen, color, (sx, sy), 6)


def draw_cliffs():
    for x, y in cliffs:
        sx, sy = world_to_screen(x, y)
        pygame.draw.circle(screen, (80, 40, 0), (sx, sy), 5)


# ─────────────────────────────────────────────
# MAIN LOOP (FULL ENGINE)
# ─────────────────────────────────────────────

running = True

while running:
    clock.tick(60)

    # ── INPUT
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    keys = pygame.key.get_pressed()

    if keys[pygame.K_LEFT]: camera_x -= 5
    if keys[pygame.K_RIGHT]: camera_x += 5
    if keys[pygame.K_UP]: camera_y -= 5
    if keys[pygame.K_DOWN]: camera_y += 5
    if keys[pygame.K_q]: zoom *= 1.02
    if keys[pygame.K_e]: zoom *= 0.98

    # ── CLEAR FRAME
    screen.fill((30, 30, 30))

    # ── DRAW WORLD
    draw_cliffs()
    draw_objects()

    for rid, (x, y, h) in robots.items():
        draw_robot(x, y, h, rid)

    # ── UI
    info = font.render(f"Robots: {len(robots)}  Objects: {len(objects)}", True, (255,255,255))
    screen.blit(info, (10, 10))

    pygame.display.flip()

# ─────────────────────────────────────────────

pygame.quit()
client.loop_stop()
client.disconnect()