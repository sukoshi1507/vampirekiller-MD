/*
 * Flamme de mort (objet 1E, research/death-flame.md), incluse par enemies.c.
 *
 * 9A3E (appelee par le fouet 7DFC, les armes lancees 8075 et le contact avec C434 7EB9) : l'ennemi tue est efface (99F6), puis
 * 5F1D cree un objet de type 1E dans le premier bloc libre, en (X, Y - D) de l'ennemi (D : table 9B33, 10h pour les types 1, 2, 3, 6, 9,
 * 0A, 0B, 0 pour les autres), avec +B = 85, +C = 10h, +E = 0 (ni contact ni fouet), +6 = 0 (immobile), +7E = 0 (D010 ne la fige pas).
 * Routine 9B71, une fois par passe : +B = 85 si le bit 2 de +C est nul, 86 sinon ; +C - 1 ; a 0 l'objet est efface (sans dessin).
 * Elle dure 16 passes dans la boucle des objets, dessinee des la passe de la mort (pose 85) : 32 images a l'ecran.
 * Dessin : une entree SAT (X - 8, Y - 16 pose 85 / Y - 15 pose 86), motifs F4 / F8, couleur 8 (une couche).
 * Port : x, y = X, Y de l'objet (comme les types 5 et 2), anim = +C, pose = +B. age part de 1 : la premiere mise a jour a lieu
 * deux images apres la creation (la passe suivante du MSX), puis toutes les deux images.
 * Drop (9ABE-9B21, 9B90 ; traces dans research/death-flame.md) : CF40 + 1 a chaque mort ; tous les 32 (CF40 & 1F = 0) une flamme d'objet 26
 * portant l'objet 13h (5000 points) est creee a la place de la flamme de mort ; sinon, R & 3F = 0 : flamme 26 portant l'objet 02 (grand coeur) ;
 * sinon, R & 3 = 0 : la flamme de mort porte +1F = 1 (champ state du port) et, a sa 16e mise a jour (9B90), un objet 24 (petit coeur) est cree en
 * (X, Y) de la flamme ; sinon rien. Les objets 24 et 26 sont ceux de candles.c (file Enemies_drops, videe par main.c).
 * Non portes : le type 0E (quatre flammes), les cas 9A6B (objets 09, 11, 18, 22, 24, 26), le compteur CE08, le porteur (+1F de l'ennemi, toujours 0 :
 * 5F1D l'efface et seul 5F1F le prend, avec B = 0), le bloc C800 que tient la flamme 26 pendant ses ~45 passes.
 */

#define ENEMY_DEATH_START 0x10      /* +C a la creation : 16 mises a jour */
#define ENEMY_DEATH_POSE_A 0x85
#define ENEMY_DEATH_POSE_B 0x86
#define ENEMY_DEATH_PATTERN_A 0     /* bank 2 : cellule F4 */
#define ENEMY_DEATH_PATTERN_B 4     /* bank 2 : cellule F8 */

EnemyDrop Enemies_drops[ENEMY_DROP_MAX];
unsigned char Enemies_dropCount;
static EnemyU8 deathKills;          /* CF40 : morts depuis le debut de la partie (44C6 le met a 0) */

static void deathInit(void)
{
    deathKills = 0;
    Enemies_dropCount = 0;
}

static void dropPush(EnemyU8 kind, int x, int y)
{
    EnemyDrop *d;
    if (Enemies_dropCount >= ENEMY_DROP_MAX) return;
    d = &Enemies_drops[Enemies_dropCount++];
    d->kind = kind;
    d->x = (short)x;
    d->y = (unsigned char)y;
}

/* 9AD9-9AF2 : 0 rien, 1 la flamme porte le petit coeur, sinon l'objet d'une flamme 26 (02 ou 13h). Le registre R n'est lu que si CF40 ne tombe pas
 * sur un multiple de 32 (9AE2 saute avant 9AE5). */
static EnemyU8 deathRoll(void)
{
    EnemyU8 r;
    deathKills++;
    if ((deathKills & 0x1F) == 0) return 0x13;
    r = s7Random();
    if ((r & 0x3F) == 0) return 2;
    return (r & 3) == 0 ? 1 : 0;
}

/* Decalage Y de la flamme (table 9B33 indexee par le type ROM de l'ennemi mort). */
static int deathOffset(EnemyU8 type)
{
    switch (type) {
    case ENEMY_TYPE_A: case ENEMY_TYPE_B: case ENEMY_TYPE_2: case ENEMY_TYPE_6:
    case ENEMY_TYPE_STATUE: case ENEMY_TYPE_HOPPER:
        return 0x10;
    case ENEMY_TYPE_EAGLE:
        return 0x18;               /* 9B33[0F] (lue dans la ROM, flamme de l'aigle non tracee) */
    default:
        return 0;
    }
}

/* 9A3E apres l'effacement de l'ennemi mort (e->active deja a 0, ses champs intacts) : cree la flamme au meme endroit. Les
 * projectiles D700 (boule, os) ne passent pas par 9A3E (822A). Sans bloc libre ou sans entrees SAT (5F1D), rien. */
static void deathFlame(EnemyState *s, const Enemy *dead)
{
    int x, y;
    EnemyU8 roll;
    Enemy *f;
    if (isProjectile(dead->type) || dead->type == ENEMY_TYPE_SPLASH || dead->type == ENEMY_TYPE_DEATH) return;
    if (dead->type == ENEMY_TYPE_DRAGON) {     /* 9AF3 : quatre flammes en (X + 16 k, Y), chacune avec un petit coeur (+1F = 1), sans tirage ni CF40 */
        int k;
        for (k = 0; k < 4; k++) {
            f = newObject(s, ENEMY_TYPE_DEATH);
            if (!f) return;
            f->state = 1;
            f->x = (EnemyS16)(dead->x + 16 * k);
            f->y = dead->y;
            f->anim = ENEMY_DEATH_START;
            f->pose = ENEMY_DEATH_POSE_A;
            f->age = 1;
        }
        return;
    }
    if (isObjS4(dead->type) || dead->type == ENEMY_TYPE_5 || dead->type == ENEMY_TYPE_2) { x = dead->x; y = dead->y; }
    else if (dead->type == ENEMY_TYPE_4) { x = dead->x + ENEMY_OBJ_X; y = dead->y + ENEMY_TYPE4_OBJ_Y; }
    else { x = dead->x + ENEMY_OBJ_X; y = dead->y + ENEMY_OBJ_Y; }
    y -= deathOffset(dead->type);
    roll = deathRoll();
    f = newObject(s, ENEMY_TYPE_DEATH);        /* 5F1D : premier bloc libre, entrees SAT */
    if (!f) return;
    if (roll > 1) {                            /* 9B13 : la flamme 26 prend la place de la flamme de mort (candles.c) */
        clearEnemy(f);                         /* seul le test du bloc libre et des entrees SAT */
        dropPush(roll, x, y);
        return;
    }
    f->state = roll;                           /* +1F : 1 = un objet 24 a la fin de la flamme */
    f->x = (EnemyS16)x;
    f->y = (EnemyU8)y;
    f->anim = ENEMY_DEATH_START;
    f->pose = ENEMY_DEATH_POSE_A;
    f->age = 1;
}

/* 9B71 : une passe. */
static void deathStep(Enemy *e)
{
    e->pose = (e->anim & 4) ? ENEMY_DEATH_POSE_B : ENEMY_DEATH_POSE_A;
    if (--e->anim == 0) {
        e->active = 0;
        if (e->state) dropPush(1, e->x, e->y);  /* 9B8B-9B98 : +1F non nul, l'objet 24 nait en (X, Y) de la flamme */
    }
}
