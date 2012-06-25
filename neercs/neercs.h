//
// Neercs
//
// Copyright: (c) 2012 Sam Hocevar <sam@hocevar.net>
//

#if !defined __NEERCS_H__
#define __NEERCS_H__

#include <caca.h>

#include "video/render.h"

class Neercs : public WorldEntity
{
public:
    Neercs();
    virtual ~Neercs();

    char const *GetName() { return "<neercs>"; }

protected:
    virtual void TickGame(float seconds);
    virtual void TickDraw(float seconds);

private:
    bool m_ready;
    caca_canvas_t *m_caca;
    Render *m_render;
    float m_time;
};

#endif // __NEERCS_H__

