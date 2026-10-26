#ifndef VK_INPUT_H
#define VK_INPUT_H

#include <genesis.h>

typedef struct {
    u16 keys;
    u16 pressed;
    int direction;
    bool jump_pressed;
} VKInput;

/* Zero-initialize once, then poll once per frame after SYS_doVBlankProcess. */
void handleInput(VKInput *input);

#endif
