/**
 * $Id$
 */

#ifndef __TYPEDEFS_H
#define __TYPEDEFS_H

#include "direction.h"

typedef unsigned int TileId;
typedef unsigned char MapId;
//typedef unsigned char MapTile;

typedef enum {
    FAST,
    SLOW,
    VSLOW,
    VVSLOW
} TileSpeed;

typedef enum {
    EFFECT_NONE,
    EFFECT_FIRE,
    EFFECT_SLEEP,
    EFFECT_POISON,
    EFFECT_POISONFIELD,
    EFFECT_ELECTRICITY,
    EFFECT_LAVA
} TileEffect;

typedef enum {
    ANIM_NONE,
    ANIM_SCROLL,
    ANIM_CAMPFIRE,
    ANIM_CITYFLAG,
    ANIM_CASTLEFLAG,
    ANIM_SHIPFLAG,    
    ANIM_LCBFLAG,
    ANIM_FRAMES
} TileAnimationStyle;

/**
 * @todo
 *  <ul> 
 *      <li>relationship between MapTile, Tile, and TileSet is fuzzy</li>
 *  </ul>
 */
class MapTile {
public:
    MapTile() : id(0), frame(0), type(0) {}
    MapTile(const TileId &i, unsigned char f = 0) : id(i), frame(f), type(0) {}

    // Operators
    MapTile& operator=(const int &i) {
        id = i;
        frame = 0;
        return *this;
    }
    MapTile& operator=(const TileId &i) {
        id = i;
        frame = 0;
        return *this;
    }
    bool operator==(const MapTile &m) const  { return id == m.id; }
    bool operator==(const TileId &i) const   { return id == i; }
    bool operator!=(const MapTile &m) const  { return id != m.id; }
    bool operator!=(const TileId &i) const   { return id != i; }
    bool operator<(const MapTile &m) const   { return id < m.id; } /* for std::less */
    
    Direction getDirection() const;
    bool setDirection(Direction d);

    bool canWalkOn(Direction d) const;
    bool canWalkOff(Direction d) const;
    bool canAttackOver() const;
    bool canLandBalloon() const;
    bool isReplacement() const;
    bool isWalkable() const;
    bool isCreatureWalkable() const;
    bool isDungeonWalkable() const;
    bool isDungeonFloor() const;
    bool isSwimable() const;
    bool isSailable() const;
    bool isWater() const;
    bool isFlyable() const;
    bool isDoor() const;
    bool isLockedDoor() const;
    bool isChest() const;    
    bool isShip() const;    
    bool isPirateShip() const;
    bool isHorse() const;    
    bool isBalloon() const;    
    bool canDispel() const;
    bool canTalkOver() const;
    TileSpeed getSpeed() const;
    TileEffect getEffect() const;
    bool isOpaque() const;
    bool isForeground() const;

    static bool canTalkOverTile(MapTile tile);
    static bool canAttackOverTile(MapTile tile);
    static MapTile tileForClass(int klass);

    // Properties
    TileId id;
    unsigned char frame;
    unsigned char type;
};

#endif
