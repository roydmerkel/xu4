/*
 * $Id$
 */

#ifndef LOCATION_H
#define LOCATION_H

#include "map.h"
#include "movement.h"
#include "types.h"

struct _Tileset;

typedef xu4_list<MapTile>* MapTileList;

typedef MapTile (*TileAt)(const Map *map, MapCoords coords, int withObjects);

typedef enum {
    CTX_WORLDMAP    = 0x0001,
    CTX_COMBAT      = 0x0002,
    CTX_CITY        = 0x0004,
    CTX_DUNGEON     = 0x0008,
    CTX_ALTAR_ROOM  = 0x0010
} LocationContext;

#define CTX_ANY             (LocationContext)(0xffff)
#define CTX_NORMAL          (LocationContext)(CTX_WORLDMAP | CTX_CITY)
#define CTX_NON_COMBAT      (LocationContext)(CTX_ANY & ~CTX_COMBAT)
#define CTX_CAN_SAVE_GAME   (LocationContext)(CTX_WORLDMAP | CTX_DUNGEON)

typedef void (*FinishTurnCallback)(void);

typedef struct _Location {
    MapCoords coords;    
    Map *map;
    int viewMode;
    LocationContext context;
    FinishTurnCallback finishTurn;
    MoveCallback move;
    TileAt tileAt;
    struct _Tileset *tileset;
    int activePlayer;
    struct _Location *prev;
} Location;

Location *locationNew(MapCoords coords, Map *map, int viewmode, LocationContext ctx, FinishTurnCallback finishTurnCallback, MoveCallback moveCallback, TileAt tileAtCallback, struct _Tileset *tileset, Location *prev);
MapTile locationVisibleTileAt(Location *location, MapCoords coords, int *focus);
MapTileList locationTilesAt(Location *location, MapCoords coords, int *focus);
MapTile locationGetReplacementTile(Location *location, MapCoords coords);
int locationGetCurrentPosition(Location *location, MapCoords *coords);
void locationFree(Location **stack);

#endif
