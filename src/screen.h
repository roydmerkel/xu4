/*
 * $Id$
 */

#ifndef SCREEN_H
#define SCREEN_H

#include "direction.h"
#include "dngview.h"
#include "u4file.h"

#if __GNUC__
#define PRINTF_LIKE(x,y)  __attribute__ ((format (printf, (x), (y))))
#else
#define PRINTF_LIKE(x,y)
#endif

struct _Tileset;
struct _Image;

typedef enum {
    BKGD_SHAPES,
    BKGD_CHARSET,
    BKGD_BORDERS,
    BKGD_INTRO,
    BKGD_INTRO_EXTENDED,
    BKGD_TREE,
    BKGD_PORTAL,
    BKGD_OUTSIDE,
    BKGD_INSIDE,
    BKGD_WAGON,
    BKGD_GYPSY,
    BKGD_ABACUS,
    BKGD_HONCOM,
    BKGD_VALJUS,
    BKGD_SACHONOR,
    BKGD_SPIRHUM,
    BKGD_ANIMATE,
    BKGD_KEY,
    BKGD_HONESTY,
    BKGD_COMPASSN,
    BKGD_VALOR,
    BKGD_JUSTICE,
    BKGD_SACRIFIC,
    BKGD_HONOR,
    BKGD_SPIRIT,
    BKGD_HUMILITY,
    BKGD_TRUTH,
    BKGD_LOVE,
    BKGD_COURAGE,
    BKGD_STONCRCL,
    BKGD_RUNE_INF,
    BKGD_SHRINE_HON,
    BKGD_SHRINE_COM,
    BKGD_SHRINE_VAL,
    BKGD_SHRINE_JUS,
    BKGD_SHRINE_SAC,
    BKGD_SHRINE_HNR,
    BKGD_SHRINE_SPI,
    BKGD_SHRINE_HUM,
    BKGD_GEMTILES,
    BKGD_MAX
} BackgroundType;

typedef enum {
    MC_DEFAULT,
    MC_WEST,
    MC_NORTH,
    MC_EAST,
    MC_SOUTH
} MouseCursor;

typedef struct _MouseArea {
    int npoints;
    struct {
        int x, y;
    } point[4];
    MouseCursor cursor;
    int command[3];
} MouseArea;

#define SCR_CYCLE_PER_SECOND 4

void screenInit(void);
void screenDelete(void);
void screenReInit(void);

char **screenGetImageSetNames(void);
char **screenGetGemLayoutNames(void);

int screenLoadBackground(BackgroundType bkgd);
void screenDrawBackground(BackgroundType bkgd);
void screenDrawBackgroundInMapArea(BackgroundType bkgd);
void screenFreeBackgrounds();

void screenCycle(void);
void screenEraseMapArea(void);
void screenEraseTextArea(int x, int y, int width, int height);
void screenFindLineOfSight(void);
void screenGemUpdate(void);
void screenInvertRect(int x, int y, int w, int h);
void screenMessage(const char *fmt, ...) PRINTF_LIKE(1, 2);
void screenPrompt(void);
void screenRedrawMapArea(void);
void screenRedrawScreen(void);
void screenRedrawTextArea(int x, int y, int width, int height);
void screenScrollMessageArea(void);
void screenShake(int iterations);
void screenShowTile(struct _Tileset* tileset, unsigned char tile, int focus, int x, int y);
void screenShowGemTile(unsigned char tile, int focus, int x, int y);
void screenShowChar(int chr, int x, int y);
void screenShowCharMasked(int chr, int x, int y, unsigned char mask);
void screenTextAt(int x, int y, const char *fmt, ...) PRINTF_LIKE(3, 4);
void screenUpdate(int showmap, int blackout);
void screenUpdateCursor(void);
void screenUpdateMoons(void);
void screenUpdateWind(void);
unsigned char screenViewportTile(unsigned int width, unsigned int height, int x, int y, int *focus);

void screenAnimateIntro(int frame);
void screenFreeIntroAnimations();
void screenFreeIntroBackgrounds();
void screenShowCard(int pos, int card);
void screenShowAbacusBeads(int row, int selectedVirtue, int rejectedVirtue);
void screenShowBeastie(int beast, int vertoffset, int frame);

void screenShowCursor(void);
void screenHideCursor(void);
void screenEnableCursor(void);
void screenDisableCursor(void);
void screenSetCursorPos(int x, int y);

void screenDungeonDrawTile(int distance, unsigned char tile);
void screenDungeonDrawWall(int xoffset, int distance, Direction orientation, DungeonGraphicType type);

void screenSetMouseCursor(MouseCursor cursor);
int screenPointInMouseArea(int x, int y, MouseArea *area);

extern int screenCurrentCycle;

#define SCR_CYCLE_MAX 16

#endif
