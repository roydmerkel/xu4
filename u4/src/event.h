/*
 * $Id$
 */

#ifndef EVENT_H
#define EVENT_H

#define U4_UP '['
#define U4_DOWN '/'
#define U4_LEFT ';'
#define U4_RIGHT '\''
#define U4_BACKSPACE 8
#define U4_SPACE ' '
#define U4_ESC 27
#define U4_ENTER 13
#define U4_ALT 128
#define U4_FKEY 282

extern int eventTimerGranularity;

typedef int (*KeyHandler)(int, void *);
typedef void (*TimerCallback)(void *);

typedef struct KeyHandlerInfo {
    KeyHandler kh;
    void *data;
} KeyHandlerInfo;

/** Additional information to be passed as data param for read buffer key handler */
typedef struct ReadBufferActionInfo {
    int (*handleBuffer)(const char *);
    char *buffer;
    int bufferLen;
    int screenX, screenY;
} ReadBufferActionInfo;

/** Additional information to be passed as data param for get choice key handler */
typedef struct GetChoiceActionInfo {
    const char *choices;
    int (*handleChoice)(int);
} GetChoiceActionInfo;

void eventHandlerInit(void);
void eventHandlerDelete(void);
void eventHandlerSleep(int usec);
void eventHandlerMain(void (*updateScreen)(void));
int eventHandlerTimerQueueEmpty(void);
void eventHandlerSetExitFlag(int flag);
int eventHandlerGetExitFlag();
void eventHandlerAddTimerCallback(TimerCallback callback, int interval);
void eventHandlerAddTimerCallbackData(TimerCallback callback, void *data, int interval);
void eventHandlerRemoveTimerCallback(TimerCallback callback);
void eventHandlerRemoveTimerCallbackData(TimerCallback callback, void *data);
void eventHandlerCallTimerCallbacks();
void eventHandlerResetTimerCallbacks();
void eventHandlerPushKeyHandler(KeyHandler kh);
void eventHandlerPushKeyHandlerData(KeyHandler kh, void *data);
void eventHandlerPopKeyHandler();
void eventHandlerSetKeyHandler(KeyHandler kh);
KeyHandler eventHandlerGetKeyHandler();
void *eventHandlerGetKeyHandlerData();
int eventKeyboardSetKeyRepeat(int delay, int interval);

int keyHandlerDefault(int key, void *data);
int keyHandlerIgnoreKeys(int key, void *data);
int keyHandlerReadBuffer(int key, void *data);
int keyHandlerGetChoice(int key, void *data);

#endif
