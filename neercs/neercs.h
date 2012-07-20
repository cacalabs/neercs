//
// Neercs
//
// Copyright: (c) 2012 Sam Hocevar <sam@hocevar.net>
//

#if !defined __NEERCS_H__
#define __NEERCS_H__

#include <caca.h>

#include "video/render.h"

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

    int hex_color(float r, float g, float b);

protected:
    virtual void TickGame(float seconds);
    virtual void TickDraw(float seconds);

private:
    bool m_ready;
    caca_canvas_t *m_caca;
    Render *m_render;
    float m_time;

    /* Old neercs stuff */
    char *m_buf;
    struct screen_list *m_screen_list;
};

#endif // __NEERCS_H__

