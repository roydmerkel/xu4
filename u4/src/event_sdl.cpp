/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <SDL.h>
#include "u4.h"

#include "context.h"
#include "error.h"
#include "event.h"
#include "screen.h"
#include "settings.h"
#include "u4_sdl.h"

extern bool verbose, quit;
extern int eventTimerGranularity;

KeyHandler::KeyHandler(Callback func, void *d, bool asyncronous) :
    handler(func),
    async(asyncronous),
    data(d)
{}

/**
 * Sets the key-repeat characteristics of the keyboard.
 */
int KeyHandler::setKeyRepeat(int delay, int interval) {
    return SDL_EnableKeyRepeat(delay, interval);
}

/**
 * Handles any and all keystrokes.
 * Generally used to exit the application, switch applications,
 * minimize, maximize, etc.
 */
bool KeyHandler::globalHandler(int key) {
    switch(key) {
#if defined(MACOSX)
    case U4_META + 'q': /* Cmd+q */
    case U4_META + 'x': /* Cmd+x */
#endif
    case U4_ALT + 'x': /* Alt+x */
#if defined(WIN32)
    case U4_ALT + U4_FKEY + 3:
#endif
        quit = true;
        EventHandler::setExitFlag();
        return true;
    default: return false;
    }
}

/**
 * A default key handler that should be valid everywhere
 */
bool KeyHandler::defaultHandler(int key, void *data) {
    bool valid = true;

    switch (key) {
    case '`':
        if (c && c->location)
            printf("x = %d, y = %d, level = %d, tile = %d\n", c->location->coords.x, c->location->coords.y, c->location->coords.z, c->location->map->tileAt(c->location->coords, WITH_OBJECTS)->id);
        break;
    default:
        valid = false;
        break;
    }

    return valid;
}

/**
 * A key handler that ignores keypresses
 */
bool KeyHandler::ignoreKeys(int key, void *data) {
    return true;
}

/**
 * Handles a keypress.
 * First it makes sure the key combination is not ignored
 * by the current key handler. Then, it passes the keypress
 * through the global key handler. If the global handler
 * does not process the keystroke, then the key handler
 * handles it itself by calling its handler callback function.
 */ 
bool KeyHandler::handle(int key) {
    bool processed = false;
    if (!isKeyIgnored(key)) {
        processed = globalHandler(key);
        if (!processed)
            processed = handler(key, data);
    }
    
    return processed;
}

/**
 * Returns true if the key or key combination is always ignored by xu4
 */
bool KeyHandler::isKeyIgnored(int key) {
    switch(key) {
    case U4_RIGHT_SHIFT:
    case U4_LEFT_SHIFT:
    case U4_RIGHT_CTRL:
    case U4_LEFT_CTRL:
    case U4_RIGHT_ALT:
    case U4_LEFT_ALT:
    case U4_RIGHT_META:
    case U4_LEFT_META:
    case U4_TAB:
        return true;
    default: return false;
    }
}

bool KeyHandler::operator==(Callback cb) const {
    return (handler == cb) ? true : false;        
}

/**
 * Constructs a timed event manager object.
 * Adds a timer callback to the SDL subsystem, which
 * will drive all of the timed events that this object
 * controls.
 */
TimedEventMgr::TimedEventMgr(int i) : baseInterval(i) {
    /* start the SDL timer */    
    if (instances == 0) {
        if (u4_SDL_InitSubSystem(SDL_INIT_TIMER) < 0)
            errorFatal("unable to init SDL: %s", SDL_GetError());
    }

    id = static_cast<void*>(SDL_AddTimer(i, &TimedEventMgr::callback, this));
    instances++;
}

/**
 * Destructs a timed event manager object.
 * It removes the callback timer and un-initializes the
 * SDL subsystem if there are no other active TimedEventMgr
 * objects.
 */
TimedEventMgr::~TimedEventMgr() {
    SDL_RemoveTimer(static_cast<SDL_TimerID>(id));
    id = NULL;
    
    if (instances == 1)
        u4_SDL_QuitSubSystem(SDL_INIT_TIMER);

    if (instances > 0)
        instances--;
}

/**
 * Adds an SDL timer event to the message queue.
 */
unsigned int TimedEventMgr::callback(unsigned int interval, void *param) {
    SDL_Event event;

    event.type = SDL_USEREVENT;
    event.user.code = 0;
    event.user.data1 = param;
    event.user.data2 = NULL;
    SDL_PushEvent(&event);

    return interval;
}

/**
 * Re-initializes the timer manager to a new timer granularity
 */ 
void TimedEventMgr::reset(unsigned int interval) {
    baseInterval = interval;
    SDL_RemoveTimer(static_cast<SDL_TimerID>(id));
    id = static_cast<void*>(SDL_AddTimer(baseInterval, &TimedEventMgr::callback, this));
}

/**
 * Constructs an event handler object. 
 */
EventHandler::EventHandler() : timer(eventTimerGranularity) {    
    SDL_EnableUNICODE(1);
}

/**
 * Delays program execution for the specified number of milliseconds.
 */
void EventHandler::sleep(unsigned int usec) {
    SDL_Delay(usec);
}

void EventHandler::main(void (*updateScreen)(void)) {
    if (updateScreen)
        (*updateScreen)();
    screenRedrawScreen();

    while (!exit) {
        int processed = 0;
        SDL_Event event;
        MouseArea *area;
        KeyHandler *kh = getKeyHandler();

        SDL_WaitEvent(&event);

        switch (event.type) {
        case SDL_KEYDOWN: {
            int key;

            if (event.key.keysym.unicode != 0)
                key = event.key.keysym.unicode & 0x7F;
            else
                key = event.key.keysym.sym;

            if (event.key.keysym.mod & KMOD_ALT)
#if defined(MACOSX)
                key = U4_ALT + event.key.keysym.sym; // macosx translates alt keys into strange unicode chars
#else
                key += U4_ALT;
#endif
            if (event.key.keysym.mod & KMOD_META)
                key += U4_META;

            if (event.key.keysym.sym == SDLK_UP)
                key = U4_UP;
            else if (event.key.keysym.sym == SDLK_DOWN)
                key = U4_DOWN;
            else if (event.key.keysym.sym == SDLK_LEFT)
                key = U4_LEFT;
            else if (event.key.keysym.sym == SDLK_RIGHT)
                key = U4_RIGHT;
            else if (event.key.keysym.sym == SDLK_BACKSPACE ||
                     event.key.keysym.sym == SDLK_DELETE)
                key = U4_BACKSPACE;

            if (verbose)
                printf("key event: unicode = %d, sym = %d, mod = %d; translated = %d\n", 
                       event.key.keysym.unicode, 
                       event.key.keysym.sym, 
                       event.key.keysym.mod, 
                       key);

            /* handle the keypress */
            processed = kh->handle(key);           

            if (processed) {
                if (updateScreen)
                    (*updateScreen)();
                screenRedrawScreen();
            }
            break;
        }

        case SDL_MOUSEBUTTONDOWN: {
            int button = event.button.button - 1;

            if (!settings.mouseOptions.enabled)
                break;

            if (button > 2)
                button = 0;
            area = eventHandler.mouseAreaForPoint(event.button.x, event.button.y);
            if (!area || area->command[button] == 0)
                break;
            kh->handle(area->command[button]);            
            if (updateScreen)
                (*updateScreen)();
            screenRedrawScreen();
            break;
        }

        case SDL_MOUSEMOTION:
            if (!settings.mouseOptions.enabled)
                break;

            area = eventHandler.mouseAreaForPoint(event.button.x, event.button.y);
            if (area)
                screenSetMouseCursor(area->cursor);
            else
                screenSetMouseCursor(MC_DEFAULT);
            break;

        case SDL_USEREVENT:
            eventHandler.getTimer()->tick();            
            break;

        case SDL_ACTIVEEVENT:
            if (event.active.state & SDL_APPACTIVE) {            
                // application was previously iconified and is now being restored
                if (event.active.gain) {
                    if (updateScreen)
                        (*updateScreen)();
                    screenRedrawScreen();
                }                
            }
            break;

        case SDL_QUIT:
            ::exit(0);
            break;
        }
    }
}

/**
 * Returns true if the queue is empty of events that match 'mask'. 
 */
 bool EventHandler::timerQueueEmpty() {
    SDL_Event event;

    if (SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_EVENTMASK(SDL_USEREVENT)))
        return false;
    else
        return true;
}

/**
 * Adds a key handler to the stack.
 */ 
void EventHandler::pushKeyHandler(KeyHandler kh) {
    KeyHandler *new_kh = new KeyHandler(kh);
    keyHandlers.push_back(new_kh);
}

/**
 * Pops a key handler off the stack.
 * Returns a pointer to the resulting key handler after
 * the current handler is popped.
 */ 
KeyHandler *EventHandler::popKeyHandler() {
    if (!keyHandlers.empty()) {
        delete keyHandlers.back();
        keyHandlers.pop_back();
        return getKeyHandler();
    }
    else return NULL;
}

/**
 * Returns a pointer to the current key handler.
 * Returns NULL if there is no key handler.
 */ 
KeyHandler *EventHandler::getKeyHandler() const {
    if (keyHandlers.empty())
        return NULL;
    else return keyHandlers.back();
}

/**
 * Eliminates all key handlers and begins stack with new handler.
 * This pops all key handlers off the stack and adds
 * the key handler provided to the stack, making it the
 * only key handler left. Use this function only if you
 * are sure the key handlers in the stack are disposable.
 */ 
void EventHandler::setKeyHandler(KeyHandler kh) {
    while (popKeyHandler() != NULL) {}
    pushKeyHandler(kh);
}

MouseArea* EventHandler::mouseAreaForPoint(int x, int y) {
    int i;
    MouseArea *areas = getMouseAreaSet();

    if (!areas)
        return NULL;

    for (i = 0; areas[i].npoints != 0; i++) {
        if (screenPointInMouseArea(x, y, &(areas[i]))) {
            return &(areas[i]);
        }
    }
    return NULL;
}