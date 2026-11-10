#include <string.h>
static unsigned char random_values[16];
static int random_index;
unsigned char Enemies_randomR(void){return random_values[random_index++];}
#include "../sgdk/src/dracula.c"
static VKDracula d;
void d18_init(void){vk_dracula_init(&d);}
void d18_step(int x,int y,int difficulty,int tick,int frozen,int finished){vk_dracula_pass(&d,x,y,difficulty,tick,frozen,finished);}
void d18_object(int k,unsigned char *out){memcpy(out,d.object[k],32);}
void d18_shot(int k,unsigned char *out){memcpy(out,d.shot[k],32);}
void d18_ce(unsigned char *out){memcpy(out,d.ce,128);}
int d18_hit(int weapon){return vk_dracula_hit(&d,weapon);}
int d18_energy(void){return d.energy;}

void d18_random(const unsigned char *values){memcpy(random_values,values,16);random_index=0;}

unsigned long d18_score(void){return d.score;}
int d18_sounds(unsigned char *out){memcpy(out,d.sound,d.sound_count);return d.sound_count;}
int d18_exit(void){return d.exit_requested;}

int d18_sprites(int *out){VKDraculaSprite s[VK_DRACULA_MAX_SPRITES];int i,n=vk_dracula_sprites(&d,s);for(i=0;i<n;i++){out[3*i]=(unsigned char)(s[i].y-1);out[3*i+1]=(unsigned char)s[i].x;out[3*i+2]=s[i].pattern;}return n;}
void d18_transition(int cycles,int *out){vk_dracula_transition(&d,cycles);out[0]=d.completed_cycles;out[1]=d.return_title;out[2]=d.next_stage;}

void d18_palette(const unsigned char *p){memcpy(d.palette,p,32);}

void d18_place(int type,int projectile){unsigned char *p=projectile?d.shot[0]:d.object[0];memset(p,0,32);p[0]=(unsigned char)type;p[3]=160;p[5]=128;}
int d18_contact(int slot,int x,int y,int state){return vk_dracula_contact(&d,slot,x,y,state);}
int d18_whip(int slot,int x,int y,int left,int state,int chain){return vk_dracula_whip(&d,slot,x,y,left,state,chain);}

void d18_clock(int n){d.shot_clock=(unsigned char)n;}
void d18_target(int slot,int *out){vk_dracula_target(&d,slot,&out[0],&out[1],&out[2],&out[3]);}

int d18_random_used(void){return random_index;}
void d18_entry(int *out){out[0]=VK_DRACULA_STAGE;out[1]=VK_DRACULA_ROOM;out[2]=VK_DRACULA_DOOR_ROOM;out[3]=VK_DRACULA_ENTRY_X;out[4]=VK_DRACULA_ENTRY_MUSIC;}

void d18_visual(int *out){out[0]=d.upper;out[1]=d.lower;out[2]=d.bust;out[3]=d.bust_x;out[4]=d.blank;out[5]=d.paint;out[6]=d.bust_count;{int i;for(i=0;i<4;i++)out[7+i]=d.bust_layers[i];}}

int d18_strike(int slot,int weapon,int power){return vk_dracula_strike(&d,slot,weapon,power);}
void d18_rearm(void){vk_dracula_rearm(&d);}

void d18_second(void){vk_dracula_init(&d);d.ce[1]=4;}
