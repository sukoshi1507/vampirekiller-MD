/*
 * Logique pure des ennemis du hall (voir enemies.h et research/enemy-spawn.md, enemy-behavior.md).
 *
 * Traduction des routines Z80 :
 *   - 9CE6 (banque 2) : canal type 1, appele depuis 5EF6 si bit0 du drapeau ROM.
 *   - 9CC3 (banque 2) : minuterie partagee CF10 + compte a rebours CF00.
 *   - 9CFC (banque 2) : position de spawn (E = Y, D = X de spawn).
 *   - 9E16 (banque 2) : garde de spawn selon Simon (C427) et D.
 *   - 9D97 (banque 2) : canal type 4 (drapeau bit 3) : 9CC3 sur CF06/CF07, table 9DBB, garde 9E16.
 *   - 5F1D (banque 0) : creation de l'entite dans le premier bloc libre.
 *   - 9D4B, A2E0, A310 : type 2, l'homme-poisson des salles 4 et 5 du stage 2 ; 9C66/9CA3 :
 *     ses gouttes (objet 0x20). Voir la section « Type 2 » de enemy-behavior.md.
 *
 * Niveau de confiance :
 *   - Tables kEnemy*                                       verifie (ROM)
 *   - Minuterie type 1 (4 appels par decrement, reload = periode - 2*D012 + 1)   verifie
 *   - Garde 9E16                                           verifie code + 6 cas de trace
 *   - Cote = type (A droite, B gauche)                     enemy-behavior 6.3
 *   - Mouvement, animation, sortie du type 1               observe (trace frames.txt)
 *   - Type 4 : canal 9D97 (premier tir, cadence, cote, 9E16) code + trace (scratchpad/fidelity)
 *   - Type 4 : 1,5 px/image, sortie                        observe (4 creations)
 *   - Contact -2, invulnerabilite 180 images               observe (run 3, run 6) ;
 *     le recul lui-meme est dans gameplay.c (vk_game_hit)
 *   - Type 1 du stage 2 (salle 2) : spawn 9CFC (E = A0, cote = bit 1 de CF01), sortie 99E5,
 *     contact 8290 et fouet 7DAD de la classe 1 : code lu et trace (enemy-behavior.md,
 *     section « Type 1 au stage 2 »). Mouvement et motifs : ceux du hall (meme routine A964).
 *
 * PROVISOIRE (non verifie, a confirmer en emulation) :
 *   - Type 1 du hall : cote corrige pour lire C42C (facingLeft), comme 9D30.
 *   - Boite de contact du type 4 : celle de la ROM (8390) appliquee a la position affichee
 *     (objet = SAT + 8, + 15) ; la SAT suit l'objet une image sur deux.
 *   - extraPhase (1 pour A, 15 pour B) : observe, cause inconnue.
 *   - Type 2 : premier tir 51 images apres l'entree (2 mesures, depend de la phase de CF10) ;
 *     +1D, +12 et +15 de l'objet repris a 0 (la ROM garde ceux de l'occupant precedent du
 *     bloc) ; premiere mise a jour des gouttes (la ROM en fait une de plus, voir 9CA3) ;
 *     objet 1E, score et sons 7/8/9 non portes.
 *   - Salle 1 avec Simon haut (E = 60) : condition C425 < 0x88 (module), pas une regle verifiee.
 *
 * Contact (8290 -> 847A, Enemies_touches) : boite de la classe du type (82CB), selon C420
 * (EnemyRoomVars.simonState) : debout, accroupi, saut ; code lu (research/enemy-behavior.md,
 * section « Contact selon la posture de Simon »).
 */
#include "enemies.h"
#include "item_shield.h"
#include "player.h"
#include "generated/enemy_rooms.h"
#include "generated/late_fixed_enemies.h"
#include "generated/late_enemy_flags.h"

EnemyU8 Enemies_roomFlags(EnemyU8 stage, EnemyU8 room)
{
    if (stage >= 12 && stage <= 18)
        return room < vk_late_enemy_room_counts[stage - 12] ? vk_late_enemy_flags[stage - 12][room] : 0;
    return stage <= 11 && room < vk_enemy_room_count[stage] ? vk_enemy_room_flags[stage][room] : 0;
}


#define ENEMY_SPAWN_X_RIGHT 0xE5   /* 229 : cote droit, observe (SAT X de 909) */
#define ENEMY_SPAWN_X_LEFT  0x0A   /* 10 : cote gauche, observe (SAT X de 1013) */
#define ENEMY_SPAWN_X_LEFT_T4 0x0B /* 11 : type 4 a gauche, observe */
#define ENEMY_EXIT_X_LEFT   0      /* dx < 0 : limite x <= 0 ; masque a l'image suivante (observe 1125 -> 1126) */
#define ENEMY_EXIT_X_RIGHT  216    /* dx > 0 : limite x >= 216 ; masque a l'image suivante (observe 1207 -> 1208) */
#define ENEMY_EXIT_X_LEFT_T4 1     /* type 4, sens gauche : sort a X 01 */
#define ENEMY_EXIT_X_RIGHT_T4 0xE6 /* type 4, sens droite : sort a X E6 */
#define ENEMY_ROW_OFFSET    16     /* rang bas = Y + 16 (observe B0 pour A0) */
#define ENEMY_SIMON_HALF_W  8      /* boite de Simon : X-8 a X+15 (sprite 24 px) */
#define ENEMY_SIMON_RIGHT   16
#define ENEMY_SIMON_H       31     /* boite de Simon : Y-31 a Y (pieds en Y, provisoire) */
#define ENEMY_TYPE1_ROW_H   32
/* Type 1 du stage 2 (salle 2), research/enemy-behavior.md section « Type 1 au stage 2 ». */
#define ENEMY_S2_TYPE1_E      0xA0 /* 9D1B : E = A0 (Y de l'objet = pieds, comme C425) */
#define ENEMY_S2_TYPE1_Y      (ENEMY_S2_TYPE1_E - 0x20) /* Y SAT du rang haut : 80 (trace : 128 a chaque tir) */
#define ENEMY_S2_EXIT_X_RIGHT 0xE7 /* 99E5 : objet supprime a X >= F1 ; dernier X SAT E7 (trace) */
#define ENEMY_OBJ_X 8              /* X de l'objet = X SAT + 8 (enregistrements +21 de l'objet) */
#define ENEMY_OBJ_Y 31             /* Y de l'objet = Y affiche du rang haut + 31 (une fois pose) */
#define ENEMY_TYPE4_OBJ_Y 15       /* type 4 : Y de l'objet = Y SAT + 15 (+3 de l'objet, prouve) */

/* Tables ROM salle de stage 1 (D000 = 1).
 * kEnemyStage1RoomFlags : ROM 0x85CF-0x85D6 (pointeur 85A8 = 85CF pour D000 = 1).
 *   D001 = 0 -> 01, 1 -> 01, 2 -> 00, 3 -> 00, 4 -> 08 (type 4), 5 -> 01, 6 -> 01, 7 -> 00.
 * kEnemySpawnPeriod : ROM 0x9D43-0x9D4A (banque 2). */
const EnemyU8 kEnemyStage1RoomFlags[8] = { 0x01, 0x01, 0x00, 0x00, 0x08, 0x01, 0x01, 0x00 };
const EnemyU8 kEnemyStage2RoomFlags[6] = { 0x08, 0x08, 0x01, 0x08, 0x02, 0x02 };
/* Stage 3: exact six room bytes from MSX2 ROM 0x1C5DD (pointer at 0x1C5AC).
 * Bit 7 is retained verbatim; its behavior is not yet ported. */
static const EnemyU8 kEnemyStage3RoomFlags[6] = { 0x00, 0x00, 0x00, 0x81, 0x08, 0x00 };
const EnemyU8 kEnemySpawnPeriod[8] = { 0x0C, 0x12, 0x0C, 0x0C, 0x0C, 0x12, 0x0C, 0x0C };
/* 9DBB-9DC2 (banque 2) : periodes du canal type 4, index CF07 & 7 avant l'increment. */
static const EnemyU8 kType4Period[8] = { 0x14, 0x14, 0x14, 0x28, 0x14, 0x14, 0x14, 0x28 };

#define ENEMY_FLAG_TYPE1 0x01
#define ENEMY_FLAG_TYPE2 0x02
#define ENEMY_FLAG_TYPE3 0x04   /* stage 10 : canal 9D52 (objet 03) */
#define ENEMY_FLAG_TYPE4 0x08

/* Motifs : [type][pose][entree] ; entrees 0,1 = rang haut ; 2,3 = rang bas. */
static const EnemyU8 kPatterns[2][2][4] = {
    { { 0x40, 0x44, 0x48, 0x4C },   /* type A, pose A */
      { 0x40, 0x44, 0x50, 0x54 } }, /* type A, pose B */
    { { 0x58, 0x5C, 0x60, 0x64 },   /* type B, pose A */
      { 0x58, 0x5C, 0x68, 0x6C } }  /* type B, pose B */
};

/* Type 4 : Y SAT par pas de 2 images depuis le spawn (77 pas, identique dans les deux
 * sens et a chaque tir, 8 episodes mesures en salle 4). Le pas 18 manque a la mesure
 * (clignotement du multiplexage) : 178, interpole. */
#define ENEMY_TYPE4_Y_STEPS 77
static const EnemyU8 kType4Y[ENEMY_TYPE4_Y_STEPS] = {
    170, 171, 172, 174, 175, 175, 176, 177, 178, 178, 179, 179, 179, 179, 179, 179,
    179, 179, 178, 178, 177, 177, 176, 175, 174, 173, 172, 171, 170, 168, 167, 166,
    164, 163, 162, 162, 161, 160, 160, 159, 159, 159, 158, 158, 158, 158, 159, 159,
    159, 160, 161, 161, 162, 163, 164, 165, 166, 168, 169, 170, 172, 173, 174, 175,
    175, 176, 177, 177, 178, 178, 178, 178, 178, 178, 178, 178, 178
};
/* Type 4 : battement d'ailes, 4 poses de 16 images (premiere couche ; la seconde est N+4).
 * Vers la gauche 48, 50, 48, 58 ; vers la droite 60, 68, 60, 70 (mesure). */
static const EnemyU8 kType4Patterns[2][4] = {
    { 0x48, 0x50, 0x48, 0x58 },
    { 0x60, 0x68, 0x60, 0x70 }
};

/* Type 5 : position (X, Y) de la panthere par salle du hall, 0 = aucune. Liste d'objets
 * en ROM fichier 0x1C674 (paires type, position YX par salle) : salle 2 en 80,80,
 * salle 3 en 60,40, salle 7 en C0,60. */
#define ENEMY_TYPE5_VX 4            /* 0x0400 par mise a jour (une image sur deux) */
#define ENEMY_TYPE5_WAKE 0x40       /* A891 : activation si |C427 - X| < 0x40 */
#define ENEMY_TYPE5_JUMP_VY (-0x0108)
#define ENEMY_TYPE5_GRAVITY 0x00A0
#define ENEMY_TYPE5_VY_MAX 0x07FF

/* Type 2 (homme-poisson). 9D4B : X du tir = kType2X[CF03 & 7] (CF03 deja incremente), Y C8 ;
 * 9D8F : compte a rebours suivant = kType2Period[CF03 & 7] (avant l'increment) - 2 x D012 + 1,
 * decremente toutes les 8 images (9CC3). A4F3 : mises a jour avant le choix de sens suivant,
 * index +1D & 3 (+19 = 1 a la creation). */
static const EnemyU8 kType2X[8] = { 0x60, 0xD0, 0x30, 0x90, 0xA0, 0x40, 0x60, 0xB0 };
static const EnemyU8 kType2Period[8] = { 0x01, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18 };
static const EnemyU8 kType2Turn[4] = { 0x10, 0x20, 0x18, 0x30 };
#define ENEMY_TYPE2_Y 0xC8          /* 9D60 : sous l'eau */
#define ENEMY_TYPE2_NEAR 0x18       /* 9D6E : X decale de 32 px vers le centre si |C427 - X| < 24 */
#define ENEMY_TYPE2_JUMP_VY (-0x0B80) /* A2F1 : +7/+8 = F480 */
#define ENEMY_TYPE2_GRAVITY 0x0080  /* A32D, 9CA3 */
#define ENEMY_TYPE2_VX 0x0180       /* A3EA/A3F9 */
#define ENEMY_TYPE2_WATER 0xC0      /* A383 : sans sol sous Y C0, il replonge */
#define ENEMY_TYPE2_STEP 8          /* A414 : une pose de marche par 8 mises a jour */
#define ENEMY_FISH_KEEP_POSE 0x01   /* +18 : tombe d'un bord, garde sa pose */
#define ENEMY_FISH_RIGHT 0x02       /* +12 : tourne vers la droite */
#define ENEMY_FISH_STEP 0x04        /* +14 & 1 : seconde pose de marche */
#define ENEMY_FISH_XHALF 0x08       /* +4 = 80 : demi-pixel de X (vx = 1,5 px) */
#define ENEMY_SPLASH_VX 0x0100      /* 9C90 : goutte impaire (CF30) +1, paire -1 */
#define ENEMY_SPLASH_VY_ODD (-0x0500)
#define ENEMY_SPLASH_VY_EVEN (-0x0600)
#define ENEMY_VY_MAX 0x07FF         /* A549 */
#define ENEMY_SPLASH_PATTERN 0x90

/* Pose B : a partir de l'age 15, par blocs de 8 images (observe). */
static EnemyU8 poseOf(EnemyU16 age)
{
    if (age >= 15 && ((age - 15) % 16) < 8) return 1;
    return 0;
}

/*75ED then7EBC: preserve both requests without changing EnemyState166. */
EnemyU8 Enemies_itemSoundCount,Enemies_itemSounds[2];
static void contactSound(EnemyState *s,EnemyU8 id){
 if(Enemies_itemSoundCount>=2)__builtin_trap();
 Enemies_itemSounds[Enemies_itemSoundCount++]=id;s->itemSound=id;
}
Enemy vk_projectiles[ENEMY_PROJECTILE_MAX];
static EnemyLootHandler loot_handler;
void Enemies_loot_handler(EnemyLootHandler callback){loot_handler=callback;}
#include "generated/c800_sat.h"
/* SAT18 bitmap: C800 scans0..13, D700 scans17..10 (four overlapping entries).
 * These are logical reservations, not raw VRAM.  Traced on the ROM (research/sat-owners-lot15.md):
 * - CE03 is only read (9FC2) and never written (tools/verify_sat_colour_writers.py, openMSX: 0 in every room of stages 1-18), so D700 always
 *   scans 17..10 and sat_boss stays 0 in play (the 17..16 restriction is kept for the pools tests only);
 * - colour bytes (+25+5k, 651B turns a zero low nibble into E1): the only writers that can zero a layer are 09h (AF4A/B048 zero, AF7E
 *   restores; rows per state in c800_sat.h) and the stage 18 body 11h (ACB2); the other writers (26h flame 9C2A, statue A28A, 14h flash
 *   B96F) never store a zero nibble;
 * - bodies held by bosses 12h/13h/14h x2: actors.c boss15_pool_sync, tools/verify_boss_bodies_sat.py;
 * - species of stages 2-11 that keep the object Y: rows of the poses met by tools/run_species_trace.py (research/legacy-species-msx.json) plus
 *   the poses the ROM handlers reach without the traces showing them (build_c800_sat.py LEGACY_EXTRA, checked by tools/verify_legacy_poses.py);
 * - the stage 6/9/18 D700 shots and the stage 18 body (11h): held by actors.c through Enemies_boss15_reserve, compared with 5F1D/9F6D/99F6 by
 *   tests/run_boss_bodies_sat.sh (including the stage 12 rider 15h + 18h and its 0Ah shots);
 * PROVISIONAL, limited to: the stage 12 death flame that replaces a body, the other stage 18 C800 objects
 * (44h/45h/46h/30/23 of dracula.c) and types 01/04, whose port Y is a SAT Y (their entries stay reserved). */
static unsigned long sat_used,c800_sat[ENEMY_MAX],d700_sat[ENEMY_PROJECTILE_MAX];
static EnemyU8 sat_boss,renderStage;
/* LOT14G: boss15 owns physical C800[0] and D700 slots in this same allocator. */
static unsigned long boss15_sat[10]; /* 0: C800[0], 1..8: D700[0..7], 9: C800[1] (second 14h walker of stage 9) */
static int isProjectile(EnemyU8 type);
static EnemyU8 satCount(EnemyU8 type);
static unsigned long *sat_owner(int i){return i<ENEMY_MAX?&c800_sat[i]:&d700_sat[i-ENEMY_MAX];}
static unsigned long sat_reserve(int projectile,int count){
 int i;unsigned long mask=0;
 for(i=projectile?17:0;projectile?i>=(sat_boss?16:10):i<14;projectile?i--:i++){
  unsigned long bit=1UL<<i;if(sat_used&bit)continue;sat_used|=bit;mask|=bit;if(--count==0)return mask;
 }
 return 0; /* already marked free entries remain reserved, exactly5F61/9FCA */
}
static void sat_sync(EnemyState *s){
 /* Same two passes as before (release the inactive owners, then assign in index order), without the per-slot calls: this runs several
  * times per frame and in every allocation. */
 int i;Enemy *e;unsigned long *o;
 for(i=0,e=s->enemy,o=c800_sat;i<ENEMY_MAX;i++,e++,o++)if(!e->active && *o){sat_used&=~*o;*o=0;}
 for(i=0,e=vk_projectiles,o=d700_sat;i<ENEMY_PROJECTILE_MAX;i++,e++,o++)if(!e->active && *o){sat_used&=~*o;*o=0;}
 for(i=0,e=s->enemy,o=c800_sat;i<ENEMY_MAX;i++,e++,o++)if(e->active && !*o)*o=sat_reserve(isProjectile(e->type),isProjectile(e->type)?2:satCount(e->type));
 for(i=0,e=vk_projectiles,o=d700_sat;i<ENEMY_PROJECTILE_MAX;i++,e++,o++)if(e->active && !*o)*o=sat_reserve(isProjectile(e->type),isProjectile(e->type)?2:satCount(e->type));
}
/* count=0 releases; slot0 is the body, slots1..8 are physical D700 indices, slot9 is C800[1].
 * Persistent masks retain holes: do not recompute them from active counts. */
int Enemies_boss15_reserve(EnemyState *s,int slot,int count){
 unsigned long mask;int i,n=0;
 if(slot<0 || slot>9)return 0;
 sat_sync(s);mask=boss15_sat[slot];
 for(i=0;i<18;i++)n+=(int)((mask>>i)&1);
 if(n==count)return 1;
 if(mask){sat_used&=~mask;boss15_sat[slot]=0;}
 if(!count)return 1;
 if(slot==9 ? s->enemy[1].active : slot ? vk_projectiles[slot-1].active : s->enemy[0].active)return -1;
 mask=sat_reserve(slot!=0 && slot!=9,count);if(!mask)return 0;
 boss15_sat[slot]=mask;return 1;
}
/* Executed6445 ->64EC/64E5: 1E/0C pose85/86 has one coloured
 * SAT entry and one colour-zero entry forced toE1 by651B.
 * C800 puts colour8 first; D700 puts it second (descending SAT refs).
 * Rows: c800_sat.h (5F1D + 6445 on the ROM, tools/build_c800_sat.py); boss holds: boss15_sat (see the comment of the allocator above). */
void Enemies_sat_render(EnemyState *s){
 int i;
 if(!sat_used){   /* nothing reserved: only a new object could change anything */
  for(i=0;i<ENEMY_MAX;i++)if(s->enemy[i].active)break;
  if(i==ENEMY_MAX){for(i=0;i<ENEMY_PROJECTILE_MAX;i++)if(vk_projectiles[i].active)break;if(i==ENEMY_PROJECTILE_MAX)return;}
 }
 sat_sync(s);
 for(i=0;i<ENEMY_ALL;i++){
  Enemy *e=Enemies_slot(s,(EnemyU8)i);unsigned long mask=*sat_owner(i);int j;
  if(!e->active || !mask)continue;
  sat_used|=mask;
  if(i>=ENEMY_MAX){
   EnemySprite sprites[ENEMY_SPRITES];int n=Enemies_sprites(s,(EnemyU8)i,sprites);
   if(!n || (EnemyU8)(sprites[n-1].y-1)!=0xE0)continue;
   /*9F6D colours: bones42/0C and return shots42/05 both have
    * nonzero layers. Fireballs/0C have00/08, only the lower ref frees. */
   if(e->type==ENEMY_TYPE_BONE || e->type==ENEMY_TYPE_10_SHOT){sat_used&=~mask;continue;}
  }else{
   unsigned int row;int found=0;
   if(e->type==ENEMY_TYPE_DRAGON){
    EnemySprite sprites[ENEMY_SPRITES];int entry=0;Enemies_sprites(s,(EnemyU8)i,sprites);
    /*AACD/AB0E: four dynamic cells, each has colours02/4C. */
    for(j=0;j<18 && entry<8;j++)if(mask&(1UL<<j)){
     if((EnemyU8)(sprites[entry/2].y-1)==0xE0)sat_used&=~(1UL<<j);
     entry++;
    }
    continue;
   }
   if(e->type<sizeof(c800_sat_ranges)/sizeof(c800_sat_ranges[0]))for(row=c800_sat_ranges[e->type].first;row<(unsigned int)c800_sat_ranges[e->type].first+c800_sat_ranges[e->type].count;row++){
    const VKC800SATLayout *layout=&c800_sat_layouts[row];int entry=0;
    if(layout->pose!=e->pose)continue;
    for(j=0;j<18 && entry<layout->count;j++)if(mask&(1UL<<j)){
     if((layout->visible&(1U<<entry)) && (EnemyU8)(e->y+layout->dy[entry])==0xE0)sat_used&=~(1UL<<j);
     entry++;
    }
    found=1;break;
   }
   if(found)continue;
   if(e->type!=ENEMY_TYPE_DEATH)continue;
   if((EnemyU8)(e->y-(e->pose==0x86?15:16))!=0xE0)continue;
  }
  for(j=0;j<18;j++)
   if(mask&(1UL<<j)){sat_used&=~(1UL<<j);break;}
 }
}
void Enemies_sat_boss(EnemyU8 on){sat_boss=on;}
unsigned long Enemies_sat_mask(EnemyState *s){sat_sync(s);return sat_used;}
Enemy *Enemies_slot(EnemyState *s,EnemyU8 i){return i<ENEMY_MAX?&s->enemy[i]:i<ENEMY_ALL?&vk_projectiles[i-ENEMY_MAX]:0;}
const Enemy *Enemies_object(const EnemyState *s,EnemyU8 i){return i<ENEMY_MAX?&s->enemy[i]:i<ENEMY_ALL?&vk_projectiles[i-ENEMY_MAX]:0;}
static int objectIndex(const EnemyState *s,const Enemy *e){int i;for(i=0;i<ENEMY_MAX;i++)if(e==&s->enemy[i])return i;for(i=0;i<ENEMY_PROJECTILE_MAX;i++)if(e==&vk_projectiles[i])return ENEMY_MAX+i;return -1;}

static void clearEnemy(Enemy *e)
{
    e->active = 0;
    e->exiting = 0;
    e->type = 0;
    e->extraPhase = 0;
    e->x = 0;
    e->y = 0;
    e->dx = 0;
    e->sub = 0;
    e->age = 0;
    e->vy = 0;
    e->state = 0;
    e->anim = 0;
    e->pose = 0;
    e->romBox = 0;
    e->timer = 0;
}

void Enemies_setRoom(EnemyState *s)
{
    EnemyU8 i;
    for (i = 0; i < ENEMY_ALL; i++) clearEnemy(Enemies_slot(s,i));
    {int j;sat_used=0;sat_boss=0;for(j=0;j<10;j++)boss15_sat[j]=0;for(j=0;j<ENEMY_MAX;j++)c800_sat[j]=0;for(j=0;j<ENEMY_PROJECTILE_MAX;j++)d700_sat[j]=0;}
    s->parity = 0;
    s->type4Count = ENEMY_TYPE4_COUNT;   /* 9CA9 : CF06 = 08, CF07 = 00 */
    s->type4Side = ENEMY_SIDE_RIGHT;
    s->type5Timer = 0;
    s->type4Shots = 0;
    s->type2Shots = 0;               /* 9CA9 au chargement de la salle : CF02 = 04, CF03 = 00 */
    s->type2Frames = 0;
    s->type2Count = ENEMY_TYPE2_COUNT;
}

static void s8Reset(void);
static void fish3Reset(void);    /* enemies_fish3.inc */

static void deathInit(void);     /* enemies_death.inc : CF40 et la file des objets laisses */

void Enemies_init(EnemyState *s)
{
    Enemies_setRoom(s);
    renderStage=0;
    deathInit();                     /* CF40 = 0 au debut de la partie (44C6) */
    fish3Reset();
    s8Reset();                       /* octets d'objet du stage 8 : memoire effacee au debut de la partie (44C6) */
    s->splashCount = 0;              /* CF30 : 0 au depart (premiere goutte du stage 2 a droite, trace) */
    s->energy = ENEMY_ENERGY_MAX;
    s->invuln = 0;
    s->invulnPhase = 0;
    s->recoil = 0;
    s->recoilDir = 0;
    Enemies_death(s);
    s->itemPhase = 0;
    s->contactScore = 0;
    s->itemSound = 0;
    Enemies_itemSoundCount=Enemies_itemSounds[0]=Enemies_itemSounds[1]=0;
    s->fishSoundCount = 0;
    s->tick = 0;
    s->silhouette = 0;
    s->hidden = 0;
    s->flash = 0;
    s->border = 0;
    s->timeStopEnd = 0;
}

void Enemies_death(EnemyState *s)
{
    s->killTimer = 0;                /* 70C7 */
    s->guardTimer = 0;               /* 70CA */
    s->timeStop = 0;                 /* 70CD */
    s->spawnPause = 0;               /* 70D0 */
    s->frozen = 0;                   /* 70D3 : D010 a 0, sans son FC */
    s->armor = 0;                    /* 70DC (C701 bit 4) et 70EE (C441) ; C701 &= 80h
                                      * (objet 0A compris) : vk_candles_death */
}

int Enemies_useTimeStop(EnemyState *s, EnemyU8 *hearts, EnemyU8 c431, EnemyU8 c701)
{
    if (!(c701 & ENEMY_TIME_STOP_ITEM)) return 0;       /* 713B : C701 bit 6 */
    if (s->timeStop) return 0;                          /* 715F : C43B != 0 */
    if (*hearts < ENEMY_TIME_STOP_HEARTS) return 0;     /* 7164 : C417 < 5 */
    *hearts = (EnemyU8)(*hearts - ENEMY_TIME_STOP_HEARTS);  /* 7170-7179, puis 4566 (HUD) */
    s->timeStop = (c431 & 0x04) ? ENEMY_TIME_STOP_LONG : ENEMY_TIME_STOP;   /* 717C-7187 */
    s->frozen = 1;                                      /* 718A : SET 0,(D010) */
    /* 710D tourne dans une passe de la routine de Simon, qui baisse C43B aussitot (75F2,
     * 5A puis 59 a la meme image dans la trace) : la passe des minuteries est calee sur
     * cette image (decalage d'une image au plus des autres minuteries d'objet). */
    s->itemPhase = 1;
    return 1;
}

static void crossClear(EnemyState *s);

void Enemies_item(EnemyState *s, EnemyU8 kind, EnemyU8 c431)
{
    const EnemyU8 timer = (c431 & 0x04) ? ENEMY_ITEM_TIMER_LONG : ENEMY_ITEM_TIMER;
    switch (kind) {
    case 0x03:                       /* 8D98 : C701 bit 4 (bit 5 efface), C441 = 10h */
        s->armor = ENEMY_ARMOR_HITS;
        break;
    case 0x04:                       /* 8DA1: C441=10h, D700 interception8610 */
        s->armor = ENEMY_ARMOR_HITS;
        break;
    case 0x05:                       /* 8DBB -> 7806 : tous les objets C800 et D700 detruits
                                      * (9A3A, sans points) ; 8DC5 : C43E = 18h, eclair */
        crossClear(s);
        s->flash = ENEMY_FLASH_START;
        break;
    case 0x07:                       /* 8DDE : 4605 avec B = 08 */
    case 0x16:                       /* 8E0D : 4605 avec B = 20h */
        {
            const int energy = s->energy + (kind == 0x07 ? ENEMY_ITEM_ENERGY : ENEMY_ENERGY_MAX);
            s->energy = (EnemyU8)(energy > ENEMY_ENERGY_MAX ? ENEMY_ENERGY_MAX : energy);
        }
        break;
    case 0x08: s->guardTimer = timer; break;    /* 8DCD */
    case 0x09: s->killTimer = timer; break;     /* 8DE5 */
    /* 0A (8D94 : C701 |= 40h, sans minuterie) : seulement le bit de C701 (candles.c). */
    /* 10, 11 (livres a croix, 8E1D / 8E26) : seulement C702 bit 6 / 7, lu par le seul
     * marchand (9418) ; aucun effet sur les ennemis (trace openMSX, castle-candles.md 9.8). */
    case 0x06:                       /* 8D7C */
    case 0x1A: case 0x1B: case 0x1C: case 0x1D: /* 8D70 -> 8D7C */
        s->spawnPause = timer;
        break;
    default: break;
    }
}

void Enemies_spawnTimerInit(EnemySpawnTimer *t)
{
    /* 44C6 : la RAM C405-E000 est effacee en debut de partie, CF10 compris (trace :
     * CF10 = 0 jusqu'au premier appel de 9CC3, dans la salle 0 du hall). */
    t->phase = 0;
    Enemies_spawnTimerRoom(t);
}

void Enemies_spawnTimerRoom(EnemySpawnTimer *t)
{
    /* 9CA9 (table 9CBB) : CF00 = 04, CF01 = 00. CF10 n'est ecrit que par 9CC3. */
    t->counter = 4;
    t->index = 0;
}

int Enemies_isVisible(const Enemy *e)
{
    if (!e->active) return 0;
    if (e->x < 0 || e->x > 255) return 0;
    return 1;
}

int Enemies_spawn(EnemyState *s, EnemyU8 side, EnemyU8 type, EnemyU8 y, EnemyU8 extraPhase)
{
    EnemyU8 i;
    for (i = 0; i < ENEMY_MAX; i++) {
        if (!s->enemy[i].active) {
            Enemy *e = &s->enemy[i];
            e->active = 1;
            e->exiting = 0;
            e->type = type;
            e->extraPhase = extraPhase;
            e->y = y;
            e->age = 0;
            e->sub = 0;
            e->romBox = 0;
            if (side == ENEMY_SIDE_RIGHT) {
                e->x = ENEMY_SPAWN_X_RIGHT;
                e->dx = -1;
            } else {
                e->x = (type == ENEMY_TYPE_4) ? ENEMY_SPAWN_X_LEFT_T4 : ENEMY_SPAWN_X_LEFT;
                e->dx = 1;
            }
            return (int)i;
        }
    }
    return -1; /* plein : spawn perdu (comportement MSX si plein non verifie) */
}

/* Stage 2 : un appel de 9CC3 puis 9CE6, comme au hall, avec la branche D000 = 2 de 9CFC
 * (9D1B) : E = A0, D = F0, ou 10 si le bit 1 de CF01 (deja incremente par 9CD1) est mis.
 * Trace (scratchpad/s2t1, salle 2) : cotes D, G, G, D, D, G, G, cadence 104/152 images. */
static int stage2Type1Tick(EnemySpawnTimer *t, const EnemyRoomVars *v, EnemyState *s)
{
    EnemyU8 period, c, dCoord;
    EnemyU8 side;
    int k;

    if (!(Enemies_roomFlags(v->stage, v->room) & ENEMY_FLAG_TYPE1)) return 0;
    /* 9CC3 : CF10, puis CF00 tous les 4 appels ; reload = max(periode - 2*D012, 0) + 1. */
    t->phase--;
    if ((t->phase & 0x03) != 0) return 0;
    t->counter--;
    if (t->counter != 0) return 0;
    /* 9CD0-9CD3 : lire CF01 avant son increment, comme au stage 1.
     * Le nouvel index sert ensuite au choix du cote dans 9D1D. */
    period = kEnemySpawnPeriod[t->index & 7];
    t->index++;
    c = (EnemyU8)(v->difficulty << 1);
    t->counter = (EnemyU8)(((period >= c) ? (EnemyU8)(period - c) : 0) + 1);

    /* 9D1D-9D27 : cote selon CF01 apres l'increment. */
    dCoord = (t->index & 0x02) ? 0x10 : 0xF0;
    /* 9E16 : garde selon X de Simon (C427) et D. */
    if (v->simonX >= 0xC0 && dCoord >= 0xC0) return 0;
    if (v->simonX < 0x40 && dCoord < 0x40) return 0;

    side = (dCoord == 0xF0) ? ENEMY_SIDE_RIGHT : ENEMY_SIDE_LEFT;
    k = Enemies_spawn(s, side, side == ENEMY_SIDE_RIGHT ? ENEMY_TYPE_A : ENEMY_TYPE_B,
                      v->stage == 11 ? 0xB0 - 0x20 : v->stage == 3 ? 0xA0 : ENEMY_S2_TYPE1_Y, side == ENEMY_SIDE_RIGHT ? 1 : 15);
    if (k < 0) return 0;
    s->enemy[k].romBox = 1;
    return 1;
}

/* Un appel de 9CC3 (minuterie) puis, si le compte a rebours expire, 9CE6 (spawn). */
int Enemies_spawnTimerTick(EnemySpawnTimer *t, const EnemyRoomVars *v, EnemyState *s)
{
    EnemyU8 idx, period, reload, c, dCoord, yCoord, slot, i;
    EnemyU8 side;
    EnemyU8 type;

    /* 9CFC : D000 = 11 -> 9D13, E = B0h (trace R, salle 0 : bloc en Y B0h). As for E = C0h of stage 1 (Y A0h), the port's
     * Y is E - 20h: with B0h itself the zombies were born 32 px under the floor of room 0, fell and wrapped (tester 2026-11-11). */
    if (v->stage == 2 || v->stage == 3 || v->stage == 11) return stage2Type1Tick(t, v, s);

    /* Tables des autres stages non extraites : pas de tir. */
    if (v->stage != 1 || v->room > 7) return 0;
    /* 5EE7-5EF6 : 9CE6 n'est appele que si bit0 du drapeau de la salle est a 1. */
    if ((kEnemyStage1RoomFlags[v->room] & ENEMY_FLAG_TYPE1) == 0) return 0;

    /* 9CC3 : DEC CF10 ; RET NZ si (CF10 & 3) != 0. */
    t->phase--;
    if ((t->phase & 0x03) != 0) return 0;
    /* 9CC3 : DEC (canal) ; RET NZ tant que le compte a rebours n'est pas nul. */
    t->counter--;
    if (t->counter != 0) return 0;

    /* 9CD0-9CD3 : idx = CF01 puis CF01++ ; periode = table[idx & 7]. */
    idx = t->index;
    t->index++;
    period = kEnemySpawnPeriod[idx & 7];
    /* 9CD8-9CE3 : reload = max(periode - 2*D012, 0) + 1. */
    c = (EnemyU8)(v->difficulty << 1);
    reload = (period >= c) ? (EnemyU8)(period - c) : 0;
    t->counter = (EnemyU8)(reload + 1);

    /* 9CFC : E (Y) et D (X) de spawn. Branche L = D000 = 1 (stage 1). */
    yCoord = 0xC0;
    if (v->room == 1 && v->simonY < 0x88) {
        /* 9D38 -> DE = F060 : cas sans cote (D = F0 = droite). */
        yCoord = 0x60;
        dCoord = 0xF0;
    } else {
        /* 9D2C/9D2E : D = F0 ; D = 10 si C42C != 0. */
        dCoord = (v->sideFlag != 0) ? 0x10 : 0xF0;
    }

    /* 9E16 : garde selon X de Simon (C427) et D. Refus = RET C (9CF6). */
    if (v->simonX >= 0xC0) {
        if (dCoord >= 0xC0) return 0;          /* 9E29-9E2D : D >= C0 -> carry, refus */
    } else if (v->simonX >= 0x40) {
        /* 9E21 : pas de condition */
    } else {
        if (dCoord < 0x40) return 0;           /* 9E22-9E28 : D < 40 -> carry, refus */
    }

    /* 5F1D : premier bloc libre (5F42 : premier bloc a [0] nul). */
    slot = ENEMY_MAX;
    for (i = 0; i < ENEMY_MAX; i++) {
        if (!s->enemy[i].active) { slot = i; break; }
    }
    if (slot == ENEMY_MAX) return 0;           /* plein : spawn perdu */

    /* Le cote donne le type : droite = A (motifs 40), gauche = B (motifs 58). */
    side = (dCoord == 0xF0) ? ENEMY_SIDE_RIGHT : ENEMY_SIDE_LEFT;
    type = (side == ENEMY_SIDE_RIGHT) ? ENEMY_TYPE_A : ENEMY_TYPE_B;

    /* Y de sprite = E - 0x20 (observe pour E = C0 -> A0). */
    yCoord = (EnemyU8)(yCoord - 0x20);
    {
        /* Meme objet que le type 1 du stage 2 (routine A964) : sortie 99E5, contact 8290 et
         * fouet 7DAD de la ROM (trace du hall : suppression a X 06.40 et F1.40, Y C0). */
        const int k = Enemies_spawn(s, side, type, yCoord, (type == ENEMY_TYPE_A) ? 1 : 15);
        if (k < 0) return 0;
        s->enemy[k].romBox = 1;
        return 1;
    }
}

/* Chute : un ennemi sans sol sous lui tombe (ENEMY_FALL_SPEED px par image). Le sol se teste
 * comme celui de Simon (vk_grounded) : centre du sprite en X (sprite X + 8 en repere de jeu),
 * pieds en Y + 31 (bas du sprite de 32 px). Vitesse provisoire. */
static void fall(Enemy *e, const EnemyU8 *map, EnemyU8 stage)
{
    int step;
    for (step = 0; step < ENEMY_FALL_SPEED; step++) {
        if (vk_grounded(map, (VKByte)(e->x + 8), (VKByte)(e->y + 31), stage, 0)) break;
        e->y++;
    }
}

/* Type 5 : sol au point (X, Y) ou (X - 10, Y) (7B98 = 7D2F + 7C5E). Sans carte, sol partout. */
static int pantherGround(const Enemy *e, const EnemyU8 *map, EnemyU8 stage)
{
    if (!map) return 1;
    return vk_tile_solid(vk_sample_tile(map, (VKByte)e->x, e->y), stage, 0) ||
           vk_tile_solid(vk_sample_tile(map, (VKByte)(e->x - 10), e->y), stage, 0);
}

/* Type 5 : vitesse X vers Simon (C427 >= X : droite). */
static void pantherToward(Enemy *e, EnemyU8 simonX)
{
    e->dx = (EnemyS8)(simonX >= e->x ? 1 : -1);
}

/* Type 5 : une mise a jour (une image sur deux), dans l'ordre de la boucle 98EC :
 * routine A87D, deplacement 99B9 (Y += vy, X += vx en 8.8), bornes 99E5. */
static void pantherStep(Enemy *e, const EnemyRoomVars *v)
{
    int y;
    if (e->x < 0x18) {         /* sortie a gauche, avant la routine */
        e->active = 0;
        return;
    }
    if (e->state == 0) {       /* A891 : assise, attend Simon a moins de 64 px (sans condition en Y) */
        const int d = (int)v->simonX - e->x;
        if ((d < 0 ? -d : d) < ENEMY_TYPE5_WAKE) {
            pantherToward(e, v->simonX);
            e->state = 1;
        }
    } else if (e->state == 1) { /* A8AC : course, animation A909, saut au bord du sol */
        const EnemyU8 base = e->dx < 0 ? 0x41 : 0x45;
        if (++e->anim > 12) e->anim = 1;
        e->pose = (EnemyU8)(e->anim <= 3 ? base : e->anim <= 9 ? base + 1 : base - 1);
        if (!pantherGround(e, v->map, v->stage)) {
            e->vy = ENEMY_TYPE5_JUMP_VY;
            e->pose = e->dx < 0 ? 0x42 : 0x46;
            e->state = 2;
        }
    } else {                   /* A8D2 : saut ou chute, pose figee */
        e->vy = (EnemyS16)(e->vy + ENEMY_TYPE5_GRAVITY);
        if (e->vy > ENEMY_TYPE5_VY_MAX) e->vy = ENEMY_TYPE5_VY_MAX;
        if (pantherGround(e, v->map, v->stage)) {
            pantherToward(e, v->simonX);
            e->anim = 0;
            e->state = 1;
            e->y &= 0xF8;
            e->sub = 0;
            e->vy = 0;
        }
    }
    y = (int)((e->y << 8) | e->sub) + e->vy;
    e->y = (EnemyU8)(y >> 8);
    e->sub = (EnemyU8)y;
    e->x += (EnemyS16)(ENEMY_TYPE5_VX * e->dx);
    if (e->y >= 0xE4 || e->x >= 0xF1 || e->x < 0x07) e->active = 0;
}

/* Type 5 : creation (61BB, 5F1F, A85C). Pose assise tournee vers Simon, etat 0. */
static void pantherSpawn(EnemyState *s, EnemyU8 x, EnemyU8 y, EnemyU8 simonX)
{
    EnemyU8 i;
    for (i = 0; i < ENEMY_MAX; i++) {
        Enemy *e = &s->enemy[i];
        if (e->active) continue;
        clearEnemy(e);
        e->active = 1;
        e->type = ENEMY_TYPE_5;
        e->x = x;
        e->y = y;
        e->pose = simonX >= x ? 0x43 : 0x3F;
        return;
    }
}

/* Octet de drapeaux de la salle (85A6 + 2 x D000), 0 hors des stages 1 et 2. */
static EnemyU8 roomFlags(const EnemyRoomVars *v)
{
    if (v->stage == 3 && v->room < 6) return kEnemyStage3RoomFlags[v->room];
    return Enemies_roomFlags(v->stage, v->room);
}

/* Objets des stages 4 a 6, mis a jour par Enemies_frame (objets 06, 08, 07, 0A et la boule de la statue). */
static int isLoot(EnemyU8 type){return type==ENEMY_TYPE_LOOT_HEART || type==ENEMY_TYPE_LOOT_FLAME;}
static int isObjS4(EnemyU8 type)
{
    return isLoot(type) || type == ENEMY_TYPE_6 || type == ENEMY_TYPE_8 || type == ENEMY_TYPE_7 ||
           type == ENEMY_TYPE_STATUE || type == ENEMY_TYPE_FIREBALL ||
           type == ENEMY_TYPE_HOPPER || type == ENEMY_TYPE_LEAPER || type == ENEMY_TYPE_BONE ||
           type == ENEMY_TYPE_FLEA || type == ENEMY_TYPE_GHOST || type == ENEMY_TYPE_FISH3 || type == ENEMY_TYPE_EAGLE || type == ENEMY_TYPE_DRAGON || type == ENEMY_TYPE_12 || type == ENEMY_TYPE_09 || type == ENEMY_TYPE_10 || type == ENEMY_TYPE_10_SHOT;
}

/* Objets D700 (boules et os) : boite 8318 / 8358, degat 1 sans armure, detruits par le contact et le fouet. */
static int isProjectile(EnemyU8 type)
{
    return type == ENEMY_TYPE_FIREBALL || type == ENEMY_TYPE_BONE || type == ENEMY_TYPE_10_SHOT || type == ENEMY_TYPE_SHIELD_SPARK;
}

/* Boite de contact, du fouet et des armes (L, H de 8378 / 8390 / 8318 / 8358, classes 82CB et 830D). */
static void boxOf(EnemyU8 type, int *l, int *h)
{
    switch (type) {
    case ENEMY_TYPE_LOOT_HEART: case ENEMY_TYPE_LOOT_FLAME: case ENEMY_TYPE_8: case ENEMY_TYPE_7: case ENEMY_TYPE_LEAPER: case ENEMY_TYPE_FLEA: case ENEMY_TYPE_GHOST: case ENEMY_TYPE_DRAGON:
        *l = 0x08; *h = 0x10; break;   /* classe 2 de 82CB (objet 1C compris) */
    case ENEMY_TYPE_EAGLE: *l = 0x0C; *h = 0x18; break;   /* classe 4 de 82CB : 83FA */
    case ENEMY_TYPE_FIREBALL: *l = 0x03; *h = 0x06; break;
    case ENEMY_TYPE_12: case ENEMY_TYPE_10: *l = 0x0C; *h = 0x18; break;
    case ENEMY_TYPE_10_SHOT: *l = 0x06; *h = 0x0C; break;
    case ENEMY_TYPE_BONE: *l = 0x06; *h = 0x0C; break;
    default: *l = 0x05; *h = 0x18; break;   /* 06, 0A, 0B : classe 1 */
    }
}

/* Objets a points de vie (+D) dans extraPhase, avec le drapeau ENEMY_T6_HIT (+E bit 0). */
static int hasHp(EnemyU8 type)
{
    return type == ENEMY_TYPE_6 || type == ENEMY_TYPE_7 || type == ENEMY_TYPE_STATUE ||
           type == ENEMY_TYPE_HOPPER || type == ENEMY_TYPE_LEAPER || type == ENEMY_TYPE_FLEA || type == ENEMY_TYPE_GHOST ||
           type == ENEMY_TYPE_FISH3 || type == ENEMY_TYPE_EAGLE || type == ENEMY_TYPE_DRAGON || type == ENEMY_TYPE_12 || type == ENEMY_TYPE_09 || type == ENEMY_TYPE_10;
}

/* Une image pour un ennemi deja actif. */
static void advance(Enemy *e, EnemyU8 frozen)
{
    if (e->type == ENEMY_TYPE_5 || e->type == ENEMY_TYPE_2 || e->type == ENEMY_TYPE_SPLASH || (e->type == ENEMY_TYPE_DEATH || e->type == ENEMY_TYPE_SHIELD_SPARK) ||
        isObjS4(e->type))
        return;                            /* mis a jour par Enemies_frame (carte, Simon) */
    if (e->exiting) {          /* masque a l'image suivant la limite (observe 1126, 1208) */
        e->active = 0;
        return;
    }
    /* 98F9 -> 992F : D010 != 0 et +7E = 1 (objet cree par 5F35) : ni comportement (993B,
     * pose comprise) ni vitesse (99B9) ; l'objet reste dessine a sa place. */
    if (frozen) return;
    e->age++;
    if (e->type == ENEMY_TYPE_4) {
        /* La SAT suit l'objet une image sur deux : 3 px en X et une marche de la
         * courbe Y a chaque age pair (mesure image par image, scratchpad/type4/gfx). */
        if ((e->age & 1) == 0) e->x += (EnemyS16)(3 * e->dx);
        /* La courbe est relative a la Y du spawn (sub = Y du spawn - ENEMY_TYPE4_Y). */
        e->y = (EnemyU8)(kType4Y[(e->age / 2) < ENEMY_TYPE4_Y_STEPS ? e->age / 2 : ENEMY_TYPE4_Y_STEPS - 1] + e->sub);
        /* Derniere position tenue deux images : X 01 (gauche, ages 152-153), E6 (droite, 146-147). */
        if ((e->age & 1) && e->dx < 0 && e->x <= ENEMY_EXIT_X_LEFT_T4) e->exiting = 1;
        if ((e->age & 1) && e->dx > 0 && e->x >= ENEMY_EXIT_X_RIGHT_T4) e->exiting = 1;
        return;
    }
    /* Type 1 : pas de base de 2 px toutes les 2 images, a partir de l'age 3, et 1 px
     * supplementaire tous les 16 images (phase extraPhase). Somme : 229 px en ~218 images. */
    if (e->age >= 3 && (e->age & 1)) {
        e->x += 2 * e->dx;
    }
    if (e->age >= 3 && ((e->age - e->extraPhase) % 16) == 0) {
        e->x += e->dx;
    }
    if (e->romBox) {
        /* 99E5 (X < 07 ou X >= F1) : la derniere position, X SAT 00 ou E7, reste deux
         * images (ages 217-218 et 209-210 en salle 2 du stage 2), puis l'objet disparait. */
        if ((e->age & 1) == 0 && ((e->dx < 0 && e->x <= ENEMY_EXIT_X_LEFT) ||
                                  (e->dx > 0 && e->x >= ENEMY_S2_EXIT_X_RIGHT))) e->exiting = 1;
        return;
    }
    if (e->dx < 0 && e->x <= ENEMY_EXIT_X_LEFT) e->exiting = 1;
    if (e->dx > 0 && e->x >= ENEMY_EXIT_X_RIGHT) e->exiting = 1;
}

void Enemies_update(EnemyState *s)
{
    EnemyU8 i;
    for (i = 0; i < ENEMY_ALL; i++) {
        if (Enemies_slot(s,i)->active) advance(Enemies_slot(s,i), s->frozen);
    }
}

/* Boite de contact 847A (banque 2), appelee par 8290 avec les parametres de la classe de
 * l'ennemi (table 82CB, puis 8378 / 8390 / 83D2 : HL = H L, D = X, E = Y de l'objet) :
 *   |C427 - X| < L + 5 et (C425' - 2 - Y + H) & FF < H + 1A,
 * ou H + 12 si C420 vaut 1 (saut) ou 2 (accroupi), et C425' = C425 - 8 si C420 = 1.
 * Debout, l'ennemi touche de Y = C425 + H - 2 - (H + 19) a C425 + H - 2 ; accroupi, le haut
 * descend de 8 px ; en saut, le bas monte de 8 px. Escalier (3) et chute (4) : boite debout. */
static int romContact(const EnemyRoomVars *v, int objX, int objY, int l, int h)
{
    const int d = (int)v->simonX - objX;
    const int crouchOrJump = v->simonState == ENEMY_SIMON_JUMP || v->simonState == ENEMY_SIMON_CROUCH;
    const int tall = h + (crouchOrJump ? 0x12 : 0x1A);
    const int y = v->simonState == ENEMY_SIMON_JUMP ? (int)v->simonY - 8 : (int)v->simonY;
    l += 5;
    return d > -l && d < l && (EnemyU8)(y - 2 - objY + h) < tall;
}

/* Classes 82CB : types 1, 2, 3 -> 8378 (L = 05, H = 18) ; type 4 -> 8390 (L = 08, H = 10) ;
 * type 5 -> 83D2 (L = 0C, H = 0A). X, Y de l'objet MSX : romBox et type 4 sont stockes en
 * coordonnees de sprite (objet = SAT + 8 en X ; + 31 en Y pour le rang haut du type 1,
 * + 15 pour le type 4), les types 2 et 5 en coordonnees d'objet. */
int Enemies_touches(const Enemy *e, const EnemyRoomVars *v)
{
    if (e->type == ENEMY_TYPE_09 && !e->state) return 0;
    if (isObjS4(e->type)) {
        int l, h;
        boxOf(e->type, &l, &h);   /* 06, 0A, 0B : 8378 ; 08, 07, 0C : 8390 ; boule 8318 ; os 8358 */
        return romContact(v, e->x, e->y, l, h);
    }
    if (e->type == ENEMY_TYPE_5) return romContact(v, e->x, e->y, 0x0C, 0x0A);
    if (e->type == ENEMY_TYPE_2) return romContact(v, e->x, e->y, 0x05, 0x18);
    if (e->type == ENEMY_TYPE_4)
        return romContact(v, e->x + ENEMY_OBJ_X, e->y + ENEMY_TYPE4_OBJ_Y, 0x08, 0x10);
    if (e->romBox) return romContact(v, e->x + ENEMY_OBJ_X, e->y + ENEMY_OBJ_Y, 0x05, 0x18);
    /* Type 1 sans romBox (tests seulement : tous les tirs du port sont romBox) : boites
     * des sprites, Simon 24 x 32 en (X-8, Y-31), ennemi [x, x+16) x [y, y+32). */
    return e->x < v->simonX + ENEMY_SIMON_RIGHT &&
           e->x + 16 > v->simonX - ENEMY_SIMON_HALF_W &&
           e->y < v->simonY + 1 &&
           e->y + ENEMY_TYPE1_ROW_H > v->simonY - ENEMY_SIMON_H;
}

#define ENEMY_SCREEN_TO_GAME_X 8   /* Simon et les ennemis : sprite X = x de jeu - 8 */

static void deathFlame(EnemyState *s, const Enemy *dead);   /* enemies_death.inc : 9A3E, flamme de mort a la place de l'ennemi tue */

int Enemies_whipHit(EnemyState *s, int left, int top, int right, int bottom)
{
    EnemyU8 i;
    int killed = 0;
    for (i = 0; i < ENEMY_ALL; i++) {
        Enemy *e = Enemies_slot(s,i);
        int ex, ey, eh;
        if (!e->active || e->exiting || e->type == ENEMY_TYPE_5 || e->romBox ||
            isObjS4(e->type) ||
            e->type == ENEMY_TYPE_2 || e->type == ENEMY_TYPE_4 || e->type == ENEMY_TYPE_SPLASH ||
            (e->type == ENEMY_TYPE_DEATH || e->type == ENEMY_TYPE_SHIELD_SPARK)) continue;
        ex = e->x + ENEMY_SCREEN_TO_GAME_X;
        ey = e->y;
        eh = ENEMY_TYPE1_ROW_H;
        if (right < ex || left >= ex + 16 || bottom < ey || top >= ey + eh) continue;
        /* Un coup suffit pour le type 1 (zombies, observe par le joueur). */
        e->active = 0;
        deathFlame(s, e);
        killed++;
    }
    return killed;
}

int Enemies_whipHitType5(EnemyState *s, int simonX, int simonY, int facingLeft, int crouched, int chain)
{
    const int reach = chain ? 24 : 20;     /* 84D9 : B = 18h si C416 = 1 */
    const int whipX = facingLeft ? simonX - reach : simonX + reach;
    const int l = 0x0C + (chain ? 16 : 12);   /* 84A7 : L de la classe + 0Ch, 10h si C416 = 1 */
    const int b = crouched ? 10 : 18;
    EnemyU8 i;
    int killed = 0;
    for (i = 0; i < ENEMY_MAX; i++) {
        Enemy *e = &s->enemy[i];
        const int d = whipX - e->x;
        if (!e->active || e->type != ENEMY_TYPE_5) continue;
        if (d <= -l || d >= l || e->y < simonY - b - 3 || e->y > simonY - b + 10) continue;
        e->active = 0;         /* points de vie 1 : mort au premier coup */
        deathFlame(s, e);
        killed++;
    }
    return killed;
}

int Enemies_whipHitType4(EnemyState *s, int simonX, int simonY, int facingLeft, int crouched, int chain)
{
    /* 84D9 : X du fouet du cote du regard (C42C) ; une retenue annule le test (84B6). */
    const int reach = chain ? 24 : 20;     /* 84D9 : B = 18h si C416 = 1 */
    const int whipX = facingLeft ? simonX - reach : simonX + reach;
    const int l = 0x08 + (chain ? 16 : 12);   /* 84A7 : L de la classe + 0Ch, 10h si C416 = 1 */
    const int b = crouched ? 10 : 18;
    EnemyU8 i;
    int killed = 0;
    if (whipX < 0 || whipX > 255) return 0;
    for (i = 0; i < ENEMY_MAX; i++) {
        Enemy *e = &s->enemy[i];
        int d, h;
        if (!e->active || e->exiting || e->type != ENEMY_TYPE_4) continue;
        d = whipX - (e->x + ENEMY_OBJ_X);
        h = (simonY - b - (e->y + ENEMY_TYPE4_OBJ_Y) + 0x10) & 0xFF;
        if (d <= -l || d >= l || h >= 0x14) continue;
        /* Points de vie 1 : mort au premier coup (trace : 13 morts en phase 3, chacune
         * predite par 84A7, research/enemy-behavior.md). */
        e->active = 0;
        deathFlame(s, e);
        killed++;
    }
    return killed;
}

int Enemies_whipHitType1Rom(EnemyState *s, int simonX, int simonY, int facingLeft, int crouched, int chain)
{
    /* 84D9 : X du fouet sur 8 bits ; une retenue (bord d'ecran) annule le test (84B6). */
    const int reach = chain ? 24 : 20;     /* 84D9 : B = 18h si C416 = 1 */
    const int whipX = facingLeft ? simonX - reach : simonX + reach;
    const int l = 0x05 + (chain ? 16 : 12);   /* 84A7 : L de la classe + 0Ch, 10h si C416 = 1 */
    const int b = crouched ? 10 : 18;
    EnemyU8 i;
    int killed = 0;
    if (whipX < 0 || whipX > 255) return 0;
    for (i = 0; i < ENEMY_MAX; i++) {
        Enemy *e = &s->enemy[i];
        int d, h;
        if (!e->active || e->exiting || !e->romBox) continue;
        d = whipX - (e->x + ENEMY_OBJ_X);
        h = simonY - b - (e->y + ENEMY_OBJ_Y) + 24;
        if (d <= -l || d >= l || h < 0 || h >= 28) continue;
        /* Points de vie 1 (+D = 01) : mort au premier coup (trace : 6 morts en phase 3,
         * |X fouet - X| de 0 a 15). Le drop n'est pas porte. */
        e->active = 0;
        deathFlame(s, e);
        killed++;
    }
    return killed;
}

int Enemies_whipHitType2(EnemyState *s, int simonX, int simonY, int facingLeft, int crouched, int chain)
{
    /* 84D9 : X du fouet sur 8 bits ; une retenue (bord d'ecran) annule le test (84B6). */
    const int reach = chain ? 24 : 20;     /* 84D9 : B = 18h si C416 = 1 */
    const int whipX = facingLeft ? simonX - reach : simonX + reach;
    const int l = 0x05 + (chain ? 16 : 12);   /* 84A7 : L de la classe + 0Ch, 10h si C416 = 1 */
    const int b = crouched ? 10 : 18;
    EnemyU8 i;
    int killed = 0;
    if (whipX < 0 || whipX > 255) return 0;
    for (i = 0; i < ENEMY_MAX; i++) {
        Enemy *e = &s->enemy[i];
        const int d = whipX - e->x;
        const int h = simonY - b - e->y + 24;
        if (!e->active || e->type != ENEMY_TYPE_2) continue;
        if (d <= -l || d >= l || h < 0 || h >= 28) continue;
        e->active = 0;         /* points de vie 1 : mort au premier coup */
        deathFlame(s, e);
        killed++;
    }
    return killed;
}

static void projectileKill(EnemyState *s,Enemy *e);
static Enemy *newObject(EnemyState *s, EnemyU8 type);
static int ghostInert(const Enemy *e);   /* enemies_ghost.inc : +E = 0, ni contact ni fouet */

EnemyU8 Enemies_god;
EnemyU8 Enemies_shotPower[2];
EnemyU8 Enemies_killScore;

/* 81CE : points (centaines) de l'objet detruit, premier octet de la paire. Objet D700 : 822A, 100 points. */
static EnemyU8 s4Score(EnemyU8 type)
{
    return type == ENEMY_TYPE_12 ? 20 : type == ENEMY_TYPE_DRAGON ? 10 : type == ENEMY_TYPE_EAGLE ? 4 : (type == ENEMY_TYPE_STATUE || type == ENEMY_TYPE_10) ? 3 : isProjectile(type) || type == ENEMY_TYPE_LEAPER || type == ENEMY_TYPE_GHOST ? 1 : 2;   /* 81CE, 822A */
}

#define ENEMY_T6_HP_MASK 0x7F
#define ENEMY_T6_HIT 0x80          /* +E bit 0 efface : deja touche, 999F le remet */
#define ENEMY_STAGE4_DAMAGE 4      /* 81CE : base 2, doublee */

void Enemies_rearm(EnemyState *s)
{
    EnemyU8 i;
    for (i = 0; i < ENEMY_ALL; i++)
        if (hasHp(Enemies_slot(s,i)->type)) Enemies_slot(s,i)->extraPhase &= (EnemyU8)~ENEMY_T6_HIT;
}

int Enemies_whipHitStage4(EnemyState *s, int simonX, int simonY, int facingLeft, int crouched, int chain, int *hits)
{
    const int reach = chain ? 24 : 20;
    const int whipX = facingLeft ? simonX - reach : simonX + reach;
    const int b = crouched ? 10 : 18;
    EnemyU8 i;
    int killed = 0;
    *hits = 0;
    Enemies_killScore = 0;
    if (whipX < 0 || whipX > 255) return 0;
    for (i = 0; i < ENEMY_ALL; i++) {
        Enemy *e = Enemies_slot(s,i);
        int l, h, d;
        if (!e->active || e->exiting) continue;
        if (!isObjS4(e->type) || isLoot(e->type) || ghostInert(e) || (e->type == ENEMY_TYPE_09 && !e->state)) continue;
        boxOf(e->type, &l, &h);                  /* 84A7 : L de la classe + 0Ch (10h avec la chaine) ; boule 8320, os 8360 */
        l += chain ? 16 : 12;
        d = whipX - e->x;
        if (d <= -l || d >= l || ((simonY - b - e->y + h) & 0xFF) >= h + 4) continue;
        (*hits)++;
        if (hasHp(e->type)) {
            const int hp = (e->extraPhase & ENEMY_T6_HP_MASK) - (chain ? 2 : 1);   /* 7DE6 */
            if (hp > 0) { e->extraPhase = (EnemyU8)((e->extraPhase & ~ENEMY_T6_HP_MASK) | hp | ENEMY_T6_HIT); continue; }
        }
        e->active = 0;
        Enemies_killScore = (EnemyU8)(Enemies_killScore + s4Score(e->type));
        if(isProjectile(e->type))projectileKill(s,e);else deathFlame(s, e);       /* 9A1A / 9A3E */
        killed++;
    }
    return killed;
}

/* Objets 06 et 08, une passe. Type 6 : sub fraction de X, vy vitesse X (+9/+A), timer +C, anim
 * +10, state +11, extraPhase points de vie. Type 8 : sub fraction de Y, timer fraction de X,
 * state cible Y (+10), anim +C, vy vitesse Y, dx sens. */
static int s4Solid(const EnemyRoomVars *v, int x, int y)
{
    return !v->map || vk_tile_solid(vk_sample_tile(v->map, (VKByte)x, (VKByte)y), v->stage, 0);
}

static int s4Remove(const Enemy *e)
{
    return e->y >= 0xE4 || e->x >= 0xF1 || e->x < 7;   /* 99E5 */
}

/* A653 : la difficulte ajoute D012 x 32 a la vitesse, dans son sens. */
static EnemyS16 s4Speed(int speed, EnemyU8 difficulty)
{
    const int extra = (EnemyU8)(difficulty << 5);
    return (EnemyS16)(speed < 0 ? speed - extra : speed + extra);
}

static void type6Step(Enemy *e, const EnemyRoomVars *v)
{
    static const EnemyU8 poses[8] = { 0x50, 0x51, 0x52, 0x51, 0x53, 0x54, 0x55, 0x54 };
    const int left = e->vy < 0;
    const int lead = (EnemyU8)(e->x + (left ? -8 : 0x14));
    int reverse = 0, x16;
    e->timer--;
    if (!s4Solid(v, lead, e->y) && !s4Solid(v, lead - 10, e->y)) reverse = 1;         /* 7B98 */
    else if (v->map && (s4Solid(v, e->x + (left ? -8 : 8), e->y - 8) || s4Solid(v, e->x + (left ? -8 : 8), e->y - 16) ||
                        s4Solid(v, e->x + (left ? -8 : 8), e->y - 24) || s4Solid(v, e->x + (left ? -8 : 8), e->y - 32)))
        reverse = 1;                                                                  /* 7BBE, 7C1A : quatre hauteurs */
    else if (e->x >= 0xF0) reverse = !left;
    else if (e->x < 0x0F) reverse = left;
    if (!reverse) {
        if ((EnemyU8)(v->simonY - e->y + 8) < 0x10 && !(e->state & 1)) {              /* A5DC */
            e->vy = s4Speed(v->simonX < e->x ? -0x160 : 0x160, v->difficulty);
            e->state = 1;
        } else {
            if ((EnemyU8)(v->simonY - e->y + 8) >= 0x10) e->state = 0;
            if (e->timer == 0) { e->timer = 0x60; reverse = 1; }
        }
    }
    if (reverse) e->vy = (EnemyS16)-e->vy;                                            /* A642 */
    e->anim++;
    e->pose = poses[((e->anim >> 2) & 3) + (e->vy < 0 ? 0 : 4)];
    x16 = (e->x << 8) + e->sub + e->vy;                                               /* 99B9 */
    e->x = (EnemyS16)((x16 >> 8) & 0xFF);
    e->sub = (EnemyU8)x16;
    if (s4Remove(e)) e->active = 0;
}

static void type8Step(Enemy *e, const EnemyRoomVars *v)
{
    int vy, y16, x16;
    e->anim++;
    /* A526 : 71, 72 a gauche ; 73, 74 a droite. */
    e->pose = (EnemyU8)(0x71 + ((e->anim & 4) ? 1 : 0) + (e->dx < 0 ? 0 : 2));
    vy = e->vy + (e->y < e->state ? 0x40 : -0x40);                                    /* A53B */
    if (vy >= 0x800) vy = 0x7FF;
    e->vy = (EnemyS16)vy;
    y16 = (e->y << 8) + e->sub + vy;
    e->y = (EnemyU8)(y16 >> 8);
    e->sub = (EnemyU8)y16;
    x16 = (e->x << 8) + e->timer + (int)s4Speed(e->dx < 0 ? -0x280 : 0x280, v->difficulty);
    e->x = (EnemyS16)((x16 >> 8) & 0xFF);
    e->timer = (EnemyU8)x16;
    if (s4Remove(e)) e->active = 0;
}

/* 9DD5 -> 9D9F : comme le canal du type 4, table 9DDF. CF0A et CF0B partagent les champs du
 * type 4 : aucune salle n'a les deux bits (08 et 20). */
static Enemy *newObject(EnemyState *s, EnemyU8 type);
static void batAim(Enemy *e, const EnemyRoomVars *v);

/* 9DC3 (objet 07, table 9DCD) et 9DD5 (objet 08, table 9DDF) passent par le meme 9D9F. */
static void type8Channel(EnemyState *s, EnemySpawnTimer *t, const EnemyRoomVars *v, EnemyU8 type)
{
    static const EnemyU8 periods8[8] = { 0x0C, 0x0C, 0x0C, 0x18, 0x0C, 0x0C, 0x0C, 0x18 };
    static const EnemyU8 periods7[8] = { 0x1C, 0x1C, 0x1C, 0x48, 0x1C, 0x1C, 0x1C, 0x48 };
    const EnemyU8 *const periods = type == ENEMY_TYPE_7 ? periods7 : periods8;
    EnemyU8 period, c, d;
    Enemy *e;
    t->phase--;
    if ((t->phase & 0x03) != 0) return;
    if (--s->type4Count != 0) return;
    period = periods[s->type4Shots & 7];
    s->type4Shots++;
    c = (EnemyU8)(v->difficulty << 1);
    s->type4Count = (EnemyU16)(((period >= c) ? period - c : 0) + 1);
    d = (s->type4Shots & 1) ? 0xF0 : 0x10;
    if (v->simonX >= 0xC0) { if (d >= 0xC0) d = 0x10; }
    else if (v->simonX < 0x40) { if (d < 0x40) d = 0xF0; }
    e = newObject(s, type);
    if (!e) return;
    e->x = d;
    e->y = (EnemyU8)(v->simonY - 8);
    if (type == ENEMY_TYPE_7) {      /* B061 : pose, +C = 1, puis B086 */
        e->extraPhase = 2;           /* +D, trace */
        e->anim = 1;
        e->pose = 0x17;
        batAim(e, v);
        return;
    }
    e->state = e->y;                 /* A4FB */
    e->vy = -0x380;
    e->dx = (EnemyS8)(d < 0x80 ? 1 : -1);
    e->extraPhase = 1;
    e->pose = 0x71;
}

/* B086 : vy -100h au-dessus de Simon + 1, 100h au-dessous de Simon + 1 - 18h, 0 entre les deux ; vx 120h vers Simon.
 * Objet 07 : sub fraction de Y, timer fraction de X, anim le compteur +C, dx le sens de vx. */
static void batAim(Enemy *e, const EnemyRoomVars *v)
{
    const EnemyU8 sy = (EnemyU8)(v->simonY + 1);
    if (sy < e->y) e->vy = -0x100;
    else e->vy = ((EnemyU8)(sy - 0x18) < e->y) ? 0 : 0x100;
    e->dx = (EnemyS8)(e->x >= v->simonX ? -1 : 1);
}

/* B07B puis 99B9 et 99E5 : la pose suit +C avant sa baisse, le but est revu tous les 16 passes. */
static void batStep(Enemy *e, const EnemyRoomVars *v)
{
    int y16, x16;
    e->pose = (EnemyU8)(0x16 + ((e->anim & 4) ? 1 : 0) + (e->dx > 0 ? 2 : 0));         /* B0B5 */
    if (--e->anim == 0) {
        e->anim = 0x10;
        batAim(e, v);                                                                  /* B082 */
    }
    y16 = (e->y << 8) + e->sub + e->vy;
    e->y = (EnemyU8)(y16 >> 8);
    e->sub = (EnemyU8)y16;
    x16 = (e->x << 8) + e->timer + (e->dx < 0 ? -0x120 : 0x120);
    e->x = (EnemyS16)((x16 >> 8) & 0xFF);
    e->timer = (EnemyU8)x16;
    if (s4Remove(e)) e->active = 0;
}

#define ENEMY_STATUE_FLASH 0x40      /* extraPhase : les yeux prennent la couleur 8 au lieu de 12 (A253) */

/* A222 puis A22E. Objet 0A : state +1, timer +10, anim +11, sub +13, pose +B (4 : Simon a gauche, 5 : a droite). */
static void statueSpawn(EnemyState *s, EnemyU8 x, EnemyU8 y, const EnemyRoomVars *v)
{
    Enemy *e = newObject(s, ENEMY_TYPE_STATUE);
    if (!e) return;
    e->x = x;
    e->y = y;
    e->extraPhase = 8;               /* +D, trace */
    e->dx = 1;                       /* +9/+A = 0120 : le recul va a droite (7ED0) */
    e->timer = 8;                    /* A225 */
    e->anim = 0x20;
    e->pose = (EnemyU8)(v->simonX < x ? 4 : 5);
}

static void fireballStep(Enemy *e);

/* 9F6D, type D700 3 : creee en (X, Y - 14h) avec vx +-400h, deplacee dans la meme passe (9E31). */
static void statueFire(EnemyState *s, const Enemy *statue)
{
    Enemy *e = newObject(s, ENEMY_TYPE_FIREBALL);
    if (!e) return;
    if(renderStage>=12)e->romBox=2;
    e->dx = (EnemyS8)((statue->pose & 1) ? 1 : -1);          /* A271 : BIT 0 de +B */
    e->x = statue->x;
    e->y = (EnemyU8)(statue->y - 0x14);
    /* 9E31 suit 98E5 : la boule est deplacee dans la passe de sa creation, la meme que celle de la statue
     * (trace R2 : X et compteur de la statue changent aux memes images). Un emplacement apres celui de la
     * statue est vu par la boucle de Enemies_frame dans cette image (age 0 -> 1, impair) ; sinon on la
     * deplace ici (age 1, saute ensuite). Avant, age 1 pour les deux : la boule marchait une image avant. */
    e->age = 0;
}

static void statueStep(EnemyState *s, Enemy *e, const EnemyRoomVars *v)
{
    e->pose = (EnemyU8)(v->simonX < e->x ? 4 : 5);                                  /* A2A8 */
    if (e->state == 0) {                                                            /* A23A */
        if (--e->timer != 0) return;
        e->state = 1;
        e->anim = 0x18;
        e->timer = (EnemyU8)(0x28 - (EnemyU8)(v->difficulty << 3));
    } else if (e->state == 1) {                                                     /* A253 */
        if (e->anim & 1) e->extraPhase |= ENEMY_STATUE_FLASH;
        else e->extraPhase &= (EnemyU8)~ENEMY_STATUE_FLASH;
        if (--e->anim != 0) return;
        e->sub = 0x12;
        e->state = 2;
        statueFire(s, e);
        e->extraPhase &= (EnemyU8)~ENEMY_STATUE_FLASH;                              /* A288 */
    } else {                                                                        /* A299 */
        if (--e->sub != 0) return;
        e->state = 0;
        statueFire(s, e);
    }
}

/* 9E31 : 9E77 ne fait rien, puis 99B9 et 99E5. */
static void fireballStep(Enemy *e)
{
    if(e->romBox==3)e->romBox=2;
    if (e->state) {          /* tir vise du dragon du stage 11 (enemies_dragon.inc) : vx 16 bits dans (pose, anim), vy, fractions dans timer et sub, 99B9 puis 99E5 */
        const int vx = (EnemyS16)((e->pose << 8) | e->anim);
        const int x16 = ((EnemyU8)e->x << 8) + e->timer + vx, y16 = (e->y << 8) + e->sub + e->vy;
        e->x = (EnemyS16)((x16 >> 8) & 0xFF);
        e->timer = (EnemyU8)x16;
        e->y = (EnemyU8)(y16 >> 8);
        e->sub = (EnemyU8)y16;
        if (e->y >= 0xE4 || e->x >= 0xF1 || e->x < 7) e->active = 0;
        return;
    }
    e->x = (EnemyS16)(e->x + (e->anim ? e->anim : 4) * e->dx);   /* anim : 3 pour la boule de l'homme-poisson (vx 300h), sinon 4 (400h) */
    if (e->x >= 0xF1 || e->x < 7) e->active = 0;
}

/* ---- Stage 8 : objets 0B (B344, B37E) et 0C (A670, A67D), os D700 de type 4 (9F07) ----
 * Etat de la routine ROM qui n'a pas de place dans Enemy : vitesse X (+9/+A), drapeau mobile (+6), compteurs
 * +10, +11, +12, +18, +1E. Un tableau par bloc C800, jamais efface en cours de partie : sur MSX ces octets gardent la
 * valeur de l'objet precedent du meme bloc (99F6 n'efface que +0 et +E). Pour un os, c12 est l'octet +C de
 * l'emplacement D700 (c10 : numero de cet emplacement, 0 a 7). La fraction de Y est dans sub, celle de X dans
 * timer, vy dans vy, Y dans y, X dans x, la pose dans pose, les points de vie dans extraPhase. */
typedef struct { EnemyS16 vx; EnemyU8 mobile, c10, c11, c12, c18, c1e, c14; } S8Ext;
#include "loot.inc"
static VKLoot enemyLoot[ENEMY_MAX];
static void lootSync(Enemy *e,VKLoot *l){
 const VKByte *b=l->raw;e->active=b[0]!=0;e->state=b[1];e->sub=b[2];e->y=b[3];e->timer=b[4];e->x=b[5];e->vy=(EnemyS16)(b[7]|((EnemyU16)b[8]<<8));e->pose=b[11];e->anim=b[12];e->extraPhase=b[13];
}
static void lootSpawn(EnemyState *s,Enemy *e,int type,int kind){VKLoot *l=&enemyLoot[e-s->enemy];vk_loot_init(l,type,(VKByte)e->x,e->y,kind);e->type=type==36?ENEMY_TYPE_LOOT_HEART:ENEMY_TYPE_LOOT_FLAME;e->age=1;lootSync(e,l);}
static S8Ext s8x[ENEMY_MAX];
static S8Ext s8q[ENEMY_PROJECTILE_MAX];
static EnemyU8 s8BoneC[8];       /* +C des 8 emplacements D700, persistent */
static EnemyU8 s7Random(void);   /* enemies_stage7.inc : LD A,R, fixable par les tests */

static S8Ext *ext(const EnemyState *s,const Enemy *e){int i=objectIndex(s,e);if(i<0 || i>=ENEMY_ALL)__builtin_trap();return i<ENEMY_MAX?&s8x[i]:&s8q[i-ENEMY_MAX];}

static void s8Reset(void)
{
    EnemyU8 i;
    for (i = 0; i < ENEMY_MAX; i++) {
        enemyLoot[i].raw[0]=0;
        s8x[i].vx = 0;
        s8x[i].mobile = s8x[i].c10 = s8x[i].c11 = s8x[i].c12 = s8x[i].c18 = s8x[i].c1e = s8x[i].c14 = 0;
    }
    for (i = 0; i < 8; i++) s8BoneC[i] = 0;
    for(i=0;i<ENEMY_PROJECTILE_MAX;i++){
        s8q[i].vx=0;s8q[i].mobile=s8q[i].c10=s8q[i].c11=s8q[i].c12=s8q[i].c18=s8q[i].c1e=s8q[i].c14=0;
    }
}

/* A56C et A653 : vx = DE ; A653 y ajoute D012 x 32 dans le sens de la vitesse, sauf pour 0. */
static void setVx(S8Ext *x, Enemy *e, int vx, const EnemyRoomVars *v, int withDifficulty)
{
    x->vx = (EnemyS16)((withDifficulty && vx) ? s4Speed(vx, v->difficulty) : vx);
    e->dx = (EnemyS8)(x->vx < 0 ? -1 : 1);
}

/* A55D, A549 : vy + DE plafonnee a 07FFh quand l'octet fort est >= 8 et positif. */
static void addVy(Enemy *e, int de)
{
    int vy = e->vy + de;
    if (vy >= 0x800) vy = 0x7FF;
    e->vy = (EnemyS16)vy;
}

/* 99B9 : Y et X sont des mots de 8 bits entiers et 8 bits de fraction, deplaces si l'objet est mobile (+6). */
static void moveObj(Enemy *e, const S8Ext *x)
{
    int y16, x16;
    if (!x->mobile) return;
    y16 = (e->y << 8) + e->sub + e->vy;
    e->y = (EnemyU8)(y16 >> 8);
    e->sub = (EnemyU8)y16;
    x16 = (e->x << 8) + e->timer + x->vx;
    e->x = (EnemyS16)((x16 >> 8) & 0xFF);
    e->timer = (EnemyU8)x16;
}

/* 7B98 : sol en (X, Y) ou (X - 10, Y). */
static int floorAt(const EnemyRoomVars *v, int x, int y)
{
    return s4Solid(v, (EnemyU8)x, y) || s4Solid(v, (EnemyU8)(x - 10), y);
}

/* 7BBE (a droite) et 7C1A (a gauche) avec BC = 080C : quatre points, X +- 8, Y - 12, - 20, - 28, - 36. */
static int wallAhead(const EnemyRoomVars *v, const Enemy *e, int left)
{
    const int px = (EnemyU8)(e->x + (left ? -8 : 8));
    int k;
    if (!v->map) return 0;
    for (k = 0; k < 4; k++)
        if (s4Solid(v, px, (EnemyU8)(e->y - 12 - 8 * k))) return 1;
    return 0;
}

static void hopperSpawn(EnemyState *s, EnemyU8 x, EnemyU8 y, const EnemyRoomVars *v)
{
    Enemy *e = newObject(s, ENEMY_TYPE_HOPPER);
    S8Ext *q;
    if (!e) return;
    q = ext(s, e);
    e->x = x;
    e->y = y;
    e->extraPhase = 4;                         /* +D, table 60E1 */
    setVx(q, e, v->simonX >= x ? 0x240 : -0x240, v, 1);       /* B344 */
    e->vy = 0;                                 /* B356 : A55D avec DE = 0 */
    q->mobile = 1;
    q->c11 = 0;
    /* B344 n'a pas de RET : il tombe dans B364, qui fait +12 + 1 et choisit la pose (verifie sur le desassemblage et
     * par les vecteurs du stage 7 executes dans l'emulateur Z80). */
    q->c12++;
    e->pose = (EnemyU8)((v->simonX < x ? 0x47 : 0x49) + ((q->c12 & 8) ? 0 : 1));
}

static void boneStep(EnemyState *s, Enemy *e);

static void boneSpawn(EnemyState *s, const Enemy *src, int vy, int vx)
{
    Enemy *e = newObject(s, ENEMY_TYPE_BONE);
    S8Ext *q;
    EnemyU8 slot, i;
    int used;
    if (!e) return;
    q = ext(s, e);
    /* Emplacement D700 : le premier libre parmi 8 (9F97). */
    for (slot = 0; slot < 8; slot++) {
        used = 0;
        for (i = 0; i < ENEMY_ALL; i++)
            if (Enemies_slot(s,i)->active && isProjectile(Enemies_slot(s,i)->type) && Enemies_slot(s,i) != e && ext(s,Enemies_slot(s,i))->c10 == slot) used = 1;
        if (!used) break;
    }
    if (slot > 7) slot = 7;
    q->c10 = slot;
    q->c12 = s8BoneC[slot];
    q->mobile = 1;
    e->x = src->x;
    e->y = (EnemyU8)(src->y - 0x10);           /* 9F61 */
    e->vy = (EnemyS16)vy;
    q->vx = (EnemyS16)vx;
    e->dx = (EnemyS8)(vx < 0 ? -1 : 1);
    e->pose = 3;
    /* 9E31 suit 98E5 : l'os est deplace dans la passe de sa creation. Un emplacement apres celui du lanceur est vu
     * par la boucle de Enemies_frame dans cette image (age 0 -> 1) ; sinon on le deplace ici (age 1, saute ensuite). */
    e->age = 0;
}

static void boneStep(EnemyState *s, Enemy *e)
{
    S8Ext *q = ext(s, e);
    q->c12 = (EnemyU8)(q->c12 + 1 >= 12 ? 0 : q->c12 + 1);              /* 9F07 */
    s8BoneC[q->c10 & 7] = q->c12;
    e->pose = (EnemyU8)(0x4B + ((q->c12 >> 2) & 3));
    addVy(e, 0x50);                                                    /* A549 avec DE = 0050 */
    moveObj(e, q);
    if (s4Remove(e)) e->active = 0;
}

#include "enemies_type10.inc"
#include "enemies_type09.inc"

/* B37E : trois etats, 0 marche, 1 saut, 2 lancer. */
static void hopperStep(EnemyState *s, Enemy *e, const EnemyRoomVars *v)
{
    S8Ext *q = ext(s, e);
    const int bx = v->simonX;
    int de, ahead, edge = 0;
    e->pose = (EnemyU8)(bx < e->x ? 0x47 : 0x49);                        /* B364 */
    q->c12++;
    if (!(q->c12 & 8)) e->pose++;
    if (e->state == 1) {                                                /* B451 */
        if (wallAhead(v, e, q->vx < 0)) setVx(q, e, 0, v, 0);
        addVy(e, 0x68);
        if (e->vy < 0) goto done;
        ahead = q->vx < 0 ? 8 : (q->vx >> 8) ? -3 : 5;
        if (!floorAt(v, e->x + ahead, e->y)) goto done;
        e->state = 0;
        e->sub = 0;
        e->y = (EnemyU8)(e->y & 0xF8);                                  /* A9A2 */
        e->vy = 0;
        goto done;
    }
    if (e->state == 2) {                                                /* B4A9 */
        if (--q->c10 != 0) goto done;
        boneSpawn(s, e, (s7Random() & 1) ? -0x800 : -0x500, bx >= e->x ? 0x180 : -0x180);
        e->state = 0;
        q->mobile = 1;
        goto done;
    }
    /* B38D, etat 0 */
    if ((EnemyU8)(e->x + 0x30 - bx) < 0x60) de = bx > e->x ? -0x240 : 0x240;      /* trop pres : recule */
    else if ((EnemyU8)(e->x + 0x50 - bx) < 0xA0) de = 0;                          /* a distance : garde sa vitesse */
    else de = bx > e->x ? 0x240 : -0x240;                                        /* trop loin : approche */
    if (de) {
        setVx(q, e, de, v, 1);                                                   /* B3C3 */
        if (!(q->c12 & 6)) {                                                     /* B3CD : prepare le lancer */
            q->c10 = 0x10;
            e->state = 2;
            q->mobile = 0;
            edge = 1;
        }
    }
    if (!edge) {                                                                 /* B3DB */
        if (q->vx >= 0) { if (wallAhead(v, e, 0)) setVx(q, e, -0x240, v, 1); }
        else if (wallAhead(v, e, 1)) setVx(q, e, 0x240, v, 0);
        ahead = q->vx == 0 ? 4 : q->vx > 0 ? 9 : -4;                             /* B403 */
        if (!floorAt(v, e->x + ahead, e->y)) {                                   /* B41F : saute dans le vide */
            setVx(q, e, q->vx < 0 ? -0x240 : 0x240, v, 0);
            e->vy = (EnemyS16)0xFB8F;
            e->state = 1;
            goto done;
        }
    }
    if (e->x < 0x10) setVx(q, e, 0x240, v, 1);                                   /* B437 */
    else if (e->x >= 0xEE) setVx(q, e, -0x240, v, 1);
    else setVx(q, e, q->vx, v, 1);
done:
    moveObj(e, q);
    if (s4Remove(e)) e->active = 0;
}

static void leaperSpawn(EnemyState *s, EnemyU8 x, EnemyU8 y)
{
    Enemy *e = newObject(s, ENEMY_TYPE_LEAPER);
    S8Ext *q;
    if (!e) return;
    q = ext(s, e);
    e->x = x;
    e->y = y;
    e->extraPhase = 1;                         /* +D, table 60E1 */
    q->c10 = 0x18;                             /* A670 */
    e->pose = 0x89;
    q->c12 = 0;
}

/* A797 : pose 8A, 8B vers la droite, 87, 88 vers la gauche, selon le bit 3 de +12. */
static void leaperPose(Enemy *e, S8Ext *q)
{
    q->c12++;
    e->pose = (EnemyU8)((q->vx < 0 ? 0x87 : 0x8A) + ((q->c12 & 8) ? 1 : 0));
}

/* A82D : bord de l'ecran atteint, pause de 18h passes (etat 2). */
static int leaperEdge(Enemy *e, S8Ext *q)
{
    int edge;
    if (e->vy < 0) edge = e->y < 0x20;
    else edge = e->y >= 0xC0;
    if (!edge) edge = q->vx >= 0 ? e->x >= 0xE8 : e->x < 0x10;
    if (edge) { q->c1e = 0x18; e->state = 2; }
    return edge;
}

/* A7BD : bond. vx 200h vers Simon (X >= C0h : toujours a gauche, X < 40h : toujours a droite), vy selon la table
 * A813 (Simon plus haut) ou A81B, indice +18 & 3 ; le bond va vers le bas s'il part du haut de l'ecran. */
static void leaperAim(Enemy *e, S8Ext *q, const EnemyRoomVars *v)
{
    static const EnemyS16 up[4] = { -0x220, -0x280, -0x300, -0x480 };
    static const EnemyS16 down[4] = { 0x0220, 0x0280, 0x0300, 0x0380 };
    int vx = v->simonX < e->x ? -0x200 : 0x200;
    int vy = v->simonY < e->y ? up[q->c18 & 3] : down[q->c18 & 3];
    if (e->x >= 0xC0) vx = -0x200;
    else if (e->x < 0x40) vx = 0x200;
    setVx(q, e, vx, v, 0);
    if (vy < 0) { if (e->y < 0x40) vy = -vy; }
    else if (e->y >= 0xC0) vy = -vy;
    e->vy = (EnemyS16)vy;
}

static void leaperStep(EnemyState *s, Enemy *e, const EnemyRoomVars *v)
{
    S8Ext *q = ext(s, e);
    q->c18++;
    switch (e->state) {
    case 0:                                                             /* A696 */
        if (--q->c10 == 0) { q->mobile = 1; e->state = 1; leaperAim(e, q, v); }
        break;
    case 1: {                                                           /* A6A5 */
        int vy;
        if (leaperEdge(e, q)) break;
        leaperPose(e, q);
        vy = e->vy < 0 ? e->vy + 0x20 : e->vy - 0x20;
        e->vy = (EnemyS16)vy;
        if (vy == 0) { q->c1e = 0x18; e->state = 2; }
        break;
    }
    case 2:                                                             /* A6D7 */
        leaperPose(e, q);
        q->mobile = 0;
        if (--q->c1e == 0) { q->mobile = 1; e->state = 3; }
        break;
    case 3:                                                             /* A6EA */
        e->state = 1;
        if ((EnemyU8)(v->simonY - e->y) >= 0x18) leaperAim(e, q, v);
        else e->state = 4;
        break;
    case 4:                                                             /* A6FE : charge */
        setVx(q, e, v->simonX < e->x ? -0x300 : 0x300, v, 0);
        e->vy = 0;
        e->state = 5;
        q->c10 = 0x30;
        leaperPose(e, q);
        break;
    case 5:                                                             /* A728 */
        if (leaperEdge(e, q)) break;
        leaperPose(e, q);
        if (--q->c10 == 0) { leaperAim(e, q, v); e->state = 1; break; }
        if (q->vx < 0) {
            q->c11 = 0;
            if ((EnemyU8)(v->simonX - 8) >= (EnemyU8)e->x) e->state = 6;
        } else {
            q->c11 = 1;
            if ((EnemyU8)(v->simonX + 8) < (EnemyU8)e->x) e->state = 6;
        }
        break;
    case 6: {                                                           /* A764 : demi-tour progressif */
        int vx;
        if (leaperEdge(e, q)) break;
        leaperPose(e, q);
        vx = (q->c11 & 1) ? q->vx - 0x20 : q->vx + 0x20;
        q->vx = (EnemyS16)vx;
        if (((vx >> 8) & 0xFF) == ((q->c11 & 1) ? 0xFE : 0x02)) { e->state = 7; q->vx = (EnemyS16)(vx & ~0xFF); }
        break;
    }
    default:                                                            /* A793 */
        if (leaperEdge(e, q)) break;
        leaperPose(e, q);
        break;
    }
    e->dx = (EnemyS8)(q->vx < 0 ? -1 : 1);
    moveObj(e, q);
    if (s4Remove(e)) e->active = 0;
}

#include "enemies_stage7.inc"
#include "enemies_ghost.inc"
#include "enemies_death.inc"
/* 9A1A: free the old D700 block, allocate a type0C flame at first free. */
static void projectileKill(EnemyState *s,Enemy *e)
{
 EnemyS16 x=e->x;EnemyU8 y=e->y;Enemy *f;e->active=0;
 f=newObject(s,ENEMY_TYPE_SHIELD_SPARK);if(!f)return;
 f->x=x;f->y=y;f->state=0;f->extraPhase=0;f->anim=16;f->pose=0x85;f->age=1;
 f->sub=f->timer=0;f->vy=0;f->dx=0;ext(s,f)->vx=0;ext(s,f)->mobile=1;s8BoneC[ext(s,f)->c10]=16;
}

static void crossClear(EnemyState *s)
{
 int i;for(i=0;i<ENEMY_MAX;i++){
  Enemy *e=&s->enemy[i];if(!e->active)continue;
  if(isProjectile(e->type)){e->active=0;continue;}
  if(isLoot(e->type)){
   EnemyU8 kind;vk_loot_pick(&enemyLoot[i],&kind);lootSync(e,&enemyLoot[i]);
   if(loot_handler)loot_handler(s,kind);else Enemies_lootPick=kind;
  }else{int saved=deathCross;e->active=0;deathCross=1;deathFlame(s,e);deathCross=saved;}
 }
 for(i=0;i<ENEMY_PROJECTILE_MAX;i++)if(vk_projectiles[i].active)projectileKill(s,&vk_projectiles[i]);
}


static void type6Spawn(EnemyState *s, EnemyU8 x, EnemyU8 y)
{
    Enemy *e = newObject(s, ENEMY_TYPE_6);
    if (!e) return;
    e->x = x;
    e->y = y;
    e->vy = 0x160;                   /* A573 */
    e->timer = 0x60;
    e->extraPhase = 4;               /* +D, trace */
    e->pose = 0x50;
}


int Enemies_shotHit(EnemyState *s, ShotTouch touch, const void *ctx, int *slot, int *score)
{
    EnemyU8 i;
    for (i = 0; i < ENEMY_ALL; i++) {
        Enemy *e = Enemies_slot(s,i);
        int x, y, h, l, k;
        if (!e->active || e->exiting || isLoot(e->type) || e->type == ENEMY_TYPE_SPLASH || (e->type == ENEMY_TYPE_DEATH || e->type == ENEMY_TYPE_SHIELD_SPARK)) continue;
        /* 82CB puis 8388 / 83AA / 83E2 : classe de l'objet, D = X, E = Y de l'objet MSX. */
        if (isObjS4(e->type)) {
            if (ghostInert(e) || (e->type == ENEMY_TYPE_09 && !e->state)) continue;
            x = e->x; y = e->y;
            boxOf(e->type, &l, &h);
            if (e->extraPhase & ENEMY_T6_HIT) continue;      /* +E bit 0 (7DBF, 8033) */
            for (k = 0; k < 2; k++) {
                int hp;
                if (!touch(ctx, k, x, y, h, l)) continue;
                *slot = k;
                if (isProjectile(e->type)) {                 /* 7F79 -> 822A, 9A1A */
                    projectileKill(s,e);
                    *score = 1;
                    return 1;
                }
                hp = (e->extraPhase & ENEMY_T6_HP_MASK) - Enemies_shotPower[k];    /* 8066 */
                if (hp > 0) {
                    e->extraPhase = (EnemyU8)((e->extraPhase & ~ENEMY_T6_HP_MASK) | hp | ENEMY_T6_HIT);   /* 8083 */
                    *score = 0;
                } else {
                    e->active = 0;
                    *score = s4Score(e->type);
                    if(isProjectile(e->type))projectileKill(s,e);else deathFlame(s, e);       /* 9A1A / 9A3E */
                }
                return 1;
            }
            continue;
        }
        if (e->type == ENEMY_TYPE_5) { x = e->x; y = e->y; h = 0x0A; l = 0x0C; }
        else if (e->type == ENEMY_TYPE_2) { x = e->x; y = e->y; h = 0x18; l = 0x05; }
        else if (e->type == ENEMY_TYPE_4) { x = e->x + ENEMY_OBJ_X; y = e->y + ENEMY_TYPE4_OBJ_Y; h = 0x10; l = 0x08; }
        else { x = e->x + ENEMY_OBJ_X; y = e->y + ENEMY_OBJ_Y; h = 0x18; l = 0x05; }
        for (k = 0; k < 2; k++) {
            if (!touch(ctx, k, x, y, h, l)) continue;
            /* 8066 : +0D - degat (8048 : dague 1, hache 4, croix 2, eau 2) ; les types
             * portes ont 1 point de vie : 81AB (points 81CE : 200 pour le type 2, 100
             * pour les autres) et 9A3E. Un seul ennemi par passe (804C ... RET). */
            e->active = 0;
            *slot = k;
            *score = e->type == ENEMY_TYPE_2 ? 2 : 1;
            if(isProjectile(e->type))projectileKill(s,e);else deathFlame(s, e);       /* 9A1A / 9A3E */
            return 1;
        }
    }
    return 0;
}

/* Entrees SAT d'un objet (table 6057 + type) : 4 pour les types 1, 2 et 5, 2 pour le type 4 et la goutte. */
static EnemyU8 satCount(EnemyU8 type)
{
    if(isLoot(type))return 2;
    if (isProjectile(type)) return 0;            /* objet D700 : ses entrees SAT (D67C) sont a part */
    return (EnemyU8)((type == ENEMY_TYPE_4 || type == ENEMY_TYPE_SPLASH || type == ENEMY_TYPE_8 || type == ENEMY_TYPE_7 ||
                      type == ENEMY_TYPE_LEAPER || type == ENEMY_TYPE_FLEA || type == ENEMY_TYPE_GHOST ||
                      type == ENEMY_TYPE_DEATH) ? 2 : type == ENEMY_TYPE_EAGLE ? 6 : (type == ENEMY_TYPE_DRAGON || type == ENEMY_TYPE_12) ? 8 : 4);   /* table 6057 */
}

/* 5F1D : premier bloc libre, puis assez d'entrees SAT libres parmi les 14 ; sinon rien. */
static Enemy *newObject(EnemyState *s,EnemyU8 type)
{
 int i;Enemy *slot=0;unsigned long mask;
 sat_sync(s);
 if(isProjectile(type)){
  for(i=0;i<ENEMY_PROJECTILE_MAX;i++)if(!vk_projectiles[i].active && !boss15_sat[i+1]){
   int k,busy=0;for(k=0;k<ENEMY_MAX;k++)if(s->enemy[k].active && isProjectile(s->enemy[k].type) && ext(s,&s->enemy[k])->c10==i)busy=1;
   if(!busy){slot=&vk_projectiles[i];break;}
  }
 }else for(i=0;i<ENEMY_MAX;i++)if(!s->enemy[i].active && !(i==0 && boss15_sat[0]) && !(i==1 && boss15_sat[9])){slot=&s->enemy[i];break;}
 if(!slot)return 0;
 mask=sat_reserve(isProjectile(type),isProjectile(type)?2:satCount(type));if(!mask)return 0;
 *sat_owner(objectIndex(s,slot))=mask;clearEnemy(slot);slot->active=1;slot->type=type;
 if(isProjectile(type)){ext(s,slot)->c10=(EnemyU8)i;if(type==ENEMY_TYPE_FIREBALL && renderStage>=12)slot->romBox=2;}
 return slot;
}

/* Goutte (9C66) : CF30 + 1 ; impaire, vers la droite a -5 px, paire, vers la gauche a -6 px.
 * age 0 : pas de mise a jour dans l'image de sa creation. */
static void splashSpawn(EnemyState *s, EnemyU8 x, EnemyU8 y)
{
    Enemy *e = newObject(s, ENEMY_TYPE_SPLASH);
    if (!e) return;
    e->x = x;
    e->y = y;
    s->splashCount++;
    e->dx = (EnemyS8)((s->splashCount & 1) ? 1 : -1);
    e->vy = (s->splashCount & 1) ? ENEMY_SPLASH_VY_ODD : ENEMY_SPLASH_VY_EVEN;
}

/* 99B9 puis 99E5 : Y += vy, X += vx en 8.8 (X n'a que des demi-pixels), suppression si
 * Y >= E4, X >= F1 ou X < 07. */
static void moveObject(Enemy *e, int vx)
{
    const int y = (int)((e->y << 8) | e->sub) + e->vy;
    const int x = (int)e->x * 256 + ((e->extraPhase & ENEMY_FISH_XHALF) ? 0x80 : 0) + vx;
    e->y = (EnemyU8)(y >> 8);
    e->sub = (EnemyU8)y;
    e->x = (EnemyS16)(x >> 8);
    if (x & 0x80) e->extraPhase |= ENEMY_FISH_XHALF;
    else e->extraPhase &= (EnemyU8)~ENEMY_FISH_XHALF;
    if (e->y >= 0xE4 || e->x >= 0xF1 || e->x < 0x07) e->active = 0;
}

/* A549 : vy += 0x80, plafonnee a 0x07FF quand l'octet fort atteint 8. */
static void fallSpeed(Enemy *e)
{
    e->vy = (EnemyS16)(e->vy + ENEMY_TYPE2_GRAVITY);
    if (e->vy > ENEMY_VY_MAX) e->vy = ENEMY_VY_MAX;
}

/* Type 2 : sol au point (X + dx, Y) ou (X + dx - 10, Y) (7B98). Sans carte, sol partout. */
static int fishGround(const Enemy *e, const EnemyRoomVars *v, int dx)
{
    if (!v->map) return 1;
    return vk_tile_solid(vk_sample_tile(v->map, (VKByte)(e->x + dx), e->y), v->stage, 0) ||
           vk_tile_solid(vk_sample_tile(v->map, (VKByte)(e->x + dx - 10), e->y), v->stage, 0);
}

/* A4B3 : pose tournee vers Simon, 0B (droite) si C427 >= X, sinon 08. */
static void fishFace(Enemy *e, EnemyU8 simonX)
{
    e->pose = (EnemyU8)(simonX >= e->x ? 0x0B : 0x08);
}

/* Type 2 : une mise a jour (une image sur deux), routine A310 puis 99B9 et 99E5. */
static void fishStep(EnemyState *s, Enemy *e, const EnemyRoomVars *v)
{
    e->age++;                                  /* +1D */
    if (e->state == 0) {                       /* A31E : saut hors de l'eau ou chute */
        if (!(e->extraPhase & ENEMY_FISH_KEEP_POSE)) fishFace(e, v->simonX);
        e->dx = 0;
        fallSpeed(e);
        if (e->vy >= 0) {                      /* A333 : pas de test pendant la montee */
            if (fishGround(e, v, (e->extraPhase & ENEMY_FISH_RIGHT) ? 8 : 0)) {
                e->y &= 0xF8;                  /* A352 : la fraction de Y est gardee */
                e->timer = 1;
                e->anim = ENEMY_TYPE2_STEP;
                e->extraPhase &= (EnemyU8)~ENEMY_FISH_STEP;
                e->state = 1;
                e->vy = 0;
            } else if (e->y >= ENEMY_TYPE2_WATER) {
                const EnemyU8 x = (EnemyU8)e->x, y = e->y;
                e->active = 0;                 /* A386 : replonge, deux gouttes */
                splashSpawn(s, x, y);
                splashSpawn(s, x, y);
                return;
            }
        }
    } else {                                   /* A3DE : marche */
        if (--e->timer == 0) {                 /* A4CF : sens vers Simon */
            e->timer = kType2Turn[e->age & 3];
            if (v->simonX >= e->x) {
                e->extraPhase |= ENEMY_FISH_RIGHT;
                e->dx = 1;
            } else {
                e->extraPhase &= (EnemyU8)~ENEMY_FISH_RIGHT;
                e->dx = -1;
            }
            e->anim = 1;
        }
        if (--e->anim == 0) {                  /* A414 : pose suivante (tables A452/A456) */
            e->anim = ENEMY_TYPE2_STEP;
            e->extraPhase ^= ENEMY_FISH_STEP;
            e->pose = (EnemyU8)(((e->extraPhase & ENEMY_FISH_RIGHT) ? 0x0B : 0x08) +
                                ((e->extraPhase & ENEMY_FISH_STEP) ? 1 : 0));
        }
        if (!fishGround(e, v, e->dx > 0 ? 8 : 0)) {   /* A43E : bord du sol, chute */
            e->vy = 0;
            e->state = 0;
            e->extraPhase |= ENEMY_FISH_KEEP_POSE;
        }
    }
    moveObject(e, ENEMY_TYPE2_VX * e->dx);
}

/* Type 2 : creation (5F1D, A2E0) en Y C8, pose vers Simon, vy -0x0B80, deux gouttes. */
static void fishSpawn(EnemyState *s, EnemyU8 x, const EnemyRoomVars *v)
{
    Enemy *e = newObject(s, ENEMY_TYPE_2);
    if (!e) return;
    e->x = x;
    e->y = ENEMY_TYPE2_Y;
    fishFace(e, v->simonX);
    e->vy = ENEMY_TYPE2_JUMP_VY;
    splashSpawn(s, x, ENEMY_TYPE2_Y);
    splashSpawn(s, x, ENEMY_TYPE2_Y);
}

#include "enemies_fish3.inc"
#include "enemies_stage11.inc"
#include "enemies_dragon.inc"
#include "enemies_type12.inc"

/* Canal 9D4B (bit 1 du drapeau), un appel par passe de l'etat 5 (une image sur deux, a partir
 * de la premiere) : 9CC3 sur CF02/CF03 comme le canal 9D97 (DEC CF10 partage, puis CF02 tous
 * les 4 appels ; recharge table 9D8F + 1). X de la table 9D87 a CF03 apres l'increment, garde
 * 9D6E. Un tir sans place est perdu, l'index avance. */
static void type2Channel(EnemyState *s, EnemySpawnTimer *t, const EnemyRoomVars *v, EnemyU8 kind)
{
    EnemyU8 period, c, x;
    if (!(++s->type2Frames & 1)) return;
    if (s->spawnPause) return;                      /* 5EB8 : C440 != 0, boucle 5ED8 sautee */
    if (s->frozen) return;                          /* 98E5 : D010 != 0, 5EB8 n'est pas appele */
    t->phase--;                                     /* 9CC3 : DEC (CF10) */
    if ((t->phase & 0x03) != 0) return;
    if (--s->type2Count != 0) return;               /* DEC (CF02) */
    period = kType2Period[s->type2Shots & 7];       /* 9CD0 : CF03 avant l'increment */
    s->type2Shots++;
    c = (EnemyU8)(v->difficulty << 1);
    s->type2Count = (EnemyU16)((period >= c ? period - c : 0) + 1);
    x = kType2X[s->type2Shots & 7];
    if ((EnemyU8)(v->simonX - x + ENEMY_TYPE2_NEAR) < 2 * ENEMY_TYPE2_NEAR)
        x = (EnemyU8)(x < 0x80 ? x + 0x20 : x - 0x20);
    if (kind == 3) fish3Spawn(s, x, v);                 /* 9D52 : C = 3, meme minuterie et meme table que 9D4B */
    else fishSpawn(s, x, v);
}

/* Types 2 et gouttes : tir puis mises a jour, une image sur deux (valeurs impaires de
 * type2Frames, celles des tirs : le type 2 bouge des l'image de sa creation, la goutte a
 * partir de la mise a jour suivante). */
static void type2Frame(EnemyState *s, EnemySpawnTimer *t, const EnemyRoomVars *v)
{
    EnemyU8 i;
    const EnemyU8 channel = (EnemyU8)(roomFlags(v) & (ENEMY_FLAG_TYPE2 | ENEMY_FLAG_TYPE3));
    if (!channel && v->stage != 10) return;
    if (channel) type2Channel(s, t, v, (channel & ENEMY_FLAG_TYPE3) ? 3 : 2);
    else s->type2Frames++;                          /* stage 10, salles sans canal : les objets 21h et les gouttes suivent la meme cadence */
    if (s->type2Frames & 1) {
        for (i = 0; i < ENEMY_MAX; i++) {
            Enemy *e = &s->enemy[i];
            if (!e->active) continue;
            /* Type 2 fige par D010 ; la goutte (+7E = 0, 9C84) tombe toujours. */
            /* Channels/fixed lists run after shared C800: only newborn fish3
             * need their first A310 here. Existing objects were visited above. */
            if (e->type == ENEMY_TYPE_FISH3 && !e->age) { if (!s->frozen) fish3Step(s, e, v); e->age=1; }
            else if (e->type == ENEMY_TYPE_2) { if (!s->frozen) fishStep(s, e, v); }
            else if (e->type == ENEMY_TYPE_SPLASH && e->age) {   /* 9CA3 : gravite seule */
                fallSpeed(e);
                moveObject(e, ENEMY_SPLASH_VX * e->dx);
            }
        }
    }
    for (i = 0; i < ENEMY_MAX; i++)
        if (s->enemy[i].active && s->enemy[i].type == ENEMY_TYPE_SPLASH) s->enemy[i].age = 1;
}

/* Canal 9D97 (bit 3 du drapeau), un appel par passe de l'etat 5 (une image sur deux) :
 * 9CC3 sur CF06/CF07 (CF06 = 8 a l'entree, recharge table 9DBB + 1), cote D = F0 si le bit 0
 * de CF07 (deja incremente) vaut 1, sinon 10 ; 9E16 deplace D de l'autre cote si Simon est a
 * moins de 64 px de ce bord (pas de refus : 9D97 ignore la retenue) ; E = C425 - 8 (9DB2). */
static void type4Channel(EnemyState *s, EnemySpawnTimer *t, const EnemyRoomVars *v)
{
    EnemyU8 period, c, d;
    int k;
    t->phase--;                                     /* 9CC3 : DEC (CF10) */
    if ((t->phase & 0x03) != 0) return;
    if (--s->type4Count != 0) return;               /* DEC (CF06) */
    period = kType4Period[s->type4Shots & 7];       /* 9CD0 : CF07 avant l'increment */
    s->type4Shots++;
    c = (EnemyU8)(v->difficulty << 1);
    s->type4Count = (EnemyU16)(((period >= c) ? period - c : 0) + 1);
    d = (s->type4Shots & 1) ? 0xF0 : 0x10;          /* 9DA5-9DAD */
    if (v->simonX >= 0xC0) { if (d >= 0xC0) d = 0x10; }   /* 9E29-9E2E */
    else if (v->simonX < 0x40) { if (d < 0x40) d = 0xF0; } /* 9E22-9E26 */
    s->type4Side = (d == 0xF0) ? ENEMY_SIDE_RIGHT : ENEMY_SIDE_LEFT;
    k = Enemies_spawn(s, s->type4Side, ENEMY_TYPE_4, (EnemyU8)(v->simonY - ENEMY_TYPE4_Y_FROM_SIMON), 0);
    if (k >= 0) s->enemy[k].sub = (EnemyU8)(s->enemy[k].y - ENEMY_TYPE4_Y);
}

/* Premiere cellule des puces dans le bloc de tuiles : 80 au stage 7, A4 au stage 9 (les cellules 84 a 94 y sont
 * celles de l'objet 0C du stage 8, que le stage 9 garde dans le meme bloc). */
static EnemyU8 fleaBase = 0x80;
static EnemyU8 fireballCell = 0x60;   /* cellule de la boule (D700 type 1, motif F0) : 60 aux stages 6 et 10, 9C au stage 11 (stage11_enemies_md.c) */

int Enemies_frame(EnemyState *s, EnemySpawnTimer *t, const EnemyRoomVars *v)
{
    EnemyU8 guardWarning=0;
    Enemies_itemSoundCount=0;
    EnemyRoomVars w = *v;
    renderStage=v->stage;
    EnemyU8 i;
    int hit = 0;

    /* 6B27/75C0 precedes98E5/5EB8: the channel gate sees C440 AFTER
     * decrement, including the very last pass. Use the channel's30Hz phase. */
    if(s->parity==0 && s->spawnPause)s->spawnPause--;
    s->fishSoundCount = 0; /* une nouvelle image, les requêtes précédentes ont été consommées */
    fleaBase = v->stage == 7 || v->stage == 9 ? 0xA4 : 0x80;   /* stage 7 and 9 blocks: stage 8 cells + flea at A4 */
    fireballCell = v->stage == 11 ? 0x9C : 0x60;
    ghostCell = Enemies_ghostCell(v->stage);

    /* 75F2-7601 (6B2D) : C43B baisse a chaque passe de la routine de Simon (les images
     * ou itemPhase repasse a 0 plus bas), avant la boucle des objets de la meme passe
     * (trace : les ennemis repartent dans la passe ou D010 s'efface). A 0, D010 bit 0
     * efface et son FC. */
    s->timeStopEnd = 0;
    if (s->timeStop && s->itemPhase && --s->timeStop == 0) {
        s->frozen = 0;
        s->timeStopEnd = 1;
    }

    /* 75C0/75E2 precede7E67: expiration is visible to this pass's contact. */
    if(s->itemPhase){
        if(s->killTimer)s->killTimer--;
        if(s->guardTimer && --s->guardTimer==ENEMY_GUARD_WARNING)guardWarning=0x17;
    }
    Enemies_update(s);
    if (v->map) {
        for (i = 0; i < ENEMY_MAX; i++) {
            Enemy *e = &s->enemy[i];
            /* Le type 4 vole : sa hauteur suit kType4Y, sans chute. */
            if (e->active && !e->exiting && !s->frozen && e->type != ENEMY_TYPE_4 &&
                e->type != ENEMY_TYPE_5 && e->type != ENEMY_TYPE_2 && e->type != ENEMY_TYPE_SPLASH &&
                e->type != ENEMY_TYPE_DEATH && e->type != ENEMY_TYPE_SHIELD_SPARK && !isObjS4(e->type))
                fall(e, v->map, v->stage);
        }
    }
    /* Shared98E5 traversal.9B90 can return an earlier IX, so legacy
     * panther/fish3 AI must be revisited too, without advancing frame phase. */
    {
        int revisit_until=-1;
        for(i=0;i<ENEMY_MAX;i++){
            Enemy *e=&s->enemy[i];int again=i<=revisit_until;
            if(!e->active)continue;
            if(isLoot(e->type) && (s->itemPhase || again)){
                VKByte floor[3];VKLoot *l=&enemyLoot[i];int event=vk_loot_step(l,v->map,v->stage,(VKByte)(s->tick+(s->itemPhase?1:0)),floor);
                lootSync(e,l);
                if(event==1 && Enemies_dropCount<ENEMY_DROP_MAX){dropPush(floor[0],floor[1],floor[2]);Enemies_drops[Enemies_dropCount-1].landed=1;}
            }else if((e->type==ENEMY_TYPE_DEATH) && (again || (++e->age&1))){
                int next=deathStep(s,e);
                if(next<i){revisit_until=i;i=next;}
            }else if(e->type==ENEMY_TYPE_5 && (again || (++e->age&1)) && !s->frozen){
                pantherStep(e,v);
            }else if(e->type==ENEMY_TYPE_FISH3 && (again || !(s->type2Frames&1))){
                if(!s->frozen)fish3Step(s,e,v);
                e->age=1; /* prevents a second update in type2Frame */
            }else if(isObjS4(e->type) && !isLoot(e->type) && !isProjectile(e->type) && e->type!=ENEMY_TYPE_FISH3 && (again || (++e->age&1))){
            if(!s->frozen){
            if (e->type == ENEMY_TYPE_12) type12Step(s,e,v);
            else if (e->type == ENEMY_TYPE_09) type09Step(s,e,v);
            else if (e->type == ENEMY_TYPE_10) type10Step(s, e, v);
            else if (e->type == ENEMY_TYPE_6) type6Step(e, v);
            else if (e->type == ENEMY_TYPE_8) type8Step(e, v);
            else if (e->type == ENEMY_TYPE_7) batStep(e, v);
            else if (e->type == ENEMY_TYPE_STATUE) statueStep(s, e, v);
            else if (e->type == ENEMY_TYPE_HOPPER) hopperStep(s, e, v);
            else if (e->type == ENEMY_TYPE_LEAPER) leaperStep(s, e, v);
            else if (e->type == ENEMY_TYPE_BONE) boneStep(s, e);
            else if (e->type == ENEMY_TYPE_FLEA) fleaStep(s, e, v);
            else if (e->type == ENEMY_TYPE_EAGLE) eagleStep(s, e, v);
            else if (e->type == ENEMY_TYPE_DRAGON) dragonStep(s, e, v);
            else if (e->type == ENEMY_TYPE_GHOST) ghostStep(s, e, v);
            else fireballStep(e);
            }
            /*98E5 still calls99E5 when992F suppresses the AI. */
            if(s4Remove(e))e->active=0;
            }
        }
    }

    /* Une passe de l'etat 5 sur deux images, a partir de la premiere image de jeu de la
     * salle : canal type 1 (9CE6, cote alterne par tir, provisoire) puis canal type 4 (9D97,
     * hall salle 4, stage 2 salles 0, 1, 3), dans l'ordre des bits de la boucle 5ED8. */
    /* 98E5 : D010 != 0 saute tout l'appel de 5EB8 (CF10 compris). */
    if (s->parity == 0 && !s->spawnPause && !s->frozen) {   /* 5EB8 : rien tant que C440 != 0 */
        w.sideFlag = v->facingLeft; /* Original 9D30 reads C42C, not CF01. */
        (void)Enemies_spawnTimerTick(t, &w, s);
        if (roomFlags(v) & ENEMY_FLAG_TYPE4) type4Channel(s, t, v);
        if (((v->stage >= 4 && v->stage <= 6) || v->stage == 8 || v->stage == 9) && (roomFlags(v) & 0x20)) type8Channel(s, t, v, ENEMY_TYPE_8);
        if (v->stage == 6 && (roomFlags(v) & 0x10)) type8Channel(s, t, v, ENEMY_TYPE_7);
        if ((v->stage == 11 || (v->stage >= 12 && v->stage <= 18)) && (roomFlags(v) & 0x40)) eagleChannel(s, t, v);   /* 9DE7 */
    }
    s->parity ^= 1;

    /* Type 5 : une panthere par visite des salles 2, 3 et 7, a la premiere image de jeu apres
     * l'etat 9 (62F5 recopie la liste a chaque chargement de salle et la cree en 61BB). */
    if (v->stage >= 1 && v->stage <= 11 && v->room < vk_enemy_room_count[v->stage] &&
        s->type5Timer < ENEMY_TYPE5_DELAY) {
        if (++s->type5Timer == ENEMY_TYPE5_DELAY) {
            EnemyU8 slot;
            for (slot = 0; slot < 4; slot++) {
                const VKFixedEnemy *def = &vk_fixed_enemies[v->stage][v->room][slot];
                if ((def->raw_type & 0x7F) == 5) pantherSpawn(s, def->x, def->y, v->simonX);
                if (v->stage >= 4 && v->stage <= 6 && (def->raw_type & 0x7F) == 6) type6Spawn(s, def->x, def->y);
                if ((v->stage == 6 || v->stage == 8 || v->stage == 9) && (def->raw_type & 0x7F) == 10) statueSpawn(s, def->x, def->y, v);
                if ((v->stage >= 7 && v->stage <= 9) && (def->raw_type & 0x7F) == 11) hopperSpawn(s, def->x, def->y, v);
                if ((v->stage == 7 || v->stage == 8) && (def->raw_type & 0x7F) == 12) leaperSpawn(s, def->x, def->y);
                if ((v->stage == 7 || v->stage == 9 || v->stage == 11) && (def->raw_type & 0x7F) == 13) fleaSpawn(s, def->x, def->y);
                if (v->stage == 10 && (def->raw_type & 0x7F) == 33) fish3SpawnFixed(s, def->x, def->y);
                if (v->stage == 11 && (def->raw_type & 0x7F) == 14) dragonSpawn(s, def->x, def->y);
            }
        }
    }

    /* 6154/5F1D: fixed entries12..18, including09 and12. The late ROM
     * maps/objects and generated sprite banks are connected by LOT11. */
    if (v->stage >= 12 && v->stage <= 18 &&
        v->room < vk_late_enemy_room_counts[v->stage - 12] &&
        s->type5Timer < ENEMY_TYPE5_DELAY) {
        if (++s->type5Timer == ENEMY_TYPE5_DELAY) {
            EnemyU8 slot;
            for (slot = 0; slot < 4; slot++) {
                const unsigned char *b = vk_late_fixed_raw[v->stage - 12][v->room][slot];
                const EnemyU8 type = (EnemyU8)(b[0] & 0x7F);
                const EnemyU8 x = (EnemyU8)((b[1] & 15) << 4);
                const EnemyU8 y = (EnemyU8)(b[1] & 0xF0);
                if (type == 0x0B) hopperSpawn(s, x, y, v);
                else if (type == 0x0D) fleaSpawn(s, x, y);
                else if (type == 0x0E) dragonSpawn(s, x, y);
                else if (type == 0x10) type10Spawn(s, x, y);
                else if (type == 0x09) type09Spawn(s,x,y);
                else if (type == 0x12) type12Spawn(s,x,y);
            }
        }
    }

    /* Type 2 (stage 2, salles 4 et 5) et ses gouttes. */
    type2Frame(s, t, v);

    /* Contact (7E67) : desactive pendant le recul, l'invulnerabilite et C43A (objet 08) ;
     * avec C434 (objet 09) ces gardes sont sautees et le contact detruit l'ennemi.
     * D010 (objet 0A) ne coupe ni le contact ni le fouet (seuls 98F9 et 9E3E le lisent). */
    s->contactScore = 0;
    /* 9E31 follows the complete C800 pass, including newly created shots. */
    for(i=0;i<ENEMY_ALL;i++){
        Enemy *e=Enemies_slot(s,i);
        if(!e->active || !isProjectile(e->type) || !(++e->age&1))continue;
        if(e->type==ENEMY_TYPE_SHIELD_SPARK){e->pose=(e->anim&4)?0x86:0x85;if(--e->anim==0)e->active=0;s8BoneC[ext(s,e)->c10]=e->anim;}
        else if(!s->frozen){
            if(e->type==ENEMY_TYPE_BONE)boneStep(s,e);
            else if(e->type==ENEMY_TYPE_10_SHOT)type10ShotStep(s,e);
            else fireballStep(e);
        }
        /* 9E49/9E4C applies99E5 even when992F skipped the AI. */
        if(s4Remove(e))e->active=0;
    }

    s->itemSound = guardWarning;
    if(guardWarning)contactSound(s,guardWarning);
    { int contact_pool;for(contact_pool=0;contact_pool<2;contact_pool++)
    if (s->killTimer || (s->recoil == 0 && s->invuln == 0 && s->guardTimer == 0 && v->simonState < 5)) {
        for (i = 0; i < ENEMY_ALL; i++) {
            Enemy *e = Enemies_slot(s,i);
            if (!e->active || e->exiting || isProjectile(e->type)!=contact_pool) continue;
            if (e->type == ENEMY_TYPE_SPLASH || (e->type == ENEMY_TYPE_DEATH || e->type == ENEMY_TYPE_SHIELD_SPARK) || ghostInert(e)) continue;   /* goutte, flamme de mort, objet 1C attendant : +E = 0, ni contact ni fouet */
            EnemyU8 damage = e->type == ENEMY_TYPE_10 ? 6 : e->type == ENEMY_TYPE_DRAGON ? DRAGON_DAMAGE : (e->type == ENEMY_TYPE_6 || e->type == ENEMY_TYPE_8 || e->type == ENEMY_TYPE_GHOST || e->type == ENEMY_TYPE_FISH3 || e->type == ENEMY_TYPE_EAGLE || e->type == ENEMY_TYPE_12 || e->type == ENEMY_TYPE_10) ? ENEMY_STAGE4_DAMAGE :
                             (e->type == ENEMY_TYPE_7 || e->type == ENEMY_TYPE_STATUE ||
                              e->type == ENEMY_TYPE_HOPPER || e->type == ENEMY_TYPE_LEAPER || e->type == ENEMY_TYPE_FLEA) ? 2 :   /* 81CE : base 1, doublee */
                             isProjectile(e->type) ? 1 :                                       /* 7F3D : 462B avec B = 1 */
                             e->type == ENEMY_TYPE_5 ? ENEMY_TYPE5_DAMAGE :
                             e->type == ENEMY_TYPE_2 ? ENEMY_TYPE2_DAMAGE : ENEMY_CONTACT_DAMAGE;
            if(isLoot(e->type) && !s->itemPhase)continue;
            if (!isLoot(e->type) && Enemies_god && !s->killTimer)continue;
            if (!Enemies_touches(e, v)) continue;
            if(isLoot(e->type)){vk_loot_pick(&enemyLoot[i],&Enemies_lootPick);lootSync(e,&enemyLoot[i]);if(loot_handler){loot_handler(s,Enemies_lootPick);Enemies_lootPick=0;}break;}
            if (s->killTimer) {
                /* 7EB0 : 81AB (points de la table 81CE, comme au fouet), 9A3E, effet 0D ;
                 * un seul ennemi par passe (JP 509F). */
                /*7E67 and7EE4 each award points.822A (D700) has no0D request. */
                s->contactScore=(EnemyU8)(s->contactScore+(isProjectile(e->type)?1:isObjS4(e->type)?s4Score(e->type):(EnemyU8)(e->type==ENEMY_TYPE_2?2:1)));
                if (isProjectile(e->type)) e->active=0;
                else { e->active = 0; deathFlame(s, e);contactSound(s,0x0D); }
                break;
            }
            /* 8179 : avec C701 bit 4 (objet 03) et Simon tourne vers l'ennemi
             * (C427 - X et C42C), le degat n'est pas double et C441 baisse. X de
             * l'ennemi : centre de sa boite (PROVISOIRE pour les types 1 et 4). Seul le
             * bit 4 compte : C441 a 0 (stage change, 63B7) passe a FFh au coup suivant
             * (819B DEC, 819C JR NZ), l'armure tient alors 256 coups (trace, castle-candles.md 9.8). */
            if (v->armorOn && !isProjectile(e->type)) {
                const int ex = (e->type == ENEMY_TYPE_5 || e->type == ENEMY_TYPE_2 ||
                                isObjS4(e->type)) ? e->x :
                               e->romBox ? e->x + ENEMY_OBJ_X : e->x + 8;
                if ((v->simonX < ex) != (v->facingLeft != 0)) {
                    damage = (EnemyU8)(damage / 2);
                    s->armor--;              /* 819B ; a 0, 819E efface C701 bit 4 (actors.c) */
                }
            }
            s->energy = (s->energy > damage) ? (EnemyU8)(s->energy - damage) : 0;
            s->invuln = ENEMY_INVULN_START;
            s->invulnPhase = 0;
            /* 78CC suit 6F9F dans la passe du coup : hors image de passe, Simon est cache
             * des maintenant si le bit 1 de C003 est mis (sinon voir plus bas). */
            if (s->itemPhase == 0) s->hidden = (EnemyU8)(s->tick & 2);
            s->recoil = 1;    /* C420 := 5 : l'appelant l'efface a la fin du recul */
            /* Panthere assise (dx 0) : recul a l'oppose d'elle (PROVISOIRE, non mesure). */
            /* 7ED1 : C43C = 1 si le bit 7 de +A (vitesse X) est nul. L'objet 06 garde sa vitesse X dans vy. */
            s->recoilDir = e->type == ENEMY_TYPE_6 ? (EnemyS8)(e->vy < 0 ? -1 : 1) :
                           e->dx ? e->dx : (EnemyS8)(v->simonX >= e->x ? 1 : -1);
            if (isProjectile(e->type)) e->active = 0;               /* 7F10 : 99F6 */
            hit = 1;
            break;
        }
    }

    }

    /* 75C0-75E9 : C440, C434 et C43A baissent a chaque passe de la routine de Simon,
     * comme C42D (une image sur deux). C43A tombe a 10h : effet 17. */
    s->itemPhase ^= 1;
    if (s->itemPhase == 0) {
        s->tick++;                   /* C003 (414D), au debut de la passe */
        /* 7604 : tant que C43E != 0, F3EB = 0Eh si le bit 1 de C43E (avant la baisse) est
         * mis, 0 sinon, puis CHGCLR : 24 passes, 4 images de chaque couleur, fin a 0.
         * Second declencheur : 75D4 (6B30) met C43E = 18h quand C445 tombe a 0, 20 passes
         * apres la defaite d'un boss (7E17 : jauge ENEMY C418 a 0). Porte dans boss.c
         * (flash_wait = 14h, VK_BOSS_EV_FLASH) : actors.c pose alors flash ici. */
        if (s->flash) {
            s->border = (s->flash & 2) ? ENEMY_FLASH_COLOR : 0;
            s->flash--;
        }

        /* 790C et 78CC remplissent D400/D600 apres ces baisses ; la passe suivante les
         * envoie en VRAM (trace openMSX, research/castle-candles.md 9.8). L'objet ramasse
         * plus loin (step_candles, comme 7FE2) attend donc la passe suivante. */
        s->silhouette = !(s->tick & 1) ? 0 : s->guardTimer ? ENEMY_SIMON_GUARD_COLOR :
                        s->killTimer ? ENEMY_SIMON_KILL_COLOR : 0;
        /* C42D du MSX baisse dans la passe du coup ; ici invuln baisse a l'image suivante :
         * C42D apres la baisse de la passe = invuln - 1 - invulnPhase. */
        s->hidden = (EnemyU8)((s->tick & 2) && s->invuln > 1 + s->invulnPhase);
    }

    /* C42D ne decroit qu'une image sur deux : 0x5A x 2 = 180 images. */
    if (s->invuln) {
        s->invulnPhase ^= 1;
        if (s->invulnPhase == 0) s->invuln--;
    }
    return hit;
}

EnemyU8 Enemies_simonColor(const EnemyState *s)
{
    /* 7924 / 7932 figes a la passe (RRA de C003) ; une fin de mort (70BF) les coupe. */
    return (s->guardTimer || s->killTimer) ? s->silhouette : 0;
}

EnemyU8 Enemies_simonHidden(const EnemyState *s)
{
    /* 78D2 : bit 1 de C003 ; une mort (7064) remet C42D a 0. */
    return (EnemyU8)(s->invuln && s->hidden);
}

int Enemies_contact(EnemyState *s, const EnemyRoomVars *v, EnemyU8 damage, int from_x, int armored)
{
    if (Enemies_god || s->recoil || s->invuln || s->guardTimer || s->killTimer) return 0;
    if (armored && v->armorOn && (v->simonX < from_x) != (v->facingLeft != 0)) {
        damage = (EnemyU8)(damage / 2);
        s->armor--;
    }
    s->energy = (s->energy > damage) ? (EnemyU8)(s->energy - damage) : 0;
    s->invuln = ENEMY_INVULN_START;
    s->invulnPhase = 0;
    if (s->itemPhase == 0) s->hidden = (EnemyU8)(s->tick & 2);
    s->recoil = 1;
    s->recoilDir = (EnemyS8)(v->simonX >= from_x ? 1 : -1);
    return 1;
}

int Enemies_sprites(const EnemyState *s, EnemyU8 index, EnemySprite out[ENEMY_SPRITES])
{
    const Enemy *e;
    EnemyU8 pose, t, k;

    if (index >= ENEMY_ALL) return 0;
    e = Enemies_object(s,index);
    if (!e->active) return 0;

    if (e->type == ENEMY_TYPE_5) {
        /* Deux cellules 16x16 a deux couches (N, N+4), N = 40 + 16 x (pose - 3F) a gauche,
         * 80 + 16 x (pose - 43) a droite (miroir). Decalages mesures sur 981 images ; lignes
         * Y-15 a Y. Le retard d'affichage MSX d'une mise a jour n'est pas reproduit. */
        const int right = e->pose >= 0x43;
        const int sitting = e->pose == 0x3F || e->pose == 0x43;
        const EnemyU8 n = (EnemyU8)(right ? 0x80 + 16 * (e->pose - 0x43) : 0x40 + 16 * (e->pose - 0x3F));
        const EnemyU8 left = (EnemyU8)(right ? n + 8 : n);
        const EnemyU8 rightCell = (EnemyU8)(right ? n : n + 8);
        const EnemyS16 xl = (EnemyS16)(e->x + (sitting ? (right ? -23 : -9) : -16));
        const EnemyS16 xr = (EnemyS16)(e->x + (sitting ? (right ? -7 : 7) : 0));
        for (k = 0; k < ENEMY_SPRITES; k++) {
            out[k].y = (EnemyU8)(e->y - 15);
            out[k].x = k < 2 ? xl : xr;
            out[k].pattern = (EnemyU8)((k < 2 ? left : rightCell) + (k & 1 ? 4 : 0));
            out[k].bank = 1;
        }
        return ENEMY_SPRITES;
    }

    if (e->type == ENEMY_TYPE_2) {
        /* Une colonne de deux cellules 16x16 a deux couches (N, N+4), lignes Y-31 a Y, X-8 :
         * haut 40 (gauche) ou 68 (droite, miroir) ; bas 48/50 (poses 08/09) ou 70/78 (0B/0C).
         * Mesure sur 5395 images ; la SAT MSX montre l'objet d'une mise a jour plus tot
         * (retard non reproduit, comme pour le type 5). */
        const int right = e->pose >= 0x0B;
        const EnemyU8 top = (EnemyU8)(right ? 0x68 : 0x40);
        const EnemyU8 bottom = (EnemyU8)((right ? 0x70 : 0x48) + ((e->pose == 0x09 || e->pose == 0x0C) ? 8 : 0));
        for (k = 0; k < ENEMY_SPRITES; k++) {
            out[k].y = (EnemyU8)(e->y - (k < 2 ? 31 : 15));
            out[k].x = (EnemyS16)(e->x - 8);
            out[k].pattern = (EnemyU8)((k < 2 ? top : bottom) + (k & 1 ? 4 : 0));
            out[k].bank = 1;
        }
        return ENEMY_SPRITES;
    }

    if (e->type == ENEMY_TYPE_FISH3) return fish3Sprites(e, out);
    if (e->type == ENEMY_TYPE_EAGLE) return eagleSprites(e, out);
    if (e->type == ENEMY_TYPE_12) return type12Sprites(e,out);
    if (e->type == ENEMY_TYPE_DRAGON) return dragonSprites(s, e, out);

    if (e->type == ENEMY_TYPE_6) {
        /* Trace : haut 80 ou A0, bas 88, 90, 98 ou A8, B0, B8 ; lignes Y-31 a Y, X-8. Les deux
         * couches CC sont composees dans le bloc (build_stage4_enemies_md.py). */
        const int right = e->pose >= 0x53;
        out[0].y = (EnemyU8)(e->y - 31);
        out[1].y = (EnemyU8)(e->y - 15);
        out[0].x = out[1].x = (EnemyS16)(e->x - 8);
        out[0].pattern = (EnemyU8)(right ? 0xA0 : 0x80);
        out[1].pattern = (EnemyU8)((right ? 0xA8 + 8 * (e->pose - 0x53) : 0x88 + 8 * (e->pose - 0x50)));
        out[0].bank = out[1].bank = 1;
        return 2;
    }

    if (e->type == ENEMY_TYPE_7) {
        /* Trace (stage 6 salle 0) : poses 16, 17, 18, 19 : motifs A0, A8, B0, B8 ; SAT (X-7, Y-15). */
        out[0].y = (EnemyU8)(e->y - 14);
        out[0].x = (EnemyS16)(e->x - 7);
        out[0].pattern = (EnemyU8)(0xA0 + 8 * (e->pose - 0x16));
        out[0].bank = 1;
        return 1;
    }

    if (e->type == ENEMY_TYPE_STATUE) {
        /* Trace (salle 2) : colonne C0/C4 puis C8/CC, SAT (X-8, Y-32) et (X-8, Y-16) ; le motif C4/CC prend la
         * couleur 8 au lieu de 12 un passage sur deux avant le tir. Les deux couches sont composees. */
        const int flash = (e->extraPhase & ENEMY_STATUE_FLASH) != 0;
        out[0].y = (EnemyU8)(e->y - 31);
        out[1].y = (EnemyU8)(e->y - 15);
        out[0].x = out[1].x = (EnemyS16)(e->x - 8);
        out[0].pattern = (EnemyU8)(flash ? 0x54 : 0x44);
        out[1].pattern = (EnemyU8)(flash ? 0x5C : 0x4C);
        out[0].bank = out[1].bank = 1;
        return 2;
    }

    if (e->type == ENEMY_TYPE_HOPPER) {
        /* Trace (stage 8 salles 2, 3, 7) : poses 47, 48 (gauche) et 49, 4A (droite), colonne de deux cellules SAT
         * (X-8, Y-32) et (X-8, Y-16), deux couches composees dans chaque cellule. */
        const EnemyU8 pose = e->pose < 0x47 ? 0x47 : e->pose;
        out[0].y = (EnemyU8)(e->y - 31);
        out[1].y = (EnemyU8)(e->y - 15);
        out[0].x = out[1].x = (EnemyS16)(e->x - 8);
        out[0].pattern = (EnemyU8)(0x64 + 8 * (pose - 0x47));
        out[1].pattern = (EnemyU8)(0x68 + 8 * (pose - 0x47));
        out[0].bank = out[1].bank = 1;
        return 2;
    }

    if (e->type == ENEMY_TYPE_LEAPER) {
        /* Trace (stage 8 salles 5, 6, 7) : poses 87, 88, 89, 8A, 8B, une cellule en SAT (X-8, Y-15). */
        const EnemyU8 pose = e->pose < 0x87 ? 0x89 : e->pose;
        out[0].y = (EnemyU8)(e->y - 14);
        out[0].x = (EnemyS16)(e->x - 8);
        out[0].pattern = (EnemyU8)(0x84 + 4 * (pose - 0x87));
        out[0].bank = 1;
        return 1;
    }

    if (e->type == ENEMY_TYPE_09) {
        if(!e->pose){ /* Constructor placeholder: E8/02 + E8/45 at X-4,Y-8. */
            out[0].x=(EnemyU8)(e->x-4);out[0].y=(EnemyU8)(e->y-7);out[0].pattern=4;out[0].bank=4;return 1;
        }
        const int asleep = !e->state;
        const int n = asleep ? 2 : 4;
        const EnemyU8 base = (EnemyU8)(asleep ? 0x80+8*(e->pose-0x25) : 0x40+16*(e->pose-0x21));
        for(k=0;k<n;k++) {
            out[k].x=(EnemyS16)(e->x-8);
            out[k].y=(EnemyU8)(e->y-(asleep ? 14 : k<2 ? 31:15));
            out[k].pattern=(EnemyU8)(base+4*k);out[k].bank=1;
        }
        return n;
    }
    if((e->type==ENEMY_TYPE_BONE || e->type==ENEMY_TYPE_10_SHOT) && e->pose==3){
        out[0].x=(EnemyU8)(e->x-8);out[0].y=(EnemyU8)(e->y-11);out[0].pattern=0xF0;out[0].bank=1;return 1;
    }
    if (e->type == ENEMY_TYPE_10 || e->type == ENEMY_TYPE_10_SHOT) {
        const int shot = e->type == ENEMY_TYPE_10_SHOT;
        const EnemyU8 base = shot ? (EnemyU8)(0x80 + 8 * (e->pose - 0x63)) : (EnemyU8)(0x40 + 16 * (e->pose - 0x5F));
        const int n = shot ? 2 : 4;
        for (k = 0; k < n; k++) {
            out[k].y = (EnemyU8)(e->y - (shot ? 14 : (k < 2 ? 31 : 15)));
            out[k].x = shot ? (EnemyU8)(e->x - 8) : (EnemyS16)(e->x - 8);
            out[k].pattern = (EnemyU8)(base + 4*k);
            out[k].bank = 1;
        }
        return n;
    }

    if (e->type == ENEMY_TYPE_FLEA) {
        /* Poses 67, 68, 6A, 6B : une cellule en (X-8, Y-15), cellules A4 a B0 des blocs des stages 7 et 9, 80 a 8C ailleurs. */
        out[0].y = (EnemyU8)(e->y - 14);
        out[0].x = (EnemyS16)(e->x - 8);
        out[0].pattern = (EnemyU8)(fleaBase + 4 * (e->pose < 0x69 ? e->pose - 0x67 : e->pose - 0x68));
        out[0].bank = 1;
        return 1;
    }

    if(isLoot(e->type)){
        const VKLoot *l=&enemyLoot[index];const int heart=e->type==ENEMY_TYPE_LOOT_HEART;
        out[0].x=(EnemyS16)(e->x-(heart?4:8));out[0].y=(EnemyU8)(e->y-(heart?7:14));
        out[0].bank=4;out[0].pattern=(EnemyU8)(heart?0:l->colour==8?1:l->colour==1?2:3);return 1;
    }
    if ((e->type == ENEMY_TYPE_DEATH || e->type == ENEMY_TYPE_SHIELD_SPARK)) {
        /* Trace (stages 1, 2, 4, 6, 7, 8, 9) : une entree SAT (X-8, Y-16) en pose 85 (motif F4), (X-8, Y-15) en pose 86 (motif F8),
         * couleur 8 sur les 16 lignes ; la seconde entree de l'objet est hors ecran. */
        const int b = e->pose == ENEMY_DEATH_POSE_B;
        out[0].y = (EnemyU8)(e->y - (b ? 14 : 15));
        out[0].x = (EnemyS16)(e->x - 8);
        out[0].pattern = b ? ENEMY_DEATH_PATTERN_B : ENEMY_DEATH_PATTERN_A;
        out[0].bank = 2;
        return 1;
    }

    if (e->type == ENEMY_TYPE_GHOST) {
        /* Poses 9B..A2 (une cellule, deux couches composees) en SAT (X-8, Y-15) : cellules B4 (pose impaire) et B8 (paire) du
         * bloc, chargees par la salle (actors.c, vk_ghost_tiles). */
        out[0].y = (EnemyU8)(e->y - 14);
        out[0].x = (EnemyS16)(e->x - 8);
        out[0].pattern = (EnemyU8)((e->pose & 1) ? ghostCell : ghostCell + 4);
        out[0].bank = 1;
        return 1;
    }

    if (e->type == ENEMY_TYPE_BONE) {
        /* Trace (stage 8 salle 2) : poses 4B, 4C, 4D, motifs 90, 98, A0 ; SAT (X-4, Y-15) pour 4B, (X-8, Y-15) sinon. */
        const EnemyU8 pose = e->pose < 0x4B ? 0x4B : e->pose;
        out[0].y = (EnemyU8)(e->y - 14);
        out[0].x = (EnemyU8)(e->x - (pose == 0x4B ? 4 : 8));
        out[0].pattern = (EnemyU8)(0x98 + 4 * (pose - 0x4B));
        out[0].bank = 1;
        return 1;
    }

    if (e->type == ENEMY_TYPE_FIREBALL) {
        /* Trace : motif F0 couleur 8 en SAT (X-4, Y-12), place dans la cellule 60 du bloc. */
        out[0].y = (EnemyU8)(e->y - 11);
        /* La boule de l'homme-poisson (stage 10, anim = 3) : SAT en X - 5 vers la droite et X - 11 vers la gauche (trace R : 725 et 463 images). */
        out[0].x = (EnemyS16)(e->anim ? e->x - (e->dx > 0 ? 5 : 11) : e->x - 4);
        /* Le tir vise du dragon (stage 11, etat 1) : SAT (X - 4, Y - 13) (trace R5, 27 echantillons : un pixel de moins ou de plus selon la fraction). */
        if (e->state) { out[0].y = (EnemyU8)(e->y - 12); out[0].x = (EnemyS16)(e->x - 4); }
        if(e->romBox==2) { out[0].y=(EnemyU8)(e->y-11);out[0].x=(EnemyU8)(e->x-8); }
        out[0].pattern = e->romBox==2 ? 0xF0 : fireballCell;
        out[0].bank = 1;
        return 1;
    }

    if (e->type == ENEMY_TYPE_8) {
        /* Trace : poses 71, 72 : motifs 40, 48. 73, 74 : 50, 58 (voir research/stage4-enemies.md). */
        out[0].y = (EnemyU8)(e->y - 14);
        out[0].x = (EnemyS16)(e->x - 8);
        out[0].pattern = (EnemyU8)(0x40 + 8 * (e->pose - 0x71));
        out[0].bank = 1;
        return 1;
    }

    if (e->type == ENEMY_TYPE_SPLASH) {
        /* Goutte : motif 90 seul (couleur 6), SAT (X-8, Y-15), lignes Y-14 a Y+1 (4004 images). */
        out[0].y = (EnemyU8)(e->y - 14);
        out[0].x = (EnemyS16)(e->x - 8);
        out[0].pattern = ENEMY_SPLASH_PATTERN;
        out[0].bank = 1;
        return 1;
    }

    if (e->type == ENEMY_TYPE_4) {
        /* Une cellule 16x16, deux couches (N et N+4), 4 poses de 16 images. Les motifs
         * portent ENEMY_PATTERN_TYPE4 : leurs numeros recouvrent ceux des types A et B. */
        const EnemyU8 pattern = kType4Patterns[e->dx < 0 ? 0 : 1][(e->age / 16) & 3];
        out[0].y = e->y;
        out[0].x = e->x;
        out[0].pattern = (EnemyU8)(ENEMY_PATTERN_TYPE4 | pattern);
        out[1].y = e->y;
        out[1].x = e->x;
        out[1].pattern = (EnemyU8)(ENEMY_PATTERN_TYPE4 | (pattern + 4));
        out[0].bank = out[1].bank = 0;
        return ENEMY_TYPE4_SPRITES;
    }

    pose = poseOf(e->age);
    t = (e->type == ENEMY_TYPE_B) ? 1 : 0;
    for (k = 0; k < ENEMY_SPRITES; k++) {
        out[k].y = (EnemyU8)(k < 2 ? e->y : e->y + ENEMY_ROW_OFFSET);
        out[k].x = e->x;
        out[k].pattern = kPatterns[t][k < 2 ? 0 : pose][k];
        out[k].bank = 0;
    }
    return ENEMY_SPRITES;
}

/* 8610: no facing-of-projectile test, no immunity/state test. A zero armor
 * counter wraps toFF. Even after the last charge all8 slots are still scanned. */
int Enemies_shield(EnemyState *s,const EnemyRoomVars *v,EnemyU8 *inventory)
{
    EnemyU8 i,slot;int sounds=0;
    if(!(*inventory&0x20))return 0;
    /* 8610 scans physical D700 slots, independently of the shared C array. */
    for(slot=0;slot<8;slot++){
        Enemy *e=0;EnemyU8 raw,target,j;
        for(i=0;i<ENEMY_ALL;i++)if(Enemies_slot(s,i)->active && isProjectile(Enemies_slot(s,i)->type) && ext(s,Enemies_slot(s,i))->c10==slot){e=Enemies_slot(s,i);break;}
        if(!e)continue;
        if(!e->active || !isProjectile(e->type) || e->type==ENEMY_TYPE_SHIELD_SPARK)continue;
        raw=e->type==ENEMY_TYPE_FIREBALL?3:e->type==ENEMY_TYPE_BONE?4:9;
        if(!vk_item_shield_touch(raw,(VKByte)v->simonX,(VKByte)v->simonY,v->facingLeft,(VKByte)e->x,e->y))continue;
        /* 9A1A frees the old slot, then9F6D allocates the first free D700. */
        for(target=0;target<8;target++){
            for(j=0;j<ENEMY_ALL;j++)if(Enemies_slot(s,j)!=e && Enemies_slot(s,j)->active && isProjectile(Enemies_slot(s,j)->type) && ext(s,Enemies_slot(s,j))->c10==target)break;
            if(j==ENEMY_ALL)break;
        }
        if(objectIndex(s,e)>=ENEMY_MAX){
            EnemyS16 x=e->x;EnemyU8 y=e->y;e->active=0;
            e=newObject(s,ENEMY_TYPE_SHIELD_SPARK);
            if(!e){sounds++;if(--s->armor==0)*inventory&=(EnemyU8)~0x20;continue;}
            e->x=x;e->y=y;
        }
        ext(s,e)->c10=target;
        e->type=ENEMY_TYPE_SHIELD_SPARK;e->state=0;e->extraPhase=0;e->anim=16;e->pose=0x85;e->age=1;
        e->sub=0;e->vy=0;e->dx=0;e->timer=0;ext(s,e)->vx=0;ext(s,e)->mobile=1;s8BoneC[ext(s,e)->c10]=16;
        sounds++;if(--s->armor==0)*inventory&=(EnemyU8)~0x20;
    }
    return sounds;
}

/* 89BF uses5F1F with B=content and DE=(candleX+8,candleY+16). */
int Enemies_candleLoot(EnemyState *s,unsigned char kind,unsigned char x,unsigned char y)
{
 Enemy *e=newObject(s,kind==1?ENEMY_TYPE_DEATH:ENEMY_TYPE_LOOT_FLAME);
 if(!e)return 0;
 e->x=(EnemyU8)(x+8);e->y=(EnemyU8)(y+16);
 if(kind==1){
  VKLoot *l=&enemyLoot[e-s->enemy];vk_loot_init(l,30,(VKByte)e->x,e->y,1);
  e->state=1;e->anim=16;e->pose=0x85;e->extraPhase=1;e->age=1;
 }else lootSpawn(s,e,38,kind);
 return 1;
}
