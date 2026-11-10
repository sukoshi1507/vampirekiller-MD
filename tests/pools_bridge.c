/* Common raw constructor outputs are test inputs; allocation and stepping are
 * the production pool functions. Existing source-constructor traces remain. */
#include "item_bridge.c"
static unsigned char pool_meta[8][32];
static int pool_step_sound_count;
int pools_step_sounds(void){return pool_step_sound_count;}
void pools_begin(const unsigned char *map){pool_step_sound_count=0;Enemies_loot_handler(0);candle_begin(map,13,3,0);Enemies_god=1;renderStage=13;memset(pool_meta,0,sizeof pool_meta);}
int pools_spawn(int kind,const unsigned char *raw){Enemy *e=newObject(&s,kind==4?ENEMY_TYPE_BONE:kind==9?ENEMY_TYPE_10_SHOT:kind==12?ENEMY_TYPE_SHIELD_SPARK:ENEMY_TYPE_FIREBALL);S8Ext *q;int i;if(!e)return -1;i=objectIndex(&s,e)-ENEMY_MAX;q=ext(&s,e);memcpy(pool_meta[i],raw,32);e->x=raw[5];e->y=raw[3];e->sub=raw[2];e->timer=raw[4];e->vy=(EnemyS16)(raw[7]|raw[8]<<8);q->vx=(EnemyS16)(raw[9]|raw[10]<<8);q->mobile=raw[6];e->dx=(EnemyS8)(q->vx<0?-1:q->vx>0?1:0);e->pose=raw[11];e->anim=raw[12];q->c12=raw[12];q->c11=raw[19];q->c18=0;if(kind==3)e->romBox=2;e->state=kind==9?raw[16]:0;e->age=0;return i;}
void pools_record(int i,unsigned char *out,int *sp){Enemy *e=&vk_projectiles[i];S8Ext *q=ext(&s,e);EnemySprite sat[8];int n,j;memcpy(out,pool_meta[i],32);out[0]=e->active?(e->type==ENEMY_TYPE_SHIELD_SPARK?12:pool_meta[i][0]):0;out[2]=e->sub;out[3]=e->y;out[4]=e->timer;out[5]=(VKByte)e->x;out[6]=q->mobile;out[7]=(VKByte)e->vy;out[8]=(VKByte)(e->vy>>8);out[9]=(VKByte)q->vx;out[10]=(VKByte)(q->vx>>8);out[11]=e->pose;if(e->type==ENEMY_TYPE_BONE||e->type==ENEMY_TYPE_10_SHOT)out[12]=q->c12;if(e->type==ENEMY_TYPE_10_SHOT){out[16]=e->state;out[19]=q->c11;}if(e->type==ENEMY_TYPE_SHIELD_SPARK){out[1]=out[13]=out[14]=out[31]=0;out[12]=e->anim;}
 n=Enemies_sprites(&s,(EnemyU8)(7+i),sat);sp[0]=n;for(j=0;j<n;j++){sp[1+3*j]=sat[j].y-1;sp[2+3*j]=sat[j].x;sp[3+3*j]=sat[j].pattern;}memcpy(pool_meta[i],out,32);}
void pools_free(int i){vk_projectiles[i].active=0;}
int pools_shield(int charges){s.armor=(EnemyU8)charges;c.c701=32;v.simonX=88;v.simonY=172;v.facingLeft=0;return Enemies_shield(&s,&v,&c.c701);}
int pools_armor(void){return s.armor;}
int pools_inventory(void){return c.c701;}
void pools_step(int frozen){pool_step_sound_count=0;s.frozen=frozen;s.guardTimer=255;v.simonX=0;v.simonY=32;s.spawnPause=255;Enemies_frame(&s,&t,&v);pool_step_sound_count+=s.fishSoundCount+(s.itemSound!=0);s.spawnPause=255;Enemies_frame(&s,&t,&v);pool_step_sound_count+=s.fishSoundCount+(s.itemSound!=0);}
int pools_count(int projectiles){int i,n=0;for(i=0;i<(projectiles?8:7);i++)n+=projectiles?vk_projectiles[i].active:s.enemy[i].active;return n;}
void pools_hopper(int i,const unsigned char *b){Enemy *e=&s.enemy[i];S8Ext *q=ext(&s,e);memcpy(enemy_metadata[i],b,32);clearEnemy(e);e->active=1;e->type=ENEMY_TYPE_HOPPER;e->state=b[1];e->sub=b[2];e->y=b[3];e->timer=b[4];e->x=b[5];q->mobile=b[6];e->vy=(EnemyS16)(b[7]|b[8]<<8);q->vx=(EnemyS16)(b[9]|b[10]<<8);e->pose=b[11];e->extraPhase=b[13];q->c10=b[16];q->c11=b[17];q->c12=b[18];e->age=1;}
void pools_c800_record(int i,unsigned char *b){Enemy *e=&s.enemy[i];S8Ext *q=ext(&s,e);if(e->type!=ENEMY_TYPE_HOPPER){item_enemy_prefix(i,b);return;}memcpy(b,enemy_metadata[i],32);b[0]=e->active?11:0;b[1]=e->state;b[2]=e->sub;b[3]=e->y;b[4]=e->timer;b[5]=e->x;b[6]=q->mobile;b[7]=e->vy;b[8]=e->vy>>8;b[9]=q->vx;b[10]=q->vx>>8;b[11]=e->pose;b[13]=e->extraPhase&15;b[16]=q->c10;b[17]=q->c11;b[18]=q->c12;}
int pools_sat_alloc(int kind,int x,int y){Enemy *e;if(kind==14){e=dragonSpawn(&s,x,y);return e?objectIndex(&s,e):-1;}e=newObject(&s,ENEMY_TYPE_DEATH);if(!e)return -1;e->x=x;e->y=y;e->anim=255;e->pose=0x85;e->extraPhase=1;e->age=1;return objectIndex(&s,e);}
int pools_orphans(void){unsigned long owned=0,u=Enemies_sat_mask(&s);int i,n=0;for(i=0;i<7;i++)owned|=c800_sat[i];for(i=0;i<8;i++)owned|=d700_sat[i];u&=~owned;for(i=0;i<18;i++)n+=(u>>i)&1;return n;}
static int contact_sound_count;static unsigned char contact_sounds[16];
static void pool_take(EnemyState *state,EnemyU8 kind){int ev=vk_candles_give(&c,kind);if(ev&VK_CANDLE_EVENT_PICK)Enemies_item(state,kind,c.c431);if(contact_sound_count<16)contact_sounds[contact_sound_count++]=vk_candles_sound(kind);}
void pools_contact_enable(void){Enemies_loot_handler(pool_take);Enemies_god=0;s.frozen=1;s.guardTimer=0;s.itemPhase=1;v.simonX=88;v.simonY=144;contact_sound_count=0;}
void pools_contact_step(void){int i;contact_sound_count=0;for(i=0;i<2;i++){s.spawnPause=255;Enemies_frame(&s,&t,&v);if(s.itemSound && contact_sound_count<16)contact_sounds[contact_sound_count++]=s.itemSound;}}
int pools_sounds(unsigned char *out){memcpy(out,contact_sounds,contact_sound_count);return contact_sound_count;}
void pools_cross(void){int sound;contact_sound_count=0;sound=item_give(5);contact_sounds[contact_sound_count++]=sound;}
void pools_set_kind(int kind){enemyLoot[0].raw[31]=(VKByte)kind;}
int pools_kills(void){return deathKills;}

void pools_boss(int on){Enemies_sat_boss((EnemyU8)on);}

void pools_render(void){Enemies_sat_render(&s);}
unsigned long pools_mask(void){return Enemies_sat_mask(&s);}
void pools_flame_pose(int i,int y,int pose){Enemy *e=Enemies_slot(&s,(EnemyU8)i);e->y=(EnemyU8)y;e->pose=(EnemyU8)pose;}
unsigned long pools_owner(int i){return *sat_owner(i);}

void legacy_import(int kind,int slot,const unsigned char *b,int stage,int room){
 Enemy *e=&s.enemy[slot];S8Ext *q=ext(&s,e);Fish3Ext *f=fx(&s,e);
 memcpy(enemy_metadata[slot],b,32);clearEnemy(e);e->active=1;e->type=kind==5?ENEMY_TYPE_5:ENEMY_TYPE_FISH3;
 e->state=b[1];e->sub=b[2];e->y=b[3];e->timer=b[4];e->x=b[5];q->mobile=b[6];e->vy=(EnemyS16)(b[7]|b[8]<<8);q->vx=(EnemyS16)(b[9]|b[10]<<8);e->dx=q->vx<0?-1:q->vx>0?1:0;e->pose=b[11];e->anim=b[12];e->extraPhase=b[13];e->age=1;
 q->c10=b[16];q->c11=b[17];q->c12=b[18];f->c13=b[19];f->c14=b[20];f->c15=b[21];q->c18=b[24];f->c19=b[25];f->c1b=b[27];f->c1c=b[28];f->c1d=b[29];v.stage=stage;v.room=room;
}
void legacy_record(int slot,unsigned char *b,int *sp){
 Enemy *e=&s.enemy[slot];S8Ext *q=ext(&s,e);Fish3Ext *f=fx(&s,e);EnemySprite sat[8];int n,j;
 if(e->type!=ENEMY_TYPE_5 && e->type!=ENEMY_TYPE_FISH3){pools_c800_record(slot,b);sp[0]=0;return;}
 memcpy(b,enemy_metadata[slot],32);b[0]=e->active?(e->type==ENEMY_TYPE_5?5:(b[0]==33 && b[1]==1 && f->c1d!=b[29]?3:b[0])):0;
 b[1]=e->state;b[2]=e->sub;b[3]=e->y;b[4]=e->timer;b[5]=e->x;b[7]=e->vy;b[8]=e->vy>>8;b[11]=e->pose;if(!e->active)b[14]=0;
 if(e->type==ENEMY_TYPE_5){EnemyS16 vx=ENEMY_TYPE5_VX*256*e->dx;b[9]=vx;b[10]=vx>>8;b[12]=e->anim;}
 else{b[6]=q->mobile;b[9]=q->vx;b[10]=q->vx>>8;b[13]=e->extraPhase;b[16]=q->c10;b[17]=q->c11;b[18]=q->c12;b[19]=f->c13;b[20]=f->c14;b[21]=f->c15;b[24]=q->c18;b[25]=f->c19;b[27]=f->c1b;b[28]=f->c1c;b[29]=f->c1d;}
 memcpy(enemy_metadata[slot],b,32);n=Enemies_sprites(&s,slot,sat);sp[0]=n;for(j=0;j<n;j++){sp[1+3*j]=sat[j].y-1;sp[2+3*j]=sat[j].x;sp[3+3*j]=sat[j].pattern;}
}
static int legacy_sound_count;
int legacy_sounds(void){return legacy_sound_count;}
void legacy_step(int frozen,int sx,int sy){int i;legacy_sound_count=0;s.frozen=frozen;s.guardTimer=255;v.simonX=sx;v.simonY=sy;for(i=0;i<2;i++){s.spawnPause=255;Enemies_frame(&s,&t,&v);legacy_sound_count+=s.fishSoundCount+(s.itemSound!=0);}}
void legacy_shot_record(int i,unsigned char *out){Enemy *e=&vk_projectiles[i];EnemyS16 vx=3*256*e->dx;memset(out,0,32);if(e->type!=ENEMY_TYPE_FIREBALL)return;out[0]=e->active?1:0;out[2]=e->sub;out[3]=e->y;out[4]=e->timer;out[5]=e->x;out[6]=1;out[7]=e->vy;out[8]=e->vy>>8;out[9]=vx;out[10]=vx>>8;out[11]=3;}

static unsigned char layout_meta[32];
int c800_layout_import(int internal,const unsigned char *b){Enemy *e=newObject(&s,(EnemyU8)internal);S8Ext *q;if(!e)return -1;memcpy(layout_meta,b,32);q=ext(&s,e);e->state=b[1];e->sub=b[2];e->y=b[3];e->timer=b[4];e->x=b[5];q->mobile=b[6];e->vy=(EnemyS16)(b[7]|b[8]<<8);q->vx=(EnemyS16)(b[9]|b[10]<<8);e->pose=b[11];e->anim=b[12];e->extraPhase=b[13];if(isLoot(e->type)){memcpy(enemyLoot[0].raw,b,32);enemyLoot[0].colour=b[0]==36?8:14;lootSync(e,&enemyLoot[0]);}return objectIndex(&s,e);}
void c800_layout_record(unsigned char *out){Enemy *e=&s.enemy[0];S8Ext *q=ext(&s,e);if(isLoot(e->type)){item_enemy_prefix(0,out);return;}memcpy(out,layout_meta,32);out[0]=e->active?layout_meta[0]:0;out[1]=e->state;out[2]=e->sub;out[3]=e->y;out[4]=e->timer;out[5]=e->x;out[6]=q->mobile;out[7]=e->vy;out[8]=e->vy>>8;out[9]=q->vx;out[10]=q->vx>>8;out[11]=e->pose;out[12]=e->anim;out[13]=e->extraPhase;if(!e->active)out[14]=0;}

int c800_dragon_import(const unsigned char *b){Enemy *e=dragonSpawn(&s,b[5],b[3]);if(!e)return -1;memcpy(layout_meta,b,32);return objectIndex(&s,e);}
void c800_dragon_pattern(int pattern){int i;for(i=0;i<8;i++)dg[0].ent[i][0]=(pattern&(1<<(i/2)))?224:96;}
void c800_dragon_record(unsigned char *b,int *sp){Enemy *e=&s.enemy[0];DragonExt *d=dgx(&s,e);EnemySprite sat[4];int i,n;memcpy(b,layout_meta,32);b[0]=e->active?14:0;b[1]=e->state;b[3]=e->y;b[5]=e->x;b[12]=d->c0c;b[13]=e->extraPhase;for(i=0;i<8;i++)b[16+i]=d->tgt[i];b[24]=d->t18;b[25]=d->i19;b[26]=d->y1a;b[27]=d->x1b;if(!e->active)b[14]=0;n=Enemies_sprites(&s,0,sat);sp[0]=n;for(i=0;i<n;i++){sp[1+2*i]=sat[i].y-1;sp[2+2*i]=sat[i].x;}}

void c800_layout_sprites(int *sp){EnemySprite sat[4];int n=Enemies_sprites(&s,0,sat),i;sp[0]=n;for(i=0;i<n;i++){sp[1+2*i]=sat[i].y-1;sp[2+2*i]=sat[i].x;}}

static unsigned long touch09_score;static int touch09_sfx_count;static unsigned char touch09_sfx[8];
void touch09_body(const unsigned char *b){Enemy *e=newObject(&s,ENEMY_TYPE_09);S8Ext *q=ext(&s,e);int i=objectIndex(&s,e);memcpy(enemy_metadata[i],b,32);e->state=b[1];e->sub=b[2];e->y=b[3];e->timer=b[4];e->x=b[5];q->mobile=b[6];e->vy=(EnemyS16)(b[7]|b[8]<<8);q->vx=(EnemyS16)(b[9]|b[10]<<8);e->pose=b[11];q->c12=b[12];e->extraPhase=b[13];q->c11=b[17];q->c18=b[18];}
void touch09_begin(int guard){Enemies_item(&s,9,c.c431);s.guardTimer=guard;s.itemPhase=1;s.frozen=1;v.simonX=88;v.simonY=172;Enemies_god=0;touch09_score=0;}
void touch09_step(void){s.spawnPause=255;touch09_sfx_count=0;Enemies_frame(&s,&t,&v);touch09_score+=s.contactScore;{int i;for(i=0;i<Enemies_itemSoundCount;i++)touch09_sfx[touch09_sfx_count++]=Enemies_itemSounds[i];}}
unsigned long touch09_points(void){return touch09_score*100;}
int touch09_sounds(unsigned char *out){memcpy(out,touch09_sfx,touch09_sfx_count);return touch09_sfx_count;}
void touch09_sprites(int slot,int *sp){EnemySprite a[4];int i,n=Enemies_sprites(&s,(EnemyU8)slot,a);sp[0]=n;for(i=0;i<n;i++){sp[1+3*i]=a[i].y-1;sp[2+3*i]=a[i].x;sp[3+3*i]=a[i].pattern;}}

int boss15_reserve(int slot,int count){return Enemies_boss15_reserve(&s,slot,count);}
void boss15_pools_begin(const unsigned char *map){candle_begin(map,15,9,1);Enemies_god=1;s.frozen=1;s.spawnPause=255;}
void boss15_sprites(int slot,int *out){EnemySprite sat[8];int n=Enemies_sprites(&s,slot,sat),i;out[0]=n;for(i=0;i<n;i++){out[1+3*i]=sat[i].y-1;out[2+3*i]=sat[i].x;out[3+3*i]=sat[i].pattern;}}
