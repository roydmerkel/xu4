/*
 * $Id$
 */

#ifndef OBJECT_H
#define OBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

struct _Person;
struct _Monster;

typedef enum {
    MOVEMENT_FIXED,
    MOVEMENT_WANDER,
    MOVEMENT_FOLLOW_AVATAR,
    MOVEMENT_ATTACK_AVATAR
} ObjectMovementBehavior;

typedef enum {
    OBJECT_UNKNOWN,
    OBJECT_PERSON,
    OBJECT_MONSTER    
} ObjectType;

typedef struct _Object {
    unsigned char tile, prevtile;
    unsigned short x, y, z;
    unsigned short prevx, prevy;
    ObjectMovementBehavior movement_behavior;
    union {
        const struct _Person *person;
        const struct _Monster *monster;
    };
    int objType;
    int hasFocus;
    int isVisible;
    int canAnimate;
    struct _Object *next;
} Object;

#ifdef __cplusplus
}
#endif

#endif
