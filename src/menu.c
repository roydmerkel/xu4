/*
 * $Id$
 */

#include <stdlib.h>
#include "menu.h"

#include "list.h"
#include "screen.h"

/**
 * Adds an item to the menu list and returns the menu
 */
Menu menuAddItem(Menu menu, unsigned char id, char *text, short x, short y, ActivateMenuItem activate, ActivateAction activateOn) {
    MenuItem *menuItem = (MenuItem *)malloc(sizeof(MenuItem));

    menuItem->id = id;
    menuItem->text = text;
    menuItem->x = x;
    menuItem->y = y;
    menuItem->isHighlighted = 0;
    menuItem->isVisible = 1;
    menuItem->activateMenuItem = activate;
    menuItem->activateOn = activateOn;
    
    return listAppend(menu, menuItem);
}

/**
 * Returns the MenuItem structure attached to the current menu item
 */
MenuItem *menuGetItem(Menu current) {
    return ((MenuItem *)current->data);
}

/**
 * Displays the menu
 */
int menuShow(Menu menu) {
    Menu current;

    for (current = menu; current; current = current->next) {
        MenuItem *menuItem = menuGetItem(current);
        
        if (menuItem->isVisible) {
            screenTextAt(menuItem->x, menuItem->y, menuItem->text);
            if (menuItem->isHighlighted) {
                screenEnableCursor();
                screenSetCursorPos(menuItem->x - 2, menuItem->y);
                screenShowCursor();
            }
        }
    }
    return 1;
}

/**
 * Returns the next available menu item
 */
Menu menuGetNextItem(Menu current) {
    if (current->next != NULL) {
        /* if the item is not visible, skip it! */
        if (menuGetItem(current->next)->isVisible)
            return current->next;
        return menuGetNextItem(current->next);
    }
    /* wrap around to first node in the list */
    else return menuGetRoot(current);
}

/**
 * Returns the previous available menu item
 */
Menu menuGetPreviousItem(Menu current) {
    if (current->prev != NULL) {
        /* if the item is not visible, skip it! */
        if (menuGetItem(current->prev)->isVisible)
            return current->prev;
        else return menuGetPreviousItem(current->prev);        
    }
    else {
        /* wrap around to last node in the list */
        Menu m = current;
        while (m->next)
            m = m->next;
        return m;
    }
}

/**
 * Returns the base (root) menu item
 */
Menu menuGetRoot(Menu current) {
    Menu m = current;
    while (m->prev)
        m = m->prev;
    return m;
}

/**
 * Removes highlighting from 'oldItem' and adds it to 'newItem'
 */
Menu menuHighlightNew(Menu oldItem, Menu newItem) {
    if (oldItem) menuGetItem(oldItem)->isHighlighted = 0;
    if (newItem) menuGetItem(newItem)->isHighlighted = 1;
    return newItem;
}

/**
 * Sets the visibility of a menu item.  If menu items are
 * not visible, then they cannot be selected (just as if
 * they didn't exist).
 */
void menuItemSetVisible(Menu item, int visible) {
    menuGetItem(item)->isVisible = visible;    
}

/**
 * Deletes the menu and frees the memory associated with it
 */
void menuDelete(Menu menu) {
    Menu current;

    for (current = menuGetRoot(menu); current; current = current->next) {
        if (current->data)
            free(current->data);
    }

    listDelete(menu);
}

/**
 * 'Resets' the menu.  This does the following:
 *      - un-highlights all menu items
 *      - highlights the first menu item
 *      - returns the menu to the first item in the list
 */
Menu menuReset(Menu current) {
    Menu item,
         m = menuGetRoot(current);    

    /* un-highlight each menu item */
    for (item = m; item; item = item->next)
        menuGetItem(item)->isHighlighted = 0;        

    /* highlight the first menu item */
    menuGetItem(m)->isHighlighted = 1;    
    return m;
}

/**
 * Returns the menu item associated with the given 'id'
 */
Menu menuGetItemById(Menu menu, unsigned char id) {        
    return listFind(menu, (void *)id, &menuCompareFindItemById);
}

/**
 * Used internally to compare 'id's of menu items
 */
int menuCompareFindItemById(void *val1, void *val2) {
    MenuItem *menuItem = (MenuItem *)val1;
    unsigned char id = (unsigned char)val2;

    if (menuItem->id > id)
        return 1;
    else if (menuItem->id < id)
        return -1;
    return 0;
}

/**
 * Activates the menu item given by 'id', using 'action' to
 * activate it.  If the menu item cannot be activated using
 * 'action', then it is not activated.  This also un-highlights
 * the menu item given by 'menu' and highlights the new menu
 * item that was found for 'id'.
 */
Menu menuActivateItem(Menu menu, short id, ActivateAction action) {
    Menu m, newItem;
    MenuItem *mi;

    m = newItem = menu;    
    
    /* find the given id */
    if (id >= 0)
        newItem = menuGetItemById(menuGetRoot(menu), (unsigned char)id);

    m = menuHighlightNew(menu, newItem);

    mi = menuGetItem(m);
    /* make sure the action given will activate the menu item */
    if (mi && (mi->activateOn & action) && mi->activateMenuItem)
        (*mi->activateMenuItem)(m, action);

    return m;
}
