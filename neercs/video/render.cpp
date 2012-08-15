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

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>

#include "core.h"
#include "lolgl.h"

using namespace std;
using namespace lol;

#include "../neercs.h"
#include "render.h"
#include "text-render.h"

extern char const *lolfx_blurh;
extern char const *lolfx_blurv;
extern char const *lolfx_remanency;
extern char const *lolfx_glow;
extern char const *lolfx_postfx;
extern char const *lolfx_radial;
extern char const *lolfx_simple;

#define PID M_PI/180.0f    // pi ratio
#define CR 1.0f/256.0f     // color ratio

/*
 * Various variables
 */

int active = true;         // window active flag
float nearplane = 0.1f;    // nearplane
float farplane = 1000.0f;  // farplane
int polygon_fillmode = GL_FILL; // fill mode
/* timer variable */
float timer = 0;           // timer
float timer_key = 0;       // key timer
float timer_key_repeat = 0.25f;// key repeat delay
/* window variable */
ivec2 screen_size;         // screen size
vec3 screen_color = CR * vec3(32, 32, 32); // screen color
/* object variable */
float main_angle = 0.0f;   // main angle
float part_angle = 0.0f;   // part angle
float fx_angle;            // current angle
/* fs_quad variable */
float fs_quad_vtx[] = {-1.0f, 1.0f, 0, 1.0f, -1.0f, -1.0f, 0, 1.0f, 1.0f, -1.0f, 0, 1.0f, 1.0f, 1.0f, 0, 1.0f};
float fs_quad_tex[] = {0, 1.0f, 0, 0, 1.0f, 0, 1.0f, 1.0f};
/* flash variable */
bool flash_flag = false;   // flag
float flash_angle = 0;     // angle
float flash_value = 0;     // value
float flash_speed = 1.5f;  // speed
/* fade variable */
bool fade_flag = false;    // flag
float fade_angle = 0;      // angle
float fade_value = 0;      // value
float fade_speed = 0.2f;   // speed
/* sync variable */
bool sync_flag = false;    // flagsh
float sync_angle = 0;      // angle
float sync_value = 1.0f;   // value
float sync_speed = 1.0f;   // speed
/* beat variable */
bool beat_flag = false;    // flag
float beat_angle = 0;      // angle
float beat_value = 0;      // value
float beat_speed = 2.0f;   // speed
/* window variable */
ivec2 border;              // border width
/* text variable */
ivec2 ratio_2d(2,4);       // 2d ratio
ivec2 map_size(256,256);   // texture map size
ivec2 font_size(8,8);      // font size
ivec2 canvas_char(0,0);    // canvas char number
ivec2 canvas_size(0,0);    // caca size
/* setup variable */
bool setup_switch=false;        // switch [option/item]
int setup_option=0;             // selected option
int setup_option_n=9;           // option number
int setup_item=0;               // selected item
int setup_item_n=8;             // item number
int setup_item_key=0;           // item array key
ivec2 setup_p(1,1);             // position [x,y]
ivec3 setup_size(30,0,12);      // size [w,h,split]
ivec2 setup_color(0x678,0x234); // size [w,h]
char const *setup_text[] = {
    "remanency",
        "enable",
        "buffer new frame",
        "buffer old frame",
        "source mix",
        "buffer mix",
        "",
        "",
        "",
    "glow",
        "enable",
        "source mix",
        "glow mix",
        "large center",
        "large corner",
        "small center",
        "small corner",
        "",
    "blur",
        "enable",
        "blur center",
        "blur corner",
        "",
        "",
        "",
        "",
        "",
    "screen",
        "enable",
        "deform ratio",
        "zoom base",
        "zoom variable",
        "corner radius",
        "corner blur",
        "",
        "",
    "color",
        "filter red",
        "filter green",
        "filter blue",
        "brightness",
        "contrast",
        "grayscale",
        "",
        "",
    "noise",
        "offset h",
        "offset v",
        "noise",
        "aberration",
        "retrace strength",
        "retrace length",
        "retrace speed",
        "",
    "ghost",
        "back distance",
        "back strength",
        "front distance",
        "front strength",
        "",
        "",
        "",
        "",
    "moire",
        "h base",
        "h variable",
        "h repeat x",
        "h repeat y",
        "v base",
        "v variable",
        "v repeat x",
        "v repeat y",
    "scanline",
        "h base",
        "h variable",
        "h repeat x",
        "h repeat y",
        "v base",
        "v variable",
        "v repeat x",
        "v repeat y"
    };
/* common variable */
float value, angle, radius, scale, speed;
/* shader variable */
vec2 buffer(0.7f,0.3f);         // [new frame mix,old frame mix]
vec2 remanency(0.3f,0.7f);      // remanency [source mix,buffer mix]
vec2 glow_mix(0.6f,0.4f);       // glow mix [source mix,glow mix]
vec2 glow_large(2.0f,2.0f);     // large glow radius [center,corner]
vec2 glow_small(1.0f,1.0f);     // small glow radius [center,corner]
vec2 blur(0.25f,0.5f);          // glow radius [center,corner]
//vec3 radial(2.0f,0.8f,0);     // radial [mix,strength,color mode]
//------------------------------// [IDEAS] http://www.youtube.com/watch?v=d1qEP2vMe-I
vec2 postfx_deform(0.7f,0.54f); // deformation [ratio,zoom]
vec3 postfx_filter(0.8f,0.9f,0.4f);   // color filter [red,green,blue]
vec3 postfx_color(1.8f,1.5f,0.5f);    // color modifier [brightness,contrast,grayscale]
vec2 postfx_corner(0.75f,0.95f);      // corner [radius,blur]
vec3 postfx_retrace(0.05f,2.0f,4.0f); // retrace [strength,length,speed]
vec2 postfx_offset(3.0f,3.0f);  // random line [horizontal,vertical]
float postfx_noise = 0.15f;     // noise
float postfx_aberration = 3.0f; // chromatic aberration
vec4 postfx_ghost(0.1f,0.25f,0.1f,0.5f);        // ghost picture [distance,strength,distance,strength]
vec4 postfx_moire_h(0.75f,-0.25f,0.0f,1.0f);    // vertical moire [base,variable,repeat x,repeat y]
vec4 postfx_moire_v(0.75f,-0.25f,1.0f,1.5f);    // horizontal moire [base,variable,repeat x,repeat y]
vec4 postfx_scanline_h(0.75f, 0.25f,0.0f,2.0f); // vertical scanline [base,variable,repeat x,repeat y]
vec4 postfx_scanline_v(0.75f,-0.25f,2.0f,0.0f); // horizontal scanline [base,variable,repeat x,repeat y]
//------------------------------//
vec4 setup_var[]={
    vec4(0), /* remanency */
        vec4(0, 1, 1, 1),
        vec4(0.0f, 1.0f, 0.1f, buffer.x),
        vec4(0.0f, 1.0f, 0.1f, buffer.y),
        vec4(0.0f, 1.0f, 0.1f, remanency.x),
        vec4(0.0f, 1.0f, 0.1f, remanency.y),
        vec4(0),
        vec4(0),
        vec4(0),
    vec4(0), /* glow */
        vec4(0, 1, 1, 1),
        vec4(0.0f, 1.0f, 0.1f, glow_mix.x),
        vec4(0.0f, 1.0f, 0.1f, glow_mix.y),
        vec4(0.0f, 4.0f, 0.1f, glow_large.x),
        vec4(0.0f, 4.0f, 0.1f, glow_large.y),
        vec4(0.0f, 2.0f, 0.1f, glow_small.x),
        vec4(0.0f, 2.0f, 0.1f, glow_small.y),
        vec4(0),
    vec4(0), /* blur */
        vec4(0, 1, 1, 1),
        vec4(0.0f, 2.0f, 0.1f, blur.x),
        vec4(0.0f, 2.0f, 0.1f, blur.y),
        vec4(0),
        vec4(0),
        vec4(0),
        vec4(0),
        vec4(0),
    vec4(0), /* screen */
        vec4(0, 1, 1, 1),
        vec4(0.0f, 1.0f, 0.05f, postfx_deform.x),
        vec4(0.5f, 0.7f, 0.01f, postfx_deform.y),
        vec4(0.0f, 1.0f, 0.05f, postfx_corner.x),
        vec4(0.0f, 1.0f, 0.05f, postfx_corner.y),
        vec4(0),
        vec4(0),
        vec4(0),
    vec4(0), /* color */
        vec4(0.0f, 1.0f, 0.1f, postfx_filter.x),
        vec4(0.0f, 1.0f, 0.1f, postfx_filter.y),
        vec4(0.0f, 1.0f, 0.1f, postfx_filter.z),
        vec4(0.0f, 4.0f, 0.1f, postfx_color.x),
        vec4(0.0f, 4.0f, 0.1f, postfx_color.y),
        vec4(0.0f, 1.5f, 0.1f, postfx_color.z),
        vec4(0),
        vec4(0),
    vec4(0), /* noise */
        vec4(0.0f, 4.0f, 0.5f, postfx_offset.x),
        vec4(0.0f, 4.0f, 0.5f, postfx_offset.y),
        vec4(0.0f, 1.0f, 0.05f, postfx_noise),
        vec4(0.0f, 5.0f, 0.5f, postfx_aberration),
        vec4(0.0f, 0.2f, 0.01f, postfx_retrace.x),
        vec4(0.0f, 8.0f, 0.5f, postfx_retrace.y),
        vec4(0.0f, 4.0f, 0.25f, postfx_retrace.z),
        vec4(0),
    vec4(0), /* ghost */
        vec4(0.0f, 0.2f, 0.01f, postfx_ghost.x),
        vec4(0.0f, 1.0f, 0.05f, postfx_ghost.y),
        vec4(0.0f, 0.2f, 0.01f, postfx_ghost.z),
        vec4(0.0f, 1.0f, 0.05f, postfx_ghost.w),
        vec4(0),
        vec4(0),
        vec4(0),
        vec4(0),
    vec4(0), /* moire */
        vec4(0.5f, 1.0f, 0.05f, postfx_moire_h.x),
        vec4(-0.5f, 0.5f, 0.05f, postfx_moire_h.y),
        vec4(0.0f, 4.0f, 0.5f, postfx_moire_h.z),
        vec4(0.0f, 4.0f, 0.5f, postfx_moire_h.w),
        vec4(0.5f, 1.0f, 0.1f, postfx_moire_v.x),
        vec4(-0.5f, 0.5f, 0.1f, postfx_moire_v.y),
        vec4(0.0f, 4.0f, 0.5f, postfx_moire_v.z),
        vec4(0.0f, 4.0f, 0.5f, postfx_moire_v.w),
    vec4(0), /* scanline */
        vec4(0.5f, 1.0f, 0.05f, postfx_scanline_h.x),
        vec4(-0.5f, 0.5f, 0.05f, postfx_scanline_h.y),
        vec4(0.0f, 4.0f, 0.5f, postfx_scanline_h.z),
        vec4(0.0f, 4.0f, 0.5f, postfx_scanline_h.w),
        vec4(0.5f, 1.0f, 0.1f, postfx_scanline_v.x),
        vec4(-0.5f, 0.5f, 0.1f, postfx_scanline_v.y),
        vec4(0.0f, 4.0f, 0.5f, postfx_scanline_v.z),
        vec4(0.0f, 4.0f, 0.5f, postfx_scanline_v.w),
    vec4(0)
    };

void Render::UpdateVar()
{
    int k = 1;
    m_shader_remanency = (setup_var[k].w == 1) ? true : false; k++;
    buffer = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    remanency = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    k += 4;
    m_shader_glow = (setup_var[k].w == 1) ? true : false; k++;
    glow_mix = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    glow_large = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    glow_small = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    k += 2;
    m_shader_blur = (setup_var[k].w == 1) ? true : false; k++;
    blur = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    k += 6;
    m_shader_postfx = (setup_var[k].w == 1) ? true : false; k++;
    postfx_deform = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    postfx_corner = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    k += 4;
    postfx_filter = vec3(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w); k += 3;
    postfx_color = vec3(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w); k += 3;
    k += 3;
    postfx_offset = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    postfx_noise = setup_var[k].w; k++;
    postfx_aberration = setup_var[k].w; k++;
    postfx_retrace = vec3(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w); k += 3;
    k += 2;
    postfx_ghost = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;
    k += 5;
    postfx_moire_h = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;
    postfx_moire_v = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;
    k++;
    postfx_scanline_h = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;
    postfx_scanline_v = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;
}

Shader *shader_simple;
Shader *shader_blur_h, *shader_blur_v;
Shader *shader_remanency, *shader_glow, *shader_radial, *shader_postfx;
// shader variables
ShaderUniform shader_simple_texture;
ShaderUniform shader_remanency_source,
              shader_remanency_buffer,
              shader_remanency_mix;
ShaderUniform shader_glow_glow,
              shader_glow_source,
              shader_glow_mix;
ShaderUniform shader_blur_h_texture,
              shader_blur_h_radius,
              shader_blur_v_texture,
              shader_blur_v_radius;
ShaderUniform shader_radial_texture,
              shader_radial_screen_size,
              shader_radial_time,
              shader_radial_value1,
              shader_radial_value2,
              shader_radial_color;
ShaderUniform shader_postfx_texture,
              shader_postfx_texture_2d,
              shader_postfx_screen_size,
              shader_postfx_time,
              shader_postfx_deform,
              shader_postfx_ghost,
              shader_postfx_filter,
              shader_postfx_color,
              shader_postfx_corner,
              shader_postfx_retrace,
              shader_postfx_offset,
              shader_postfx_noise,
              shader_postfx_aberration,
              shader_postfx_moire_h,
              shader_postfx_moire_v,
              shader_postfx_scanline_h,
              shader_postfx_scanline_v,
              shader_postfx_flash,
              shader_postfx_sync;

FrameBuffer *fbo_back, *fbo_front, *fbo_buffer;
FrameBuffer *fbo_blur_h, *fbo_blur_v, *fbo_ping, *fbo_pong;

TextRender *text_render;

void fs_quad()
{
    glLoadIdentity();
    glDrawArrays(GL_QUADS, 0, 4);
}

void draw_shader_simple(FrameBuffer *fbo_output, int n)
{
    shader_simple->Bind();
    shader_simple->SetTexture(shader_simple_texture, fbo_output->GetTexture(), n);
    fs_quad();
    shader_simple->Unbind();
}

void rectangle(int x, int y, int w, int h)
{
    glLoadIdentity();
    glBegin(GL_QUADS);
        glVertex2i(x+w, y  );
        glVertex2i(x  , y  );
        glVertex2i(x  , y+h);
        glVertex2i(x+w, y+h);
    glEnd();
}

int Render::InitDraw(void)
{
    glDepthMask(GL_TRUE);     // do not write z-buffer
    glEnable(GL_CULL_FACE);   // disable cull face
    glCullFace(GL_BACK);      // don't draw front face

    /* Initialise framebuffer objects */
    fbo_back = new FrameBuffer(screen_size);
    fbo_front = new FrameBuffer(screen_size);
    fbo_buffer = new FrameBuffer(screen_size);
    fbo_blur_h = new FrameBuffer(screen_size);
    fbo_blur_v = new FrameBuffer(screen_size);
    fbo_ping = new FrameBuffer(screen_size);
    fbo_pong = new FrameBuffer(screen_size);
    // shader simple
    shader_simple = Shader::Create(lolfx_simple);
    shader_simple_texture = shader_simple->GetUniformLocation("texture");
    // shader remanency
    shader_remanency = Shader::Create(lolfx_remanency);
    shader_remanency_source = shader_remanency->GetUniformLocation("source");
    shader_remanency_buffer = shader_remanency->GetUniformLocation("buffer");
    shader_remanency_mix = shader_remanency->GetUniformLocation("mix");
    // shader glow
    shader_glow = Shader::Create(lolfx_glow);
    shader_glow_glow = shader_glow->GetUniformLocation("glow");
    shader_glow_source = shader_glow->GetUniformLocation("source");
    shader_glow_mix = shader_glow->GetUniformLocation("mix");
    // shader blur horizontal
    shader_blur_h = Shader::Create(lolfx_blurh);
    shader_blur_h_texture = shader_blur_h->GetUniformLocation("texture");
    shader_blur_h_radius = shader_blur_h->GetUniformLocation("radius");
    // shader blur vertical
    shader_blur_v = Shader::Create(lolfx_blurv);
    shader_blur_v_texture = shader_blur_v->GetUniformLocation("texture");
    shader_blur_v_radius = shader_blur_v->GetUniformLocation("radius");
    // shader radial
    shader_radial = Shader::Create(lolfx_radial);
    shader_radial_texture = shader_radial->GetUniformLocation("texture");
    shader_radial_screen_size = shader_radial->GetUniformLocation("screen_size");
    shader_radial_time = shader_radial->GetUniformLocation("time");
    shader_radial_value1 = shader_radial->GetUniformLocation("value1");
    shader_radial_value2 = shader_radial->GetUniformLocation("value2");
    shader_radial_color = shader_radial->GetUniformLocation("color");
    // shader postfx
    shader_postfx = Shader::Create(lolfx_postfx);
    shader_postfx_texture = shader_postfx->GetUniformLocation("texture");
    shader_postfx_texture_2d = shader_postfx->GetUniformLocation("texture_2d");
    shader_postfx_screen_size = shader_postfx->GetUniformLocation("screen_size");
    shader_postfx_time = shader_postfx->GetUniformLocation("time");
    shader_postfx_deform = shader_postfx->GetUniformLocation("deform");
    shader_postfx_ghost = shader_postfx->GetUniformLocation("ghost");
    shader_postfx_filter = shader_postfx->GetUniformLocation("filter");
    shader_postfx_color = shader_postfx->GetUniformLocation("color");
    shader_postfx_corner = shader_postfx->GetUniformLocation("corner");
    shader_postfx_retrace = shader_postfx->GetUniformLocation("retrace");
    shader_postfx_offset = shader_postfx->GetUniformLocation("offset");
    shader_postfx_noise = shader_postfx->GetUniformLocation("noise");
    shader_postfx_aberration = shader_postfx->GetUniformLocation("aberration");
    shader_postfx_moire_h = shader_postfx->GetUniformLocation("moire_h");
    shader_postfx_moire_v = shader_postfx->GetUniformLocation("moire_v");
    shader_postfx_scanline_h = shader_postfx->GetUniformLocation("scanline_h");
    shader_postfx_scanline_v = shader_postfx->GetUniformLocation("scanline_v");
    shader_postfx_flash = shader_postfx->GetUniformLocation("flash");
    shader_postfx_sync = shader_postfx->GetUniformLocation("sync");

    return true;
}

int Render::CreateGLWindow()
{
    screen_size = Video::GetSize();
    border = 18 * ratio_2d;
    border.y = border.x; // enabled to get same border everywhere
    canvas_char = (screen_size - border * 2) / (font_size * ratio_2d);
    canvas_size = canvas_char * font_size * ratio_2d;

    border = (screen_size - canvas_size) / 2;

    caca_set_canvas_size(m_caca, canvas_char.x, canvas_char.y);

    setup_size.y = ((setup_option_n > setup_item_n) ? setup_option_n : setup_item_n) + 1;
    setup_p = (canvas_char - setup_size.xy) / 2;

    InitDraw();
    return true;
}

Render::Render(caca_canvas_t *caca)
  : m_caca(caca),
    m_ready(false),
    m_pause(false),
    m_polygon(true),
    m_setup(true),
    m_shader(true),
    m_shader_remanency(true),
    m_shader_glow(true),
    m_shader_blur(true),
    m_shader_postfx(true)
{
    text_render = new TextRender(m_caca, font_size);
}

void Render::TickGame(float seconds)
{
    Entity::TickGame(seconds);
}

void Render::Pause()
{
    m_pause=!m_pause;
}

void Render::TickDraw(float seconds)
{
    /* keyboard manager */
    if (Input::GetButtonState(27/*SDLK_ESCAPE*/))
        Ticker::Shutdown();
    if (Input::GetButtonState(282/*SDLK_F1*/) && (timer - timer_key > timer_key_repeat))
    {
        m_setup = !m_setup;
        timer_key = timer;
    }
    if (Input::GetButtonState(283/*SDLK_F2*/) && (timer - timer_key > timer_key_repeat))
    {
        m_polygon = !m_polygon;
        polygon_fillmode = (m_polygon)?GL_FILL:GL_LINE;
        glPolygonMode(GL_FRONT, polygon_fillmode);
        timer_key = timer;
    }
    if (Input::GetButtonState(284/*SDLK_F3*/) && (timer - timer_key > timer_key_repeat))
    {
        m_shader_glow = !m_shader_glow;
        m_shader_blur = !m_shader_blur;
        timer_key = timer;
    }
    if (Input::GetButtonState(285/*SDLK_F4*/)&&(timer-timer_key>timer_key_repeat))
    {
        m_shader_postfx = !m_shader_postfx;
        timer_key = timer;
    }
    if (Input::GetButtonState(286/*SDLK_F5*/))
    {
        Pause();
    }
   if (Input::GetButtonState(9/*SDLK_TAB*/)&&(timer-timer_key>timer_key_repeat))
    {
        if (m_setup)
        {
            setup_switch = !setup_switch;
        }
        timer_key = timer;
    }
    if (Input::GetButtonState(273/*SDLK_UP*/)&&(timer-timer_key>timer_key_repeat))
    {
        if (m_setup)
        {
            if (!setup_switch)
            {
                setup_option--;
                if (setup_option < 0) setup_option = setup_option_n - 1;
                setup_item = 0;
            }
            else
            {
                setup_item--;
                if (setup_item < 0) setup_item = setup_item_n - 1;
            }
        }
        timer_key = timer;
    }
    if (Input::GetButtonState(274/*SDLK_DOWN*/)&&(timer-timer_key>timer_key_repeat))
    {
        if (m_setup)
        {
            if (!setup_switch)
            {
                setup_option++;
                if (setup_option > setup_option_n - 1) setup_option = 0;
                setup_item = 0;
            }
            else
            {
                setup_item++;
                if (setup_item > setup_item_n - 1) setup_item = 0;
            }
        }
        timer_key = timer;
    }
    if (Input::GetButtonState(280/*SDLK_PAGEUP*/)&&(timer-timer_key>timer_key_repeat))
    {
        if (m_setup)
        {
            if (!setup_switch)
            {
                setup_option = 0;
            }
            else
            {
                setup_item = 0;
            }
        }
        timer_key = timer;
    }
    if (Input::GetButtonState(281/*SDLK_PAGEDOWN*/)&&(timer-timer_key>timer_key_repeat))
    {
        if (m_setup)
        {
            if (!setup_switch)
            {
                setup_option = setup_option_n - 1;
                setup_item = 0;
            }
            else
            {
                setup_item = setup_item_n - 1;
            }
        }
        timer_key = timer;
    }
    if (Input::GetButtonState(276/*SDLK_LEFT*/)&&(timer-timer_key>timer_key_repeat))
    {
        if (m_setup && setup_switch)
        {
            setup_var[setup_item_key].w -= setup_var[setup_item_key].z;
            if (setup_var[setup_item_key].w < setup_var[setup_item_key].x) setup_var[setup_item_key].w = setup_var[setup_item_key].x;
            Render::UpdateVar();
        }
        timer_key = timer;
    }
    if (Input::GetButtonState(275/*SDLK_RIGHT*/)&&(timer-timer_key>timer_key_repeat))
    {
        if (m_setup && setup_switch)
        {
            setup_var[setup_item_key].w += setup_var[setup_item_key].z;
            if (setup_var[setup_item_key].w > setup_var[setup_item_key].y) setup_var[setup_item_key].w = setup_var[setup_item_key].y;
            Render::UpdateVar();
        }
        timer_key = timer;
    }
    if (Input::GetButtonState(13/*SDLK_RETURN*/)&&(timer-timer_key>timer_key_repeat))
    {
        timer_key = timer;
    }

    Entity::TickDraw(seconds);

    if (!m_ready)
    {
        CreateGLWindow();
        text_render->Init();
        m_ready = true;
    }

    // timer
    if (!m_pause)
    {
        timer += seconds;
        main_angle = timer * 100.0f * PID;
    }
    if (sync_flag)
    {
        angle=(main_angle-sync_angle)*sync_speed;
        sync_value=1.0f-sinf(angle);
        if (angle>90.0f*PID)
        {
            sync_value=0;
            sync_flag=false;
        }
    }
    if (beat_flag)
    {
        angle=(main_angle-beat_angle)*beat_speed;
        beat_value=1.0f-sinf(angle);
        if (angle>90.0f*PID)
        {
            beat_value=0;
            beat_flag=false;
        }
    }
    if (flash_flag)
    {
        angle=(main_angle-flash_angle)*flash_speed;
        flash_value=1.0f-sinf(angle);
        if (angle>90.0f*PID)
        {
            flash_value=0;
            flash_flag=false;
        }
    }
    if (fade_flag)
    {
        angle=(main_angle-fade_angle)*fade_speed;
        fade_value=1.0f-sinf(angle);
        if (angle>90.0f*PID)
        {
            fade_value=0;
            fade_flag=false;
        }
    }
    /* draw setup */
    if (m_setup)
    {
        /* background */
        caca_set_color_argb(m_caca, setup_color.x, setup_color.y);
        caca_fill_box(m_caca, setup_p.x, setup_p.y, setup_size.x + 1, setup_size.y,' ');
        caca_draw_line(m_caca, setup_p.x + setup_size.z - 1, setup_p.y + 1, setup_p.x + setup_size.z - 1, setup_p.y + setup_size.y - 1,'|');
        /* title */
        caca_set_color_argb(m_caca, setup_color.y, setup_color.x);
        caca_draw_line(m_caca, setup_p.x, setup_p.y, setup_p.x + setup_size.x, setup_p.y,' ');
        caca_put_str(m_caca, setup_p.x + setup_size.x / 2 - 3, setup_p.y, "SETUP");
        /* display option */
        for (int i = 0; i < setup_option_n; i++)
        {
            int y = setup_p.y + 1 + i;
            int k = i * (setup_item_n + 1);
            if (setup_option != i || setup_switch)
            {
                caca_set_color_argb(m_caca, setup_color.x, setup_color.y);
                caca_put_str(m_caca, setup_p.x + 1, y, setup_text[k]);
            }
            else
            {
                caca_set_color_argb(m_caca, setup_color.y, setup_color.x);
                caca_draw_line(m_caca, setup_p.x, y, setup_p.x + setup_size.z - 2, y,' ');
                caca_put_str(m_caca, setup_p.x + 1, y, setup_text[k]);
            }
        }
        /* display item */
        for (int i = 0; i < setup_item_n; i++)
        {
            int y = setup_p.y + 1 + i;
            int k = setup_option * (setup_item_n + 1) + 1;
            if (setup_item != i || !setup_switch)
            {
                caca_set_color_argb(m_caca, setup_color.x, setup_color.y);
                caca_put_str(m_caca, setup_p.x + setup_size.z + 1, y, setup_text[k + i]);
            }
            else
            {
                caca_set_color_argb(m_caca, setup_color.y, setup_color.x);
                caca_draw_line(m_caca, setup_p.x + setup_size.z, y, setup_p.x + setup_size.x, y,' ');
                caca_put_str(m_caca, setup_p.x + setup_size.z + 1, y, setup_text[k + i]);
            }
        }
        /* display variable */
        int y = setup_p.y + setup_size.y;
        setup_item_key = setup_option * (setup_item_n + 1) + 1 + setup_item;
        caca_set_color_argb(m_caca, setup_color.y, setup_color.x);
        caca_draw_line(m_caca, setup_p.x, y, setup_p.x + setup_size.x, y,' ');
        if (setup_switch && setup_text[setup_item_key] != "")
        {
            int w = setup_size.x - 3 - 4;
            int bar_w = (w / (setup_var[setup_item_key].y - setup_var[setup_item_key].x) * setup_var[setup_item_key].w);
            if ((setup_var[setup_item_key].y - setup_var[setup_item_key].x) / setup_var[setup_item_key].z > 2)
            {
                caca_printf(m_caca, setup_p.x + setup_size.x - 4, y, "%4.2f", setup_var[setup_item_key].w);
                caca_draw_line(m_caca, setup_p.x + 1, y, setup_p.x + 1 + w, y,'.');
                if(setup_var[setup_item_key].w > setup_var[setup_item_key].x) caca_draw_line(m_caca, setup_p.x + 1, y, setup_p.x + 1 + bar_w, y,'x');
            }
            else
            {
                caca_put_str(m_caca, setup_p.x + setup_size.x - 3, y, (setup_var[setup_item_key].w == setup_var[setup_item_key].y)?"YES":" NO");
            }
        }
    }

    Draw2D();
    Draw3D();

}

void Render::Draw2D()
{
    /* Draw text in an offline buffer */
    text_render->Render();

    if (m_shader)
        fbo_back->Bind();

    glViewport(0, 0, screen_size.x, screen_size.y);

    /* Clear the back buffer */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_COLOR,GL_DST_ALPHA);
    glClearColor(screen_color.r, screen_color.g, screen_color.b, 1.0f);
    glClearDepth(1.0f); // set depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    text_render->Blit(border, canvas_size);

    //if (m_polygon) glEnable(GL_LINE_SMOOTH); else glDisable(GL_LINE_SMOOTH);
    glLineWidth((m_polygon)?2.0f:1.0f);
    fx_angle=main_angle-part_angle;
    if (m_polygon)
        glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    mat4 m = mat4::ortho(0, screen_size.x, screen_size.y, 0, -1.f, 1.f);
    glLoadMatrixf(&m[0][0]);
    glMatrixMode(GL_MODELVIEW);
}

void Render::Draw3D()
{
    if (!m_shader)
        return;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(4, GL_FLOAT, 0, fs_quad_vtx);

    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, 0, fs_quad_tex);

    if (m_shader_remanency)
    {
        // shader remanency
        fbo_ping->Bind();
        shader_remanency->Bind();
        shader_remanency->SetTexture(shader_remanency_source, fbo_back->GetTexture(), 0);
        shader_remanency->SetTexture(shader_remanency_buffer, fbo_buffer->GetTexture(), 1);
        shader_remanency->SetUniform(shader_remanency_mix, remanency);
        fs_quad();
        shader_remanency->Unbind();
        fbo_ping->Unbind();
        // shader simple
        fbo_back->Bind();
        draw_shader_simple(fbo_ping, 0);
        fbo_back->Unbind();
        // save previous fbo
        fbo_ping->Bind();
        shader_remanency->Bind();
        shader_remanency->SetTexture(shader_remanency_source, fbo_front->GetTexture(), 0);
        shader_remanency->SetTexture(shader_remanency_buffer, fbo_buffer->GetTexture(), 1);
        shader_remanency->SetUniform(shader_remanency_mix, buffer);
        fs_quad();
        shader_remanency->Unbind();
        fbo_ping->Unbind();
        // shader simple
        fbo_buffer->Bind();
        draw_shader_simple(fbo_ping, 0);
        fbo_buffer->Unbind();
    }

    // shader glow
    if (m_shader_glow)
    {
        // shader blur horizontal
        fbo_blur_h->Bind();
        shader_blur_h->Bind();
        shader_blur_h->SetTexture(shader_blur_h_texture, fbo_back->GetTexture(), 0);
        shader_blur_h->SetUniform(shader_blur_h_radius, glow_large / screen_size.x);
        fs_quad();
        shader_blur_h->Unbind();
        fbo_blur_h->Unbind();
        // shader blur vertical
        fbo_blur_v->Bind();
        shader_blur_v->Bind();
        shader_blur_v->SetTexture(shader_blur_v_texture, fbo_blur_h->GetTexture(), 0);
        shader_blur_v->SetUniform(shader_blur_v_radius, glow_large / screen_size.y);
        fs_quad();
        shader_blur_v->Unbind();
        fbo_blur_v->Unbind();
        // shader blur horizontal
        fbo_blur_h->Bind();
        shader_blur_h->Bind();
        shader_blur_h->SetTexture(shader_blur_h_texture, fbo_blur_v->GetTexture(), 0);
        shader_blur_h->SetUniform(shader_blur_h_radius, glow_small / screen_size.x);
        fs_quad();
        shader_blur_h->Unbind();
        fbo_blur_h->Unbind();
        // shader blur vertical
        fbo_blur_v->Bind();
        shader_blur_v->Bind();
        shader_blur_v->SetTexture(shader_blur_v_texture, fbo_blur_h->GetTexture(), 0);
        shader_blur_v->SetUniform(shader_blur_v_radius, glow_small / screen_size.y);
        fs_quad();
        shader_blur_v->Unbind();
        fbo_blur_v->Unbind();
        // shader glow
        fbo_front->Bind();
        shader_glow->Bind();
        shader_glow->SetTexture(shader_glow_glow, fbo_blur_v->GetTexture(), 0);
        shader_glow->SetTexture(shader_glow_source, fbo_back->GetTexture(), 1);
        shader_glow->SetUniform(shader_glow_mix, glow_mix);
        fs_quad();
        shader_glow->Unbind();
        fbo_front->Unbind();
    }
    else
    {
        // shader simple
        fbo_front->Bind();
        draw_shader_simple(fbo_back, 0);
        fbo_front->Unbind();
    }

    if (m_shader_blur)
    {
        // shader blur horizontal
        fbo_ping->Bind();
        shader_blur_h->Bind();
        shader_blur_h->SetTexture(shader_blur_h_texture, fbo_front->GetTexture(), 0);
        shader_blur_h->SetUniform(shader_blur_h_radius, blur / screen_size.x);
        fs_quad();
        shader_blur_h->Unbind();
        fbo_ping->Unbind();
        // shader blur vertical
        fbo_front->Bind();
        shader_blur_v->Bind();
        shader_blur_v->SetTexture(shader_blur_v_texture, fbo_ping->GetTexture(), 0);
        shader_blur_v->SetUniform(shader_blur_v_radius, blur / screen_size.y);
        fs_quad();
        shader_blur_v->Unbind();
        fbo_front->Unbind();
    }

    if (m_shader_postfx)
    {
        // shader postfx
        shader_postfx->Bind();
        shader_postfx->SetTexture(shader_postfx_texture, fbo_front->GetTexture(), 0);
        shader_postfx->SetUniform(shader_postfx_screen_size, (vec2)screen_size);
        shader_postfx->SetUniform(shader_postfx_time, fx_angle);
        shader_postfx->SetUniform(shader_postfx_deform, postfx_deform);
        shader_postfx->SetUniform(shader_postfx_ghost, postfx_ghost);
        shader_postfx->SetUniform(shader_postfx_filter, postfx_filter);
        shader_postfx->SetUniform(shader_postfx_color, postfx_color);
        shader_postfx->SetUniform(shader_postfx_corner, postfx_corner);
        shader_postfx->SetUniform(shader_postfx_retrace, postfx_retrace);
        shader_postfx->SetUniform(shader_postfx_offset, postfx_offset);
        shader_postfx->SetUniform(shader_postfx_noise, postfx_noise);
        shader_postfx->SetUniform(shader_postfx_aberration, postfx_aberration);
        shader_postfx->SetUniform(shader_postfx_moire_h, postfx_moire_h);
        shader_postfx->SetUniform(shader_postfx_moire_v, postfx_moire_v);
        shader_postfx->SetUniform(shader_postfx_scanline_h, postfx_scanline_h);
        shader_postfx->SetUniform(shader_postfx_scanline_v, postfx_scanline_v);
        shader_postfx->SetUniform(shader_postfx_flash, flash_value);
        shader_postfx->SetUniform(shader_postfx_sync, (float)fabs(beat_value*cosf((main_angle-beat_angle)*8.0f)));
        fs_quad();
        shader_postfx->Unbind();
    }
    else
    {
        // shader simple
        draw_shader_simple(fbo_front, 0);
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

Render::~Render()
{
}