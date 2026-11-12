/* LOT13 C500 fallback after failed89BF allocation. Port of8A99..8BCB.
 * All16 bytes retained; no C800 reservation is fabricated on this path. */
static VKByte fallback_raw[VK_DROP_MAX][16];
static VKCandles *fallback_owner;
static void fallback_reset(VKCandles *c){int i,j;fallback_owner=c;for(i=0;i<VK_DROP_MAX;i++)for(j=0;j<16;j++)fallback_raw[i][j]=0;}
void vk_candles_sync(VKByte clock,VKByte pass){fallback_clock=clock;fallback_pass=pass;}
const VKByte *vk_candles_fallback_record(const VKCandles *c,int slot){return c==fallback_owner && slot>=0 && slot<VK_DROP_MAX?fallback_raw[slot]:0;}
static void fallback_init(VKCandles *c,VKDrop *d,VKByte kind,VKByte x,VKByte y){
 int i=(int)(d-c->drop);VKByte *b=fallback_raw[i];fallback_owner=c;
 b[0]=1;b[1]=y;b[2]=x;b[3]=0;b[4]=kind;b[5]=kind<2?0:2;b[6]=0;
 d->state=VK_DROP_BURST;d->x=x;d->y=y;d->kind=kind;d->timer=0;d->fixed=d->content=0;d->catchable=0x40;
}
static int fallback_step(VKCandles *c,VKDrop *d,const VKByte *map){
 VKByte *b=fallback_raw[d-c->drop];unsigned de,v,ax;int picked=0;
 if(b[3]==1){b[3]=0;b[0]=0;b[5]=255;picked=1;}
 else switch(b[0]){
 case 1:
  if((++b[6]&15)==0){b[0]=0x82;b[5]=b[4]==1?1:2;
   if(b[4]==1){b[6]=0;b[9]=2;b[10]=0;b[11]=0;b[12]=32;}}
  break;
 case 0x82:
  de=(((unsigned)b[2]<<8)|b[1])+0x810;de&=65535;
  if((de&255)>0xDB){b[0]=0;b[5]=255;break;}
  if((VKByte)(vk_sample_tile(map,(VKByte)(de>>8),(VKByte)de)-1)<9){b[5]=255;b[0]++;break;}
  if(b[4]!=1){b[1]+=8;break;}
  b[6]++;if(fallback_clock&1)b[1]++;
  v=(((unsigned)b[2]<<8)|b[8])+(((unsigned)b[9]<<8)|b[10]);b[2]=(VKByte)(v>>8);b[8]=(VKByte)v;
  v=(((unsigned)b[9]<<8)|b[10])-(((unsigned)b[11]<<8)|b[12]);b[9]=(VKByte)(v>>8);b[10]=(VKByte)v;
  if(b[6]==32){b[6]=0;ax=b[12];b[11]=(VKByte)~b[11];b[12]=(VKByte)(0-ax);}
  break;
 case 0x83:b[1]=(VKByte)((b[1]+4)&248);b[2]=(VKByte)((b[2]+4)&248);b[6]=b[4]>=23 && b[4]<26?255:8;b[0]++;break;
 case 0x84:if(b[6]!=255 && !(fallback_clock&15) && --b[6]==0)b[0]=0;break;
 default:break;
 }
 d->state=b[0]==1?VK_DROP_BURST:b[0]==0x82?VK_DROP_FALL:b[0]?VK_DROP_REST:VK_DROP_NONE;
 d->x=b[2];d->y=b[1];d->kind=b[4];d->timer=b[6];
 return picked?pick(c,d):0;
}

/* 8C60..8CBC: visible dynamic C500 sprite; floor84 is drawn separately. */
int vk_candles_fallback_sprite(const VKCandles *c,int slot,VKByte out[4]){
 const VKByte *b=vk_candles_fallback_record(c,slot);static const VKByte colours[4]={8,1,14,1};
 if(!b || !b[0] || (b[0]&15)==4 || b[4]==25 || b[5]>2)return 0;
 out[0]=b[2];out[1]=b[1];out[3]=8;
 if(!b[5]){if(fallback_clock&1)return 0;out[2]=(fallback_clock&4)?0xF4:0xF8;}
 else if(b[5]==1)out[2]=0xE8;
 else {out[2]=0xEC;out[3]=colours[fallback_clock&3];}
 return 1;
}
