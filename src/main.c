#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "raymath.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 450;
    bool showMessageBox = false;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(
        screenWidth, screenHeight, "raylib [core] example - window letterbox"
    );
    SetWindowMinSize(320, 240);

    int gameScreenWidth = 640;
    int gameScreenHeight = 480;

    RenderTexture2D target =
        LoadRenderTexture(gameScreenWidth, gameScreenHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

    Color colors[10] = {0};
    for (int i = 0; i < 10; i++)
        colors[i] = (Color){
            GetRandomValue(100, 250),
            GetRandomValue(50, 150),
            GetRandomValue(10, 100),
            255
        };

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float scale =
            MIN((float)GetScreenWidth() / gameScreenWidth,
                (float)GetScreenHeight() / gameScreenHeight);

        if (IsKeyPressed(KEY_SPACE)) {
            for (int i = 0; i < 10; i++)
                colors[i] = (Color){
                    GetRandomValue(100, 250),
                    GetRandomValue(50, 150),
                    GetRandomValue(10, 100),
                    255
                };
        }

        Vector2 mouse = GetMousePosition();
        Vector2 virtualMouse = {0};
        virtualMouse.x =
            (mouse.x - (GetScreenWidth() - (gameScreenWidth * scale)) * 0.5f) /
            scale;
        virtualMouse.y =
            (mouse.y -
             (GetScreenHeight() - (gameScreenHeight * scale)) * 0.5f) /
            scale;
        virtualMouse = Vector2Clamp(
            virtualMouse,
            (Vector2){0, 0},
            (Vector2){(float)gameScreenWidth, (float)gameScreenHeight}
        );

        BeginTextureMode(target);
        ClearBackground(RAYWHITE);

        if (GuiButton((Rectangle){24, 24, 120, 30}, "#191#Show Message"))
            showMessageBox = true;

        if (showMessageBox) {
            int btnActive = -1;
            GuiMessageBox(
                (Rectangle){85, 70, 250, 100},
                "#191#Message Box",
                "Hi! This is a message!",
                "Nice;Cool",
                &btnActive
            );

            if (btnActive >= 0)
                showMessageBox = false;
        }
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);

        DrawTexturePro(
            target.texture,
            (Rectangle){
                0.0f,
                0.0f,
                (float)target.texture.width,
                (float)-target.texture.height
            },
            (Rectangle){
                (GetScreenWidth() - ((float)gameScreenWidth * scale)) * 0.5f,
                (GetScreenHeight() - ((float)gameScreenHeight * scale)) * 0.5f,
                (float)gameScreenWidth * scale,
                (float)gameScreenHeight * scale
            },
            (Vector2){0, 0},
            0.0f,
            WHITE
        );
        EndDrawing();
    }

    UnloadRenderTexture(target);

    CloseWindow();

    return 0;
}
