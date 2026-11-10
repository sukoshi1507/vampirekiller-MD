#include <string.h>
#include "../sgdk/src/wall_view.c"
u16 vk_front_mode=FRONT_INTERIOR;
static int sprite[6], loads;
void VDP_loadTileData(const u32 *p,u16 t,u16 n,int mode){(void)p;(void)t;(void)n;(void)mode;loads++;}
void VDP_setSpriteFull(u16 i,s16 x,s16 y,u16 size,u16 attr,u16 link){sprite[0]=i;sprite[1]=x;sprite[2]=y;sprite[3]=size;sprite[4]=attr;sprite[5]=link;}
void SYS_disableInts(void){} void SYS_enableInts(void){}
u16 VDP_getPlaneAddress(VDPPlane p,u16 x,u16 y){(void)p;(void)x;(void)y;return 0;}
void VDP_setTileMapDataRow(VDPPlane p,const u16 *d,u16 y,u16 x,u16 n,int m){(void)p;(void)d;(void)y;(void)x;(void)n;(void)m;}
/* wall.c is linked separately; its logic is tested in trade_bridge.c. */
void trade_view(int stage,int x,int y,int frame,int *out,unsigned char *pixels){
 unsigned r,c;vk_walls.stage=stage;vk_walls.obj[0].state=0x82;vk_walls.obj[0].x=x;vk_walls.obj[0].y=y;vk_walls.obj[0].frame=frame;vk_walls.obj[1].state=0;
 vk_wall_view_room();loads=0;out[0]=vk_wall_view_sprites(12);memcpy(out+1,sprite,sizeof sprite);out[7]=loads;
 for(r=0;r<24;r++)for(c=0;c<32;c++){unsigned t=(c/8)*3+r/8;u32 word=vk_wall_frame_tiles[frame*96+t*8+r%8];pixels[r*32+c]=(word>>(28-4*(c%8)))&15;}
}
int trade_compose(int sale,int price,unsigned char *out){unsigned r,c;int count;vk_walls.sale=sale;vk_walls.price=price;compose();count=make_tiles();
 /* Reconstruct uploaded cells, testing deduplication as well as composition. */
 for(r=0;r<VK_SHOP_H;r++)for(c=0;c<VK_SHOP_W;c++){u16 cell=win_cells[r/8][c/8];u32 word=cell?win_tiles[((cell&2047)-MSX_TEXT_TILE_INDEX)*8+r%8]:0;out[r*VK_SHOP_W+c]=(word>>(28-4*(c%8)))&15;}
 return count;
}
void trade_view_window(int x,int y,unsigned char *out){vk_walls.slot=0;vk_walls.obj[0].x=x;vk_walls.obj[0].y=y;vk_shop_window(&vk_walls,out,out+1);}
