/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "context.h"
#include "savegame.h"
#include "map.h"
#include "item.h"
#include "player.h"

extern Map world_map;
extern Map britain_map;
extern Map yew_map;
extern Map minoc_map;
extern Map trinsic_map;
extern Map jhelom_map;
extern Map moonglow_map;
extern Map lcb_1_map;
extern Map paws_map;
extern Map cove_map;
extern Map lycaeum_map;
extern Map empath_map;
extern Map serpent_map;

int isRuneInInventory(void *virt);
void putRuneInInventory(void *virt);
int isStoneInInventory(void *virt);
void putStoneInInventory(void *virt);
int isItemInInventory(void *item);
void putItemInInventory(void *item);
int isMysticInInventory(void *mystic);
void putMysticInInventory(void *mystic);
int isReagentInInventory(void *reag);
void putReagentInInventory(void *reag);

static const ItemLocation items[] = {
    { "Mandrake Root", 182, 54, &world_map, &isReagentInInventory, &putReagentInInventory, (void *) REAG_MANDRAKE, SC_NEWMOONS | SC_REAGENTDELAY },
    { "Mandrake Root", 100, 165, &world_map, &isReagentInInventory, &putReagentInInventory, (void *) REAG_MANDRAKE, SC_NEWMOONS | SC_REAGENTDELAY },
    { "Nightshade", 46, 149, &world_map, &isReagentInInventory, &putReagentInInventory, (void *) REAG_NIGHTSHADE, SC_NEWMOONS | SC_REAGENTDELAY},
    { "Nightshade", 205, 44, &world_map, &isReagentInInventory, &putReagentInInventory, (void *) REAG_NIGHTSHADE, SC_NEWMOONS | SC_REAGENTDELAY },
    { "the Bell of Courage", 176, 208, &world_map, &isItemInInventory, &putItemInInventory, (void *) ITEM_BELL, 0 },
    { "A Silver Horn", 45, 173, &world_map, &isItemInInventory, &putItemInInventory, (void *) ITEM_HORN, 0 },
    { "the Wheel from the H.M.S. Cape", 96, 215, &world_map, &isItemInInventory, &putItemInInventory, (void *) ITEM_WHEEL, 0 },
    { "the Skull of Modain the Wizard", 197, 245, &world_map, &isItemInInventory, &putItemInInventory, (void *) ITEM_SKULL, SC_NEWMOONS },
    { "the Black Stone", 224, 133, &world_map, &isStoneInInventory, &putStoneInInventory, (void *) STONE_BLACK, SC_NEWMOONS },
    { "the White Stone", 64, 80, &world_map, &isStoneInInventory, &putStoneInInventory, (void *) STONE_WHITE, 0 },
    { "the Book of Truth", 6, 6, &lycaeum_map, &isItemInInventory, &putItemInInventory, (void *) ITEM_BOOK, 0 },
    { "the Candle of Love", 22, 1, &cove_map, &isItemInInventory, &putItemInInventory, (void *) ITEM_CANDLE, 0 },
    /* FIXME: object at 22, 3 in the lycaeum */
    { "Mystic Armor", 22, 4, &empath_map, &isMysticInInventory, &putMysticInInventory, (void *) ARMR_MYSTICROBES, SC_FULLAVATAR },
    { "Mystic Swords", 8, 15, &serpent_map, &isMysticInInventory, &putMysticInInventory, (void *) WEAP_MYSTICSWORD, SC_FULLAVATAR },
    { "the rune of Honesty", 8, 6, &moonglow_map, &isRuneInInventory, &putRuneInInventory, (void *) RUNE_HONESTY, 0 },
    { "the rune of Compassion", 25, 1, &britain_map, &isRuneInInventory, &putRuneInInventory, (void *) RUNE_COMPASSION, 0 },
    { "the rune of Valor", 30, 30, &jhelom_map, &isRuneInInventory, &putRuneInInventory, (void *) RUNE_VALOR, 0 },
    { "the rune of Justice", 13, 6, &yew_map, &isRuneInInventory, &putRuneInInventory, (void *) RUNE_JUSTICE, 0 },
    { "the rune of Sacrifice", 28, 30, &minoc_map, &isRuneInInventory, &putRuneInInventory, (void *) RUNE_SACRIFICE, 0 },
    { "the rune of Honor", 2, 29, &trinsic_map, &isRuneInInventory, &putRuneInInventory, (void *) RUNE_HONOR, 0 },
    { "the rune of Spirituality", 17, 8, &lcb_1_map, &isRuneInInventory, &putRuneInInventory, (void *) RUNE_SPIRITUALITY, 0 },
    { "the rune of Humility", 29, 29, &paws_map, &isRuneInInventory, &putRuneInInventory, (void *) RUNE_HUMILITY, 0 }
};

int isRuneInInventory(void *virt) {
    return c->saveGame->runes & (int)virt;
}

void putRuneInInventory(void *virt) {
    c->saveGame->players[0].xp += 100;
    playerAdjustKarma(c->saveGame, KA_FOUND_ITEM);
    c->saveGame->runes |= (int)virt;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

int isStoneInInventory(void *virt) {
    return c->saveGame->stones & (int)virt;
}

void putStoneInInventory(void *virt) {
    c->saveGame->players[0].xp += 200;
    playerAdjustKarma(c->saveGame, KA_FOUND_ITEM);
    c->saveGame->stones |= (int)virt;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

int isItemInInventory(void *item) {
    return c->saveGame->items & (int)item;
}

void putItemInInventory(void *item) {
    c->saveGame->players[0].xp += 400;
    playerAdjustKarma(c->saveGame, KA_FOUND_ITEM);
    c->saveGame->items |= (int)item;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

int isMysticInInventory(void *mystic) {
    if (((int)mystic) == WEAP_MYSTICSWORD)
        return c->saveGame->weapons[WEAP_MYSTICSWORD] > 0;
    else if (((int)mystic) == ARMR_MYSTICROBES)
        return c->saveGame->armor[ARMR_MYSTICROBES] > 0;
    else {
        assert(0);
        return 0;
    }
}

void putMysticInInventory(void *mystic) {
    c->saveGame->players[0].xp += 400;
    playerAdjustKarma(c->saveGame, KA_FOUND_ITEM);
    if (((int)mystic) == WEAP_MYSTICSWORD)
        c->saveGame->weapons[WEAP_MYSTICSWORD] += 8;
    else if (((int)mystic) == ARMR_MYSTICROBES)
        c->saveGame->armor[ARMR_MYSTICROBES] += 8;
    else
        assert(0);
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

int isReagentInInventory(void *reag) {
    return 0;
}

void putReagentInInventory(void *reag) {
    playerAdjustKarma(c->saveGame, KA_FOUND_ITEM);
    c->saveGame->reagents[(int)reag] += rand() % 8 + 2;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

int itemConditionsMet(unsigned char conditions) {
    int i;

    if ((conditions & SC_NEWMOONS) &&
        !(c->saveGame->trammelphase == 7 && c->saveGame->feluccaphase == 7))
        return 0;

    if (conditions & SC_FULLAVATAR) {
        for (i = 0; i < VIRT_MAX; i++) {
            if (c->saveGame->karma[i] != 0)
                return 0;
        }
    }

    if ((conditions & SC_REAGENTDELAY) &&
        (c->saveGame->moves & 0xF0) == c->saveGame->lastreagent)
        return 0;

    return 1;
}

/**
 * Returns an item location record if a searchable object exists at
 * the given location. NULL is returned if nothing is there.
 */
const ItemLocation *itemAtLocation(const Map *map, unsigned char x, unsigned char y) {
    int i;
    for (i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
        if (items[i].map == map && 
            items[i].x == x && 
            items[i].y == y &&
            itemConditionsMet(items[i].conditions))
            return &(items[i]);
    }
    return NULL;
}

