/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "item.h"

#include "annotation.h"
#include "codex.h"
#include "combat.h"
#include "context.h"
#include "debug.h"
#include "dungeon.h"
#include "event.h"
#include "game.h"
#include "location.h"
#include "map.h"
#include "mapmgr.h"
#include "names.h"
#include "player.h"
#include "portal.h"
#include "savegame.h"
#include "screen.h"
#include "tileset.h"
#include "u4.h"
#include "utils.h"

using std::string;

DestroyAllCreaturesCallback destroyAllCreaturesCallback;

void itemSetDestroyAllCreaturesCallback(DestroyAllCreaturesCallback callback) {
    destroyAllCreaturesCallback = callback;
}

int needStoneNames = 0;
unsigned char stoneMask = 0;

bool isRuneInInventory(void *virt);
void putRuneInInventory(void *virt);
bool isStoneInInventory(void *virt);
void putStoneInInventory(void *virt);
bool isItemInInventory(void *item);
bool isSkullInInventory(void *item);
void putItemInInventory(void *item);
void useBBC(void *item);
void useHorn(void *item);
void useWheel(void *item);
void useSkull(void *item);
void useStone(void *item);
void useKey(void *item);
bool isMysticInInventory(void *mystic);
void putMysticInInventory(void *mystic);
bool isWeaponInInventory(void *weapon);
void putWeaponInInventory(void *weapon);
void useTelescope(void *notused);
bool isReagentInInventory(void *reag);
void putReagentInInventory(void *reag);
bool isAbyssOpened(const Portal *p);
void itemHandleStones(const string &color);
void itemHandleVirtues(const string &virtue);

static const ItemLocation items[] = {
    { "Mandrake Root", NULL, "mandrake1",
      &isReagentInInventory, &putReagentInInventory, NULL, (void *) REAG_MANDRAKE, SC_NEWMOONS | SC_REAGENTDELAY },
    { "Mandrake Root", NULL, "mandrake2",
      &isReagentInInventory, &putReagentInInventory, NULL, (void *) REAG_MANDRAKE, SC_NEWMOONS | SC_REAGENTDELAY },
    { "Nightshade", NULL, "nightshade1",
      &isReagentInInventory, &putReagentInInventory, NULL, (void *) REAG_NIGHTSHADE, SC_NEWMOONS | SC_REAGENTDELAY},
    { "Nightshade", NULL, "nightshade2",
      &isReagentInInventory, &putReagentInInventory, NULL, (void *) REAG_NIGHTSHADE, SC_NEWMOONS | SC_REAGENTDELAY },    
    { "the Bell of Courage", "bell", "bell",
      &isItemInInventory, &putItemInInventory, &useBBC, (void *) ITEM_BELL, 0 },
    { "the Book of Truth", "book", "book",
      &isItemInInventory, &putItemInInventory, &useBBC, (void *) ITEM_BOOK, 0 },
    { "the Candle of Love", "candle", "candle",
      &isItemInInventory, &putItemInInventory, &useBBC, (void *) ITEM_CANDLE, 0 },    
    { "A Silver Horn", "horn", "horn",
      &isItemInInventory, &putItemInInventory, &useHorn, (void *) ITEM_HORN, 0 },
    { "the Wheel from the H.M.S. Cape", "wheel", "wheel",
      &isItemInInventory, &putItemInInventory, &useWheel, (void *) ITEM_WHEEL, 0 },
    { "the Skull of Modain the Wizard", "skull", "skull",
      &isSkullInInventory, &putItemInInventory, &useSkull, (void *) ITEM_SKULL, SC_NEWMOONS },
    { "the Red Stone", "red", "redstone",
      &isStoneInInventory, &putStoneInInventory, &useStone, (void *) STONE_RED, 0 },
    { "the Orange Stone", "orange", "orangestone",
      &isStoneInInventory, &putStoneInInventory, &useStone, (void *) STONE_ORANGE, 0 },
    { "the Yellow Stone", "yellow", "yellowstone",
      &isStoneInInventory, &putStoneInInventory, &useStone, (void *) STONE_YELLOW, 0 },
    { "the Green Stone", "green", "greenstone",
      &isStoneInInventory, &putStoneInInventory, &useStone, (void *) STONE_GREEN, 0 },
    { "the Blue Stone", "blue", "bluestone",
      &isStoneInInventory, &putStoneInInventory, &useStone, (void *) STONE_BLUE, 0 },
    { "the Purple Stone", "purple", "purplestone",
      &isStoneInInventory, &putStoneInInventory, &useStone, (void *) STONE_PURPLE, 0 },
    { "the Black Stone", "black", "blackstone",
      &isStoneInInventory, &putStoneInInventory, &useStone, (void *) STONE_BLACK, SC_NEWMOONS },
    { "the White Stone", "white", "whitestone",
      &isStoneInInventory, &putStoneInInventory, &useStone, (void *) STONE_WHITE, 0 },

    /* handlers for using generic objects */
    { NULL, "stone",  NULL, &isStoneInInventory, NULL, &useStone, NULL, 0 },
    { NULL, "stones", NULL, &isStoneInInventory, NULL, &useStone, NULL, 0 },
    { NULL, "key",    NULL, &isItemInInventory, NULL, &useKey, (void *)(ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T), 0 },
    { NULL, "keys",   NULL, &isItemInInventory, NULL, &useKey, (void *)(ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T), 0 },    
    
    /* Lycaeum telescope */
    { NULL, NULL, "telescope", NULL, &useTelescope, NULL, NULL, 0 },

    { "Mystic Armor", NULL, "mysticarmor",
      &isMysticInInventory, &putMysticInInventory, NULL, (void *) ARMR_MYSTICROBES, SC_FULLAVATAR },
    { "Mystic Swords", NULL, "mysticswords",
      &isMysticInInventory, &putMysticInInventory, NULL, (void *) WEAP_MYSTICSWORD, SC_FULLAVATAR },
    { "Laser Gun", NULL, "lasergun", // lol, where'd that come from?
      &isWeaponInInventory, &putWeaponInInventory, NULL, (void *)16 },
    { "the rune of Honesty", NULL, "honestyrune",
      &isRuneInInventory, &putRuneInInventory, NULL, (void *) RUNE_HONESTY, 0 },
    { "the rune of Compassion", NULL, "compassionrune",
      &isRuneInInventory, &putRuneInInventory, NULL, (void *) RUNE_COMPASSION, 0 },
    { "the rune of Valor", NULL, "valorrune",
      &isRuneInInventory, &putRuneInInventory, NULL, (void *) RUNE_VALOR, 0 },
    { "the rune of Justice", NULL, "justicerune",
      &isRuneInInventory, &putRuneInInventory, NULL, (void *) RUNE_JUSTICE, 0 },
    { "the rune of Sacrifice", NULL, "sacrificerune",
      &isRuneInInventory, &putRuneInInventory, NULL, (void *) RUNE_SACRIFICE, 0 },
    { "the rune of Honor", NULL, "honorrune",
      &isRuneInInventory, &putRuneInInventory, NULL, (void *) RUNE_HONOR, 0 },
    { "the rune of Spirituality", NULL, "spiritualityrune",
      &isRuneInInventory, &putRuneInInventory, NULL, (void *) RUNE_SPIRITUALITY, 0 },
    { "the rune of Humility", NULL, "humilityrune",
      &isRuneInInventory, &putRuneInInventory, NULL, (void *) RUNE_HUMILITY, 0 }
};

#define N_ITEMS (sizeof(items) / sizeof(items[0]))

bool isRuneInInventory(void *virt) {
    return c->saveGame->runes & (int)virt;
}

void putRuneInInventory(void *virt) {
    c->party->member(0)->awardXp(100);    
    c->party->adjustKarma(KA_FOUND_ITEM);
    c->saveGame->runes |= (int)virt;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

bool isStoneInInventory(void *virt) {
    /* generic test: does the party have any stones yet? */
    if (virt == NULL) 
        return (c->saveGame->stones > 0);
    /* specific test: does the party have a specific stone? */
    else return c->saveGame->stones & (int)virt;
}

void putStoneInInventory(void *virt) {
    c->party->member(0)->awardXp(200);
    c->party->adjustKarma(KA_FOUND_ITEM);
    c->saveGame->stones |= (int)virt;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

bool isItemInInventory(void *item) {
    return c->saveGame->items & (int)item;
}

bool isSkullInInventory(void *) {
    return (c->saveGame->items & (ITEM_SKULL | ITEM_SKULL_DESTROYED));
}

void putItemInInventory(void *item) {
    c->party->member(0)->awardXp(400);
    c->party->adjustKarma(KA_FOUND_ITEM);
    c->saveGame->items |= (int)item;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

/**
 * Use bell, book, or candle on the entrance to the Abyss
 */
void useBBC(void *item) {
    Coords abyssEntrance(0xe9, 0xe9);
    /* on top of the Abyss entrance */
    if (c->location->coords == abyssEntrance) {
        /* must use bell first */
        if ((int)item == ITEM_BELL) {
            screenMessage("\nThe Bell rings on and on!\n");
            c->saveGame->items |= ITEM_BELL_USED;
        }
        /* then the book */
        else if (((int)item == ITEM_BOOK) && (c->saveGame->items & ITEM_BELL_USED)) {
            screenMessage("\nThe words resonate with the ringing!\n");
            c->saveGame->items |= ITEM_BOOK_USED;
        }
        /* then the candle */
        else if (((int)item == ITEM_CANDLE) && (c->saveGame->items & ITEM_BOOK_USED)) {
            screenMessage("\nAs you light the Candle the Earth Trembles!\n");    
            c->saveGame->items |= ITEM_CANDLE_USED;
        }
        else screenMessage("\nHmm...No effect!\n");
    }
    /* somewhere else */
    else screenMessage("\nHmm...No effect!\n");
}

/**
 * Uses the silver horn
 */
void useHorn(void *item) {
    screenMessage("\nThe Horn sounds an eerie tone!\n");
    c->aura->set(Aura::HORN, 10);    
}

/**
 * Uses the wheel (if on board a ship)
 */
void useWheel(void *item) {
    if ((c->transportContext == TRANSPORT_SHIP) && (c->saveGame->shiphull == 50)) {
        screenMessage("\nOnce mounted, the Wheel glows with a blue light!\n");
        c->party->setShipHull(99);
    }
    else screenMessage("\nHmm...No effect!\n");    
}

/**
 * Uses or destroys the skull of Mondain
 */
void useSkull(void *item) {
    /* FIXME: check to see if the abyss must be opened first
       for the skull to be *able* to be destroyed */

    /* destroy the skull! pat yourself on the back */
    if (c->location->coords.x == 0xe9 && c->location->coords.y == 0xe9) {
        screenMessage("\n\nYou cast the Skull of Mondain into the Abyss!\n");

        c->saveGame->items = (c->saveGame->items & ~ITEM_SKULL) | ITEM_SKULL_DESTROYED;
        c->party->adjustKarma(KA_DESTROYED_SKULL);
    }

    /* use the skull... bad, very bad */
    else {
        screenMessage("\n\nYou hold the evil Skull of Mondain the Wizard aloft....\n");
    
        /* destroy all creatures */    
        (*destroyAllCreaturesCallback)();
    
        /* we don't lose the skull until we toss it into the abyss */
        //c->saveGame->items = (c->saveGame->items & ~ITEM_SKULL);
        c->party->adjustKarma(KA_USED_SKULL);
    }
}

/**
 * Handles using the virtue stones in dungeon altar rooms and on dungeon altars
 */
void useStone(void *item) {
    MapCoords coords;
    unsigned char stone = (unsigned char)((int)item);
    
    unsigned char truth   = STONE_WHITE | STONE_PURPLE | STONE_GREEN  | STONE_BLUE;
    unsigned char love    = STONE_WHITE | STONE_YELLOW | STONE_GREEN  | STONE_ORANGE;
    unsigned char courage = STONE_WHITE | STONE_RED    | STONE_PURPLE | STONE_ORANGE;
    static unsigned char *attr   = NULL;
    
    c->location->getCurrentPosition(&coords);

    /**
     * Named a specific stone (after using "stone" or "stones")
     */    
    if (item != NULL) {
        CombatMap *cm = getCombatMap();

        if (needStoneNames) {
            /* named a stone while in a dungeon altar room */
            if (c->location->context & CTX_ALTAR_ROOM) {
                needStoneNames--;                

                switch(cm->getAltarRoom()) {
                case VIRT_TRUTH: attr = &truth; break;
                case VIRT_LOVE: attr = &love; break;
                case VIRT_COURAGE: attr = &courage; break;
                default: break;
                }
                
                /* make sure we're in an altar room */
                if (attr) {
                    /* we need to use the stone, and we haven't used it yet */
                    if ((*attr & stone) && (stone & ~stoneMask))
                        stoneMask |= stone;
                    /* we already used that stone! */
                    else if (stone & stoneMask) {
                        screenMessage("\nAlready used!\n");
                        needStoneNames = 0;
                        stoneMask = 0; /* reset the mask so you can try again */                
                        return;
                    }
                }
                else ASSERT(0, "Not in an altar room!");

                /* see if we have all the stones, if not, get more names! */
                if (attr && needStoneNames) {
                    screenMessage("\n%c:", 'E'-needStoneNames);
                    itemHandleStones(gameGetInput());
                }
                /* all the stones have been entered, verify them! */
                else {
                    unsigned short key = 0xFFFF;
                    switch(cm->getAltarRoom()) {
                        case VIRT_TRUTH:    key = ITEM_KEY_T; break;
                        case VIRT_LOVE:     key = ITEM_KEY_L; break;
                        case VIRT_COURAGE:  key = ITEM_KEY_C; break;
                        default: break;
                    }

                    /* in an altar room, named all of the stones, and don't have the key yet... */
                    if (attr && (stoneMask == *attr) && !(c->saveGame->items & key)) {
                        screenMessage("\nThou doth find one third of the Three Part Key!\n");
                        c->saveGame->items |= key;
                    }
                    else screenMessage("\nHmm...No effect!\n");

                    stoneMask = 0; /* reset the mask so you can try again */                
                }
            }

            /* Otherwise, we're asking for a stone while in the abyss on top of an altar */
            else {                
                /* see if they entered the correct stone */
                if (stone == (1 << c->location->coords.z)) {
                    if (c->location->coords.z < 7) {
                        /* replace the altar with a down-ladder */
                        MapCoords coords;
                        screenMessage("\n\nThe altar changes before thyne eyes!\n");
                        c->location->getCurrentPosition(&coords);
                        c->location->map->annotations->add(coords, c->location->map->tileset->getByName("down_ladder")->id);
                    }
                    /* start chamber of the codex sequence... */
                    else {
                        codexStart();                        
                    }
                }
                else screenMessage("\nHmm...No effect!\n");
            }
        }
        else {
            screenMessage("\nNot a Usable Item!\n");            
            stoneMask = 0; /* reset the mask so you can try again */            
        }
    }

    /**
     * in the abyss, on an altar to place the stones
     */
    else if ((c->location->map->id == MAP_ABYSS) &&
             (c->location->context & CTX_DUNGEON) && 
             (dynamic_cast<Dungeon *>(c->location->map)->currentToken() == DUNGEON_ALTAR)) {

        int virtueMask = getBaseVirtues((Virtue)c->location->coords.z);
        if (virtueMask > 0)
            screenMessage("\n\nAs thou doth approach, a voice rings out: What virtue dost stem from %s?\n\n", getBaseVirtueName(virtueMask));
        else screenMessage("\n\nA voice rings out:  What virtue exists independently of Truth, Love, and Courage?\n\n");

        itemHandleVirtues(gameGetInput());
    }

    /**
     * in a dungeon altar room, on the altar
     */
    else if ((c->location->context & CTX_ALTAR_ROOM) &&
             coords.x == 5 && coords.y == 5) {
        needStoneNames = 4;
        screenMessage("\n\nThere are holes for 4 stones.\nWhat colors:\nA:");        

        itemHandleStones(gameGetInput());
    }
    else screenMessage("\nNo place to Use them!\n");
    // This used to say "\nNo place to Use them!\nHmm...No effect!\n"
    // That doesn't match U4DOS; does it match another?
}

void useKey(void *item) {
    screenMessage("\nNo place to Use them!\n");
}

bool isMysticInInventory(void *mystic) {
    /* FIXME: you could feasibly get more mystic weapons and armor if you
       have 8 party members and equip them all with everything,
       then search for Mystic Weapons/Armor again 

       or, you could just sell them all and search again.  What an easy
       way to make some cash!

       This would be a good candidate for an xu4 "extended" savegame
       format.
    */
    if (((int)mystic) == WEAP_MYSTICSWORD)
        return c->saveGame->weapons[WEAP_MYSTICSWORD] > 0;
    else if (((int)mystic) == ARMR_MYSTICROBES)
        return c->saveGame->armor[ARMR_MYSTICROBES] > 0;
    else
        ASSERT(0, "Invalid mystic item was tested in isMysticInInventory()");    
    return false;
}

void putMysticInInventory(void *mystic) {
    c->party->member(0)->awardXp(400);
    c->party->adjustKarma(KA_FOUND_ITEM);
    if (((int)mystic) == WEAP_MYSTICSWORD)
        c->saveGame->weapons[WEAP_MYSTICSWORD] += 8;
    else if (((int)mystic) == ARMR_MYSTICROBES)
        c->saveGame->armor[ARMR_MYSTICROBES] += 8;
    else
        ASSERT(0, "Invalid mystic item was added in putMysticInInventory()");        
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;
}

bool isWeaponInInventory(void *weapon) {
    if (c->saveGame->weapons[(int)weapon])
        return true;
    else {
        int i;
        for (i = 0; i < c->party->size(); i++) {
            if (c->party->member(i)->getWeapon() == (int)weapon)
                return true;
        }
    }
    return false;
}

void putWeaponInInventory(void *weapon) {
    c->saveGame->weapons[(int)weapon]++;
}

void useTelescope(void *notused) {
    screenMessage("You see a knob\non the telescope\nmarked A-P\nYou Select:");
    int choice = AlphaActionController::get('p', "You Select:");

    if (choice == -1)
        return;

    gamePeerCity(choice, NULL);
}

bool isReagentInInventory(void *reag) {
    return false;
}

void putReagentInInventory(void *reag) {
    c->party->adjustKarma(KA_FOUND_ITEM);
    c->saveGame->reagents[(int)reag] += xu4_random(8) + 2;
    c->saveGame->lastreagent = c->saveGame->moves & 0xF0;

    if (c->saveGame->reagents[(int)reag] > 99) {
        c->saveGame->reagents[(int)reag] = 99;
        screenMessage("Dropped some!\n");
    }
}

/**
 * Returns true if the specified conditions are met to be able to get the item
 */
bool itemConditionsMet(unsigned char conditions) {
    int i;

    if ((conditions & SC_NEWMOONS) &&
        !(c->saveGame->trammelphase == 0 && c->saveGame->feluccaphase == 0))
        return false;

    if (conditions & SC_FULLAVATAR) {
        for (i = 0; i < VIRT_MAX; i++) {
            if (c->saveGame->karma[i] != 0)
                return false;
        }
    }

    if ((conditions & SC_REAGENTDELAY) &&
        (c->saveGame->moves & 0xF0) == c->saveGame->lastreagent)
        return false;

    return true;
}

/**
 * Returns an item location record if a searchable object exists at
 * the given location. NULL is returned if nothing is there.
 */
const ItemLocation *itemAtLocation(const Map *map, const Coords &coords) {
    unsigned int i;
    for (i = 0; i < N_ITEMS; i++) {
        if (!items[i].locationLabel)
            continue;
        if (map->getLabel(items[i].locationLabel) == coords &&
            itemConditionsMet(items[i].conditions))
            return &(items[i]);
    }
    return NULL;
}

/**
 * Uses the item indicated by 'shortname'
 */
void itemUse(const string &shortname) {
    unsigned int i;
    const ItemLocation *item = NULL;

    for (i = 0; i < N_ITEMS; i++) {
        if (items[i].shortname &&
            strcasecmp(items[i].shortname, shortname.c_str()) == 0) {
            
            item = &items[i];

            /* item name found, see if we have that item in our inventory */
            if (!(*items[i].isItemInInventory) || (*items[i].isItemInInventory)(items[i].data)) {       

                /* use the item, if we can! */
                if (!item || !item->useItem)
                    screenMessage("\nNot a Usable item!\n");
                else
                    (*item->useItem)(items[i].data);
            }
            else
                screenMessage("\nNone owned!\n");            

            /* we found the item, no need to keep searching */
            break;
        }
    }

    /* item was not found */
    if (!item)
        screenMessage("\nNot a Usable item!\n");
}

/**
 * Checks to see if the abyss was opened
 */
bool isAbyssOpened(const Portal *p) {
    /* make sure the bell, book and candle have all been used */
    int items = c->saveGame->items;
    int isopened = (items & ITEM_BELL_USED) && (items & ITEM_BOOK_USED) && (items & ITEM_CANDLE_USED);
    
    if (!isopened)
        screenMessage("Enter Can't!\n");
    return isopened;
}

/**
 * Handles naming of stones when used
 */
void itemHandleStones(const string &color) {
    int i;
    int found = 0;    

    for (i = 0; i < 8; i++) {        
        if (strcasecmp(color.c_str(), getStoneName((Virtue)i)) == 0 &&
            isStoneInInventory((void *)(1<<i))) {
            found = 1;
            itemUse(color.c_str());
        }
    }
    
    if (!found) {
        screenMessage("\nNone owned!\n");
        stoneMask = 0; /* make sure stone mask is reset */
    }
}

/**
 * Handles naming of virtues when you use a stone on an altar in the Abyss
 */
void itemHandleVirtues(const string &virtue) {
    eventHandler->popKeyHandler();
        
    if (strncasecmp(virtue.c_str(), getVirtueName((Virtue)c->location->coords.z), 6) == 0) {
        /* now ask for stone */
        screenMessage("\n\nThe Voice says: Use thy Stone.\n\nColor:\n");
        needStoneNames = 1;

        itemHandleStones(gameGetInput());
    }
    else {
        screenMessage("\nHmm...No effect!\n");
        (*c->location->finishTurn)();
    }
}
