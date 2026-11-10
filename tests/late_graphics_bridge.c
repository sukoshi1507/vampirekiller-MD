#include <string.h>
#include "../sgdk/src/enemies.c"
int late_patterns(int type,int pose,int stage,int *out){EnemyState s;EnemySprite sat[8];int n,i;Enemies_init(&s);s.enemy[0].active=1;s.enemy[0].type=type;s.enemy[0].pose=pose;s.enemy[0].x=128;s.enemy[0].y=160;s.enemy[0].state=type==22?(pose<0x25):0;fleaBase=stage==9?0xA4:0x80;fireballCell=stage==11?0x9C:0x60;ghostCell=Enemies_ghostCell(stage);n=Enemies_sprites(&s,0,sat);for(i=0;i<n;i++){out[4*i]=sat[i].pattern;out[4*i+1]=sat[i].bank;out[4*i+2]=(EnemyU8)sat[i].x;out[4*i+3]=(EnemyU8)(sat[i].y-1);}return n;}
