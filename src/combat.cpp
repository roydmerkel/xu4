/*
 * $Id$
 */

#include <stdlib.h>
#include <time.h>
#include "u4.h"

#include "combat.h"

#include "annotation.h"
#include "context.h"
#include "death.h"
#include "debug.h"
#include "dungeon.h"
#include "event.h"
#include "game.h"
#include "location.h"
#include "mapmgr.h"
#include "menu.h"
#include "monster.h"
#include "movement.h"
#include "names.h"
#include "object.h"
#include "player.h"
#include "portal.h"
#include "screen.h"
#include "settings.h"
#include "spell.h"
#include "stats.h"
#include "tile.h"
#include "utils.h"
#include "weapon.h"

CombatInfo combatInfo;

bool combatAttackAtCoord(MapCoords coords, int distance, void *data);
bool combatMonsterRangedAttack(MapCoords coords, int distance, void *data);
bool combatReturnWeaponToOwner(MapCoords coords, int distance, void *data);
int combatIsWon(void);
int combatIsLost(void);
void combatEnd(int adjustKarma);
void combatMoveMonsters(void);
int combatFindTargetForMonster(const Object *monster, int *distance, int ranged);
void combatApplyMonsterTileEffects(void);
int combatDivideMonster(const Object *monster);
int combatNearestPartyMember(const Object *obj, int *dist);
int combatHideOrShowCamouflageMonster(Object *monster);

/**
 * Key handlers
 */ 
bool combatChooseWeaponRange(int key, void *data);
bool combatChooseWeaponDir(int key, void *data);

/**
 * Initializes the CombatInfo structure with combat information
 */
void combatInit(const struct _Monster *m, class Object *monsterObj, MapId mapid) {
    int i;
    const Map *map = c->location->map;
    MonsterCombatInfo *monsters = &combatInfo.monsters[0];
    PartyCombatInfo *party = &combatInfo.party[0];

    combatInfo.monsterObj = monsterObj;

    combatInfo.placeMonsters = 1;
    combatInfo.placeParty = 1;    
    combatInfo.winOrLose = 1;
    combatInfo.dungeonRoom = 0;    
    combatInfo.altarRoom = 0;
    combatInfo.showCombatMessage = 1;

    /* new map for combat */
    if (mapid > 0) {
        map = combatInfo.newCombatMap = mapMgrGetById(mapid);
        ASSERT(combatInfo.newCombatMap != NULL, "bad map id: %d", mapid);        
    }
    else combatInfo.newCombatMap = NULL;
    
    /* initialize monster info */
    for (i = 0; i < AREA_MONSTERS; i++) {
        (monsters+i)->obj = NULL;
        combatInfo.monsterTable[i] = NULL;
        combatInfo.monster = NULL;
    }

    /* fill the monster table if a monster was provided to create */    
    combatFillMonsterTable(m);

    /* initialize party members */
    {
        FOCUS = 0;
        for (i = 0; i < AREA_PLAYERS; i++) {
            party[i].obj = NULL;
            party[i].player = &c->players[i];
            party[i].status = STAT_GOOD;
        }
    }    

    /* party is camping */
    if (combatInfo.camping)
        combatInfo.placeMonsters = 0;    

    if (map->type != MAPTYPE_DUNGEON) {
        /* setup player starting positions */
        for (i = 0; i < AREA_PLAYERS; i++)
            combatInfo.partyStartCoords[i] = map->area->player_start[i];            

        /* setup monster starting positions */
        for (i = 0; i < AREA_MONSTERS; i++)
            combatInfo.monsterStartCoords[i] = map->area->monster_start[i];
        
    }    
}

/**
 * Initializes information for camping
 */
void combatInitCamping(void) {
    combatInfo.camping = 1;    

    if (c->location->context & CTX_DUNGEON)
        combatInit(NULL, NULL, MAP_DUNGEON_CON); /* FIXME: use dungeon camping map */
    else
        combatInit(NULL, NULL, MAP_CAMP_CON);
}

/**
 * Initializes dungeon room combat
 */
void combatInitDungeonRoom(int room, Direction from) {
    int offset, i;    
    combatInit(NULL, NULL, 0);

    ASSERT(c->location->context & CTX_DUNGEON, "Error: called combatInitDungeonRoom from non-dungeon context");        
    {
        Dungeon *dng = c->location->map->dungeon;
        unsigned char 
            *party_x = &dng->rooms[room].party_north_start_x[0], 
            *party_y = &dng->rooms[room].party_north_start_y[0];

        /* load the dungeon room */
        dungeonLoadRoom(dng, room);
        combatInfo.newCombatMap = dng->room;
        combatInfo.winOrLose = 0;
        combatInfo.dungeonRoom = 0xD0 | room;
        combatInfo.exitDir = DIR_NONE;
        
        /* FIXME: this probably isn't right way to see if you're entering an altar room... but maybe it is */
        if ((c->location->map->id != MAP_ABYSS) && (room == 0xF)) {            
            /* figure out which dungeon room they're entering */
            if (c->location->coords.x == 3)
                combatInfo.altarRoom = VIRT_LOVE;
            else if (c->location->coords.x <= 2)
                combatInfo.altarRoom = VIRT_TRUTH;
            else combatInfo.altarRoom = VIRT_COURAGE;            
        }        
        
        /* load in monsters and monster start coordinates */
        for (i = 0; i < AREA_MONSTERS; i++) {
            if (dng->rooms[room].monster_tiles[i] > 0)
                combatInfo.monsterTable[i] = monsterForTile(dng->rooms[room].monster_tiles[i]);
            combatInfo.monsterStartCoords[i].x = dng->rooms[room].monster_start_x[i];
            combatInfo.monsterStartCoords[i].y = dng->rooms[room].monster_start_y[i];            
        }
        
        /* figure out party start coordinates */
        switch(from) {
        case DIR_WEST: offset = 3; break;
        case DIR_NORTH: offset = 0; break;
        case DIR_EAST: offset = 1; break;
        case DIR_SOUTH: offset = 2; break;
        case DIR_ADVANCE:
        case DIR_RETREAT:
        default: 
            ASSERT(0, "Invalid 'from' direction passed to combatInitDungeonRoom()");
        }

        for (i = 0; i < AREA_PLAYERS; i++) {
            combatInfo.partyStartCoords[i].x = *(party_x + (offset * AREA_PLAYERS * 2) + i);
            combatInfo.partyStartCoords[i].y = *(party_y + (offset * AREA_PLAYERS * 2) + i);
        }
    }
}

/**
 * Begin combat
 */
void combatBegin() {
    int i;
    int partyIsReadyToFight = 0;
    /*int isAbyss = c->location->map->id == MAP_ABYSS;*/
    
    /* set the new combat map if a new map was provided */
    if (combatInfo.newCombatMap != NULL) {
        gameSetMap(combatInfo.newCombatMap, 1, NULL);
    }
    
    /* place party members on the map */
    if (combatInfo.placeParty)        
        combatPlacePartyMembers();    

    /* place monsters on the map */
    if (combatInfo.placeMonsters)
        combatPlaceMonsters();

    /* camping, make sure everyone's asleep */
    if (combatInfo.camping) {
        for (i = 0; i < c->saveGame->members; i++)
            combatPutPlayerToSleep(i);        
    }

    /* if we entered an altar room, show the name */
    if (combatInfo.altarRoom) {
        screenMessage("\nThe Altar Room of %s\n", getPrincipleName(combatInfo.altarRoom));    
        c->location->context = (LocationContext)(c->location->context | CTX_ALTAR_ROOM);
    }

    /* Use the combat key handler */
    eventHandlerPushKeyHandler(&combatBaseKeyHandler);
 
    /* if there are monsters around, start combat! */    
    if (combatInfo.showCombatMessage && combatInfo.placeMonsters && combatInfo.winOrLose)
        screenMessage("\n**** COMBAT ****\n\n");
    
    /* FIXME: there should be a better way to accomplish this */
    if (!combatInfo.camping) {
        musicPlay();
    }

    /* Set focus to the first active party member, if there is one */ 
    for (i = 0; i < AREA_PLAYERS; i++) {
        if (combatSetActivePlayer(i)) {
            partyIsReadyToFight = 1;
            break;
        }
    }    

    if (!combatInfo.camping && !partyIsReadyToFight)
        (*c->location->finishTurn)();
}

/**
 * Sets the active player for combat, showing which weapon they're weilding, etc.
 */
int combatSetActivePlayer(int player) {
    PartyCombatInfo *party = combatInfo.party;

    if (!playerIsDisabled(player) && party[player].obj) {
        if (party[FOCUS].obj)
            party[FOCUS].obj->setFocus(false);
        party[player].obj->setFocus();
        FOCUS = player;

        screenMessage("%s with %s\n\020", c->players[FOCUS].name, weaponGetName(c->players[FOCUS].weapon)->c_str());
        statsUpdate(); /* If a character was awakened inbetween world view and combat, this fixes stats info */
        statsHighlightCharacter(FOCUS);
        return 1;
    }
    return 0;
}

/**
 * Puts player 'player' to sleep in combat
 */
int combatPutPlayerToSleep(int player) {
    PartyCombatInfo *party = combatInfo.party;
    
    if (!playerIsDisabled(player) && party[player].obj) {
        party[player].status = party[player].player->status; /* save old status */
        party[player].player->status = STAT_SLEEPING;
        party[player].obj->setTile(CORPSE_TILE);
        return 1;
    }
    return 0;
}

int combatAddMonster(const Monster *m, MapCoords coords) {
    int i;
    MonsterCombatInfo *monsters = combatInfo.monsters;

    if (m != NULL) {
        for (i = 0; i < AREA_MONSTERS; i++) {
            /* find a free spot to place the monster */
            if (monsters[i].obj == NULL) {
                /* place the monster! */
                monsters[i].obj = mapAddMonsterObject(c->location->map, m, coords);
                monsters[i].hp = monsterGetInitialHp(monsters[i].obj->monster);
                monsters[i].status = STAT_GOOD;

                return 1;
            }
        }
    }
    return 0;
}

/**
 * Fills the combat monster table with the monsters that the party will be facing.
 * The monster table only contains *which* monsters will be encountered and
 * *where* they are placed (by position in the table).  Information like
 * hit points and monster status will be created when the monster is actually placed
 */
void combatFillMonsterTable(const Monster *monster) {
    int i, j;
    
    if (monster != NULL) {        
        const Monster *baseMonster = monster, *current;
        int numMonsters = combatInitialNumberOfMonsters(monster);
        
        combatInfo.monster = monster;

        if (baseMonster->id == PIRATE_ID)
            baseMonster = monsterById(ROGUE_ID);

        for (i = 0; i < numMonsters; i++) {
            current = baseMonster;

            /* find a free spot in the monster table */
            do {j = xu4_random(AREA_MONSTERS) ;} while (combatInfo.monsterTable[j] != NULL);
            
            /* see if monster is a leader or leader's leader */
            if (monsterById(baseMonster->leader) != baseMonster && /* leader is a different monster */
                i != (numMonsters - 1)) { /* must have at least 1 monster of type encountered */
                
                if (xu4_random(32) == 0)       /* leader's leader */
                    current = monsterById(monsterById(baseMonster->leader)->leader);
                else if (xu4_random(8) == 0)   /* leader */
                    current = monsterById(baseMonster->leader);
            }

            /* place this monster in the monster table */
            combatInfo.monsterTable[j] = current;
        }
    }
}

/**
 * Places the party members on the map
 */
void combatPlacePartyMembers(void) {
    int i;
    for (i = 0; i < c->saveGame->members; i++) {
        PartyCombatInfo *party = combatInfo.party;
        StatusType playerStatus = party[i].player->status;
        MapTile playerTile = tileForClass(party[i].player->klass);

        /* don't place dead party members */
        if (playerStatus != STAT_DEAD) {
            
            /* add the party member to the map */
            party[i].obj = mapAddMonsterObject(c->location->map, monsterForTile(playerTile), combatInfo.partyStartCoords[i]);

            /* change the tile for the object to a sleeping person if necessary */
            if (playerStatus == STAT_SLEEPING)
                party[i].obj->setTile(CORPSE_TILE);
        }
    }
}

/**
 * Places monsters on the map from the monster table and from monsterStart_x and monsterStart_y
 */
void combatPlaceMonsters(void) {
    int i;    

    for (i = 0; i < AREA_MONSTERS; i++) {
        const Monster *m = combatInfo.monsterTable[i];
        if (m)
            combatAddMonster(m, combatInfo.monsterStartCoords[i]);
    }    
}

int combatPartyMemberAt(MapCoords coords) {
    int i;
    PartyCombatInfo *party = combatInfo.party;

    for (i = 0; i < AREA_PLAYERS; i++) {
        if (party[i].obj && 
            party[i].obj->getCoords() == coords)
            return i;
    }
    return -1;
}

int combatMonsterAt(MapCoords coords) {
    int i;
    MonsterCombatInfo *monsters = combatInfo.monsters;

    for (i = 0; i < AREA_MONSTERS; i++) {
        if (monsters[i].obj &&
            monsters[i].obj->getCoords() == coords)
            return i;
    }
    return -1;
}

MapId combatMapForTile(MapTile groundTile, MapTile transport, Object *obj) {
    unsigned int i;
    int fromShip = 0,
        toShip = 0;
    Object *objUnder = mapObjectAt(c->location->map, c->location->coords);

    static const struct {
        MapTile tile;
        MapId mapid;
    } tileToMap[] = {
        { HORSE1_TILE,  MAP_GRASS_CON },
        { HORSE2_TILE,  MAP_GRASS_CON },
        { SWAMP_TILE,   MAP_MARSH_CON },
        { GRASS_TILE,   MAP_GRASS_CON },
        { BRUSH_TILE,   MAP_BRUSH_CON },
        { FOREST_TILE,  MAP_FOREST_CON },
        { HILLS_TILE,   MAP_HILL_CON },
        { DUNGEON_TILE, MAP_HILL_CON },
        { CITY_TILE,    MAP_GRASS_CON },
        { CASTLE_TILE,  MAP_GRASS_CON },
        { TOWN_TILE,    MAP_GRASS_CON },
        { LCB2_TILE,    MAP_GRASS_CON },
        { BRIDGE_TILE,  MAP_BRIDGE_CON },
        { BALLOON_TILE, MAP_GRASS_CON },
        { NORTHBRIDGE_TILE, MAP_BRIDGE_CON },
        { SOUTHBRIDGE_TILE, MAP_BRIDGE_CON },
        { SHRINE_TILE,  MAP_GRASS_CON },
        { CHEST_TILE,   MAP_GRASS_CON },
        { BRICKFLOOR_TILE, MAP_BRICK_CON },
        { MOONGATE0_TILE, MAP_GRASS_CON },
        { MOONGATE1_TILE, MAP_GRASS_CON },
        { MOONGATE2_TILE, MAP_GRASS_CON },
        { MOONGATE3_TILE, MAP_GRASS_CON }
    };

    if (tileIsShip(transport) || (objUnder && tileIsShip(objUnder->getTile())))
        fromShip = 1;
    if (tileIsPirateShip(obj->getTile()))
        toShip = 1;

    if (fromShip && toShip)
        return MAP_SHIPSHIP_CON;

    /* We can fight monsters and townsfolk */       
    if (obj->getType() != OBJECT_UNKNOWN) {
        MapTile tileUnderneath = (*c->location->tileAt)(c->location->map, obj->getCoords(), WITHOUT_OBJECTS);

        if (toShip)
            return MAP_SHORSHIP_CON;
        else if (fromShip && tileIsWater(tileUnderneath))
            return MAP_SHIPSEA_CON;
        else if (tileIsWater(tileUnderneath))
            return MAP_SHORE_CON;
        else if (fromShip && !tileIsWater(tileUnderneath))
            return MAP_SHIPSHOR_CON;        
    }

    for (i = 0; i < sizeof(tileToMap) / sizeof(tileToMap[0]); i++) {
        if (tileToMap[i].tile == groundTile)
            return tileToMap[i].mapid;
    }    

    return MAP_BRICK_CON;
}

void combatFinishTurn() {    
    PartyCombatInfo *party = combatInfo.party;
    int quick;

    /* return to party overview */
    c->statsView = STATS_PARTY_OVERVIEW;
    statsUpdate();

    if (combatIsWon() && combatInfo.winOrLose) {
        eventHandlerPopKeyHandler();
        combatEnd(1);
        return;
    }
    
    /* make sure the player with the focus is still in battle (hasn't fled or died) */
    if (party[FOCUS].obj) {
        /* apply effects from tile player is standing on */
        playerApplyEffect(tileGetEffect((*c->location->tileAt)(c->location->map, party[FOCUS].obj->getCoords(), WITH_GROUND_OBJECTS)), FOCUS);
    }

    quick = (c->aura == AURA_QUICKNESS) && (party[FOCUS].obj != NULL) && (xu4_random(2) == 0) ? 1 : 0;

    /* check to see if the player gets to go again (and is still alive) */
    if (!quick || (c->players[FOCUS].hp <= 0)){    

        do {            
            c->location->map->annotations->passTurn();

            /* put a sleeping person in place of the player,
               or restore an awakened member to their original state */            
            if (party[FOCUS].obj) {
                /* FIXME: move this to its own function, probably combatTryToWakeUp() or something similar */
                /* wake up! */
                if (party[FOCUS].player->status == STAT_SLEEPING && (xu4_random(8) == 0)) {
                    party[FOCUS].player->status = party[FOCUS].status;                    
                    statsUpdate();
                }

                /* display a sleeping person or an awake person */                
                if (party[FOCUS].player->status == STAT_SLEEPING)
                    party[FOCUS].obj->setTile(CORPSE_TILE);
                else party[FOCUS].obj->setTile(tileForClass(party[FOCUS].player->klass));

                /* remove focus from the current party member */
                party[FOCUS].obj->setFocus(false);
            }

            /* eat some food */
            if (party[FOCUS].player->status != STAT_DEAD)
                playerAdjustFood(-1);                

            /* put the focus on the next party member */
            FOCUS++;           

            /* move monsters and wrap around at end */
            if (FOCUS >= c->saveGame->members) {   
                
                /* reset the focus to the avatar and start the party's turn over again */
                FOCUS = 0;

                gameUpdateScreen();
                eventHandlerSleep(50); /* give a slight pause in case party members are asleep for awhile */

                /* adjust moves */
                playerEndTurn();

                /* check if aura has expired */
                if (c->auraDuration > 0) {
                    if (--c->auraDuration == 0)
                        c->aura = AURA_NONE;
                }                

                /** 
                 * ====================
                 * HANDLE MONSTER STUFF
                 * ====================
                 */
            
                /* first, move all the monsters */
                combatMoveMonsters();

                /* then, apply tile effects to monsters */
                combatApplyMonsterTileEffects();                

                /* check to see if combat is over */
                if (combatIsLost()) {
                    eventHandlerPopKeyHandler();
                    combatEnd(1);
                    return;
                }

                /* end combat immediately if the enemy has fled */
                else if (combatIsWon() && combatInfo.winOrLose) {
                    eventHandlerPopKeyHandler();
                    combatEnd(1);
                    return;
                }                
            }
        } while (!party[FOCUS].obj ||    /* dead */
                 (c->players[FOCUS].status == STAT_SLEEPING) || /* sleeping */
                 ((c->location->activePlayer >= 0) && /* active player is set */
                  !playerIsDisabled(c->location->activePlayer) && /* and the active player is not disabled */
                  (combatInfo.party[c->location->activePlayer].obj != NULL) && /* and the active player is still in combat */
                  (c->location->activePlayer != FOCUS)));
    }
    else c->location->map->annotations->passTurn();

    /* FIXME: there is probably a cleaner way to do this:
       make sure the active player is back to their normal self before acting */
    party[FOCUS].obj->setTile(tileForClass(party[FOCUS].player->klass));
    combatSetActivePlayer(FOCUS);    
}

bool combatBaseKeyHandler(int key, void *data) {
    bool valid = true;
    CoordActionInfo *info;
    AlphaActionInfo *alphaInfo;
    int weapon = c->players[FOCUS].weapon;    

    switch (key) {
    case U4_UP:
    case U4_DOWN:
    case U4_LEFT:
    case U4_RIGHT:        
        (*c->location->move)(keyToDirection(key), 1);
        break;

    case U4_ESC:
        if (settings.debug) {
            eventHandlerPopKeyHandler();
            combatEnd(0); /* don't adjust karma */
        }
        else screenMessage("Bad command\n");        

        break;
        
    case ' ':
        screenMessage("Pass\n");
        break;

    case U4_FKEY:
        {
            extern void gameDestroyAllMonsters();

            if (settings.debug)
                gameDestroyAllMonsters();
            else valid = false;
            break;
        }

    case 'a':
        info = new CoordActionInfo;
        info->handleAtCoord = &combatAttackAtCoord;
        info->origin = combatInfo.party[FOCUS].obj->getCoords();
        info->prev = MapCoords(-1, -1);        
        info->range = weaponGetRange(weapon);
        info->validDirections = MASK_DIR_ALL;
        info->player = FOCUS;        
        info->blockedPredicate = weaponCanAttackThroughObjects(weapon) ?
            NULL :
            &tileCanAttackOver;
        info->blockBefore = 1;
        info->firstValidDistance = 0;
        
        eventHandlerPushKeyHandlerWithData(&combatChooseWeaponDir, info);        

        screenMessage("Dir: ");
        break;

    case 'c':
        screenMessage("Cast Spell!\n");
        gameCastForPlayer(FOCUS);
        break;

    case 'g':
        screenMessage("Get Chest!\n");
        gameGetChest(FOCUS);        
        break;

    case 'l':
        if (settings.debug) {
            MapCoords coords = combatInfo.party[FOCUS].obj->getCoords();
            screenMessage("\nLocation:\nx:%d\ny:%d\nz:%d\n", coords.x, coords.y, coords.z);
            screenPrompt();
            valid = false;
            break;            
        }

    case 'r':
        {
            extern int numWeapons;

            c->statsView = STATS_WEAPONS;
            statsUpdate();

            alphaInfo = new AlphaActionInfo;
            alphaInfo->lastValidLetter = numWeapons + 'a' - 1;
            alphaInfo->handleAlpha = readyForPlayer2;
            alphaInfo->prompt = "Weapon: ";
            alphaInfo->data = (void *)((int)FOCUS);

            screenMessage(alphaInfo->prompt.c_str());

            eventHandlerPushKeyHandlerWithData(&gameGetAlphaChoiceKeyHandler, alphaInfo);
        }
        break;

    case 't':
        if (settings.debug && combatInfo.dungeonRoom) {
            Trigger *triggers = c->location->prev->map->dungeon->currentRoom->triggers;
            int i;

            screenMessage("Triggers!\n");

            for (i = 0; i < 4; i++) {
                screenMessage("%.1d)xy tile xy xy\n", i+1);
                screenMessage("  %.1X%.1X  %.3d %.1X%.1X %.1X%.1X\n",
                    triggers[i].x, triggers[i].y,
                    triggers[i].tile,
                    triggers[i].change_x1, triggers[i].change_y1,
                    triggers[i].change_x2, triggers[i].change_y2);                
            }
            screenPrompt();
            valid = false;
            
            break;
        }

    case 'u':
        {
            extern string itemNameBuffer;
            screenMessage("Use which item:\n");
            gameGetInput(&useItem, &itemNameBuffer);

            c->statsView = STATS_ITEMS;
            statsUpdate();

            return true;
        }

    case 'v':
        if (musicToggle())
            screenMessage("Volume On!\n");
        else
            screenMessage("Volume Off!\n");
        break;

    case 'v' + U4_ALT:
        screenMessage("XU4 %s\n", VERSION);        
        break;

    case 'z': 
        {            
            c->statsView = (StatsView) (STATS_CHAR1 + FOCUS);
            statsUpdate();

            /* reset the spell mix menu and un-highlight the current item,
               and hide reagents that you don't have */            
            gameResetSpellMixing();

            eventHandlerPushKeyHandler(&gameZtatsKeyHandler);
            screenMessage("Ztats\n");        
        }
        break;    

    case 'b':
    case 'e':
    case 'd':
    case 'f':    
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 's':    
    case 'w':
    case 'x':   
    case 'y':
        screenMessage("Not here!\n");
        break;

    case '0':        
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        if (settings.enhancements && settings.enhancementsOptions.activePlayer) {
            if (key == '0') {             
                c->location->activePlayer = -1;
                screenMessage("Set Active Player: None!\n");
            }
            else if (key-'1' < c->saveGame->members) {
                c->location->activePlayer = key - '1';
                screenMessage("Set Active Player: %s!\n", c->players[c->location->activePlayer].name);
            }
        }
        else screenMessage("Bad command\n");

        break;    

    default:
        valid = false;
        break;
    }

    if (valid) {
        c->lastCommandTime = time(NULL);
        if (eventHandlerGetKeyHandler() == &combatBaseKeyHandler &&
            c->location->finishTurn == &combatFinishTurn)
            (*c->location->finishTurn)();
    }

    return valid;
}

bool combatAttackAtCoord(MapCoords coords, int distance, void *data) {
    int monster;    
    MapTile hittile, misstile;
    CoordActionInfo* info = (CoordActionInfo*)data;    
    int weapon = c->players[info->player].weapon;    
    int wrongRange = weaponRangeAbsolute(weapon) && (distance != info->range);
    MapCoords old = info->prev;    
    int attackdelay = MAX_BATTLE_SPEED - settings.battleSpeed;    
    MapTile groundTile;

    info->prev = coords;    

    hittile = weaponGetHitTile(weapon);
    misstile = weaponGetMissTile(weapon);

    /* Remove the last weapon annotation left behind */
    if ((distance > 0) && (old.x >= 0) && (old.y >= 0))
        c->location->map->annotations->remove(old, misstile);

    /* Missed */
    if (coords.x == -1 && coords.y == -1) {

        /* Check to see if the weapon is lost */
        if ((distance > 1 && weaponLoseWhenRanged(weapon)) || weaponLoseWhenUsed(weapon)) {
            if (!playerLoseWeapon(info->player))
                screenMessage("Last One!\n");
        }

        /* show the 'miss' tile */
        attackFlash(old, misstile, 3);

        /* This goes here so messages are shown in the original order */
        screenMessage("Missed!\n");
    }
    
    /* Check to see if we might hit something */
    else {
        monster = combatMonsterAt(coords);

        /* If we haven't hit a monster, or the weapon's range is absolute
           and we're testing the wrong range, stop now! */
        if (monster == -1 || wrongRange) {
        
            /* If the weapon is shown as it travels, show it now */
            if (weaponShowTravel(weapon)) {
                c->location->map->annotations->add(coords, misstile, true);
                gameUpdateScreen();
        
                /* Based on attack speed setting in setting struct, make a delay for
                   the attack annotation */
                if (attackdelay > 0)
                    eventHandlerSleep(attackdelay * 2);
            }       

            return 0;
        }
    
        /* Check to see if the weapon is lost */
        if ((distance > 1 && weaponLoseWhenRanged(weapon)) || weaponLoseWhenUsed(weapon)) {
            if (!playerLoseWeapon(info->player))
                screenMessage("Last One!\n");
        }
    
        /* Did the weapon miss? */
        if ((c->location->map->id == 24 && !weaponIsMagic(weapon)) || /* non-magical weapon in the Abyss */
            !playerAttackHit(&c->players[FOCUS])) {         /* player naturally missed */
            screenMessage("Missed!\n");
        
            /* show the 'miss' tile */
            attackFlash(coords, misstile, 3);

        } else { /* The weapon hit! */

            /* show the 'hit' tile */
            attackFlash(coords, hittile, 3);

            /* apply the damage to the monster */
            combatApplyDamageToMonster(monster, playerGetDamage(&c->players[FOCUS]), FOCUS);

            /* monster is still alive and has the chance to divide - xu4 enhancement */
            if (xu4_random(2) == 0 && combatInfo.monsters[monster].obj && monsterDivides(combatInfo.monsters[monster].obj->monster))
                combatDivideMonster(combatInfo.monsters[monster].obj);
        }
    }

    /* Check to see if the weapon returns to its owner */
    if (weaponReturns(weapon))
        combatReturnWeaponToOwner(coords, distance, data);

    /* If the weapon leaves a tile behind, do it here! (flaming oil, etc) */
    groundTile = (*c->location->tileAt)(c->location->map, coords, WITHOUT_OBJECTS);
    if (!wrongRange && (weaponLeavesTile(weapon) && tileIsWalkable(groundTile)))
        c->location->map->annotations->add(coords, weaponLeavesTile(weapon));    
    
    /* only end the turn if we're still in combat */
    if (c->location->finishTurn == &combatFinishTurn)
        (*c->location->finishTurn)();

    return true;
}

bool combatMonsterRangedAttack(MapCoords coords, int distance, void *data) {
    int player;
    const Monster *m;
    MapTile hittile, misstile;
    CoordActionInfo* info = (CoordActionInfo*)data;    
    MapCoords old = info->prev;    
    int attackdelay = MAX_BATTLE_SPEED - settings.battleSpeed;    
    MapTile groundTile;
    TileEffect effect;
    
    info->prev = coords;

    hittile = combatInfo.monsters[info->player].obj->monster->rangedhittile;
    misstile = combatInfo.monsters[info->player].obj->monster->rangedmisstile;

    /* Remove the last weapon annotation left behind */
    if ((distance > 0) && (old.x >= 0) && (old.y >= 0))
        c->location->map->annotations->remove(old, misstile);

    /* Check to see if the monster hit a party member */
    if (coords.x != -1 && coords.y != -1) {   

        player = combatPartyMemberAt(coords);

        /* If we haven't hit a player, stop now */
        if (player == -1) {
            c->location->map->annotations->add(coords, misstile, true);
            gameUpdateScreen();
    
            /* Based on attack speed setting in setting struct, make a delay for
               the attack annotation */
            if (attackdelay > 0)
                eventHandlerSleep(attackdelay * 2);

            return 0;
        }

        /* Get the effects of the tile the monster is using */
        effect = tileGetEffect(hittile);
  
        /* Did the weapon miss? */
        if (!playerIsHitByAttack(&c->players[player])) {
        
            /* show the 'miss' tile */
            attackFlash(coords, misstile, 4);

        } else { /* The weapon hit! */                   

            /* show the 'hit' tile */
            attackFlash(coords, hittile, 4);             

            /* FIXME: Will this ever be used? */

            /* These effects require the player to be hit to affect the player */
            /*switch(effect) {
            } */
        }

        m = mapObjectAt(c->location->map, info->origin)->monster;

        /* These effects happen whether or not the player was hit */
        switch(effect) {
        
        case EFFECT_ELECTRICITY:
            /* FIXME: are there any special effects here? */
            screenMessage("\n%s Electrified!\n", c->players[player].name);
            playerApplyDamage(&c->players[player], monsterGetDamage(m));
            break;
        
        case EFFECT_POISON:
        case EFFECT_POISONFIELD:
            
            screenMessage("\n%s Poisoned!\n", c->players[player].name);

            /* see if the player is poisoned */
            if ((xu4_random(2) == 0) && (c->players[player].status != STAT_POISONED))
                c->players[player].status = STAT_POISONED;
            else screenMessage("Failed.\n");
            break;
        
        case EFFECT_SLEEP:

            screenMessage("\n%s Slept!\n", c->players[player].name);

            /* see if the player is put to sleep */
            if (xu4_random(2) == 0)
                combatPutPlayerToSleep(player);            
            else screenMessage("Failed.\n");
            break;

        case EFFECT_LAVA:
        case EFFECT_FIRE:
            /* FIXME: are there any special effects here? */            
            screenMessage("\n%s %s Hit!\n", c->players[player].name,
                effect == EFFECT_LAVA ? "Lava" : "Fiery");
            playerApplyDamage(&c->players[player], monsterGetDamage(m));
            break;
                
        default: 
            /* show the appropriate 'hit' message */
            if (hittile == MAGICFLASH_TILE)
                screenMessage("\n%s Magical Hit!\n", c->players[player].name);
            else screenMessage("\n%s Hit!\n", c->players[player].name);
            playerApplyDamage(&c->players[player], monsterGetDamage(m));
            break;
        }       

    }
    else {
        m = mapObjectAt(c->location->map, info->origin)->monster;

        /* If the monster leaves a tile behind, do it here! (lava lizard, etc) */
        groundTile = (*c->location->tileAt)(c->location->map, old, WITH_GROUND_OBJECTS);
        if (monsterLeavesTile(m) && tileIsWalkable(groundTile))
            c->location->map->annotations->add(old, hittile);
    }

    return true;
}


bool combatReturnWeaponToOwner(MapCoords coords, int distance, void *data) {
    Direction dir;
    int i;
    MapTile misstile;
    CoordActionInfo* info = (CoordActionInfo*)data;
    int weapon = c->players[info->player].weapon;
    int attackdelay = MAX_BATTLE_SPEED - settings.battleSpeed;
    MapCoords new_coords = coords;
    
    misstile = weaponGetMissTile(weapon);

    /* reverse the direction of the weapon */
    dir = dirReverse(dirFromMask(info->dir));

    for (i = distance; i > 1; i--) {
        new_coords.move(dir, c->location->map);        
        
        c->location->map->annotations->add(new_coords, misstile, true);
        gameUpdateScreen();

        /* Based on attack speed setting in setting struct, make a delay for
           the attack annotation */
        if (attackdelay > 0)
            eventHandlerSleep(attackdelay * 2);
        
        c->location->map->annotations->remove(new_coords, misstile);
    }
    gameUpdateScreen();

    return true;
}

/**
 * Generate the number of monsters in a group.
 */
int combatInitialNumberOfMonsters(const Monster *monster) {
    int nmonsters;

    /* if in an unusual combat situation, generally we stick to normal encounter sizes,
       (such as encounters from sleeping in an inn, etc.) */
    if (combatInfo.camping || combatInfo.inn || mapIsWorldMap(c->location->map) || (c->location->context & CTX_DUNGEON)) {
        nmonsters = xu4_random(8) + 1;
        
        if (nmonsters == 1) {            
            if (monster && monster->encounterSize > 0)
                nmonsters = xu4_random(monster->encounterSize) + monster->encounterSize + 1;
            else
                nmonsters = 8;
        }

        while (nmonsters > 2 * c->saveGame->members) {
            nmonsters = xu4_random(16) + 1;
        }
    } else {
        if (monster && monster->id == GUARD_ID)
            nmonsters = c->saveGame->members * 2;
        else
            nmonsters = 1;
    }

    return nmonsters;
}

/**
 * Returns true if the player has won.
 */
int combatIsWon() {
    int i, activeMonsters;

    activeMonsters = 0;
    for (i = 0; i < AREA_MONSTERS; i++) {
        if (combatInfo.monsters[i].obj)
            activeMonsters++;
    }

    return activeMonsters == 0;
}

/**
 * Returns true if the player has lost.
 */
int combatIsLost() {
    int i, activePlayers;

    activePlayers = 0;
    for (i = 0; i < c->saveGame->members; i++) {
        if (combatInfo.party[i].obj)
            activePlayers++;
    }

    return activePlayers == 0;
}

void combatEnd(int adjustKarma) {
    int i;
    MapCoords coords;    
    MapTile ground;    
    
    gameExitToParentMap();
    musicPlay();    
    
    if (combatInfo.winOrLose) {
        if (combatIsWon()) {        

            if (combatInfo.monsterObj) {
                coords = combatInfo.monsterObj->getCoords();
                ground = (*c->location->tileAt)(c->location->map, coords, WITHOUT_OBJECTS);

                /* FIXME: move to separate function */
                /* add a chest, if the monster leaves one */
                if (monsterLeavesChest(combatInfo.monster) && 
                    tileIsMonsterWalkable(ground) && tileIsWalkable(ground)) {                    
                    mapAddObject(c->location->map, tileGetChestBase(), tileGetChestBase(), coords);
                }
                /* add a ship if you just defeated a pirate ship */
                else if (tileIsPirateShip(combatInfo.monsterObj->getTile())) {
                    MapTile ship = tileGetShipBase();
                    tileSetDirection(&ship, tileGetDirection(combatInfo.monsterObj->getTile()));
                    mapAddObject(c->location->map, ship, ship, coords);
                }        
            }

            screenMessage("\nVictory!\n");
        }
        else if (!playerPartyDead()) {
            /* minus points for fleeing from evil creatures */
            if (adjustKarma && combatInfo.monster && monsterIsEvil(combatInfo.monster)) {
                screenMessage("Battle is lost!\n");
                playerAdjustKarma(KA_FLED_EVIL);
            }
            else if (adjustKarma && combatInfo.monster && monsterIsGood(combatInfo.monster))
                playerAdjustKarma(KA_SPARED_GOOD);
        }
    }

    /* exiting a dungeon room */
    if (combatInfo.dungeonRoom) {
        screenMessage("Leave Room!\n");
        if (combatInfo.altarRoom) {            
            PortalTriggerAction action = ACTION_NONE;

            /* when exiting altar rooms, you exit to other dungeons.  Here it goes... */
            switch(combatInfo.exitDir) {
            case DIR_NORTH: action = ACTION_EXIT_NORTH; break;
            case DIR_EAST:  action = ACTION_EXIT_EAST; break;
            case DIR_SOUTH: action = ACTION_EXIT_SOUTH; break;
            case DIR_WEST:  action = ACTION_EXIT_WEST; break;            
            case DIR_NONE:  break;
            case DIR_ADVANCE:
            case DIR_RETREAT:
            default: ASSERT(0, "Invalid exit dir %d", combatInfo.exitDir); break;
            }

            if (action != ACTION_NONE)
                usePortalAt(c->location, c->location->coords, action);
        }
        else screenMessage("\n");

        if (combatInfo.exitDir != DIR_NONE) {
            c->saveGame->orientation = combatInfo.exitDir;  /* face the direction exiting the room */
            (*c->location->move)(DIR_NORTH, 0);             /* advance 1 space outside of the room */
        }
    }

    /* remove the monster */
    if (combatInfo.monsterObj)
        mapRemoveObject(c->location->map, combatInfo.monsterObj);

    /* If we were camping and were ambushed, wake everyone up! */
    if (combatInfo.camping) {
        for (i = 0; i < c->saveGame->members; i++) {
            if (c->players[i].status == STAT_SLEEPING)
                c->players[i].status = combatInfo.party[i].status;
        }
    }

    /* reset our combat variables */
    combatInfo.camping = 0;
    combatInfo.inn = 0;
    
    if (playerPartyDead())
        deathStart(0);
    else
        (*c->location->finishTurn)();
}

/**
 * Move a party member during combat and display the appropriate messages
 */
MoveReturnValue combatMovePartyMember(Direction dir, int userEvent) {    
    MoveReturnValue retval = movePartyMember(dir, userEvent);
    int i;

    /* active player left/fled combat */
    if ((retval & MOVE_EXIT_TO_PARENT) && (c->location->activePlayer == FOCUS)) {
        c->location->activePlayer = -1;
        /* assign active player to next available party member */
        for (i = 0; i < c->saveGame->members; i++) {
            if (combatInfo.party[i].obj && !playerIsDisabled(i)) {
                c->location->activePlayer = i;
                break;
            }
        }
    }

    screenMessage("%s\n", getDirectionName(dir));
    if (retval & MOVE_MUST_USE_SAME_EXIT)
        screenMessage("All must use same exit!\n");
    else if (retval & MOVE_BLOCKED)
        screenMessage("Blocked!\n");
    else if (retval & MOVE_SLOWED)
        screenMessage("Slow progress!\n");    

    return retval;
}

void combatMoveMonsters() {
    int i, target, distance;
    CombatAction action;
    CoordActionInfo *info;
    const Monster *m;
    PartyCombatInfo *party = combatInfo.party;
    MonsterCombatInfo *monsters = combatInfo.monsters;

    for (i = 0; i < AREA_MONSTERS; i++) {
        if (!monsters[i].obj)
            continue;
        m = monsters[i].obj->monster;

        /* see if monster wakes up if it is asleep */
        if ((monsters[i].status == STAT_SLEEPING) && (xu4_random(8) == 0)) {
            monsters[i].status = STAT_GOOD;
            monsters[i].obj->setAnimated();
        }

        /* if the monster is still asleep, then move on to the next monster */
        if (monsters[i].status == STAT_SLEEPING)
            continue;

        if (monsterNegates(m)) {
            c->aura = AURA_NEGATE;
            c->auraDuration = 2;
            statsUpdate();
        }

        /* default action */
        action = CA_ATTACK;        

        /* if the monster doesn't have something specific to do yet, let's try to find something! */
        if (action == CA_ATTACK) {
            /* monsters who teleport do so 1/8 of the time */
            if (monsterTeleports(m) && xu4_random(8) == 0)
                action = CA_TELEPORT;
            /* monsters who ranged attack do so 1/4 of the time.
               make sure their ranged attack is not negated! */
            else if (m->ranged != 0 && xu4_random(4) == 0 && 
                     ((m->rangedhittile != MAGICFLASH_TILE) || (c->aura != AURA_NEGATE)))
                action = CA_RANGED;
            /* monsters who cast sleep do so 1/4 of the time they don't ranged attack */
            else if (monsterCastSleep(m) && xu4_random(4) == 0)
                action = CA_CAST_SLEEP;
        
            else if (monsterGetStatus(m, monsters[i].hp) == MSTAT_FLEEING)
                action = CA_FLEE;
        }
        
        target = combatFindTargetForMonster(monsters[i].obj, &distance, action == CA_RANGED);
        if (target == -1 && action == CA_RANGED) {
            action = CA_ADVANCE;
            combatFindTargetForMonster(monsters[i].obj, &distance, 0);
        }
        if (target == -1)
            continue;

        if (action == CA_ATTACK && distance > 1)
            action = CA_ADVANCE;

        /* let's see if the monster blends into the background, or if he appears... */
        if (monsterCamouflages(m) && !combatHideOrShowCamouflageMonster(monsters[i].obj))
            continue; /* monster is hidden -- no action! */

        switch(action) {
        case CA_ATTACK:
            if (playerIsHitByAttack(&c->players[target])) {
                
                /* steal gold if the monster steals gold */
                if (monsterStealsGold(m) && xu4_random(4) == 0)
                    playerAdjustGold(-(xu4_random(0x3f)));
                
                /* steal food if the monster steals food */
                if (monsterStealsFood(m))
                    playerAdjustFood(-2500);
                               
                attackFlash(party[target].obj->getCoords(), HITFLASH_TILE, 3);

                playerApplyDamage(&c->players[target], monsterGetDamage(m));
                if (c->players[target].status == STAT_DEAD) {
                    MapCoords p = party[target].obj->getCoords();                    
                    mapRemoveObject(c->location->map, party[target].obj);
                    party[target].obj = NULL;
                    c->location->map->annotations->add(p, CORPSE_TILE)->setTTL(c->saveGame->members);
                    screenMessage("%s is Killed!\n", c->players[target].name);
                }
                statsUpdate();
            } else {
                attackFlash(party[target].obj->getCoords(), MISSFLASH_TILE, 3);
            }
            break;

        case CA_CAST_SLEEP:
            screenMessage("Sleep!\n");

            (*spellEffectCallback)('s', -1, (Sound)0); /* show the sleep spell effect */
            
            /* Apply the sleep spell to everyone still in combat */
            for (i = 0; i < 8; i++) {
                if ((party[i].obj != NULL) && xu4_random(2) == 0)
                    combatPutPlayerToSleep(i);                
            }

            statsUpdate();
            break;

        case CA_TELEPORT: {
                MapCoords new_c;
                int valid = 0;
                bool firstTry = true;                    
                MapTile tile;                
            
                while (!valid) {
                    new_c = MapCoords(xu4_random(c->location->map->width), xu4_random(c->location->map->height), c->location->coords.z);
                    
                    tile = (*c->location->tileAt)(c->location->map, new_c, WITH_OBJECTS);
                
                    if (tileIsMonsterWalkable(tile) && tileIsWalkable(tile)) {
                        /* If the tile would slow me down, try again! */
                        if (firstTry && tileGetSpeed(tile) != FAST)
                            firstTry = false;
                        /* OK, good enough! */
                        else
                            valid = 1;
                    }
                }
            
                /* Teleport! */
                combatInfo.monsters[i].obj->setCoords(new_c);
            }

            break;

        case CA_RANGED:
            {            
                MapCoords m_coords = combatInfo.monsters[i].obj->getCoords(),
                          p_coords = combatInfo.party[target].obj->getCoords();
            
                info = new CoordActionInfo;
                info->handleAtCoord = &combatMonsterRangedAttack;
                info->origin = m_coords;
                info->prev = MapCoords(-1, -1);
                info->range = 11;
                info->validDirections = MASK_DIR_ALL;
                info->player = i;
                info->blockedPredicate = &tileCanAttackOver;
                info->blockBefore = 1;
                info->firstValidDistance = 0;

                /* if the monster has a random tile for a ranged weapon,
                   let's switch it now! */
                if (monsterHasRandomRangedAttack(combatInfo.monsters[i].obj->monster))
                    monsterSetRandomRangedWeapon((Monster *)combatInfo.monsters[i].obj->monster);

                /* figure out which direction to fire the weapon */            
                info->dir = m_coords.getRelativeDirection(p_coords);
            
                /* fire! */
                gameDirectionalAction(info);
                delete info;
            } break;

        case CA_FLEE:
        case CA_ADVANCE:
            if (moveCombatObject(action, c->location->map, combatInfo.monsters[i].obj, combatInfo.party[target].obj->getCoords())) {
                MapCoords coords = combatInfo.monsters[i].obj->getCoords();

                if (MAP_IS_OOB(c->location->map, coords)) {
                    screenMessage("\n%s Flees!\n", m->name);
                    
                    /* Congrats, you have a heart! */
                    if (monsterIsGood(combatInfo.monsters[i].obj->monster))
                        playerAdjustKarma(KA_SPARED_GOOD);

                    mapRemoveObject(c->location->map, combatInfo.monsters[i].obj);
                    combatInfo.monsters[i].obj = NULL;
                }
            }
            
            break;
        }
    }
}

int combatFindTargetForMonster(const Object *monster, int *distance, int ranged) {
    int i, curDistance;
    int closest;    
    MapCoords m_coords = monster->getCoords();
    
    *distance = 20;
    closest = -1;
    for (i = 0; i < c->saveGame->members; i++) {
        MapCoords p_coords;

        if (!combatInfo.party[i].obj)
            continue;

        p_coords = combatInfo.party[i].obj->getCoords();

        /* find out how many moves it would take to get to the party member */
        if (ranged) 
            /* ranged attacks can go diagonally, so find the closest using diagonals */
            curDistance = m_coords.distance(p_coords);
        else
            /* normal attacks are n/e/s/w, so find the distance that way */
            curDistance = m_coords.movementDistance(p_coords);

        /* skip target if further than current target */
        if (curDistance > (*distance))
            continue;
        /* skip target 50% of time if same distance */
        if (curDistance == (*distance) && xu4_random(2) == 0)
            continue;
        
        (*distance) = curDistance;
        closest = i;
    }

    return closest;
}

/**
 * Applies 'damage' amount of damage to the monster
 */

void combatApplyDamageToMonster(int monster, int damage, int player) {
    int xp;
    const Monster *m = combatInfo.monsters[monster].obj->monster;

    /* deal the damage */
    if (m->id != LORDBRITISH_ID)
        combatInfo.monsters[monster].hp -= damage;

    switch (monsterGetStatus(m, combatInfo.monsters[monster].hp)) {

    case MSTAT_DEAD:
        xp = m->xp;
        screenMessage("%s Killed!\nExp. %d\n", m->name, xp);
        
        /* if a player killed the creature, then award the XP,
           otherwise, it died on its own */
        if (player >= 0) {
            playerAwardXp(&c->players[player], xp);
            if (monsterIsEvil(m))
                playerAdjustKarma(KA_KILLED_EVIL);
        }

        mapRemoveObject(c->location->map, combatInfo.monsters[monster].obj);
        combatInfo.monsters[monster].obj = NULL;
        break;

    case MSTAT_FLEEING:
        screenMessage("%s Fleeing!\n", m->name);
        break;

    case MSTAT_CRITICAL:
        screenMessage("%s Critical!\n", m->name);
        break;

    case MSTAT_HEAVILYWOUNDED:
        screenMessage("%s\nHeavily Wounded!\n", m->name);
        break;

    case MSTAT_LIGHTLYWOUNDED:
        screenMessage("%s\nLightly Wounded!\n", m->name);
        break;

    case MSTAT_BARELYWOUNDED:
        screenMessage("%s\nBarely Wounded!\n", m->name);
        break;
    }
}

/**
 * Show an attack flash at x, y. This is used for 'being hit' or 'being missed' 
 * by weapons, cannon fire, spells, etc.
 */

void attackFlash(MapCoords coords, MapTile tile, int timeFactor) {
    int i;
    int divisor = settings.battleSpeed;
    
    c->location->map->annotations->add(coords, tile, true);
    for (i = 0; i < timeFactor; i++) {        
        /* do screen animations while we're pausing */
        if (i % divisor == 1)
            screenCycle();

        gameUpdateScreen();       
        eventHandlerSleep(eventTimerGranularity/divisor);
    }
    c->location->map->annotations->remove(coords, tile);
}

/**
 * Key handler for choosing an attack direction
 */
bool combatChooseWeaponDir(int key, void *data) {
    CoordActionInfo *info = (CoordActionInfo *)eventHandlerGetKeyHandlerData();
    Direction dir = keyToDirection(key);
    bool valid = (dir != DIR_NONE) ? true : false;
    int weapon = c->players[info->player].weapon;    

    eventHandlerPopKeyHandler();
    info->dir = MASK_DIR(dir);

    if (valid) {
        screenMessage("%s\n", getDirectionName(dir));
        if (weaponCanChooseDistance(weapon)) {
            screenMessage("Range: ");
            eventHandlerPushKeyHandlerWithData(&combatChooseWeaponRange, info);
        }
        else gameDirectionalAction(info);        
    }

    eventHandlerPopKeyHandlerData();
    
    return valid || keyHandlerDefault(key, NULL);
}

/**
 * Key handler for choosing the range of a wepaon
 */
bool combatChooseWeaponRange(int key, void *data) {    
    CoordActionInfo *info = (CoordActionInfo *) data;    

    if ((key >= '0') && (key <= (info->range + '0'))) {
        info->range = key - '0';
        screenMessage("%d\n", info->range);
        gameDirectionalAction(info);

        eventHandlerPopKeyHandler();
        eventHandlerPopKeyHandlerData();

        return true;
    }
    
    return false;
}

/**
 * Apply tile effects to all monsters depending on what they're standing on
 */
void combatApplyMonsterTileEffects(void) {
    int i;
    bool affected = false;

    for (i = 0; i < AREA_MONSTERS; i++) {
        if (combatInfo.monsters[i].obj) {
            TileEffect effect;
            effect = tileGetEffect((*c->location->tileAt)(c->location->map, combatInfo.monsters[i].obj->getCoords(), WITH_GROUND_OBJECTS));

            if (effect != EFFECT_NONE) {

                /* give a slight pause before enacting the tile effect */
                if (!affected) {
                    gameUpdateScreen();
                    eventHandlerSleep(100);
                    affected = true;
                }

                switch(effect) {
                case EFFECT_SLEEP:
                    /* monster fell asleep! */
                    if ((combatInfo.monsters[i].obj->monster->resists != EFFECT_SLEEP) &&
                        (xu4_random(0xFF) >= combatInfo.monsters[i].hp)) {
                        combatInfo.monsters[i].status = STAT_SLEEPING;
                        combatInfo.monsters[i].obj->setAnimated(false); /* freeze monster */
                    }
                    break;

                case EFFECT_LAVA:
                case EFFECT_FIRE:
                    /* deal 0 - 127 damage to the monster if it is not immune to fire damage */
                    if (!(combatInfo.monsters[i].obj->monster->resists & (EFFECT_FIRE | EFFECT_LAVA)))
                        combatApplyDamageToMonster(i, xu4_random(0x7F), -1);
                    break;

                case EFFECT_POISONFIELD:
                    /* deal 0 - 127 damage to the monster if it is not immune to poison field damage */
                    if (combatInfo.monsters[i].obj->monster->resists != EFFECT_POISONFIELD)
                        combatApplyDamageToMonster(i, xu4_random(0x7F), -1);
                    break;

                case EFFECT_POISON:
                default: break;
                }
            }
        }
    }
}

int combatDivideMonster(const Object *obj) {
    int dirmask = mapGetValidMoves(c->location->map, obj->getCoords(), obj->getTile());
    Direction d = dirRandomDir(dirmask);

    /* this is a game enhancement, make sure it's turned on! */
    if (!settings.enhancements || !settings.enhancementsOptions.slimeDivides)
        return 0;
    
    /* make sure there's a place to put the divided monster! */
    if (d != DIR_NONE) {
        int index;
                            
        /* find the first free slot in the monster table, if there is one */
        for (index = 0; index < AREA_MONSTERS; index++) {
            if (combatInfo.monsters[index].obj == NULL) {
                MapCoords coords;                
                
                screenMessage("%s Divides!\n", obj->monster->name);

                /* find a spot to put our new monster */
                coords = obj->getCoords();
                coords.move(d, c->location->map);                

                /* create our new monster! */
                combatInfo.monsters[index].obj = mapAddMonsterObject(c->location->map, obj->monster, coords);
                combatInfo.monsters[index].hp = monsterGetInitialHp(combatInfo.monsters[index].obj->monster);
                combatInfo.monsters[index].status = STAT_GOOD;
                return 1;
            }
        }        
    }
    return 0;
}

/**
 * Returns the id of the nearest party member (0-8)
 * and fills 'dist' with the distance
 */
int combatNearestPartyMember(const Object *obj, int *dist) {
    int member, nearest = -1, d, leastDist = 0xFFFF;
    PartyCombatInfo *party = combatInfo.party;
    MapCoords o_coords = obj->getCoords();

    for (member = 0; member < c->saveGame->members; member++) {
        if (party[member].obj) {
            MapCoords p_coords = party[member].obj->getCoords();

            d = o_coords.distance(p_coords);
            if (d < leastDist) {
                nearest = member;
                leastDist = d;
            }
        }
    }

    if (nearest >= 0)
        *dist = leastDist;

    return nearest;
}

/**
 * Hides or shows a camouflaged monster, depending on its distance from
 * the nearest party member
 */
int combatHideOrShowCamouflageMonster(Object *monster) {
    /* find the nearest party member */
    int dist;
    int nearestMember = combatNearestPartyMember(monster, &dist);

    /* ok, now we've got the nearest party member.  Now, see if they're close enough */
    if (nearestMember >= 0) {
        if ((dist < 5) && !monster->isVisible())
            monster->setVisible(); /* show yourself */
        else if (dist >= 5)
            monster->setVisible(false); /* hide and take no action! */
    }

    return monster->isVisible();
}
