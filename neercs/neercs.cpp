//
// Neercs
//
// Copyright: (c) 2012 Sam Hocevar <sam@hocevar.net>
//

#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#if defined _WIN32
#   include <direct.h>
#endif

#if defined _XBOX
#   define _USE_MATH_DEFINES /* for M_PI */
#   include <xtl.h>
#   undef near /* Fuck Microsoft */
#   undef far /* Fuck Microsoft again */
#elif defined _WIN32
#   define _USE_MATH_DEFINES /* for M_PI */
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   undef near /* Fuck Microsoft */
#   undef far /* Fuck Microsoft again */
#else
#   include <cmath>
#endif

#if USE_SDL
#   include <SDL_main.h>
#endif

#include <time.h>
#include <caca.h>

#include "core.h"
#include "loldebug.h"

using namespace std;
using namespace lol;

#include "neercs.h"
#include "video/render.h"

extern "C"
{
#include "old/neercs.h"
}

Neercs::Neercs(int argc, char **argv)
  : m_ready(false),
    m_caca(caca_create_canvas(10, 10)),
    m_render(new Render(m_caca)),
    m_time(0.f)
{
    Ticker::Ref(m_render);
}

int Neercs::hex_color(float r, float g, float b)
{
return ((int)(r * 15.99f) << 8) | ((int)(g * 15.99f) << 4) | (int)(b * 15.99f);
}

void Neercs::TickGame(float seconds)
{
    WorldEntity::TickGame(seconds);

    m_time += seconds;

    /* draw something */
    int bg_color = 0x222;
    int w = caca_get_canvas_width(m_caca);
    int h = caca_get_canvas_height(m_caca);

    caca_set_color_argb(m_caca, 0xfff, bg_color);
    caca_clear_canvas(m_caca);

    caca_set_color_argb(m_caca, 0xa46, bg_color);
    for(int i = 0; i < h; i++)
    {
        float a = M_PI / 180 * i * 16 + m_time * 4;
        float b = M_PI / 180 * i * 12;
        int x = w / 2  - 15 + h / 3 * lol::cos(a) + h / 4 * lol::sin(b);
        caca_put_str(m_caca, x, i, "LOL WUT! NEERCS SI TEH RULEZ!");
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

/*
.___  __ ______ ______ ______  ______ ______,
 \° \|  /° __  \° __  \° __  \/° ___//° ___/
 /   `  >  ____/  ____/  ,_  <_ <____\___  \
/__/\___\______\______\__| \__/_____________\
*/

    int logo_x = (w - 45) / 2;
    int logo_y = h / 2 - 2;

    caca_set_color_argb(m_caca, hex_color(0.5f + 0.5f * lol::cos(m_time * 3               ), 0.5f, 0.5f + 0.25f * lol::sin(m_time * 3               )), bg_color);
    caca_put_str(m_caca, logo_x, logo_y    ,".___  __ ______ ______ ______  ______ ______,");
    caca_set_color_argb(m_caca, hex_color(0.5f + 0.5f * lol::cos(m_time * 3 + M_PI / 4 * 1), 0.5f, 0.5f + 0.25f * lol::sin(m_time * 3 + M_PI / 4 * 1)), bg_color);
    caca_put_str(m_caca, logo_x, logo_y + 1, " \\° \\|  /° __  \\° __  \\° __  \\/° ___//° ___/");
    caca_set_color_argb(m_caca, hex_color(0.5f + 0.5f * lol::cos(m_time * 3 + M_PI / 4 * 2), 0.5f, 0.5f + 0.25f * lol::sin(m_time * 3 + M_PI / 4 * 2)), bg_color);
    caca_put_str(m_caca, logo_x, logo_y + 2, " /   `  >  ____/  ____/  ,_  <_ <____\\___  \\");
    caca_set_color_argb(m_caca, hex_color(0.5f + 0.5f * lol::cos(m_time * 3 + M_PI / 4 * 3), 0.5f, 0.5f + 0.25f * lol::sin(m_time * 3 + M_PI / 4 * 3)), bg_color);
    caca_put_str(m_caca, logo_x, logo_y + 3, "/__/\\___\\______\\______\\__| \\__/_____________\\");
    caca_set_color_argb(m_caca, 0xdef, bg_color);
    caca_put_str(m_caca, logo_x + 5, logo_y + 5, "ALL YOUR TERMINALS ARE BELONG TO US");

    caca_set_color_argb(m_caca, 0x666, bg_color);
    caca_printf(m_caca, 2, h - 3, "W=%i", w);
    caca_printf(m_caca, 2, h - 2, "H=%i", h);

    caca_set_color_argb(m_caca, hex_color(0.6f + 0.4f * lol::cos(m_time * 2), 0.2f, 0.2f), bg_color);
    caca_put_str(m_caca, w - 12, h - 2, "CACA RULEZ");
}


void Neercs::TickDraw(float seconds)
{
    WorldEntity::TickDraw(seconds);
}

Neercs::~Neercs()
{
    Ticker::Unref(m_render);
}

int main(int argc, char **argv)
{
    Application app("Neercs", ivec2(800, 600), 60.0f);

#if defined _MSC_VER && !defined _XBOX
    _chdir("..");
#elif defined _WIN32 && !defined _XBOX
    _chdir("../..");
#endif

    new Neercs(argc, argv);
    new DebugFps(2, 2);
    app.ShowPointer(false);

    app.Run();

    return EXIT_SUCCESS;
}

