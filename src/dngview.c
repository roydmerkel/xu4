/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>

#include "u4.h"

#include "dngview.h"

#include "context.h"
#include "debug.h"
#include "dungeon.h"
#include "list.h"
#include "location.h"
#include "savegame.h"
#include "tile.h"

ListNode *dungeonViewGetTiles(int fwd, int side) {
    int x, y;
    int focus;

    switch (c->saveGame->orientation) {
    case DIR_WEST:
        x = c->location->x - fwd;
        y = c->location->y - side;
        break;

    case DIR_NORTH:
        x = c->location->x + side;
        y = c->location->y - fwd;
        break;

    case DIR_EAST:
        x = c->location->x + fwd;
        y = c->location->y + side;
        break;

    case DIR_SOUTH:
        x = c->location->x - side;
        y = c->location->y + fwd;
        break;
    
    case DIR_ADVANCE:
    case DIR_RETREAT:
    default:
        ASSERT(0, "Invalid dungeon orientation");        
    }
    if (MAP_IS_OOB(c->location->map, x, y)) {        
        while (x < 0)
            x += c->location->map->width;
        while (y < 0)
            y += c->location->map->height;
        while (x >= (int)c->location->map->width)
            x -= c->location->map->width;
        while (y >= (int)c->location->map->height)
            y -= c->location->map->height;
    }

    return locationTilesAt(c->location, x, y, c->location->z, &focus);
}

DungeonGraphicType dungeonViewTilesToGraphic(ListNode *tiles) {
    unsigned char tile = (unsigned char) (unsigned) tiles->data;
    DungeonToken token;

    /* 
     * check if the dungeon tile has an annotation or object on top
     * (always displayed as a tile, unless a ladder)
     */
    if (listLength(tiles) > 1) {
        switch (tile) {
        case LADDERUP_TILE:
            return DNGGRAPHIC_LADDERUP;
        case LADDERDOWN_TILE:
            return DNGGRAPHIC_LADDERDOWN;
    
        default:
            return DNGGRAPHIC_BASETILE;
        }
    }

    /* 
     * if not an annotation or object, then the tile is a dungeon
     * token 
     */
    token = dungeonTokenForTile(tile);

    switch (token) {
    case DUNGEON_TRAP:
    case DUNGEON_CORRIDOR:
        return DNGGRAPHIC_NONE;
    case DUNGEON_WALL:
    case DUNGEON_SECRET_DOOR:
        return DNGGRAPHIC_WALL;
    case DUNGEON_ROOM:
    case DUNGEON_DOOR:
        return DNGGRAPHIC_DOOR;
    case DUNGEON_LADDER_UP:
        return DNGGRAPHIC_LADDERUP;
    case DUNGEON_LADDER_DOWN:
        return DNGGRAPHIC_LADDERDOWN;
    
    default:
        return DNGGRAPHIC_DNGTILE;
    }
}