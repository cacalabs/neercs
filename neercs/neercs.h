//
// Neercs
//
// Copyright: (c) 2012 Sam Hocevar <sam@hocevar.net>
//

#if !defined __NEERCS_H__
#define __NEERCS_H__

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
    virtual void TickDraw(float seconds);

private:
    Term *m_term;
    Render *m_render;

    bool m_ready;
};

#endif // __NEERCS_H__

