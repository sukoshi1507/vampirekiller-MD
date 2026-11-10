#ifndef VK_TRADE_HOST_GENESIS_H
#define VK_TRADE_HOST_GENESIS_H
/* Capture adapter for unmodified wall_view.c; no hardware timing emulation. */
#include <stdint.h>
#include <stdbool.h>
typedef uint32_t u32; typedef uint16_t u16; typedef int16_t s16;
typedef uint8_t u8; typedef int8_t s8; typedef volatile u32 vu32; typedef volatile u16 vu16;
typedef int VDPPlane;
#define TRUE true
#define FALSE false
#define PAL0 0
#define BG_A 0
#define DMA_QUEUE 1
#define CPU 0
#define TILE_ATTR_FULL(p,pr,v,h,t) ((u16)((t)|((p)<<13)|((pr)<<15)|((v)<<12)|((h)<<11)))
#define SPRITE_SIZE(w,h) (((w)-1)*4+(h)-1)
#define VDP_CTRL_PORT 0
#define VDP_DATA_PORT 0
#define VDP_READ_VRAM_ADDR(x) (x)
void VDP_loadTileData(const u32*,u16,u16,int);
void VDP_setSpriteFull(u16,s16,s16,u16,u16,u16);
void SYS_disableInts(void);void SYS_enableInts(void);
u16 VDP_getPlaneAddress(VDPPlane,u16,u16);
void VDP_setTileMapDataRow(VDPPlane,const u16*,u16,u16,u16,int);
#endif
