/*
 * $Id$
 */

#ifndef AURA_H
#define AURA_H

#include <string>

#include "observable.h"

using std::string;

/**
 * Aura class
 */
class Aura : public Observable<string> {
public:
    enum Type {
        NONE,
        HORN,
        JINX,
        NEGATE,
        PROTECTION,
        QUICKNESS
    };

    Aura();

    int getDuration() const;
    Type getType() const;
    bool isActive() const;

    void setDuration(int d);
    void set(Type = NONE, int d = 0);
    void setType(Type t);

    bool operator==(const Type &t) const;
    bool operator!=(const Type &t) const;

    void passTurn();

private:
    Type type;
    int duration;
};

#endif
