#include "sdl_input.h"
#include "ili9488_gfx.h"
#include <SDL2/SDL.h>
#include <stdio.h>

static uint8_t g_keyStates[24] = {0};
static uint8_t g_prevKeyStates[24] = {0};

static int32_t g_mouseX = 0;
static int32_t g_mouseY = 0;
static uint8_t g_leftClick = 0;
static uint8_t g_rightClick = 0;
static int32_t g_scrollAccum = 0;
static uint8_t g_scrollPress = 0;

static SimKey sdl_key_to_sim(SDL_Keycode key)
{
    switch (key) {
        case SDLK_LEFT: return SIM_KEY_LEFT;
        case SDLK_RIGHT: return SIM_KEY_RIGHT;
        case SDLK_UP: return SIM_KEY_UP;
        case SDLK_DOWN: return SIM_KEY_DOWN;
        case SDLK_RETURN: return SIM_KEY_ENTER;
        case SDLK_ESCAPE: return SIM_KEY_ESCAPE;
        case SDLK_d: return SIM_KEY_D;
        case SDLK_l: return SIM_KEY_L;
        case SDLK_s: return SIM_KEY_S;
        case SDLK_r: return SIM_KEY_R;
        case SDLK_q: return SIM_KEY_QUIT;
        default: return SIM_KEY_NONE;
    }
}

void SDL_Input_Init(void)
{
}

void SDL_Input_Quit(void)
{
}

int32_t SDL_Input_GetScroll(void)
{
    int32_t v = g_scrollAccum;
    g_scrollAccum = 0;
    return v;
}

uint8_t SDL_Input_GetScrollPress(void)
{
    uint8_t v = g_scrollPress;
    g_scrollPress = 0;
    return v;
}

uint8_t SDL_Input_GetLeftClick(int32_t *x, int32_t *y)
{
    if (g_leftClick) {
        *x = g_mouseX;
        *y = g_mouseY;
        g_leftClick = 0;
        return 1;
    }
    return 0;
}

uint8_t SDL_Input_GetRightClick(void)
{
    uint8_t v = g_rightClick;
    g_rightClick = 0;
    return v;
}

SimKey SDL_Input_Poll(void)
{
    memcpy(g_prevKeyStates, g_keyStates, sizeof(g_keyStates));

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                g_keyStates[SIM_KEY_QUIT] = 1;
                return SIM_KEY_QUIT;

            case SDL_KEYDOWN:
            {
                SimKey k = sdl_key_to_sim(event.key.keysym.sym);
                if (k != SIM_KEY_NONE) g_keyStates[k] = 1;
                break;
            }
            case SDL_KEYUP:
            {
                SimKey k = sdl_key_to_sim(event.key.keysym.sym);
                if (k != SIM_KEY_NONE) g_keyStates[k] = 0;
                break;
            }

            case SDL_MOUSEBUTTONDOWN:
            {
                // Scale mouse coords from window pixels to LCD logical coords
                int winW, winH;
                SDL_GetWindowSize(SDL_GetWindowFromID(event.button.windowID), &winW, &winH);
                g_mouseX = (int32_t)((int64_t)event.button.x * LCD_WIDTH / winW);
                g_mouseY = (int32_t)((int64_t)event.button.y * LCD_HEIGHT / winH);
                if (event.button.button == SDL_BUTTON_LEFT) {
                    printf("[TOUCH] @ (%d,%d) -> LCD (%d,%d)\n",
                           event.button.x, event.button.y, g_mouseX, g_mouseY);
                    g_leftClick = 1;
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    g_rightClick = 1;
                } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                    g_scrollPress = 1;
                }
                break;
            }

            case SDL_MOUSEWHEEL:
                g_scrollAccum += event.wheel.y;
                break;
        }
    }
    return SIM_KEY_NONE;
}

uint8_t SDL_Input_IsPressed(SimKey key)
{
    if (key >= 24) return 0;
    return g_keyStates[key] && !g_prevKeyStates[key];
}