//
// Neercs
//

#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#if defined _XBOX
#   define _USE_MATH_DEFINES /* for M_PI */
#   include <xtl.h>
#elif defined _WIN32
#   define _USE_MATH_DEFINES /* for M_PI */
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#endif

#include "core.h"

using namespace std;
using namespace lol;

#include "../neercs.h"
#include "term.h"

Term::Term(ivec2 size)
  : m_time(0.f)
{
    m_caca = caca_create_canvas(size.x, size.y);

#if defined HAVE_PTY_H || defined HAVE_UTIL_H || defined HAVE_LIBUTIL_H
    m_pty = new Pty();
    char const *shell = getenv("SHELL");
    if (!shell)
        shell = "/bin/sh";
    shell = "cacaclock";
    shell = "cacafire";
    shell = "cacademo";
    m_pty->Run(shell, size);
#endif
}

void Term::TickGame(float seconds)
{
    Entity::TickGame(seconds);

#if defined HAVE_PTY_H || defined HAVE_UTIL_H || defined HAVE_LIBUTIL_H
    /* This is the real terminal code */
    /* XXX: for now we draw fancy shit */
    m_time += seconds;

    m_pty->SetWindowSize(ivec2(caca_get_canvas_width(m_caca),
                               caca_get_canvas_height(m_caca)));

    size_t total = 0;
    for (;;)
    {
        char buf[BUFSIZ];
        size_t current = m_pty->ReadData(buf, BUFSIZ);
        if (current <= 0)
            break;
        total += current;
        size_t processed = ReadAnsi(buf, current);
        if (processed < current)
            m_pty->UnreadData(buf + processed, current - processed);
        if (current < BUFSIZ)
            break;
//        if (total > 10000)
//            break;
    }
#else
    /* Unsupported platform - draw some fancy shit instead */
    m_time += seconds;
    DrawFancyShit();
#endif
}

void Term::TickDraw(float seconds)
{
    Entity::TickDraw(seconds);
}

Term::~Term()
{
#if defined HAVE_PTY_H || defined HAVE_UTIL_H || defined HAVE_LIBUTIL_H
    delete m_pty;
#endif
    caca_free_canvas(m_caca);
}

/*
 * XXX: fancy shit below
 */

static uint32_t hex_color(float r, float g, float b)
{
    return ((uint32_t)(r * 15.99f) << 8) |
           ((uint32_t)(g * 15.99f) << 4) |
           (uint32_t)(b * 15.99f);
}

void Term::DrawFancyShit()
{
    /* draw something awesome */
    int bg_color = 0x222;
    int w = caca_get_canvas_width(m_caca);
    int h = caca_get_canvas_height(m_caca);

    caca_set_color_argb(m_caca, 0xfff, bg_color);
    caca_clear_canvas(m_caca);

    caca_set_color_argb(m_caca, 0xfff, bg_color);
    for(int i = 0; i < h; i++)
    {
        float a = M_PI / 180 * i * 16 + m_time * 4;
        float b = M_PI / 180 * i * 12;
        int x = w / 2  - 14 + w / 4 * lol::cos(a) + w / 4 * lol::sin(b);
        caca_put_str(m_caca, x, i, "LOL WUT! NEERCS SI TEH RULEZ");
    }

    caca_set_color_argb(m_caca, 0x444, bg_color);
    for(int i = 0; i < w; i++)
    {
        float a = m_time * 1 + M_PI / 180 * i * 8;
        float b = m_time * -2 + M_PI / 180 * i * 5;
        int y = h / 2 + h / 4 * lol::cos(a) + h / 4 * lol::sin(b);
        caca_set_color_argb(m_caca, hex_color(0.25f + 0.5f / w * i - 0.25f / h * y, 0.25f, 0.25f + 0.25f / w * i + 0.25f / h * y), bg_color);
        caca_draw_line(m_caca, i, y - 1, i, y + 1,'%');
    }

/* __  _________ ______ ______ ______ ______
  /  \/  /  __  >  __  >  __  >  ___//  ___/ \x0a9
 /      /  ____/  ____/  __  <  <____\___  \
/__/\__/\_______________/  \________________\ */

    int logo_x = (w - 46) / 2;
    int logo_y = h / 2 - 2;

    caca_set_color_argb(m_caca, 0xbac, bg_color);
    //caca_set_color_argb(m_caca, hex_color(0.5f + 0.5f * lol::cos(m_time * 3               ), 0.5f, 0.5f + 0.25f * lol::sin(m_time * 3               )), bg_color);
    caca_put_str(m_caca, logo_x + 3, logo_y    ,"__  _________ ______ ______ ______ ______");
    //caca_set_color_argb(m_caca, hex_color(0.5f + 0.5f * lol::cos(m_time * 3 + M_PI / 4 * 1), 0.5f, 0.5f + 0.25f * lol::sin(m_time * 3 + M_PI / 4 * 1)), bg_color);
    caca_put_str(m_caca, logo_x + 2, logo_y + 1, "/  \\/  /  __  >  __  >  __  >  ___//  ___/ \x0a9");
    //caca_set_color_argb(m_caca, hex_color(0.5f + 0.5f * lol::cos(m_time * 3 + M_PI / 4 * 2), 0.5f, 0.5f + 0.25f * lol::sin(m_time * 3 + M_PI / 4 * 2)), bg_color);
    caca_put_str(m_caca, logo_x + 1, logo_y + 2, "/      /  ____/  ____/  __  <  <____\\___  \\");
    //caca_set_color_argb(m_caca, hex_color(0.5f + 0.5f * lol::cos(m_time * 3 + M_PI / 4 * 3), 0.5f, 0.5f + 0.25f * lol::sin(m_time * 3 + M_PI / 4 * 3)), bg_color);
    caca_put_str(m_caca, logo_x    , logo_y + 3, "/__/\\__/\\_______________/  \\________________\\");
    caca_set_color_argb(m_caca, 0x269, bg_color);
    caca_put_str(m_caca, logo_x + 5, logo_y + 5, "ALL YOUR TERMINALS ARE BELONG TO US");

    caca_set_color_argb(m_caca, 0x777, bg_color);
    caca_printf(m_caca, 2, h - 3, "W=%i", w);
    caca_printf(m_caca, 2, h - 2, "H=%i", h);

    caca_set_color_argb(m_caca, hex_color(0.6f + 0.4f * lol::cos(m_time * 2), 0.2f, 0.2f), bg_color);
    caca_put_str(m_caca, w - 12, h - 2, "CACA RULEZ");

/*
  _______
 /      /|
/______/ |
|      | |
|  :D  | /
|______|/
*/

    int lolcube_x = w / 2 - 5 + (w - 10) * lol::cos(m_time / 2);
    int lolcube_y = h - 5 - abs ((h - 5) * lol::sin(m_time * 4));

    caca_set_color_argb(m_caca, hex_color(0.5f + 0.5f * lol::sin(m_time * 2), 0.5f + 0.5f * lol::cos(m_time * 3), 0.5f + 0.5f * lol::sin(m_time * 5)), bg_color);
    caca_put_str(m_caca, lolcube_x + 2, lolcube_y    , "_______");
    caca_put_str(m_caca, lolcube_x + 1, lolcube_y + 1, "/      /|");
    caca_put_str(m_caca, lolcube_x    , lolcube_y + 2, "/______/ |");
    caca_put_str(m_caca, lolcube_x    , lolcube_y + 3, "|      | |");
    caca_put_str(m_caca, lolcube_x    , lolcube_y + 4, "|  :D  | /");
    caca_put_str(m_caca, lolcube_x    , lolcube_y + 5, "|______|/");

    caca_set_color_argb(m_caca, 0x777, bg_color);
    caca_put_str(m_caca, 0, 0, "rez@lol:~/ sudo -s");
    caca_put_str(m_caca, 0, 1, "[sudo] password for rez:");
    caca_put_str(m_caca, 0, 2, "root@lol:~/ echo LOL");
    caca_put_str(m_caca, 0, 3, "LOL");
    caca_put_str(m_caca, 0, 4, "root@lol:~/");
}
