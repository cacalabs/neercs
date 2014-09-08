//
// Neercs
//
// Copyright: (c) 2012 Sam Hocevar <sam@hocevar.net>
//

#pragma once

#include <caca.h>

#include "video/render.h"
#include "term/term.h"

extern "C"
{
#include "old/neercs.h"
}

class Neercs : public WorldEntity
{
public:
    Neercs(int argc, char **argv);
    virtual ~Neercs();

    char const *GetName() { return "<neercs>"; }

protected:
    virtual void TickGame(float seconds);
    virtual void TickDraw(float seconds, Scene &scene);

private:
    Term *m_term;
    Render *m_render;

    bool m_ready;
};

