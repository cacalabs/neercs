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

extern bool g_setup;

Term::Term(ivec2 size)
  : m_pty(0),
    m_caca(caca_create_canvas(size.x, size.y)),
    m_size(size),
    m_title(0),
    m_time(0.f),
    m_debug(false)
{
#if defined HAVE_PTY_H || defined HAVE_UTIL_H || defined HAVE_LIBUTIL_H
    m_pty = new Pty();
    char const *shell = getenv("SHELL");
    if (!shell)
        shell = "/bin/sh";
    m_pty->Run(shell, size);
#endif
}

void Term::TickGame(float seconds)
{
    Entity::TickGame(seconds);

#if defined HAVE_PTY_H || defined HAVE_UTIL_H || defined HAVE_LIBUTIL_H
    if (!g_setup)
    {
        bool have_ctrl = Input::GetStatus(Key::LeftCtrl)
                          || Input::GetStatus(Key::RightCtrl);
        bool have_shift = Input::GetStatus(Key::LeftShift)
                           || Input::GetStatus(Key::RightShift);

        /* Check for standard ASCII keys */
        for (int i = 0x0; i < 0x7f; ++i)
        {
            if (Input::WasPressed((Key::Value)i))
            {
                if (have_ctrl && i >= 'a' && i <= 'z')
                {
                    char c = i + 1 - 'a';
                    m_pty->WriteData(&c, 1);
                }
                else if (have_shift && i >= 'a' && i <= 'z')
                {
                    char c = i + 'A' - 'a';
                    m_pty->WriteData(&c, 1);
                }
                else
                {
                    char c = i;
                    m_pty->WriteData(&c, 1);
                }
            }
        }

        /* Check for special keys */
        static struct { Key::Value k; char const *str; int len; } const lut[] =
        {
            { Key::Up, "\033OA", 3 },
            { Key::Down, "\033OB", 3 },
            { Key::Right, "\033OC", 3 },
            { Key::Left, "\033OD", 3 },
            { Key::PageUp, "\033[5~", 4 },
            { Key::PageDown, "\033[6~", 4 },
            { Key::Home, "\033[1~", 4 },
            { Key::Insert, "\033[2~", 4 },
            { Key::Delete, "\033[3~", 4 },
            { Key::End, "\033[4~", 4 },
#if 0 /* FIXME: disabled for now (used by the theme system */
            { Key::F1, "\033[11~", 5 },
            { Key::F2, "\033[12~", 5 },
            { Key::F3, "\033[13~", 5 },
            { Key::F4, "\033[14~", 5 },
            { Key::F5, "\033[15~", 5 },
            { Key::F6, "\033[17~", 5 },
            { Key::F7, "\033[18~", 5 },
            { Key::F8, "\033[19~", 5 },
            { Key::F9, "\033[20~", 5 },
            { Key::F10, "\033[21~", 5 },
            { Key::F11, "\033[23~", 5 },
            { Key::F12, "\033[24~", 5 },
#endif
        };

        for (size_t i = 0; i < sizeof(lut) / sizeof(*lut); i++)
        {
            if (!have_ctrl && !have_shift)
                if (Input::WasPressed(lut[i].k))
                    m_pty->WriteData(lut[i].str, lut[i].len);
        }
    }

    m_time += seconds;

    if (m_pty->IsEof())
    {
        /* FIXME: we could do more interesting things here… */
        Ticker::Shutdown();
    }

    m_pty->SetWindowSize(ivec2(caca_get_canvas_width(m_caca),
                               caca_get_canvas_height(m_caca)));

    /* This is the real terminal code */
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

    /* Some fancy shit if we press F3 */
    if (Input::WasPressed(Key::F3))
        m_debug = !m_debug;

    if (m_debug)
    {
        m_time += seconds;
        DrawFancyShit();
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

/*_______
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
