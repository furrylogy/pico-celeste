#include "platform.h"
#include "pico8sim.h"
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdlib.h>

static uint8_t btn_state[6];

void platform_update_input(void)
{
    const uint8_t *keys = SDL_GetKeyboardState(NULL);
    btn_state[0] = keys[SDL_SCANCODE_LEFT];
    btn_state[1] = keys[SDL_SCANCODE_RIGHT];
    btn_state[2] = keys[SDL_SCANCODE_UP];
    btn_state[3] = keys[SDL_SCANCODE_DOWN];
    btn_state[4] = keys[SDL_SCANCODE_C];
    btn_state[5] = keys[SDL_SCANCODE_X];
}

uint8_t btn(uint8_t b)
{
    if (b > 5)
        return 0;
    return btn_state[b];
}

void sfx(uint8_t n)
{
    (void)n;
}

void music(int16_t n, int16_t fade_len, uint8_t channel_mask)
{
    (void)n;
    (void)fade_len;
    (void)channel_mask;
}

fix16_t rnd(fix16_t n)
{
    if (n <= 0)
        return 0;

    return (fix16_t)((uint32_t)rand() % (uint32_t)n);
}
