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

#if USE_SDL && defined __APPLE__
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

Neercs::Neercs()
  : m_ready(false),
    m_caca(caca_create_canvas(47, 32)),
    m_render(new Render(m_caca)),
    m_time(0.f)
{
    Ticker::Ref(m_render);
}

void Neercs::TickGame(float seconds)
{
    WorldEntity::TickGame(seconds);

    m_time += seconds;

    caca_set_color_argb(m_caca, 0xfff, 0x222);
    caca_clear_canvas(m_caca);

    caca_fill_ellipse(m_caca, 20+10 * lol::cos(m_time * 1.f), 10+10 * lol::sin(m_time * 1.f), 16+8 * lol::sin(m_time * 6.f), 12+6 * lol::cos(m_time * 5.f), '|');
    caca_fill_ellipse(m_caca, 20+10 * lol::cos(m_time * 1.f), 10+10 * lol::sin(m_time * 1.f), 12+8 * lol::sin(m_time * 6.f), 8+6 * lol::cos(m_time * 5.f), ' ');

    caca_set_color_argb(m_caca, 0xcef, 0x222);
    int x1 = 12 + 10 * lol::cos(m_time * 5.f);
    int y1 = 6 + 5 * lol::sin(m_time * 5.f);
    int x2 = 30 + 5 * lol::cos(m_time * 8.f);
    int y2 = 8 + 5 * lol::sin(m_time * 8.f);
    int y3 = 8 + 5 * lol::cos(m_time * 5.f);
    caca_draw_thin_line(m_caca, x1, y1, x2, y2);
    caca_draw_thin_line(m_caca, 40, y3, x2, y2);
    caca_draw_thin_line(m_caca, x1, y1, 40, y3);

    int x3 = 13 + 7 * lol::cos(m_time * 3.f);
    caca_set_color_ansi(m_caca, CACA_CYAN, CACA_BLUE);
    caca_put_str(m_caca, x3, 3, " LOL WUT ");

    int x4 = 6 + 5 * lol::cos(m_time * 2.f);
    caca_set_color_ansi(m_caca, CACA_YELLOW, CACA_RED);
    caca_put_str(m_caca, x4, 25, "Le Caca C'Est Surpuissant \\:D/");

    caca_put_str(m_caca, 0, 0, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    caca_put_str(m_caca, 0, 1, " !\"#$%&'()*+,-./0123456789");

    if (Input::GetButtonState(27 /*SDLK_ESCAPE*/))
        Ticker::Shutdown();

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

    new Neercs();
    new DebugFps(5, 5);
    app.ShowPointer(false);

    app.Run();

    return EXIT_SUCCESS;
}

