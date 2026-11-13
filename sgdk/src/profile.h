#ifndef VK_PROFILE_H
#define VK_PROFILE_H
#include <genesis.h>

/* Frame budget instrumentation (research/room-draw-budget.md).
 *
 * Always on: vk_late_turns counts the loop turns that ended after the next VBlank had already
 * started (SGDK's own "frame missed" rule: two VBlanks since the previous end of turn), i.e. lost
 * frames. vk_late_delta is the VBlank count of the last one. The replay tool reads both from RAM.
 *
 * Measurement build only (EXTRA_FLAGS=-DVK_PROFILE): VK_MARK(i) stamps the line counter
 * (vtimer * 262 + V counter) between the steps of a turn; the stamps of the last late turn stay in
 * vk_prof_late. */
extern volatile u16 vk_late_turns;
extern volatile u8 vk_late_delta;
/* Room draws (show_hall_room, show_courtyard): a late turn that draws a room is expected, it is the freeze draw, a door or a stage change. */
extern volatile u16 vk_draws;
/* Late turns without a room draw (the lost frames), and whether the last late turn drew a room. */
extern volatile u16 vk_late_plain;
extern volatile u8 vk_late_drew;
/* Loop turns done, and the lines left before the VBlank at the end of the last one. */
extern volatile u16 vk_turns;
extern volatile s16 vk_last_slack;
extern volatile u16 vk_absorbed;
/* SYS_doVBlankProcess() preceded by the lost-frame check. */
void vk_turn_sync(void);

#ifdef VK_PROFILE
#define VK_PROF_MARKS 56
extern volatile u32 vk_prof[VK_PROF_MARKS];
extern volatile u32 vk_prof_late[VK_PROF_MARKS];
static inline u32 vk_prof_lines(void)
{
    const u16 v = GET_VCOUNTER;
    const u16 line = v <= 0xEA ? v : v + 6;   /* NTSC 224: the V counter jumps back from EA to E5 */
    return vtimer * 262 + line - (line >= 224 ? 262 : 0);
}
#define VK_MARK(i) (vk_prof[i] = vk_prof_lines())
#else
#define VK_MARK(i) ((void)0)
#endif

#endif
