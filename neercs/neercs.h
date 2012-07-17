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
    Neercs(int argc, char **argv);
    virtual ~Neercs();

    char const *GetName() { return "<neercs>"; }

    int hex_color(float r, float g, float b);

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

