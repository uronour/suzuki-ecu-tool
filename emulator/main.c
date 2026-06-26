#include <stdio.h>
#include <windows.h>
#include <SDL2/SDL.h>
#include "sdl_gfx.h"
#include "sdl_input.h"
#include "sim_data.h"
#include "gauge_ui.h"
#include "ili9488_gfx.h"
#include "os_timer.h"
#include "../firmware/btt-custom/src/User/boot_screen.h"

extern OS_COUNTER os_counter;

// Gauge_TouchAt is called from SDL mouse input
void Gauge_TouchAt(uint16_t x, uint16_t y);

int main(int argc, char *argv[]);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;
    return main(__argc, __argv);
}

int main(int argc, char *argv[])
{
    (void)argv;

    printf("=== Suzuki ECU Tool K-Line Simulator ===\n");
    printf("Controls:\n");
    printf("  Scroll wheel    - Rotate encoder (nav / select)\n");
    printf("  Scroll btn      - Press encoder (execute)\n");
    printf("  Left click      - Touch screen\n");
    printf("  Right click     - Back page\n");
    printf("  Keyboard:\n");
    printf("    UP/DOWN       - Shift gear\n");
    printf("    SPACE         - Hold throttle\n");
    printf("    C             - Toggle clutch\n");
    printf("    D             - Action (keyboard fallback)\n");
    printf("    L short/long  - Press / Long-press\n");
    printf("    R             - Reset simulation\n");
    printf("    ESC           - Quit\n\n");

    printf("[DEBUG] Initializing SDL_GFX...\n");
    SDL_GFX_Init();
    printf("[DEBUG] SDL_GFX initialized, window created\n");
    
    printf("[DEBUG] Initializing SDL_Input...\n");
    SDL_Input_Init();
    
    printf("[DEBUG] Initializing SimData...\n");
    SimData_Init();
    printf("[DEBUG] SimData initialized\n");
    
    // Run boot animation
    printf("[DEBUG] Running boot animation...\n");
    Boot_Animation();
    printf("[DEBUG] Boot animation complete\n");
    
    printf("[DEBUG] Initializing UI...\n");
    Gauge_Init();
    printf("[DEBUG] UI initialized\n");

    uint8_t running = 1;
    uint8_t throttleHeld = 0;
    uint32_t frameCount = 0;
    uint32_t lastFpsTime = 0;
    uint32_t fpsFrameCount = 0;
    uint32_t fps = 0;
    uint32_t lastLPressTime = 0;
    uint32_t lastCPressTime = 0;

    while (running) {
        SimKey key = SDL_Input_Poll();

        if (key == SIM_KEY_QUIT) {
            running = 0;
        }

        // --- Mouse / scroll wheel controls (primary input) ---

        // Scroll wheel rotation = encoder rotation
        int32_t scroll = SDL_Input_GetScroll();
        while (scroll > 0) { scroll--;
            if (Gauge_IsSticky())
                Gauge_SelectUp();
            else
                Gauge_NextPage();
        }
        while (scroll < 0) { scroll++;
            if (Gauge_IsSticky())
                Gauge_SelectDown();
            else
                Gauge_PrevPage();
        }

        // Scroll wheel button press = encoder press
        if (SDL_Input_GetScrollPress()) {
            Gauge_Press();
        }

        // Left click = touch
        int32_t tx, ty;
        if (SDL_Input_GetLeftClick(&tx, &ty)) {
            Gauge_TouchAt((uint16_t)tx, (uint16_t)ty);
        }

        // Right click = back (emulator-only convenience)
        if (SDL_Input_GetRightClick()) {
            Gauge_PrevPage();
        }

        // --- Keyboard shortcuts (simulation extras, not on real hardware) ---

        // ESC = quit
        if (SDL_Input_IsPressed(SIM_KEY_ESCAPE)) {
            running = 0;
        }

        // UP/DOWN = gear shift (or ECU tool select if not using scroll)
        if (SDL_Input_IsPressed(SIM_KEY_UP)) {
            if (!Gauge_IsSticky()) SimData_ShiftUp();
        }
        if (SDL_Input_IsPressed(SIM_KEY_DOWN)) {
            if (!Gauge_IsSticky()) SimData_ShiftDown();
        }

        // Space = throttle hold
        const uint8_t *keystate = SDL_GetKeyboardState(NULL);
        throttleHeld = keystate[SDL_SCANCODE_SPACE];
        SimData_SetThrottle(throttleHeld);

        // C = clutch toggle
        if (keystate[SDL_SCANCODE_C] && SDL_GetTicks() - lastCPressTime > 200) {
            SimData_ToggleClutch();
            lastCPressTime = SDL_GetTicks();
        }

        // D = action (keyboard fallback)
        if (SDL_Input_IsPressed(SIM_KEY_D)) {
            Gauge_Press();
        }

        // L = short/long press (keyboard fallback)
        if (SDL_Input_IsPressed(SIM_KEY_L)) {
            lastLPressTime = SDL_GetTicks();
        }
        if (keystate[SDL_SCANCODE_L] && lastLPressTime > 0 && SDL_GetTicks() - lastLPressTime > 500) {
            Gauge_LongPress();
            lastLPressTime = 0;
        }
        if (!keystate[SDL_SCANCODE_L] && lastLPressTime > 0) {
            uint32_t held = SDL_GetTicks() - lastLPressTime;
            lastLPressTime = 0;
            if (held < 500) {
                Gauge_Press();
            }
        }

        // R = reset simulation
        if (SDL_Input_IsPressed(SIM_KEY_R)) {
            SimData_Init();
            Gauge_SetPage(GAUGE_PAGE_LIVE_DATA);
        }

        // Update simulation
        SimData_Update();

        // Update UI
        Gauge_Update();

        // Draw debug overlay (FPS in top-right)
        char overlayBuf[16];
        snprintf(overlayBuf, sizeof(overlayBuf), "%u FPS", fps);
        int16_t overlayW = strlen(overlayBuf) * FONT_STEP;
        LCD_FillRect(LCD_WIDTH - overlayW - 8, 2, LCD_WIDTH - 2, 2 + FONT_H + 4, RGB565(10, 10, 10));
        GFX_DrawString(LCD_WIDTH - overlayW - 4, 4, overlayBuf, RGB565(150, 200, 255), RGB565(10, 10, 10));

        SDL_GFX_SetWindowTitle("Suzuki ECU Tool - K-Line Simulator");

        // Render
        SDL_GFX_Present();

        frameCount++;
        fpsFrameCount++;
        uint32_t now = SDL_GetTicks();
        if (now - lastFpsTime >= 1000) {
            fps = fpsFrameCount;
            fpsFrameCount = 0;
            lastFpsTime = now;
        }

        if (frameCount % 60 == 0) {
            printf("[DEBUG] Frame %u, RPM=%u, Gear=%u, Page=%d\n", 
                   frameCount, g_sdsData.rpm, g_sdsData.gearPos, Gauge_GetPage());
        }

        // Update firmware timer
        os_counter.ms = SDL_GetTicks() - SDL_GFX_GetStartTime();

        // Frame rate limiting
        uint32_t frameTime = SDL_GetTicks() - now;
        if (frameTime < 16) SDL_Delay(16 - frameTime);

        // (test mode disabled - run indefinitely)
    }

    printf("\n[DEBUG] Shutting down... (frames: %u)\n", frameCount);
    SDL_Input_Quit();
    SDL_GFX_Quit();

    return 0;
}

