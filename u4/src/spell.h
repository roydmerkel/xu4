/*
 * $Id$
 */

#ifndef SPELL_H
#define SPELL_H

#include <stddef.h>

#include "savegame.h"

typedef enum {
    CASTERR_NOERROR,            /* success */
    CASTERR_NOMIX,              /* no mixture available */
    CASTERR_WRONGCONTEXT,       /* e.g. spell must be cast in combat */
    CASTERR_MPTOOLOW,           /* caster doesn't have enough mp */
    CASTERR_FAILED              /* the spell failed */
} SpellCastError;

typedef enum {
    SPELLPRM_NONE,              /* none */
    SPELLPRM_PLAYER,            /* number of a player required */
    SPELLPRM_DIR,               /* direction required */
    SPELLPRM_PHASE,             /* phase required (gate) */
    SPELLPRM_FROMDIR            /* direction from required (winds) */
} SpellParam;

typedef struct _Spell {
    const char *name;
    int components;
    int context;
    int (*spellFunc)(int);
    SpellParam paramType;
    int mp;
} Spell;

const char *spellGetName(unsigned int spell);
int spellMix(unsigned int spell, int n_regs, int *regs);
SpellParam spellGetParamType(unsigned int spell);
int spellCast(unsigned int spell, int character, int param, SpellCastError *error);

#endif
