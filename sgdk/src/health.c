#include "health.h"

/* State 5 (play) -> 6 (death) -> 4 (new life) or 7 (game over) -> 0 (title),
 * reduced to the phases the port needs (see health.h for the provenance). */
void vk_health_begin(VKHealth *h)
{
    h->lives = VK_HEALTH_LIVES_START;
    h->phase = VK_LIFE_ALIVE;
    h->timer = 0;
}

VKHealthEvent vk_health_tick(VKHealth *h, int energy)
{
    switch (h->phase)
    {
    case VK_LIFE_ALIVE:
        if (energy > 0) return VK_HEALTH_NONE;
        h->phase = VK_LIFE_DYING;
        h->timer = VK_HEALTH_DEATH_FRAMES;
        return VK_HEALTH_DIED;
    case VK_LIFE_DYING:
        if (--h->timer) return VK_HEALTH_NONE;
        /* State 6 (4294): lives left -> refill and state 4, else jingle and state 7. */
        if (h->lives)
        {
            h->phase = VK_LIFE_RESPAWN;
            h->timer = VK_HEALTH_RESPAWN_FRAMES;
            return VK_HEALTH_REFILL;
        }
        h->phase = VK_LIFE_GAME_OVER;
        h->timer = VK_HEALTH_GAMEOVER_FRAMES;
        return VK_HEALTH_GAME_OVER;
    case VK_LIFE_RESPAWN:
        if (--h->timer) return VK_HEALTH_NONE;
        /* State 4 (422C): C410 decremented, play resumes. */
        h->lives--;
        h->phase = VK_LIFE_ALIVE;
        return VK_HEALTH_RESPAWN;
    default:
        if (h->timer && --h->timer) return VK_HEALTH_NONE;
        h->timer = 0;
        return VK_HEALTH_TITLE;
    }
}

int vk_health_controls_locked(const VKHealth *h)
{
    return h->phase != VK_LIFE_ALIVE;
}

int vk_health_bar_pixels(int energy)
{
    if (energy <= 0) return 0;
    if (energy >= VK_HEALTH_ENERGY_MAX) return VK_HEALTH_BAR_PIXELS;
    return energy * VK_HEALTH_BAR_PIXELS / VK_HEALTH_ENERGY_MAX;
}
