/*
 * $Id$
 */

#ifndef CODEX_H
#define CODEX_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _CodexEjectCode {
    CODEX_EJECT_NO_3_PART_KEY,
    CODEX_EJECT_BAD_WOP,
    CODEX_EJECT_NO_FULL_PARTY,
    CODEX_EJECT_NO_FULL_AVATAR,
    CODEX_EJECT_HONESTY,
    CODEX_EJECT_COMPASSION,
    CODEX_EJECT_VALOR,
    CODEX_EJECT_JUSTICE,
    CODEX_EJECT_SACRIFICE,
    CODEX_EJECT_HONOR,
    CODEX_EJECT_SPIRITUALITY,
    CODEX_EJECT_HUMILITY,
    CODEX_EJECT_TRUTH,
    CODEX_EJECT_LOVE,
    CODEX_EJECT_COURAGE,
    CODEX_EJECT_BAD_INFINITY 
} CodexEjectCode;

void codexStart();

#ifdef __cplusplus
}
#endif

#endif
