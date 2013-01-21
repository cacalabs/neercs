//
// Neercs
//
// Copyright: (c) 2012-2013 Sam Hocevar <sam@hocevar.net>
//

#if defined HAVE_CONFIG_H
#   include "config.h"
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

#include <time.h>
#include <caca.h>

#include "core.h"

using namespace std;
using namespace lol;

#include "neercs.h"
#include "video/render.h"

extern "C"
{
#include "old/neercs.h"
}

Neercs::Neercs(int argc, char **argv)
  : m_term(new Term(ivec2(45, 16))),
    m_render(new Render(m_term->GetCaca())),
    m_ready(false)
{
    Ticker::Ref(m_term);
    Ticker::Ref(m_render);
}

void Neercs::TickGame(float seconds)
{
    WorldEntity::TickGame(seconds);
}

void Neercs::TickDraw(float seconds)
{
    WorldEntity::TickDraw(seconds);
}

Neercs::~Neercs()
{
    Ticker::Unref(m_term);
    Ticker::Unref(m_render);
}

int main(int argc, char **argv)
{
    System::Init(argc, argv);

    Application app("Neercs", ivec2(800, 600), 60.0f);

    new Neercs(argc, argv);
    app.ShowPointer(false);

    app.Run();

    return EXIT_SUCCESS;
}
