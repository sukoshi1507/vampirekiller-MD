#include <genesis.h>
#include "item_reveal_view.h"
#include "item_effects.h"
#include "frontend.h"
#include "candle_view.h"
#include "wall_view.h"
#include "stage_map_view.h"
#include "hud.h"
/* Three unused slots: after the4 weapon-icon tiles275..278, and after the
 * key/HUD/door tiles700..749, before death-flame tiles752..759. */
#define OUTLINE_TOP 279
#define OUTLINE_LEFT 750
#define OUTLINE_CORNER 751
_Static_assert(OUTLINE_TOP+1<=DEATH_TILE_INDEX,"outline before death poses");
_Static_assert(OUTLINE_CORNER+1<=DEATH_FLAME_TILE_INDEX,"outline before flame sprites");
static VKItemBlock previous[8];
static u16 previous_mask;
static u16 signature=0xFFFF;
static VKByte last_stage=255,last_room=255;
static void rectangle(const VKItemBlock *b,bool erase,int clipx,int clipy)
{
    const u16 n=b->mode==2?2:4;u16 r,c;
    for(r=0;r<n;r++)for(c=0;c<n;c++){
        u16 tile,flipx=FALSE,flipy=FALSE;const int x=b->x/8+c,y=b->y/8+r;
        if(r && r<n-1 && c && c<n-1)continue;
        if(erase){VDP_setTileMapXY(BG_B,0,x,y);continue;}
        if(clipx>=0 && x>=clipx && x<clipx+10 && y>=clipy && y<clipy+4)continue;
        if((!r||r==n-1)&&(!c||c==n-1)){tile=OUTLINE_CORNER;flipx=c!=0;flipy=r!=0;}
        else if(!r||r==n-1){tile=OUTLINE_TOP;flipy=r!=0;}
        else{tile=OUTLINE_LEFT;flipx=c!=0;}
        VDP_setTileMapXY(BG_B,TILE_ATTR_FULL(PAL0,TRUE,flipy,flipx,tile),x,y);
    }
}
void vk_item_reveal_invalidate(void){signature=0xFFFF;}
void vk_item_reveal_refresh(void)
{
    const bool play=vk_front_mode==FRONT_INTERIOR||vk_front_mode==FRONT_COURTYARD;
    const VKByte stage=vk_candles_stage_of(&vk_game),room=vk_game.room;
    u16 mask=0,i,key;int clipx=-1,clipy=-1;VKByte x,y;
    if(!play){previous_mask=0;last_stage=255;signature=0xFFFF;return;}
    if(stage>=19||room>=12)return;
    if(vk_candles.c702&1)for(i=0;i<8;i++)if(vk_item_block_present(&vk_candles,&vk_walls,stage,room,i) && (vk_candles.stage!=stage || vk_candles.room!=room || (vk_candles.lit&(1u<<i))))mask|=1u<<i;
    if(vk_stage_map_is_open())mask=0;
    if(vk_walls.shop>=VK_SHOP_WAIT){vk_shop_window(&vk_walls,&x,&y);clipx=x/8;clipy=y/8;}
    key=mask+(u16)((clipx+1)*257u)+(u16)((clipy+1)*17u);
    if(last_stage==stage && last_room==room && signature==key)return;
    SYS_disableInts();
    for(i=0;i<8;i++)if(previous_mask&(1u<<i))rectangle(&previous[i],TRUE,-1,-1);
    {
        u32 top[8],left[8],corner[8];
        for(i=0;i<8;i++){top[i]=i?0:0xEEEEEEEE;left[i]=0xE0000000;corner[i]=i?0xE0000000:0xEEEEEEEE;}
        VDP_loadTileData(top,OUTLINE_TOP,1,CPU);VDP_loadTileData(left,OUTLINE_LEFT,1,CPU);VDP_loadTileData(corner,OUTLINE_CORNER,1,CPU);
    }
    for(i=0;i<8;i++){previous[i]=vk_item_blocks[stage][room][i];if(mask&(1u<<i))rectangle(&previous[i],FALSE,clipx,clipy);}
    SYS_enableInts();
    previous_mask=mask;last_stage=stage;last_room=room;signature=key;
}
