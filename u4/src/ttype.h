/*
 * $Id$
 */

#ifndef TTYPE_H
#define TTYPE_H

#define SEA_TILE 0x0
#define GRASS_TILE 0x4
#define AVATAR_TILE 0x1f
#define MOONGATE0_TILE 0x40
#define MOONGATE1_TILE 0x41
#define MOONGATE2_TILE 0x42
#define MOONGATE3_TILE 0x43
#define BLACK_TILE 0x7e

typedef enum {
    EFFECT_NONE = 0x00,
    EFFECT_FIRE = 0x04,
    EFFECT_SLEEP = 0x08,
    EFFECT_POISON = 0x0C
} TileEffect;

typedef enum {
    ANIM_NONE,
    ANIM_SCROLL,
    ANIM_CAMPFIRE,
    ANIM_CITYFLAG,
    ANIM_CASTLEFLAG,
    ANIM_WESTSHIPFLAG,
    ANIM_EASTSHIPFLAG,
    ANIM_LCBFLAG,
    ANIM_TWOFRAMES,
    ANIM_FOURFRAMES
} TileAnimationStyle;

int tileIsWalkable(unsigned char tile);
int tileIsSlow(unsigned char tile);
int tileIsVslow(unsigned char tile);
int tileIsSailable(unsigned char tile);
int tileIsFlyable(unsigned char tile);
int tileIsDoor(unsigned char tile);
int tileIsLockedDoor(unsigned char tile);
int tileIsShip(unsigned char tile);
int tileIsHorse(unsigned char tile);
int tileIsBalloon(unsigned char tile);
unsigned int tileGetDirection(unsigned char tile);
void tileSetDirection(unsigned short *tile, unsigned int dir);
int tileCanTalkOver(unsigned char tile);
TileEffect tileGetEffect(unsigned char tile);
TileAnimationStyle tileGetAnimationStyle(unsigned char tile);
void tileAdvanceFrame(unsigned char *tile);
int tileIsOpaque(unsigned char tile);

#endif
