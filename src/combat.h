/*
 * $Id$
 */

#ifndef COMBAT_H
#define COMBAT_H

#include "savegame.h"
#include "area.h"

struct _Map;
struct _Object;
struct _Monster;

typedef enum {
    CA_ATTACK,
    CA_CAST_SLEEP,
    CA_ADVANCE,
    CA_RANGED,
    CA_FLEE,
    CA_TELEPORT,
    CA_SHOW,
    CA_HIDE
} CombatAction;

typedef struct _CombatInfo {
    struct _Object *monsterObj;
    const struct _Monster *monster;
    int focus;
    struct _Object *party[AREA_PLAYERS];
    struct _Object *monsters[AREA_MONSTERS];
    int monsterHp[AREA_MONSTERS];
    StatusType party_status[AREA_PLAYERS];
    int isNormalCombat  : 1;
    int isCamping       : 1;
} CombatInfo;

void combatBegin(struct _Map *map, struct _Object *monster, int isNormalCombat);
void combatFinishTurn(void);
void combatCreateMonster(int index, int canbeleader);
int combatBaseKeyHandler(int key, void *data);
int combatInitialNumberOfMonsters(const struct _Monster *monster);
struct _Map *getCombatMapForTile(unsigned char partytile, unsigned short transport, const struct _Monster *monster);
void attackFlash(int x, int y, int tile, int timeFactor);

#endif
