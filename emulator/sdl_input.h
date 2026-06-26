#ifndef _SDL_INPUT_H_
#define _SDL_INPUT_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SIM_KEY_NONE = 0,
    SIM_KEY_LEFT,
    SIM_KEY_RIGHT,
    SIM_KEY_UP,
    SIM_KEY_DOWN,
    SIM_KEY_ENTER,
    SIM_KEY_ESCAPE,
    SIM_KEY_QUIT,
    SIM_KEY_D,  // Detail mode toggle
    SIM_KEY_L,  // Log toggle
    SIM_KEY_S,  // Cycle dash style
    SIM_KEY_R,  // Reset simulator
} SimKey;

void SDL_Input_Init(void);
void SDL_Input_Quit(void);
SimKey SDL_Input_Poll(void);
uint8_t SDL_Input_IsPressed(SimKey key);

int32_t SDL_Input_GetScroll(void);
uint8_t SDL_Input_GetScrollPress(void);
uint8_t SDL_Input_GetLeftClick(int32_t *x, int32_t *y);
uint8_t SDL_Input_GetRightClick(void);

#endif