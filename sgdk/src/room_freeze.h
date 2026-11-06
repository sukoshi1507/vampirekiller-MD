#ifndef VK_ROOM_FREEZE_H
#define VK_ROOM_FREEZE_H

/* State 9 of the MSX game (4377), the room change: no SGDK dependency, unit-tested
 * natively (tests/test_room_freeze.c). Source: research/room-links.md, section 9.
 *
 * The state 5 pass that sends Simon over an edge sets C000 := 9 (426A). The next
 * pass runs state 9: B963 writes D001, then 62F5 loads the room: SAT cleared (63C5),
 * display off (6304 -> 47D4), drawing, candles and hidden blocks (8671), display
 * on (6321 -> 47C7), spawn timers (9CA9) and fixed objects (61BB). 62F5 spans
 * several frames, during which the guard C005 lets the interrupt play the music
 * only: no input, Simon, enemies, candles and drops stand still. Then C000 := 5. */
typedef unsigned char VKFreezeByte;

/* Frames of C000 = 9 (openMSX, sampled at 4028) and frames from its start to the
 * display off (47D4), for the arrival room. stage 0 is the courtyard. */
typedef struct {
    VKFreezeByte frames;
    VKFreezeByte dark;
} VKRoomFreezeTiming;

typedef enum {
    VK_FREEZE_OFF = 0,       /* no room change in progress: a normal game frame */
    VK_FREEZE_HOLD = 1,      /* frozen frame, the screen stays as it is */
    VK_FREEZE_DARK = 2,      /* frozen frame where the display goes off (black) */
    VK_FREEZE_SHOW = 3       /* last frozen frame: the display comes back on the new room */
} VKFreezeStep;

typedef struct {
    VKFreezeByte left;       /* frozen frames still to come */
    VKFreezeByte frames;     /* duration of this room change */
    VKFreezeByte dark;       /* frame index of the display off */
} VKRoomFreeze;

/* Timing of the arrival in a room; unknown rooms get the PROVISIONAL default. */
VKRoomFreezeTiming vk_room_freeze_timing(VKFreezeByte stage, VKFreezeByte room);
/* A room change into (stage, room) has been made in this frame. */
void vk_room_freeze_begin(VKRoomFreeze *f, VKFreezeByte stage, VKFreezeByte room);
void vk_room_freeze_clear(VKRoomFreeze *f);
int vk_room_freeze_active(const VKRoomFreeze *f);
/* One frame: VK_FREEZE_OFF when no room change is in progress, else what this
 * frozen frame shows. */
VKFreezeStep vk_room_freeze_tick(VKRoomFreeze *f);

#endif
