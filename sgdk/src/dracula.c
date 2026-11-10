#include "dracula.h"
#include "generated/dracula_data.h"
#include "generated/dracula_poses.h"
extern unsigned char Enemies_randomR(void);
/* 65B0 room script, AB42 first form; each call is one C003 pass. */
static unsigned short word(const VKDByte *p,int i){return (unsigned short)(p[i]|p[i+1]<<8);}
static void putword(VKDByte *p,int i,int v){p[i]=(VKDByte)v;p[i+1]=(VKDByte)(v>>8);}
static void kill(VKDByte *p){p[0]=p[14]=0;}
static void sound(VKDracula *d,int n){if(d->sound_count<8)d->sound[d->sound_count++]=(VKDByte)n;}
static void move(VKDByte *p){if(p[6]){putword(p,2,word(p,2)+word(p,7));putword(p,4,word(p,4)+word(p,9));}if(p[3]>=228||p[5]>=241||p[5]<7)kill(p);}
static int weight(int t){return t==17?8:t==23?0:2;}
static VKDByte *spawn(VKDracula *d,int t,int x,int y)
{
 int i,n=0,slot=0;VKDByte *p=0;
 for(i=0;i<7;i++){if(d->object[i][0])n+=weight(d->object[i][0]);else if(!p){p=d->object[i];slot=i;}}
 if(!p || n+weight(t)>14)return 0;
 d->visible[slot]=1;
 p[0]=(VKDByte)t;p[1]=p[2]=p[4]=p[6]=p[15]=p[31]=0;p[3]=(VKDByte)y;p[5]=(VKDByte)x;p[14]=7;
 if(t==17){p[6]=0;putword(p,7,-512);putword(p,9,0);p[11]=0x56;p[12]=30;p[13]=32;p[14]=p[16]=0;}
 else if(t==30){p[14]=0;p[6]=0;p[13]=1;p[11]=0x85;p[12]=16;}
 else if(t==23){p[14]=5;p[6]=0;p[13]=4;}
 else if(t==44){static const short vx[8]={256,640,384,512,576,320,768,448};int v=vx[++d->shot_clock&7];p[6]=1;p[12]=8;p[17]=0;p[11]=2;p[13]=1;putword(p,7,768);putword(p,9,d->sx<x?-v:v);}
 else if(t==46){
 static const short velocities[6][2]={{-1216,256},{-640,512},{384,800},{1024,-256},{-128,-1024},{-512,-704}};int row=d->ce[18]++;
 p[6]=1;p[14]=0;p[13]=1;p[11]=0x5A;putword(p,7,velocities[row%6][0]);putword(p,9,velocities[row%6][1]);
 }
 else if(t==45){p[6]=1;p[14]=0;p[13]=1;p[11]=(VKDByte)(d->sx<x?0x57:0x59);putword(p,9,d->sx<x?768:-768);putword(p,7,-768);}
 return p;
}
static void shot(VKDracula *d,VKDByte *body,int vy)
{
 int i;/* First form: body SAT occupies0..7, leaving D700's10..17 available. */
 for(i=0;i<8;i++)if(!d->shot[i][0]){VKDByte *p=d->shot[i];p[0]=5;p[1]=p[2]=p[4]=p[15]=p[31]=0;p[3]=(VKDByte)(body[3]-24);p[5]=body[5];p[6]=1;p[14]=0;p[13]=0;p[11]=3;putword(p,7,vy);putword(p,9,d->sx<body[5]?-640:640);return;}
}
void vk_dracula_init(VKDracula *d)
{
 unsigned i;VKDByte *p=(VKDByte *)d;for(i=0;i<sizeof(*d);i++)p[i]=0;d->energy=128;d->ce[0]=6;d->upper=1;
}
static void body(VKDracula *d,VKDByte *p)
{
 static const VKDByte xs[8]={48,208,80,160,64,192,128,176};
 switch(p[1]){
 case 0:p[11]=(VKDByte)(d->sx<p[5]?0x56:0x58);if(!--p[12]){p[6]=1;p[17]=p[3];p[1]++;}break;
 case 1:if((VKDByte)(p[17]-p[3])>=64){p[3]+=48;p[6]=0;p[11]=0x5B;p[12]=30;p[1]++;}break;
 case 2:d->visible[0]=(VKDByte)((p[12]&1)?3:15);p[11]=(VKDByte)(d->sx<p[5]?0x5B:0x5D);d->ce[14]=(VKDByte)((p[12]&1)?5:p[11]-0x5A);if(!--p[12]){d->ce[14]=(VKDByte)(p[11]-0x5A);p[12]=4;p[14]=7;d->visible[0]=15;p[1]++;}break;
 case 3:if(!--p[12]){p[11]=(VKDByte)(d->sx<p[5]?0x5C:0x5E);d->ce[14]=(VKDByte)(p[11]-0x5A);p[12]=4;p[1]++;}break;
 case 4:if(!--p[12]){shot(d,p,0);shot(d,p,384);shot(d,p,-384);p[12]=30;p[1]++;}break;
 case 5:if(!--p[12]){d->visible[0]=0;d->ce[14]=5;p[3]-=64;p[12]=90;p[14]=0;p[1]++;}break;
 case 6:if(!--p[12]){int row=p[16];p[3]+=64;p[11]=(VKDByte)(d->sx<p[5]?0x5B:0x5D);p[5]=xs[row&7];p[16]=(VKDByte)((row>=7?255:row)+1);p[12]=30;p[1]++;}break;
 case 7:d->visible[0]=(VKDByte)((p[12]&1)?0:15);p[11]=(VKDByte)(d->sx<p[5]?0x5B:0x5D);d->ce[14]=(VKDByte)((p[12]&1)?5:p[11]-0x5A);if(!--p[12]){d->ce[14]=(VKDByte)(p[11]-0x5A);p[12]=4;p[1]=3;p[14]=7;d->visible[0]=15;}break;
 case 8:{int i;d->visible[0]&=12;d->ce[14]=(VKDByte)(p[11]-0x5A);p[14]=0;p[12]=60;p[1]++;for(i=0;i<8;i++)kill(d->shot[i]);spawn(d,45,p[5],p[3]-43);break;}
 case 9:if(!--p[12]){d->ce[22]=1;d->ce[14]=5;kill(p);}break;
 }
 d->ce[15]=p[5];
}
/* 68C4/68A8: the background monster's two-step visibility. */
static void appear(VKDracula *d)
{
 if(++d->ce[55]==16){d->ce[54]=1;d->lower=1;}
 else if(d->ce[55]==32){d->ce[54]=2;d->lower=2;}
}
static void disappear(VKDracula *d,int a)
{
 d->ce[55]++;
 if(a==16){d->ce[54]=1;d->lower=1;}
 else if(d->ce[55]==16){d->ce[54]=0;d->lower=0;}
}
static void second_cycle(VKDracula *d)
{
 switch(d->ce[53]){
 case 0:case 2:if(!--d->ce[55])d->ce[53]++;break;
 case 1:disappear(d,1);if(!d->ce[54]){d->ce[55]=64;d->ce[53]++;}break;
 case 3:appear(d);if(d->ce[54]==2){d->ce[53]=0;d->ce[55]=192;}break;
 }
}
static void death_objects(VKDracula *d)
{
 int i;for(i=0;i<7;i++)if(d->object[i][0]){VKDByte *p=d->object[i];if(p[0]==17)p[1]=8;else{int x=p[5],y=(VKDByte)(p[3]-vk_dracula_death_offset[p[0]]);kill(p);spawn(d,30,x,y);}}
}
static void room(VKDracula *d,int music_finished)
{
 switch(d->ce[1]){
 case 0:d->ce[22]=d->ce[14]=0;spawn(d,17,192,208);d->ce[1]++;break;
 case 1:if(d->ce[14]){int j,k=0;if(d->ce[14]<5){for(j=0;j<d->bust_count;j++)if(d->bust_layers[j]!=d->ce[14])d->bust_layers[k++]=d->bust_layers[j];d->bust_layers[k++]=d->ce[14];d->bust_count=(VKDByte)k;}else d->bust_count=0;d->bust=d->ce[14]<5?d->ce[14]:0;d->bust_x=d->ce[15];d->ce[14]=0;}if(d->ce[22]){d->ce[21]=d->ce[14]=0;d->ce[1]++;}break;
 case 2:{int i;d->ce[18]=0;for(i=0;i<6;i++){int r=Enemies_randomR();int x=(r&15)-8+d->ce[15];int y=((r+8)&15)+152;(void)Enemies_randomR();spawn(d,46,x,y);}d->ce[2]=120;d->ce[1]++;break;}
 case 3:if(d->ce[2])d->ce[2]--;if(!d->ce[2]&&!d->object[0][0]){sound(d,0);d->ce[1]++;}break;
 case 4:d->upper=0;sound(d,0x88);d->energy=128;spawn(d,23,128,73);d->ce[54]=d->ce[55]=0;d->ce[1]++;break;
 case 5:appear(d);if(d->ce[54]==2){d->ce[56]=d->ce[57]=0;d->ce[55]=192;d->ce[1]++;}break;
 case 6:
  if(d->ce[21]){d->upper=1;d->palette[12]=d->palette[13]=0;death_objects(d);d->ce[55]=0;d->ce[1]++;}
  else {if(++d->ce[57]>=19)d->ce[57]=0;d->palette[12]=(VKDByte)(0x65+16*(d->ce[57]<=9?d->ce[57]:18-d->ce[57]));d->palette[13]=5;second_cycle(d);d->ce[56]++;if((d->ce[56]&31)==28)d->upper=1;else if((d->ce[56]&31)==31)d->upper=0;if(d->ce[54]==2 && !(d->tick&7))spawn(d,44,128,152);}break;
 case 7:disappear(d,d->ce[54]);if(!d->ce[54]){sound(d,0x8D);d->ce[2]=180;d->ce[1]++;}break;
 case 8:if(!--d->ce[2]){int i;for(i=0;i<32;i++)d->ce[96+i]=d->palette[i];d->ce[19]=128;d->ce[1]++;}break;
 case 9:{int k=(--d->ce[19]&15)*2;int v=d->palette[k];int hi=v&240,lo=v&15;d->palette[k]=(VKDByte)((hi?hi-16:0)|(lo?lo-1:0));if(d->palette[k+1])d->palette[k+1]--;d->ce[96+k]=d->palette[k];d->ce[97+k]=d->palette[k+1];if(!d->ce[19]){d->blank=1;d->ce[2]=8;d->ce[1]++;}break;}
 case 10:if(!--d->ce[2]){d->ce[0]=d->ce[1]=0;d->ce[64]=1;}break;
 }
 (void)music_finished;
}
static void credits(VKDracula *d,int music_finished)
{
 switch(d->ce[64]){
 case 1:d->blank=0;d->paint=1;d->palette[28]=255;d->palette[29]=15;sound(d,0x8E);d->ce[48]=d->ce[51]=d->ce[49]=d->ce[50]=0;d->ce[52]=1;d->ce[64]++;break;
 case 2:
  if(!(d->tick&3))d->ce[51]++;
  if(d->ce[51]==vk_dracula_credit_time[d->ce[49]]){
   if(vk_dracula_credit_x[d->ce[49]]==255){d->ce[50]=1;sound(d,255);}
   else {d->credit_line=d->ce[49];d->ce[49]++;d->paint|=2;}
  }
  d->paint|=4;d->video_scroll=d->ce[51];
  if(d->ce[50])d->ce[64]++;
  break;
 case 3:if(music_finished){d->blank=1;d->ce[2]=30;d->ce[64]++;}break;
 case 4:if(!--d->ce[2]){d->ce[52]=d->ce[64]=0;d->exit_requested=1;if(d->difficulty<2)d->difficulty++;d->credits_finished=1;}break;
 }
}
static void fly(VKDByte *p)
{
 static const VKDByte right[4]={30,31,30,32},left[4]={27,28,27,29};
 if(p[1]!=4)putword(p,7,word(p,7)-48);
 switch(p[1]){
 case 0:if(!--p[12]){p[11]=0xA5;p[12]=8;p[1]++;}break;
 case 1:if(!--p[12]){p[11]=(VKDByte)((p[10]&128)?0xA6:0xA7);p[12]=8;p[1]++;}break;
 case 2:if(!--p[12]){p[12]=24;p[1]++;}break;
 case 3:p[11]=(p[10]&128)?left[(p[17]>>1)&3]:right[(p[17]>>1)&3];p[17]++;if(!--p[12])p[1]++;break;
 case 4:p[11]=(p[10]&128)?left[(p[17]>>1)&3]:right[(p[17]>>1)&3];p[17]++;break;
 }
}

void vk_dracula_pass(VKDracula *d,int sx,int sy,int difficulty,int tick,int frozen,int music_finished)
{
 int i;if(d->exit_requested)return;d->sx=(unsigned short)sx;d->sy=(unsigned short)sy;d->difficulty=(VKDByte)difficulty;d->tick=(VKDByte)tick;d->frozen=(VKDByte)frozen;d->sound_count=0;d->paint=0;
 if(d->ce[64])credits(d,music_finished);else room(d,music_finished);
 for(i=0;i<7;i++){VKDByte *p=d->object[i];if(!p[0])continue;if(!frozen||p[0]==45||p[0]==46||p[0]==30){if(p[0]==17){int state=p[1];body(d,p);if(state==8){int k;for(k=0;k<7;k++)if(d->object[k][0]==45){move(d->object[k]);i=k;break;}continue;}}else if(p[0]==44)fly(p);else if(p[0]==30){p[11]=(p[12]&4)?0x86:0x85;if(!--p[12])kill(p);}else if(p[0]==45)putword(p,7,word(p,7)+80);move(p);}else if(p[3]>=228||p[5]>=241||p[5]<7)kill(p);}
 for(i=0;i<8;i++){VKDByte *p=d->shot[i];if(p[0] && (!frozen||p[0]==12)){if(p[0]==12){p[11]=(p[12]&4)?0x86:0x85;if(!--p[12])kill(p);}move(p);}}
}
int vk_dracula_hit(VKDracula *d,int weapon)
{
 int i,damage=(weapon==0||weapon==2)?4:6;if(d->ce[1]>=5 && weapon<2)damage*=4;if(d->ce[21])return 0;
 d->energy=(VKDByte)(d->energy>damage?d->energy-damage:0);if(!d->energy){d->ce[21]=1;sound(d,28);d->score+=d->ce[1]>=5?50000:30000;death_objects(d);
 for(i=0;i<8;i++)if(d->shot[i][0]){int j,x=d->shot[i][5],y=d->shot[i][3];kill(d->shot[i]);for(j=0;j<8;j++)if(!d->shot[j][0]){VKDByte *p=d->shot[j];p[0]=12;p[1]=p[2]=p[4]=p[15]=p[31]=0;p[3]=(VKDByte)y;p[5]=(VKDByte)x;p[6]=1;putword(p,7,0);putword(p,9,0);p[11]=0x85;p[12]=16;break;}}
 return 1;}return 0;
}

int vk_dracula_sprites(const VKDracula *d,VKDraculaSprite *out)
{
 int k,j,n=0,phase=d->ce[1]>=5 || d->ce[64];
 for(k=0;k<15;k++){
  const VKDByte *p=k<7?d->object[k]:d->shot[k-7];int mask=k<7?d->visible[k]:1;
  if(!p[0])continue;
  for(j=0;j<(int)(sizeof(vk_dracula_poses)/sizeof(vk_dracula_poses[0]));j++){
   const VKDraculaPose *pose=&vk_dracula_poses[j];int c;
   if(pose->type!=p[0] || pose->pose!=p[11] || pose->phase!=phase)continue;
   for(c=0;c<pose->count;c++)if(mask&(1<<c)){
    out[n].x=(VKDByte)(p[5]+pose->cell[c].dx);out[n].y=(short)((VKDByte)(p[3]+pose->cell[c].dy)+1);
    out[n].tile=pose->cell[c].tile;out[n].pattern=pose->cell[c].pattern;n++;
   }
   break;
  }
 }
 return n;
}
void vk_dracula_transition(VKDracula *d,int completed_cycles)
{
 d->completed_cycles=(VKDByte)(completed_cycles+1);d->return_title=d->completed_cycles==2;d->next_stage=(VKDByte)(d->return_title?255:0);
}

/* 8290/827C for C800,824C/8270 for D700. The caller selects an active slot;
 * flags, Simon's invulnerability and damage remain the combat adapter's job. */
int vk_dracula_box(const VKDracula *d,int slot,int *x,int *y,int *h,int *l)
{
 const VKDByte *p;if(slot<0||slot>=15)return 0;p=slot<7?d->object[slot]:d->shot[slot-7];if(!p[0])return 0;
 *x=p[5];*y=p[3];*h=slot>=7?6:p[0]==17?48:16;*l=slot>=7?3:p[0]==17?16:8;return 1;
}
int vk_dracula_contact(const VKDracula *d,int slot,int sx,int sy,int state)
{
 int x,y,h,l,dx,high,top;if(!vk_dracula_box(d,slot,&x,&y,&h,&l))return 0;
 dx=sx-x;high=h+((state==1||state==2)?18:26);top=sy-(state==1?8:0);
 return (dx<0?-dx:dx)<l+5 && (VKDByte)(top-2-y+h)<high;
}
int vk_dracula_whip(const VKDracula *d,int slot,int sx,int sy,int left,int state,int chain)
{
 int x,y,h,l,w,dx;if(!vk_dracula_box(d,slot,&x,&y,&h,&l))return 0;
 if(slot<7 && d->object[slot][0]==17){h=24;l=5;y=(VKDByte)(y-32);}
 w=sx+(left?-1:1)*(chain?24:20);dx=w-x;
 return w>=0&&w<=255&&(dx<0?-dx:dx)<l+(chain?16:12)&&(VKDByte)(sy-(state==2?10:18)-y+h)<h+4;
}

int vk_dracula_target(const VKDracula *d,int slot,int *x,int *y,int *h,int *l)
{
 if(!vk_dracula_box(d,slot,x,y,h,l))return 0;
 if(slot<7 && d->object[slot][0]==17){*h=24;*l=5;*y=(VKDByte)(*y-32);}
 return 1;
}

void vk_dracula_rearm(VKDracula *d)
{
 int i;for(i=0;i<7;i++)if(d->object[i][14]&4)d->object[i][14]|=1;
}
static void burst_slot(VKDracula *d,int slot)
{
 VKDByte *s=d->shot[slot];int i,x=s[5],y=s[3];kill(s);
 for(i=0;i<8;i++)if(!d->shot[i][0]){
  VKDByte *p=d->shot[i];p[0]=12;p[1]=p[2]=p[4]=p[15]=p[31]=0;p[3]=(VKDByte)y;p[5]=(VKDByte)x;p[6]=1;putword(p,7,0);putword(p,9,0);p[11]=0x85;p[12]=16;p[14]=0;return;
 }
}
/* 7DD4/7DE6,7F65/822A. Sounds and score are drained by the adapter immediately. */
int vk_dracula_strike(VKDracula *d,int slot,int weapon,int power)
{
 VKDByte *p;if(slot<0||slot>=15)return 0;p=slot<7?d->object[slot]:d->shot[slot-7];
 if(!p[0] || (slot<7 && !(p[14]&1)) || (slot>=7 && p[0]==12))return 0;
 sound(d,12);
 if(slot>=7){burst_slot(d,slot-7);d->score+=100;return 1;}
 p[14]&=254;
 if(p[0]==17||p[0]==23){vk_dracula_hit(d,weapon);return 1;}
 if(p[0]==44){
  p[13]=(VKDByte)(p[13]-power);
  if(!p[13] || p[13]>=128){int x=p[5],y=(VKDByte)(p[3]-vk_dracula_death_offset[p[0]]);kill(p);spawn(d,30,x,y);d->score+=100;}
 }
 return 1;
}
void vk_dracula_projectile_contact(VKDracula *d,int slot)
{
 if(slot>=0&&slot<8)kill(d->shot[slot]);
}

void vk_dracula_projectile_shield(VKDracula *d,int slot)
{
 if(slot>=0&&slot<8&&d->shot[slot][0]&&d->shot[slot][0]!=12)burst_slot(d,slot);
}
