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

typedef int (*KeyHandler)(int, void *);

typedef struct KeyHandlerNode {
    KeyHandler kh;
    void *data;
    struct KeyHandlerNode *next;
} KeyHandlerNode;

void eventHandlerMain();
void eventHandlerPushKeyHandler(KeyHandler kh);
void eventHandlerPushKeyHandlerData(KeyHandler kh, void *data);
void eventHandlerPopKeyHandler();
KeyHandler eventHandlerGetKeyHandler();
void *eventHandlerGetKeyHandlerData();

int keyHandlerDefault(int key, void *data);
int keyHandlerNormal(int key, void *data);
int keyHandlerGetDirection(int key, void *data);
int keyHandlerTalking(int key, void *data);
int keyHandlerQuit(int key, void *data);
int keyHandlerZtats(int key, void *data);
int keyHandlerZtats2(int key, void *data);

#endif
