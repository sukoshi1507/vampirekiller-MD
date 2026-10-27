/*
 * Logique pure des ennemis du hall (voir enemies.h et research/enemy-spawn.md, enemy-behavior.md).
 *
 * Traduction des routines Z80 :
 *   - 9CE6 (banque 2) : canal type 1, appele depuis 5EF6 si bit0 du drapeau ROM.
 *   - 9CC3 (banque 2) : minuterie partagee CF10 + compte a rebours CF00.
 *   - 9CFC (banque 2) : position de spawn (E = Y, D = X de spawn).
 *   - 9E16 (banque 2) : garde de spawn selon Simon (C427) et D.
 *   - 9D97 (banque 2) : canal type 4 (salle 4). Garde non verifiee, cadence mesuree.
 *   - 5F1D (banque 0) : creation de l'entite dans le premier bloc libre.
 *
 * Niveau de confiance :
 *   - Tables kEnemy*                                       verifie (ROM)
 *   - Minuterie type 1 (4 appels par decrement, reload = periode - 2*D012 + 1)   verifie
 *   - Garde 9E16                                           verifie code + 6 cas de trace
 *   - Cote = type (A droite, B gauche)                     enemy-behavior 6.3
 *   - Mouvement, animation, sortie du type 1               observe (trace frames.txt)
 *   - Type 4 : cadence 168 images, 1,5 px/image, sortie    observe (4 creations)
 *   - Contact -2, recul 22 images a 2 px, invulnerabilite 180 images   observe (run 3, run 6)
 *
 * PROVISOIRE (non verifie, a confirmer en emulation) :
 *   - Cote du type 1 : alternance par tir (index CF01 & 1). La vraie regle (C42C) n'est pas relevee.
 *   - Boite de contact de Simon : 16 px centres sur X (X-8 a X+7), 32 px au-dessus de Y.
 *     La boite de contact MSX n'est pas mesuree ; l'ennemi touche observe a X ~ 122 pour Simon en 120.
 *   - Recul : 2 px par image dans le sens de l'ennemi, sans blocage des commandes ni arc vertical (C420, C425).
 *   - Type 4 : phase du premier tir (168 images apres l'entree) et absence de garde 9E16 non verifiees.
 *   - Type 5 (salles 2, 3, 7) : non porte (graphisme et declencheur inconnus).
 *   - extraPhase (1 pour A, 15 pour B) : observe, cause inconnue.
 *   - Salle 1 avec Simon haut (E = 60) : condition C425 < 0x88 (module), pas une regle verifiee.
 */
#include "enemies.h"
#include "player.h"

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
#define ENEMY_TYPE4_ROW_H   16
#define ENEMY_TYPE1_ROW_H   32

/* Tables ROM salle de stage 1 (D000 = 1).
 * kEnemyStage1RoomFlags : ROM 0x85CF-0x85D6 (pointeur 85A8 = 85CF pour D000 = 1).
 *   D001 = 0 -> 01, 1 -> 01, 2 -> 00, 3 -> 00, 4 -> 08 (type 4), 5 -> 01, 6 -> 01, 7 -> 00.
 * kEnemySpawnPeriod : ROM 0x9D43-0x9D4A (banque 2). */
const EnemyU8 kEnemyStage1RoomFlags[8] = { 0x01, 0x01, 0x00, 0x00, 0x08, 0x01, 0x01, 0x00 };
const EnemyU8 kEnemySpawnPeriod[8] = { 0x0C, 0x12, 0x0C, 0x0C, 0x0C, 0x12, 0x0C, 0x0C };

#define ENEMY_FLAG_TYPE1 0x01
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
static const EnemyU8 kType5Spawn[8][2] = {
    { 0, 0 }, { 0, 0 }, { 0x80, 0x80 }, { 0x60, 0x40 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0xC0, 0x60 }
};
#define ENEMY_TYPE5_VX 4            /* 0x0400 par mise a jour (une image sur deux) */
#define ENEMY_TYPE5_WAKE 0x40       /* A891 : activation si |C427 - X| < 0x40 */
#define ENEMY_TYPE5_JUMP_VY (-0x0108)
#define ENEMY_TYPE5_GRAVITY 0x00A0
#define ENEMY_TYPE5_VY_MAX 0x07FF

/* Pose B : a partir de l'age 15, par blocs de 8 images (observe). */
static EnemyU8 poseOf(EnemyU16 age)
{
    if (age >= 15 && ((age - 15) % 16) < 8) return 1;
    return 0;
}

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
}

void Enemies_setRoom(EnemyState *s)
{
    EnemyU8 i;
    for (i = 0; i < ENEMY_MAX; i++) clearEnemy(&s->enemy[i]);
    s->parity = 0;
    s->type4Frames = 0;
    s->type4Side = ENEMY_SIDE_RIGHT;
    s->type5Timer = 0;
}

void Enemies_init(EnemyState *s)
{
    Enemies_setRoom(s);
    s->energy = ENEMY_ENERGY_MAX;
    s->invuln = 0;
    s->invulnPhase = 0;
    s->recoil = 0;
    s->recoilDir = 0;
}

void Enemies_spawnTimerInit(EnemySpawnTimer *t)
{
    /* ROM 9CBB : CF00 = 04, CF01 = 00. CF10 non initialise par cette routine :
     * sa valeur de depart n'est pas prouvee ; 0 est choisi ici (provisoire). */
    t->phase = 0;
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

/* Un appel de 9CC3 (minuterie) puis, si le compte a rebours expire, 9CE6 (spawn). */
int Enemies_spawnTimerTick(EnemySpawnTimer *t, const EnemyRoomVars *v, EnemyState *s)
{
    EnemyU8 idx, period, reload, c, dCoord, yCoord, slot, i;
    EnemyU8 side;
    EnemyU8 type;

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
    return Enemies_spawn(s, side, type, yCoord,
                         (type == ENEMY_TYPE_A) ? 1 : 15) >= 0;
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

/* Une image pour un ennemi deja actif. */
static void advance(Enemy *e)
{
    if (e->type == ENEMY_TYPE_5) return;   /* mis a jour par Enemies_frame (carte, Simon) */
    if (e->exiting) {          /* masque a l'image suivant la limite (observe 1126, 1208) */
        e->active = 0;
        return;
    }
    e->age++;
    if (e->type == ENEMY_TYPE_4) {
        /* La SAT suit l'objet une image sur deux : 3 px en X et une marche de la
         * courbe Y a chaque age pair (mesure image par image, scratchpad/type4/gfx). */
        if ((e->age & 1) == 0) e->x += (EnemyS16)(3 * e->dx);
        e->y = kType4Y[(e->age / 2) < ENEMY_TYPE4_Y_STEPS ? e->age / 2 : ENEMY_TYPE4_Y_STEPS - 1];
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
    if (e->dx < 0 && e->x <= ENEMY_EXIT_X_LEFT) e->exiting = 1;
    if (e->dx > 0 && e->x >= ENEMY_EXIT_X_RIGHT) e->exiting = 1;
}

void Enemies_update(EnemyState *s)
{
    EnemyU8 i;
    for (i = 0; i < ENEMY_MAX; i++) {
        if (s->enemy[i].active) advance(&s->enemy[i]);
    }
}

/* Boite de contact : Simon est dessine sur 24 x 32 en (X-8, Y-31) ; ennemi [x, x+16) x [y, y+h).
 * Les deux boites sont celles des sprites affiches (provisoire : la boite MSX n'est pas mesuree).
 * Type 5 : boite de la ROM (8290, classe 2, HL = 0A0C), Simon debout. */
static int touches(const Enemy *e, int simonX, int simonY)
{
    if (e->type == ENEMY_TYPE_5) {
        const int d = simonX - e->x;
        const int h = simonY + 8 - e->y;
        return d > -17 && d < 17 && h >= 0 && h < 36;
    }
    const int h = (e->type == ENEMY_TYPE_4) ? ENEMY_TYPE4_ROW_H : ENEMY_TYPE1_ROW_H;
    return e->x < simonX + ENEMY_SIMON_RIGHT &&
           e->x + 16 > simonX - ENEMY_SIMON_HALF_W &&
           e->y < simonY + 1 &&
           e->y + h > simonY - ENEMY_SIMON_H;
}

#define ENEMY_SCREEN_TO_GAME_X 8   /* Simon et les ennemis : sprite X = x de jeu - 8 */

int Enemies_whipHit(EnemyState *s, int left, int top, int right, int bottom)
{
    EnemyU8 i;
    int killed = 0;
    for (i = 0; i < ENEMY_MAX; i++) {
        Enemy *e = &s->enemy[i];
        int ex, ey, eh;
        if (!e->active || e->exiting || e->type == ENEMY_TYPE_5) continue;
        ex = e->x + ENEMY_SCREEN_TO_GAME_X;
        ey = e->y;
        eh = (e->type == ENEMY_TYPE_4) ? ENEMY_TYPE4_ROW_H : ENEMY_TYPE1_ROW_H;
        if (right < ex || left >= ex + 16 || bottom < ey || top >= ey + eh) continue;
        /* Un coup suffit pour les types 1 (zombies, observe par le joueur) et 4 (trace :
         * 12 morts sur la premiere image de fouet utile, scratchpad/type4/whip-kill-type4.md).
         * Boite du type 4 : 16 x 16 provisoire, bornes a +-1-2 px non fixees. */
        e->active = 0;
        killed++;
    }
    return killed;
}

int Enemies_whipHitType5(EnemyState *s, int simonX, int simonY, int facingLeft, int crouched)
{
    const int whipX = facingLeft ? simonX - 20 : simonX + 20;
    const int b = crouched ? 10 : 18;
    EnemyU8 i;
    int killed = 0;
    for (i = 0; i < ENEMY_MAX; i++) {
        Enemy *e = &s->enemy[i];
        const int d = whipX - e->x;
        if (!e->active || e->type != ENEMY_TYPE_5) continue;
        if (d <= -24 || d >= 24 || e->y < simonY - b - 3 || e->y > simonY - b + 10) continue;
        e->active = 0;         /* points de vie 1 : mort au premier coup (objet 1E non porte) */
        killed++;
    }
    return killed;
}

int Enemies_frame(EnemyState *s, EnemySpawnTimer *t, const EnemyRoomVars *v, int *pushX)
{
    EnemyRoomVars w = *v;
    EnemyU8 i;
    int hit = 0;

    /* Recul : 2 px par image dans le sens de l'ennemi touche (C427 de 136 a 92). */
    *pushX = 0;
    if (s->recoil) {
        *pushX = ENEMY_RECOIL_STEP * s->recoilDir;
        s->recoil--;
    }

    Enemies_update(s);
    if (v->map) {
        for (i = 0; i < ENEMY_MAX; i++) {
            Enemy *e = &s->enemy[i];
            /* Le type 4 vole : sa hauteur suit kType4Y, sans chute. */
            if (e->active && !e->exiting && e->type != ENEMY_TYPE_4 && e->type != ENEMY_TYPE_5)
                fall(e, v->map, v->stage);
        }
    }
    /* Type 5 : une mise a jour une image sur deux, a partir de l'image qui suit la creation. */
    for (i = 0; i < ENEMY_MAX; i++) {
        Enemy *e = &s->enemy[i];
        if (!e->active || e->type != ENEMY_TYPE_5) continue;
        if (++e->age & 1) pantherStep(e, v);
    }

    /* Type 1 : 9CC3 une image sur deux. Le cote alterne par tir (provisoire). */
    if (s->parity == 0) {
        w.sideFlag = (EnemyU8)(t->index & 1);
        (void)Enemies_spawnTimerTick(t, &w, s);
    }
    s->parity ^= 1;

    /* Type 4 : salle 4 (drapeau bit3). Un tir tous les 168 images, cote alterne, droite d'abord. */
    if (v->stage == 1 && v->room < 8 && (kEnemyStage1RoomFlags[v->room] & ENEMY_FLAG_TYPE4)) {
        if (++s->type4Frames >= ENEMY_TYPE4_PERIOD) {
            s->type4Frames = 0;
            (void)Enemies_spawn(s, s->type4Side, ENEMY_TYPE_4, ENEMY_TYPE4_Y, 0);
            s->type4Side ^= 1;
        }
    }

    /* Type 5 : une panthere par visite des salles 2, 3 et 7, ENEMY_TYPE5_DELAY images apres
     * l'entree (62F5 recopie la liste a chaque chargement de salle). */
    if (v->stage == 1 && v->room < 8 && kType5Spawn[v->room][0] && s->type5Timer < ENEMY_TYPE5_DELAY) {
        if (++s->type5Timer == ENEMY_TYPE5_DELAY)
            pantherSpawn(s, kType5Spawn[v->room][0], kType5Spawn[v->room][1], v->simonX);
    }

    /* Contact : desactive pendant le recul et l'invulnerabilite (C420, C42D). */
    if (s->recoil == 0 && s->invuln == 0) {
        for (i = 0; i < ENEMY_MAX; i++) {
            const Enemy *e = &s->enemy[i];
            if (!e->active || e->exiting) continue;
            const EnemyU8 damage = e->type == ENEMY_TYPE_5 ? ENEMY_TYPE5_DAMAGE : ENEMY_CONTACT_DAMAGE;
            if (!touches(e, v->simonX, v->simonY)) continue;
            s->energy = (s->energy > damage) ? (EnemyU8)(s->energy - damage) : 0;
            s->invuln = ENEMY_INVULN_START;
            s->invulnPhase = 0;
            s->recoil = ENEMY_RECOIL_FRAMES;
            /* Panthere assise (dx 0) : recul a l'oppose d'elle (PROVISOIRE, non mesure). */
            s->recoilDir = e->dx ? e->dx : (EnemyS8)(v->simonX >= e->x ? 1 : -1);
            hit = 1;
            break;
        }
    }

    /* C42D ne decroit qu'une image sur deux : 0x5A x 2 = 180 images. */
    if (s->invuln) {
        s->invulnPhase ^= 1;
        if (s->invulnPhase == 0) s->invuln--;
    }
    return hit;
}

int Enemies_sprites(const EnemyState *s, EnemyU8 index, EnemySprite out[ENEMY_SPRITES])
{
    const Enemy *e;
    EnemyU8 pose, t, k;

    if (index >= ENEMY_MAX) return 0;
    e = &s->enemy[index];
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
