#include <stdio.h>
#include "raylib.h"
#include "raygui.h"
#include "sqlite3.h"

int main(void)
{
    const char *sqlite_version = sqlite3_libversion();
    printf("sqlite %s\n", sqlite_version);
    InitWindow(640, 480, "cubing-trainer toolchain spike");
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawText("cubing-trainer toolchain spike", 24, 24, 20, DARKGRAY);
    DrawText(sqlite_version, 24, 56, 20, DARKGRAY);
    GuiLabel((Rectangle){ 24, 96, 220, 32 }, "raygui linked");
    EndDrawing();
    CloseWindow();
    return 0;
}
