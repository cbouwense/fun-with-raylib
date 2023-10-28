#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <complex.h>
#include <math.h>

#include <raylib.h>

#ifndef _WIN32
#include <signal.h> // needed for sigaction()
#endif // _WIN32

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOB_IMPLEMENTATION
#include "nob.h"

#include <raylib.h>
#include <rlgl.h>

#define GLSL_VERSION 330

#define N (1<<13)
#define FONT_SIZE 64

#define RENDER_FPS 30
#define RENDER_FACTOR 100
#define RENDER_WIDTH (16*RENDER_FACTOR)
#define RENDER_HEIGHT (9*RENDER_FACTOR)

#define COLOR_ACCENT                  ColorFromHSV(225, 0.75, 0.8)
#define COLOR_BACKGROUND              GetColor(0x151515FF)
#define COLOR_TRACK_PANEL_BACKGROUND  ColorBrightness(COLOR_BACKGROUND, -0.1)
#define COLOR_TRACK_BUTTON_BACKGROUND ColorBrightness(COLOR_BACKGROUND, 0.15)
#define COLOR_TRACK_BUTTON_HOVEROVER  ColorBrightness(COLOR_TRACK_BUTTON_BACKGROUND, 0.15)
#define COLOR_TRACK_BUTTON_SELECTED   COLOR_ACCENT
#define COLOR_TIMELINE_CURSOR         COLOR_ACCENT
#define COLOR_TIMELINE_BACKGROUND     ColorBrightness(COLOR_BACKGROUND, -0.3)
#define COLOR_HUD_BUTTON_BACKGROUND   COLOR_TRACK_BUTTON_BACKGROUND
#define COLOR_HUD_BUTTON_HOVEROVER    COLOR_TRACK_BUTTON_HOVEROVER

#define HUD_TIMER_SECS    1.0f
#define HUD_BUTTON_SIZE   50
#define HUD_BUTTON_MARGIN 50
#define HUD_ICON_SCALE    0.5

typedef struct {
    int id;
    Color color;
    Vector2 pos;
} LandPlot;

typedef struct {
    Font font;
    bool debug_mode;

    int LandPlot_count;
    LandPlot *landplots;
} Schmungo;

//--------------------------------------------------------------------------------------------------
// CSS-like helpers
//--------------------------------------------------------------------------------------------------

float sch_vh(float vh) {
    return GetRenderHeight() * vh * 0.01;
}

float sch_vw(float vw) {
    return GetRenderWidth() * vw * 0.01;
}

float sch_x_center(float width) {
    return GetRenderWidth() / 2 - width / 2;
}

float sch_y_center(float height) {
    return GetRenderHeight() / 2 - height / 2;
}

float sch_top(float height) {
    return height;
}

//--------------------------------------------------------------------------------------------------
// Components
//--------------------------------------------------------------------------------------------------

void sch_DrawLandPlotGround(const Vector2 pos, const Vector2 size)
{
    Color color = GREEN;

    DrawEllipse(
        pos.x + size.x/2,
        pos.y + size.y,
        size.x,
        size.x/3,
        color
    );
}

void sch_DrawLandPlot(const LandPlot *state)
{
    Vector2 size = { sch_vw(5), sch_vw(5) };

    sch_DrawLandPlotGround(state->pos, size);
    DrawRectangleV(state->pos, size, state->color);
}

void sch_DrawLandPlots(const Schmungo *state)
{
    for (int i = 0; i < state->LandPlot_count; i++) {
        sch_DrawLandPlot(&state->landplots[i]);
    }
}

void sch_DrawGameState(const Schmungo *state)
{
    if (!state->debug_mode) return;

    // Draw state of all landplots
    DrawText(TextFormat("LandPlot count: %d", state->LandPlot_count), 10, 30, 20, WHITE);
    for (int i = 0; i < state->LandPlot_count; i++) {
        const LandPlot landplot = state->landplots[i];

        DrawText(
            TextFormat("LandPlot %d: (%.1f, %.1f)", landplot.id, landplot.pos.x, landplot.pos.x),
            10,
            50 + (i * 20),
            20,
            WHITE
        );
    }
}

void sch_DrawNextTurnButton(const Schmungo *state)
{
    Vector2 pos = { sch_x_center(sch_vw(5)), sch_y_center(sch_vw(5)) };
    Vector2 size = { sch_vw(5), sch_vw(5) };

    DrawRectangleV(pos, size, RED);
}

//--------------------------------------------------------------------------------------------------
// Update Components
//--------------------------------------------------------------------------------------------------

const LandPlot *sch_UpdateLandPlot(const LandPlot *old_state)
{
    LandPlot *new_state = malloc(sizeof(*new_state));

    // TODO: Lock?
    new_state->id = old_state->id;
    new_state->color = old_state->color;
    new_state->pos = old_state->pos;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse_pos = GetMousePosition();
        Rectangle landplot_area = { new_state->pos.x, new_state->pos.y, sch_vw(5), sch_vw(5) };
        if (CheckCollisionPointRec(mouse_pos, landplot_area)) {
            new_state->color = GREEN;
        }
    }

    return new_state;
}

//--------------------------------------------------------------------------------------------------
// Core
//--------------------------------------------------------------------------------------------------

const Schmungo *sch_init(Schmungo *state)
{
    state = malloc(sizeof(*state));
    assert(state != NULL && "Buy more RAM lol");
    memset(state, 0, sizeof(*state));

    state->font = LoadFontEx("./resources/fonts/Alegreya-Regular.ttf", FONT_SIZE, NULL, 0);
    state->debug_mode = true;

    { // Allocate space for landplots 
        state->LandPlot_count = 9;
        state->landplots = malloc(state->LandPlot_count * sizeof(*state->landplots));
        for (int i = 0; i < state->LandPlot_count; i++) {
            state->landplots[i].id = i;
            state->landplots[i].color = RED;

            float x;
            switch (i) {
                case 0:
                case 3:
                case 6:
                    x = sch_x_center(sch_vw(5) - 250);
                    break;

                case 1:
                case 4:
                case 7:
                    x = sch_x_center(sch_vw(5));
                    break;

                case 2:
                case 5:
                case 8:
                    x = sch_x_center(sch_vw(5) + 250);
                    break;
            }
            
            float y;
            switch (i) {
                case 0:
                case 1:
                case 2:
                    y = sch_y_center(sch_vw(5) - 250);
                    break;

                case 3:
                case 4:
                case 5:
                    y = sch_y_center(sch_vw(5));
                    break;

                case 6:
                case 7:
                case 8:
                    y = sch_y_center(sch_vw(5) + 250);
                    break;
            }  
            
            state->landplots[i].pos = (Vector2) { x, y};
        }
    }

    SetMasterVolume(0.5);

    return state;
}

const Schmungo *sch_update(const Schmungo *old_state)
{
    Schmungo *new_state = malloc(sizeof(*new_state));
    assert(new_state != NULL && "Buy more RAM lol");
    memset(new_state, 0, sizeof(*new_state));

    // TODO: Do I need a lock here?
    // Copy and update landplots
    new_state->LandPlot_count = old_state->LandPlot_count;
    new_state->landplots = malloc(new_state->LandPlot_count * sizeof(*new_state->landplots));
    for (int i = 0; i < new_state->LandPlot_count; i++) {
        new_state->landplots[i] = *sch_UpdateLandPlot(&old_state->landplots[i]);
        // TODO: Freeing the old state causes a segfault but I don't know why.
        // free(&old_state->landplots[i]);
    }

    if (IsKeyPressed(KEY_SPACE)) {
        // Allocate space for the new LandPlot
        new_state->LandPlot_count++;
        new_state->landplots = realloc(new_state->landplots, new_state->LandPlot_count * sizeof(*new_state->landplots));
        assert(new_state->landplots != NULL && "Buy more RAM lol");

        // Initialize the new LandPlot
        LandPlot new_LandPlot;
        new_LandPlot.id = new_state->LandPlot_count - 1;
        new_LandPlot.color = RED;
        new_LandPlot.pos = (Vector2) { 
            .x = sch_x_center(sch_vw(5)) - (500 - ((new_state->LandPlot_count - 1) * 100)),
            .y = sch_y_center(sch_vw(5))
        };
        new_state->landplots[new_state->LandPlot_count - 1] = new_LandPlot;
    }

    if (IsKeyPressed(KEY_F1)) {
        new_state->debug_mode = !old_state->debug_mode;
    } else {
        new_state->debug_mode = old_state->debug_mode;
    }

    return new_state;
}

void sch_draw(const Schmungo *state)
{
    BeginDrawing();
    ClearBackground(COLOR_BACKGROUND);

    DrawFPS(10, 10);

    sch_DrawGameState(state);

    sch_DrawLandPlots(state);

    sch_DrawNextTurnButton(state);
    
    EndDrawing();
}

void sch_cleanup(Schmungo *state) {
    free(state->landplots);
    free(state);
    // _gup_memory_print();
}

Schmungo *persistent_state = NULL;

int main(void)
{
    Image logo = LoadImage("./resources/logo/logo-256.png");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    size_t factor = 80;
    InitWindow(factor*16, factor*9, "Schmungo");
    SetWindowIcon(logo);
    SetTargetFPS(144);
    SetExitKey(KEY_ESCAPE);
    InitAudioDevice();

    persistent_state = sch_init(persistent_state);

    while (!WindowShouldClose()) {
        // Reset the state
        if (IsKeyPressed(KEY_R)) {
            persistent_state = sch_init(persistent_state);
        }

        // Update the game state
        Schmungo *new_state = sch_update(persistent_state);

        // Draw the game state
        sch_draw(new_state);

        // Cleanup the old state
        sch_cleanup(persistent_state);

        // Update the persistent state to the new state
        persistent_state = new_state;
    }

    sch_cleanup(persistent_state);

    CloseAudioDevice();
    CloseWindow();
    UnloadImage(logo);

    return 0;
}
