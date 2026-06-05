#include <raylib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ── Windows socket includes ───────────────────────────────────────────────────
#ifdef _WIN32
  #include <winsock2.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET SocketHandle;
  #define INVALID_SOCK INVALID_SOCKET
  #define SOCK_ERR     SOCKET_ERROR
#else
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
  typedef int SocketHandle;
  #define INVALID_SOCK (-1)
  #define SOCK_ERR     (-1)
  #define closesocket  close
#endif

// ── Constants ─────────────────────────────────────────────────────────────────
#define MAX_ROBOTS   4
#define MAX_OBJECTS  256
#define MAX_CLIFFS   256
#define TCP_PORT     5555
#define TCP_HOST     "127.0.0.1"

// ── Data structures ───────────────────────────────────────────────────────────

typedef struct {
    int     id;
    Vector2 pos;
    float   heading;   // degrees
    bool    active;
} Robot;

typedef struct {
    char    type[32];  // "rock", "mountain", etc.
    Vector2 pos;
    bool    active;
} MapObject;

typedef struct {
    Vector2 pos;
    bool    active;
} Cliff;

// ── Globals ───────────────────────────────────────────────────────────────────

static Robot     robots[MAX_ROBOTS]   = {0};
static MapObject objects[MAX_OBJECTS] = {0};
static Cliff     cliffs[MAX_CLIFFS]   = {0};

static SocketHandle  sock       = INVALID_SOCK;
static char          recvBuf[4096];
static int           recvLen    = 0;   // bytes currently in recvBuf

// ── Colours per robot id ──────────────────────────────────────────────────────
static Color robotColors[] = { RED, BLUE, GREEN, ORANGE };

// ─────────────────────────────────────────────────────────────────────────────
//  Socket helpers
// ─────────────────────────────────────────────────────────────────────────────

static void socket_init(void)
{
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
}

static void socket_cleanup(void)
{
    if (sock != INVALID_SOCK) closesocket(sock);
#ifdef _WIN32
    WSACleanup();
#endif
}

/* Try to connect (or reconnect) to the Python bridge. Non-blocking after connect. */
static bool try_connect(void)
{
    if (sock != INVALID_SOCK) return true;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCK) return false;

    struct sockaddr_in server = {0};
    server.sin_family      = AF_INET;
    server.sin_port        = htons(TCP_PORT);
    server.sin_addr.s_addr = inet_addr(TCP_HOST);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == SOCK_ERR) {
        closesocket(sock);
        sock = INVALID_SOCK;
        return false;
    }

    // Set non-blocking so recv() never stalls the game loop
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif

    TraceLog(LOG_INFO, "TCP: connected to Python bridge.");
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Message parser
//  Formats (newline-terminated):
//    ROBOT,<id>,<x>,<y>,<heading>
//    OBJECT,<type>,<x>,<y>
//    CLIFF,<x>,<y>
// ─────────────────────────────────────────────────────────────────────────────

static void handle_message(const char* msg)
{
    // ── ROBOT ─────────────────────────────────────────────────────────────────
    if (strncmp(msg, "ROBOT,", 6) == 0) {
        int   id;
        float x, y, heading;
        if (sscanf(msg, "ROBOT,%d,%f,%f,%f", &id, &x, &y, &heading) == 4) {
            if (id >= 0 && id < MAX_ROBOTS) {
                robots[id].id      = id;
                robots[id].pos     = (Vector2){ x, y };
                robots[id].heading = heading;
                robots[id].active  = true;
            }
        }
        return;
    }

    // ── OBJECT ───────────────────────────────────────────────────────────────
    if (strncmp(msg, "OBJECT,", 7) == 0) {
        char  type[32];
        float x, y;
        if (sscanf(msg, "OBJECT,%31[^,],%f,%f", type, &x, &y) == 3) {
            for (int i = 0; i < MAX_OBJECTS; i++) {
                if (!objects[i].active) {
                    strncpy(objects[i].type, type, 31);
                    objects[i].pos    = (Vector2){ x, y };
                    objects[i].active = true;
                    break;
                }
            }
        }
        return;
    }

    // ── CLIFF ────────────────────────────────────────────────────────────────
    if (strncmp(msg, "CLIFF,", 6) == 0) {
        float x, y;
        if (sscanf(msg, "CLIFF,%f,%f", &x, &y) == 2) {
            for (int i = 0; i < MAX_CLIFFS; i++) {
                if (!cliffs[i].active) {
                    cliffs[i].pos    = (Vector2){ x, y };
                    cliffs[i].active = true;
                    break;
                }
            }
        }
        return;
    }

    TraceLog(LOG_WARNING, "TCP: unknown message: %s", msg);
}

/* Drain the TCP socket and dispatch complete newline-terminated messages. */
static void poll_socket(void)
{
    if (sock == INVALID_SOCK) {
        // Try to reconnect every ~60 frames (not every frame)
        static int reconnectTimer = 0;
        if (++reconnectTimer > 60) { reconnectTimer = 0; try_connect(); }
        return;
    }

    char tmp[1024];
    int  n = recv(sock, tmp, sizeof(tmp) - 1, 0);

    if (n > 0) {
        // Append to buffer
        if (recvLen + n < (int)sizeof(recvBuf)) {
            memcpy(recvBuf + recvLen, tmp, n);
            recvLen += n;
            recvBuf[recvLen] = '\0';
        }

        // Process all complete lines
        char* start = recvBuf;
        char* nl;
        while ((nl = strchr(start, '\n')) != NULL) {
            *nl = '\0';
            if (nl > start) handle_message(start);
            start = nl + 1;
        }

        // Move leftover bytes to front of buffer
        int remaining = (int)(recvBuf + recvLen - start);
        memmove(recvBuf, start, remaining);
        recvLen = remaining;

    } else if (n == 0) {
        // Connection closed by Python
        TraceLog(LOG_WARNING, "TCP: Python bridge disconnected.");
        closesocket(sock);
        sock = INVALID_SOCK;
    }
    // n < 0 → EWOULDBLOCK / EAGAIN → nothing to read, that's fine
}

// ─────────────────────────────────────────────────────────────────────────────
//  Drawing helpers
// ─────────────────────────────────────────────────────────────────────────────

static void draw_robot(const Robot* r)
{
    Color col = robotColors[r->id % 4];

    // Body
    DrawRectanglePro(
        (Rectangle){ r->pos.x, r->pos.y, 60, 60 },
        (Vector2){ 30, 30 },
        r->heading,
        col
    );

    // Direction dot
    DrawCircle(
        (int)(r->pos.x + cosf(r->heading * DEG2RAD) * 36),
        (int)(r->pos.y + sinf(r->heading * DEG2RAD) * 36),
        8, WHITE
    );

    // ID label
    char label[8];
    snprintf(label, sizeof(label), "R%d", r->id);
    DrawText(label, (int)r->pos.x - 8, (int)r->pos.y - 36, 16, col);
}

static void draw_objects(void)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (!objects[i].active) continue;
        // Rock = yellow circle, everything else = magenta
        Color col = (strcmp(objects[i].type, "rock") == 0) ? YELLOW : MAGENTA;
        DrawCircle((int)objects[i].pos.x, (int)objects[i].pos.y, 8, col);
        DrawText(objects[i].type,
                 (int)objects[i].pos.x + 10,
                 (int)objects[i].pos.y - 8,
                 12, col);
    }
}

static void draw_cliffs(void)
{
    for (int i = 0; i < MAX_CLIFFS; i++) {
        if (!cliffs[i].active) continue;
        DrawCircle((int)cliffs[i].pos.x, (int)cliffs[i].pos.y, 5, DARKBROWN);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Manual control (for local testing without a robot)
// ─────────────────────────────────────────────────────────────────────────────

static void update_manual(void)
{
    const float speed    = 3.0f;
    const float rotSpeed = 3.0f;

    // Robot 0 – arrow keys
    if (!robots[0].active) { robots[0].active = true; robots[0].pos = (Vector2){400,400}; }
    if (IsKeyDown(KEY_RIGHT)) robots[0].heading += rotSpeed;
    if (IsKeyDown(KEY_LEFT))  robots[0].heading -= rotSpeed;
    if (IsKeyDown(KEY_UP)) {
        robots[0].pos.x += cosf(robots[0].heading * DEG2RAD) * speed;
        robots[0].pos.y += sinf(robots[0].heading * DEG2RAD) * speed;
    }
    if (IsKeyDown(KEY_DOWN)) {
        robots[0].pos.x -= cosf(robots[0].heading * DEG2RAD) * speed;
        robots[0].pos.y -= sinf(robots[0].heading * DEG2RAD) * speed;
    }

    // Robot 1 – WASD
    if (!robots[1].active) { robots[1].active = true; robots[1].pos = (Vector2){450,450}; }
    if (IsKeyDown(KEY_D)) robots[1].heading += rotSpeed;
    if (IsKeyDown(KEY_A)) robots[1].heading -= rotSpeed;
    if (IsKeyDown(KEY_W)) {
        robots[1].pos.x += cosf(robots[1].heading * DEG2RAD) * speed;
        robots[1].pos.y += sinf(robots[1].heading * DEG2RAD) * speed;
    }
    if (IsKeyDown(KEY_S)) {
        robots[1].pos.x -= cosf(robots[1].heading * DEG2RAD) * speed;
        robots[1].pos.y -= sinf(robots[1].heading * DEG2RAD) * speed;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Main
// ─────────────────────────────────────────────────────────────────────────────

int main(void)
{
    socket_init();
    try_connect();   // attempt first connection (won't block if Python isn't up yet)

    InitWindow(800, 450, "Robot Map Viewer");
    SetWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
    MaximizeWindow();
    SetTargetFPS(60);

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    Camera2D camera = {0};
    camera.offset   = (Vector2){ sw / 2.0f, sh / 2.0f };
    camera.target   = (Vector2){ sw / 2.0f, sh / 2.0f };
    camera.zoom     = 1.0f;

    while (!WindowShouldClose())
    {
        // ── Input & networking ────────────────────────────────────────────────
        poll_socket();
        update_manual();    // local keyboard control still works when no TCP data

        // Camera pan with middle-mouse or IJKL
        if (IsKeyDown(KEY_L)) camera.target.x += 5;
        if (IsKeyDown(KEY_J)) camera.target.x -= 5;
        if (IsKeyDown(KEY_I)) camera.target.y -= 5;
        if (IsKeyDown(KEY_K)) camera.target.y += 5;
        camera.zoom += GetMouseWheelMove() * 0.05f;
        if (camera.zoom < 0.1f) camera.zoom = 0.1f;

        // ── Draw ──────────────────────────────────────────────────────────────
        BeginDrawing();
            ClearBackground(DARKGRAY);
            BeginMode2D(camera);

                // Grid
                for (int x = 0; x < sw * 4; x += 32)
                    DrawLine(x, 0, x, sh * 4, (Color){80,80,80,255});
                for (int y = 0; y < sh * 4; y += 32)
                    DrawLine(0, y, sw * 4, y, (Color){80,80,80,255});

                draw_cliffs();
                draw_objects();
                for (int i = 0; i < MAX_ROBOTS; i++)
                    if (robots[i].active) draw_robot(&robots[i]);

            EndMode2D();

            // HUD
            bool connected = (sock != INVALID_SOCK);
            DrawText(connected ? "TCP: connected" : "TCP: waiting for Python...",
                     10, 10, 18, connected ? GREEN : RED);
            DrawText("P1: Arrows  P2: WASD  Zoom: scroll  Pan: IJKL",
                     10, 35, 16, LIGHTGRAY);

        EndDrawing();
    }

    socket_cleanup();
    CloseWindow();
    return 0;
}
