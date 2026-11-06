#include "input.h"

/* Adapted from the user's start.zip: no shared position/velocity globals. */
void handleInput(VKInput *input)
{
    const u16 previous = input->keys;
    input->keys = JOY_readJoypad(JOY_1);
    input->pressed = input->keys & (u16) ~previous;
    input->direction = !!(input->keys & BUTTON_RIGHT) - !!(input->keys & BUTTON_LEFT);
    input->jump_pressed = !!(input->pressed & BUTTON_C);
}
