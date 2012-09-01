//
// Neercs
//

#if !defined __TERM_TERM_H__
#define __TERM_TERM_H__

#include "term/pty.h"

class Term : public Entity
{
public:
    Term(ivec2 size);
    ~Term();

    char const *GetName() { return "<term>"; }
    caca_canvas_t *GetCaca() { return m_caca; }

protected:
    virtual void TickGame(float seconds);
    virtual void TickDraw(float seconds);

private:
    Pty *m_pty;
    caca_canvas_t *m_caca;
    ivec2 m_size;

    /* Mostly for fancy shit */
    void DrawFancyShit();
    float m_time;
};

#endif // __TERM_TERM_H__

