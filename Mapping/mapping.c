#include <raylib.h>
#include <math.h>

#define DEG2RAD (3.14159265358979323846f / 180.0f)
#define MAX_BULLETS 200

typedef struct {
    Vector2 pos;
    bool active;
} Bullet;

int main(void)
{
    InitWindow(800, 450, "2D Movement");
    SetWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
    MaximizeWindow();
    SetTargetFPS(60);

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    const int tileSize = 32;
    const float speed = 3.0f;
    const float rotSpeed = 3.0f;

    Vector2 pos1 = { 400, 225 };
    Vector2 pos2 = { 450, 225 };
    float rot1 = 0.0f;
    float rot2 = 0.0f;

    Bullet bullets[MAX_BULLETS] = { 0 };

    Camera2D camera = { 0 };
    camera.target = (Vector2){ screenWidth / 2.0f, screenHeight / 2.0f };
    camera.offset = (Vector2){ screenWidth / 2.0f, screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    while (!WindowShouldClose())
    {
        // Player 1 - Left/Right to rotate, Up/Down to move
        if (IsKeyDown(KEY_RIGHT)) rot1 += rotSpeed;
        if (IsKeyDown(KEY_LEFT))  rot1 -= rotSpeed;
        if (IsKeyDown(KEY_UP))
        {
            pos1.x += cosf(rot1 * DEG2RAD) * speed;
            pos1.y += sinf(rot1 * DEG2RAD) * speed;
        }
        if (IsKeyDown(KEY_DOWN))
        {
            pos1.x -= cosf(rot1 * DEG2RAD) * speed;
            pos1.y -= sinf(rot1 * DEG2RAD) * speed;
        }

        // Player 2 - A/D to rotate, W/S to move
        if (IsKeyDown(KEY_D)) rot2 += rotSpeed;
        if (IsKeyDown(KEY_A)) rot2 -= rotSpeed;
        if (IsKeyDown(KEY_W))
        {
            pos2.x += cosf(rot2 * DEG2RAD) * speed;
            pos2.y += sinf(rot2 * DEG2RAD) * speed;
        }
        if (IsKeyDown(KEY_S))
        {
            pos2.x -= cosf(rot2 * DEG2RAD) * speed;
            pos2.y -= sinf(rot2 * DEG2RAD) * speed;
        }

        // Spawn circle - Player 1 (SPACE)
        if (IsKeyPressed(KEY_SPACE))
        {
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (!bullets[i].active)
                {
                    bullets[i].pos = (Vector2){
                        pos1.x + cosf(rot1 * DEG2RAD) * 50,
                        pos1.y + sinf(rot1 * DEG2RAD) * 50
                    };
                    bullets[i].active = true;
                    break;
                }
            }
        }

        // Spawn circle - Player 2 (ENTER)
        if (IsKeyPressed(KEY_ENTER))
        {
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (!bullets[i].active)
                {
                    bullets[i].pos = (Vector2){
                        pos2.x + cosf(rot2 * DEG2RAD) * 50,
                        pos2.y + sinf(rot2 * DEG2RAD) * 50
                    };
                    bullets[i].active = true;
                    break;
                }
            }
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);
            BeginMode2D(camera);

                // Background
                ClearBackground(GRAY);

                // Draw stationary circles
                for (int i = 0; i < MAX_BULLETS; i++)
                    if (bullets[i].active)
                        DrawCircle((int)bullets[i].pos.x, (int)bullets[i].pos.y, 5, BLACK);

                // Player 1 (Red)
                DrawRectanglePro(
                    (Rectangle){ pos1.x, pos1.y, 60, 60 },
                    (Vector2){ 30, 30 },
                    rot1,
                    RED
                );
                DrawCircle(
                    (int)(pos1.x + cosf(rot1 * DEG2RAD) * 36),
                    (int)(pos1.y + sinf(rot1 * DEG2RAD) * 36),
                    8, WHITE
                );

                // Player 2 (Blue)
                DrawRectanglePro(
                    (Rectangle){ pos2.x, pos2.y, 60, 60 },
                    (Vector2){ 30, 30 },
                    rot2,
                    BLUE
                );
                DrawCircle(
                    (int)(pos2.x + cosf(rot2 * DEG2RAD) * 36),
                    (int)(pos2.y + sinf(rot2 * DEG2RAD) * 36),
                    8, WHITE
                );

            EndMode2D();

            DrawText("P1: Left/Right=Rotate  Up/Down=Move  SPACE=Place", 20, 20, 20, RED);
            DrawText("P2: A/D=Rotate  W/S=Move  ENTER=Place", 20, 50, 20, BLUE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}