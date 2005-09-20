/*
 * $Id$
 */

#ifndef CAMP_H
#define CAMP_H

#include "combat.h"

#define CAMP_HEAL_INTERVAL  100   /* Number of moves before camping will heal the party */

class CampController : public CombatController {
public:
    CampController();
    virtual void init(Creature *m);
    virtual void begin();

private:
    bool heal();
};

#if 0
class InnController : public Controller {
public:
    void run();
    virtual bool keyPressed(int key);

private:
    bool heal();
};
#endif

void innBegin(void);

#endif
