#include "dracula_view.h"
#include "actors.h"
#include "hud.h"
#include "generated/enemy_sprites_md.h"
#include "generated/dracula_background.h"
#include "generated/dracula_credits.h"
#include "generated/dracula_md.h"

/* Preserve HUD196..274, death/stairs280.., whip404..595 and item700..751.
 * Only the blocks belonging to absent regular enemies are reused here. */
#define BACKGROUND_SPLIT 180
#define BACKGROUND_SECOND VK_ENEMY_TILE_COUNT
#define BACKGROUND_THIRD 32
#define BACKGROUND_LAST (VK_DRACULA_BACKGROUND_TILES-BACKGROUND_SPLIT-BACKGROUND_SECOND-BACKGROUND_THIRD)
_Static_assert(TILE_USER_INDEX+BACKGROUND_SPLIT<=HUD_FRAME_TILE_INDEX,"Dracula room avoids HUD frame/text");
_Static_assert(ENEMY4_TILE_INDEX+16+BACKGROUND_THIRD<=700,"Dracula extra room tiles avoid keys");
_Static_assert(VK_DRACULA_TILE_COUNT+BACKGROUND_LAST<=128,"Dracula room and sprite tails fit type5 block");
static s16 last_room=-1,last_bust=-1;
static u8 in_credits,last_blank;
static u16 credit_map[1024];
static u8 erased[32];
static u32 row_words[256]; /* Also used for the sixteen-tile bust. */

static u16 background_tile(u16 tile)
{
 if(tile<BACKGROUND_SPLIT)return (u16)(TILE_USER_INDEX+tile);
 tile-=BACKGROUND_SPLIT;if(tile<BACKGROUND_SECOND)return (u16)(ENEMY_TILE_INDEX+tile);
 tile-=BACKGROUND_SECOND;if(tile<BACKGROUND_THIRD)return (u16)(ENEMY4_TILE_INDEX+16+tile);
 tile-=BACKGROUND_THIRD;return (u16)(ENEMY5_TILE_INDEX+VK_DRACULA_TILE_COUNT+tile);
}
static void background(const VKDracula *d)
{
 const int view=d->upper*3+d->lower;int r,c;
 if(last_room==view)return;
 for(r=0;r<22;r++){
  u16 cells[32];int changed=last_room<0;
  for(c=0;c<32;c++){
   const u16 tile=vk_dracula_background_map[view][r*32+c];
   if(last_room>=0 && tile!=vk_dracula_background_map[last_room][r*32+c])changed=1;
   cells[c]=TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,background_tile(tile));
  }
  if(changed)VDP_setTileMapDataRow(BG_A,cells,(u16)(r+4),0,32,DMA_QUEUE_COPY);
 }
 last_room=(s16)view;
}
static void bust(const VKDracula *d)
{
 int i,code=0,mul=1;u8 set;
 /* The bust script keeps an ordered subset of the layers 1..4 (dracula.c): the 65 composites are generated
  * (tools/build_dracula_background.py), the frame only queues a DMA from ROM. Composing them here took 50 to 120 lines,
  * one frame lost every 5 in the Dracula room. */
 for(i=0;i<d->bust_count&&i<4;i++){code+=d->bust_layers[i]*mul;mul*=5;}
 if(last_bust==(s16)code)return;
 set=vk_dracula_bust_set_of[code];
 if(set==255)return;
 VDP_loadTileData(vk_dracula_bust_sets[set],ENEMY4_TILE_INDEX,16,DMA_QUEUE);last_bust=(s16)code;
}
static void credit_row(u16 row)
{
 u16 c,line;
 for(c=0;c<32;c++)for(line=0;line<8;line++)row_words[c*8+line]=(erased[row]&(1<<line))?0:vk_dracula_credit_tiles[credit_map[row*32+c]*8+line];
 VDP_loadTileData(row_words,(u16)(TILE_USER_INDEX+row*32),32,CPU);
}
static void credits(const VKDracula *d)
{
 u16 row,c;
 if(d->paint&1){
  DMA_flushQueue();VDP_resetSprites();VDP_updateSprites(1,CPU);VDP_clearPlane(BG_A,TRUE);
  for(row=0;row<32;row++){
   u16 cells[32];erased[row]=0;
   for(c=0;c<32;c++){credit_map[row*32+c]=vk_dracula_credit_base[row*32+c];cells[c]=TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX+row*32+c);}
   credit_row(row);VDP_setTileMapDataRow(BG_A,cells,row,0,32,CPU);
  }
  in_credits=1;
 }
 if(d->paint&2){
  const u16 line=d->credit_line;row=vk_dracula_credit_y[line]/8;erased[row]=0;
  for(c=0;c<32;c++)credit_map[row*32+c]=vk_dracula_credit_band[line][c];
  credit_row(row);
 }
 if(d->paint&4){
  const u8 y=(u8)(d->ce[51]-2);row=y/8;erased[row]|=(u8)(1<<(y&7));credit_row(row);
 }
 if(in_credits)VDP_setVerticalScroll(BG_A,(s16)d->video_scroll);
}
void vk_dracula_view_reset(void)
{
 DMA_flushQueue();
 if(in_credits)vk_hud_invalidate_tiles();
 VDP_setVerticalScroll(BG_A,0);last_room=last_bust=-1;in_credits=last_blank=0;
 VDP_setEnable(TRUE);
}
void vk_dracula_view_begin(VKDracula *d)
{
 int i;vk_dracula_view_reset();DMA_flushQueue();VDP_clearPlane(BG_A,TRUE);
 VDP_loadTileData(vk_dracula_background_tiles,TILE_USER_INDEX,BACKGROUND_SPLIT,CPU);
 VDP_loadTileData(vk_dracula_background_tiles+BACKGROUND_SPLIT*8,ENEMY_TILE_INDEX,BACKGROUND_SECOND,CPU);
 VDP_loadTileData(vk_dracula_background_tiles+(BACKGROUND_SPLIT+BACKGROUND_SECOND)*8,ENEMY4_TILE_INDEX+16,BACKGROUND_THIRD,CPU);
 VDP_loadTileData(vk_dracula_background_tiles+(BACKGROUND_SPLIT+BACKGROUND_SECOND+BACKGROUND_THIRD)*8,ENEMY5_TILE_INDEX+VK_DRACULA_TILE_COUNT,BACKGROUND_LAST,CPU);
 VDP_loadTileData(vk_dracula_tiles,ENEMY5_TILE_INDEX,VK_DRACULA_TILE_COUNT,CPU);
 for(i=0;i<32;i++)d->palette[i]=vk_dracula_initial_palette[i];
 background(d);bust(d);vk_hud_show();vk_hud_show_frame();vk_hud_update(vk_enemies.energy);
 PAL_setPalette(PAL0,vk_dracula_background_palette,CPU);PAL_setPalette(PAL2,vk_dracula_background_palette,CPU);
}
void vk_dracula_view_pass(const VKDracula *d)
{
 u16 colors[16];int i;
 if(d->ce[64] || d->credits_finished)credits(d);else {background(d);bust(d);}
 for(i=0;i<16;i++){const u8 rb=d->palette[2*i],g=d->palette[2*i+1];colors[i]=(u16)(((rb&7)*2<<8)|((g&7)*2<<4)|((rb>>4&7)*2));}
 PAL_setPalette(PAL0,colors,DMA_QUEUE_COPY);
 if(!in_credits)PAL_setPalette(PAL2,colors,DMA_QUEUE_COPY);
 if(last_blank!=d->blank){VDP_setEnable(!d->blank);last_blank=d->blank;}
}
u16 vk_dracula_view_sprites(const VKDracula *d,u16 first,u16 tail)
{
 VKDraculaSprite cells[VK_DRACULA_MAX_SPRITES];int i,n=vk_dracula_sprites(d,cells);const int extra=d->bust_count!=0;
 if(d->ce[64] || d->credits_finished || d->blank)return 0;
 for(i=0;i<n;i++)VDP_setSpriteFull((u16)(first+i),cells[i].x,cells[i].y,SPRITE_SIZE(2,2),TILE_ATTR_FULL(PAL2,TRUE,FALSE,FALSE,ENEMY5_TILE_INDEX+cells[i].tile),(u16)(i+1<n||extra?first+i+1:tail));
 if(extra){VDP_setSpriteFull((u16)(first+n),(s16)(d->bust_x-16),145,SPRITE_SIZE(4,4),TILE_ATTR_FULL(PAL2,TRUE,FALSE,FALSE,ENEMY4_TILE_INDEX),tail);n++;}
 return (u16)n;
}
