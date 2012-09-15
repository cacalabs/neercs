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

extern char const *lolfx_simple;
extern char const *lolfx_blurh;
extern char const *lolfx_blurv;
extern char const *lolfx_glow;
extern char const *lolfx_remanency;
extern char const *lolfx_copper;
extern char const *lolfx_color;
extern char const *lolfx_noise;
extern char const *lolfx_postfx;

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
float flash_speed = 2.0f;  // speed
/* fade variable */
bool fade_flag = false;    // flag
float fade_angle = 0;      // angle
float fade_value = 0;      // value
float fade_speed = 0.2f;   // speed
/* sync variable */
bool sync_flag = false;    // flagsh
float sync_angle = 0;      // angle
float sync_value = 0;      // value
float sync_speed = 1.0f;   // speed
/* beat variable */
bool beat_flag = false;    // flag
float beat_angle = 0;      // angle
float beat_value = 0;      // value
float beat_speed = 2.0f;   // speed
/* common variable */
float value, angle, radius, scale, speed;
/* shader variable */
vec2 buffer(0.7f,0.3f);         // [new frame mix,old frame mix]
vec2 remanency(0.6f,0.4f);      // remanency [source mix,buffer mix]
vec2 glow_mix(0.7f,0.3f);       // glow mix [source mix,glow mix]
vec2 glow_large(3.0f,0.0f);     // large glow radius [center,corner]
vec2 glow_small(1.5f,0.0f);     // small glow radius [center,corner]
vec2 blur(0.5f,0.0f);           // blur radius [center,corner]
vec4 copper(0.125,0.125,32,64); // copper [base,variable,repeat x,repeat y]
vec3 color_filter(0.9f,0.9f,1.0f);    // color filter [red,green,blue]
vec4 color_color(1.5f,1.2f,0.1f,0.35f);         // color modifier [brightness,contrast,level,grayscale]
vec2 noise_offset(2.0f,2.0f);         // random line [horizontal,vertical]
float noise_noise = 0.25f;            // noise
vec3 noise_retrace(1.0f,1.0f,0.5f);   // retrace [strength,length,speed]
vec2 postfx_deform(0.7f,0.54f);       // deformation [ratio,zoom]
float postfx_vignetting = 0.5f;       // vignetting strength
float postfx_aberration = 4.0f;       // chromatic aberration
vec4 postfx_ghost1(1.0f,0.0f,0.0f,-0.25f);      // ghost picture 1 [position x,position y,position z,strength]
vec4 postfx_ghost2(1.5f,0.0f,0.0f,0.25f);       // ghost picture 2 [position x,position y,position z,strength]
vec4 postfx_moire_h(0.75f,-0.25f,0.0f,1.0f);    // vertical moire [base,variable,repeat,shift]
vec4 postfx_moire_v(0.75f,-0.25f,1.0f,1.5f);    // horizontal moire [base,variable,repeat,shift]
vec4 postfx_scanline_h(0.75f,0.0f,0.0f,0.0f);   // vertical scanline [base,variable,repeat,shift]
vec4 postfx_scanline_v(0.75f,-0.25f,2.0f,0.0f); // horizontal scanline [base,variable,repeat,shift]
vec3 postfx_corner(0.0f,0.75f,0.95f);           // corner [width,radius,blur]
/* text variable */
ivec2 ratio_2d(2,4);            // 2d ratio
ivec2 map_size(256,256);        // texture map size
ivec2 font_size(8,8);           // font size
ivec2 canvas_char(0,0);         // canvas char number
ivec2 canvas_size(0,0);         // caca size
/* window variable */
ivec2 border = vec2(3,1) * ratio_2d * font_size; // border width
/* setup variable */
bool setup_switch = false;      // switch [option/item]
int setup_n = 0;                // item/option number
int setup_h = 3;                // height
int setup_cursor = 0;           // cursor position
int setup_option_i = 0;         // selected option
int setup_option_n = 10;        // option number
int setup_option_p = 0;         // option position
int setup_item_i = 0;           // selected item
int setup_item_n = 8;           // item number
int setup_item_p = 0;           // item position
int setup_item_key = 0;         // item array key
ivec2 setup_p(1,1);             // position [x,y]
ivec3 setup_size(29,4,12);      // size [w,h,split]
ivec2 setup_color(0xaaa,0x222); // color [foreground,background] 0x678,0x234
char const *setup_text[] = {
    "main",
        "2d ratio w",
        "2d ratio h",
        "border w",
        "border h",
        "",
        "",
        "",
        "",
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
        "corner width",
        "corner radius",
        "corner blur",
        "vignetting",
        "",
    "color",
        "filter red",
        "filter green",
        "filter blue",
        "brightness",
        "contrast",
        "level",
        "grayscale",
        "aberration",
    "noise",
        "enable",
        "offset h",
        "offset v",
        "noise",
        "retrace strength",
        "retrace length",
        "retrace speed",
        "",
    "ghost",
        "back x",
        "back y",
        "back z",
        "back strength",
        "front x",
        "front y",
        "front z",
        "front strength",
    "moire",
        "h base",
        "h variable",
        "h repeat",
        "h shift",
        "v base",
        "v variable",
        "v repeat",
        "v shift",
    "scanline",
        "h base",
        "h variable",
        "h repeat",
        "h shift",
        "v base",
        "v variable",
        "v repeat",
        "v shift"
    };

vec4 setup_var[]={ // setup variable [start,end,step,value]
    vec4(0), /* main */
        vec4( 1,  8, 1, ratio_2d.x),
        vec4( 1,  8, 1, ratio_2d.y),
        vec4( 0, 16, 1, border.x / ratio_2d.x / font_size.x),
        vec4( 0, 16, 1, border.y / ratio_2d.y / font_size.y),
        vec4(0),
        vec4(0),
        vec4(0),
        vec4(0),
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
        vec4(0.0f, 2.0f, 0.05f, blur.x),
        vec4(0.0f, 2.0f, 0.05f, blur.y),
        vec4(0),
        vec4(0),
        vec4(0),
        vec4(0),
        vec4(0),
    vec4(0), /* screen */
        vec4(0, 1, 1, 1),
        vec4(0.0f, 1.0f, 0.05f, postfx_deform.x),
        vec4(0.5f, 0.7f, 0.01f, postfx_deform.y),
        vec4(0.0f, 4.0f, 0.10f, postfx_corner.x),
        vec4(0.0f, 1.0f, 0.05f, postfx_corner.y),
        vec4(0.0f, 1.0f, 0.05f, postfx_corner.z),
        vec4(0.0f, 1.0f, 0.10f, postfx_vignetting),
        vec4(0),
    vec4(0), /* color */
        vec4( 0.0f, 1.0f, 0.05f, color_filter.x),
        vec4( 0.0f, 1.0f, 0.05f, color_filter.y),
        vec4( 0.0f, 1.0f, 0.05f, color_filter.z),
        vec4( 0.0f, 2.0f, 0.05f, color_color.x),
        vec4( 0.0f, 2.0f, 0.05f, color_color.y),
        vec4(-1.0f, 1.0f, 0.05f, color_color.z),
        vec4( 0.0f, 1.0f, 0.05f, color_color.w),
        vec4( 0.0f, 8.0f, 0.50f, postfx_aberration),
    vec4(0), /* noise */
        vec4( 0, 1, 1, 1),
        vec4(0.0f, 4.0f, 0.50f, noise_offset.x),
        vec4(0.0f, 4.0f, 0.50f, noise_offset.y),
        vec4(0.0f, 0.5f, 0.05f, noise_noise),
        vec4(0.0f, 4.0f, 0.25f, noise_retrace.x),
        vec4(0.0f, 8.0f, 0.50f, noise_retrace.y),
        vec4(0.0f, 4.0f, 0.25f, noise_retrace.z),
        vec4(0),
    vec4(0), /* ghost */
        vec4(-2.0f, 2.0f, 0.10f, postfx_ghost1.x),
        vec4(-2.0f, 2.0f, 0.10f, postfx_ghost1.y),
        vec4(-2.0f, 2.0f, 0.10f, postfx_ghost1.z),
        vec4(-1.0f, 1.0f, 0.05f, postfx_ghost1.w),
        vec4(-2.0f, 2.0f, 0.10f, postfx_ghost2.x),
        vec4(-2.0f, 2.0f, 0.10f, postfx_ghost2.y),
        vec4(-2.0f, 2.0f, 0.10f, postfx_ghost2.z),
        vec4(-1.0f, 1.0f, 0.05f, postfx_ghost2.w),
    vec4(0), /* moire */
        vec4( 0.5f, 1.0f, 0.05f, postfx_moire_h.x),
        vec4(-0.5f, 0.5f, 0.05f, postfx_moire_h.y),
        vec4( 0.0f, 4.0f, 0.50f, postfx_moire_h.z),
        vec4( 0.0f, 4.0f, 0.50f, postfx_moire_h.w),
        vec4( 0.5f, 1.0f, 0.05f, postfx_moire_v.x),
        vec4(-0.5f, 0.5f, 0.05f, postfx_moire_v.y),
        vec4( 0.0f, 4.0f, 0.50f, postfx_moire_v.z),
        vec4( 0.0f, 4.0f, 0.50f, postfx_moire_v.w),
    vec4(0), /* scanline */
        vec4( 0.5f, 1.0f, 0.05f, postfx_scanline_h.x),
        vec4(-0.5f, 0.5f, 0.05f, postfx_scanline_h.y),
        vec4( 0.0f, 4.0f, 0.50f, postfx_scanline_h.z),
        vec4( 0.0f, 4.0f, 0.50f, postfx_scanline_h.w),
        vec4( 0.5f, 1.0f, 0.05f, postfx_scanline_v.x),
        vec4(-0.5f, 0.5f, 0.05f, postfx_scanline_v.y),
        vec4( 0.0f, 4.0f, 0.50f, postfx_scanline_v.z),
        vec4( 0.0f, 4.0f, 0.50f, postfx_scanline_v.w),
    vec4(0)
    };

void Render::UpdateVar()
{
    int k = 1; /* main */
    ratio_2d = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    border = vec2(setup_var[k].w, setup_var[k + 1].w) * ratio_2d * font_size; k += 2;
    k += 5; /* remanency */
    m_shader_remanency = (setup_var[k].w == 1) ? true : false; k++;
    buffer = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    remanency = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    k += 4; /* glow */
    m_shader_glow = (setup_var[k].w == 1) ? true : false; k++;
    glow_mix = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    glow_large = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    glow_small = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    k += 2; /* blur */
    m_shader_blur = (setup_var[k].w == 1) ? true : false; k++;
    blur = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    k += 6; /* screen */
    m_shader_postfx = (setup_var[k].w == 1) ? true : false; k++;
    postfx_deform = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    postfx_corner = vec3(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w); k += 3;
    postfx_vignetting = setup_var[k].w; k++;
    k += 2; /* color */
    color_filter = vec3(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w); k += 3;
    color_color = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;
    postfx_aberration = setup_var[k].w; k++;
    k += 1; /* noise */
    m_shader_noise = (setup_var[k].w == 1) ? true : false; k++;
    noise_offset = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    noise_noise = setup_var[k].w; k++;
    noise_retrace = vec3(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w); k += 3;
    k += 2; /* ghost */
    postfx_ghost1 = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;
    postfx_ghost2 = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;
    k += 1; /* moire */
    postfx_moire_h = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;
    postfx_moire_v = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;
    k += 1; /* scanline */
    postfx_scanline_h = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;
    postfx_scanline_v = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;

    UpdateSize();
}

void Render::UpdateSize()
{
    screen_size = Video::GetSize();
    //border.y = border.x; // enabled to get same border everywhere
    canvas_char = (screen_size - border * 2) / (font_size * ratio_2d);
    canvas_char = max(canvas_char, ivec2(1, 1));
    canvas_size = canvas_char * font_size * ratio_2d;

    border = (screen_size - canvas_size) / 2;

    caca_set_canvas_size(m_caca, canvas_char.x, canvas_char.y);

    setup_p = (canvas_char - setup_size.xy) / 2;
}

int calc_item_length()
{
    int n = !setup_switch ? setup_option_n : setup_item_n;
    for (int i = 0; i < n; i++)
    {
        int k = !setup_switch ? (i * (setup_item_n + 1)) : (setup_option_i * (setup_item_n + 1) + 1 + i);
        if (setup_text[k][0] == '\0') return i - 1;
    }
    return n - 1;
}

Shader *shader_simple;
Shader *shader_blur_h, *shader_blur_v, *shader_glow;
Shader *shader_remanency, *shader_copper, *shader_color;
Shader *shader_noise, *shader_postfx;
// shader variables
ShaderUniform shader_simple_texture;
ShaderUniform shader_blur_h_texture,
              shader_blur_h_radius,
              shader_blur_v_texture,
              shader_blur_v_radius;
ShaderUniform shader_glow_glow,
              shader_glow_source,
              shader_glow_mix;
ShaderUniform shader_remanency_source,
              shader_remanency_buffer,
              shader_remanency_mix;
ShaderUniform shader_copper_texture,
              shader_copper_screen_size,
              shader_copper_time,
              shader_copper_copper;
ShaderUniform shader_color_texture,
              shader_color_screen_size,
              shader_color_filter,
              shader_color_color,
              shader_color_flash;
ShaderUniform shader_noise_texture,
              shader_noise_screen_size,
              shader_noise_time,
              shader_noise_offset,
              shader_noise_noise,
              shader_noise_retrace;
ShaderUniform shader_postfx_texture,
              shader_postfx_texture_2d,
              shader_postfx_screen_size,
              shader_postfx_time,
              shader_postfx_deform,
              shader_postfx_ghost1,
              shader_postfx_ghost2,
              shader_postfx_vignetting,
              shader_postfx_aberration,
              shader_postfx_moire_h,
              shader_postfx_moire_v,
              shader_postfx_scanline_h,
              shader_postfx_scanline_v,
              shader_postfx_corner,
              shader_postfx_sync,
              shader_postfx_beat;

FrameBuffer *fbo_back, *fbo_front, *fbo_buffer;
FrameBuffer *fbo_blur_h, *fbo_blur_v, *fbo_tmp;

TextRender *text_render;

void Render::TraceQuad()
{
    glLoadIdentity();
    glDrawArrays(GL_QUADS, 0, 4);
}

void Render::ShaderSimple(FrameBuffer *fbo_output, int n)
{
    shader_simple->Bind();
    shader_simple->SetUniform(shader_simple_texture, fbo_output->GetTexture(), n);
    TraceQuad();
    shader_simple->Unbind();
}

int Render::InitDraw(void)
{
    glDepthMask(GL_TRUE);     // do not write z-buffer
    glEnable(GL_CULL_FACE);   // disable cull face
    glCullFace(GL_BACK);      // don't draw front face

    /* initialise framebuffer objects */
    fbo_back = new FrameBuffer(screen_size);
    fbo_front = new FrameBuffer(screen_size);
    fbo_buffer = new FrameBuffer(screen_size);
    fbo_blur_h = new FrameBuffer(screen_size);
    fbo_blur_v = new FrameBuffer(screen_size);
    fbo_tmp = new FrameBuffer(screen_size);
    // shader simple
    shader_simple = Shader::Create(lolfx_simple);
    shader_simple_texture = shader_simple->GetUniformLocation("texture");
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
    // shader remanency
    shader_remanency = Shader::Create(lolfx_remanency);
    shader_remanency_source = shader_remanency->GetUniformLocation("source");
    shader_remanency_buffer = shader_remanency->GetUniformLocation("buffer");
    shader_remanency_mix = shader_remanency->GetUniformLocation("mix");
    // shader copper
    shader_copper = Shader::Create(lolfx_copper);
    shader_copper_texture = shader_copper->GetUniformLocation("texture");
    shader_copper_screen_size = shader_copper->GetUniformLocation("screen_size");
    shader_copper_time = shader_copper->GetUniformLocation("time");
    shader_copper_copper = shader_copper->GetUniformLocation("copper");
    // shader color
    shader_color = Shader::Create(lolfx_color);
    shader_color_texture = shader_color->GetUniformLocation("texture");
    shader_color_screen_size = shader_color->GetUniformLocation("screen_size");
    shader_color_filter = shader_color->GetUniformLocation("filter");
    shader_color_color = shader_color->GetUniformLocation("color");
    shader_color_flash = shader_color->GetUniformLocation("flash");
    // shader noise
    shader_noise = Shader::Create(lolfx_noise);
    shader_noise_texture = shader_noise->GetUniformLocation("texture");
    shader_noise_screen_size = shader_noise->GetUniformLocation("screen_size");
    shader_noise_time = shader_noise->GetUniformLocation("time");
    shader_noise_offset = shader_noise->GetUniformLocation("offset");
    shader_noise_noise = shader_noise->GetUniformLocation("noise");
    shader_noise_retrace = shader_noise->GetUniformLocation("retrace");
    // shader postfx
    shader_postfx = Shader::Create(lolfx_postfx);
    shader_postfx_texture = shader_postfx->GetUniformLocation("texture");
    shader_postfx_texture_2d = shader_postfx->GetUniformLocation("texture_2d");
    shader_postfx_screen_size = shader_postfx->GetUniformLocation("screen_size");
    shader_postfx_time = shader_postfx->GetUniformLocation("time");
    shader_postfx_deform = shader_postfx->GetUniformLocation("deform");
    shader_postfx_ghost1 = shader_postfx->GetUniformLocation("ghost1");
    shader_postfx_ghost2 = shader_postfx->GetUniformLocation("ghost2");
    shader_postfx_vignetting = shader_postfx->GetUniformLocation("vignetting");
    shader_postfx_aberration = shader_postfx->GetUniformLocation("aberration");
    shader_postfx_moire_h = shader_postfx->GetUniformLocation("moire_h");
    shader_postfx_moire_v = shader_postfx->GetUniformLocation("moire_v");
    shader_postfx_scanline_h = shader_postfx->GetUniformLocation("scanline_h");
    shader_postfx_scanline_v = shader_postfx->GetUniformLocation("scanline_v");
    shader_postfx_corner = shader_postfx->GetUniformLocation("corner");
    shader_postfx_sync = shader_postfx->GetUniformLocation("sync");
    shader_postfx_beat = shader_postfx->GetUniformLocation("beat");
    // initialize setup
    setup_n = calc_item_length();
    return true;
}

int Render::CreateGLWindow()
{
    UpdateSize();
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
    m_shader_glow(true),
    m_shader_blur(true),
    m_shader_remanency(true),
    m_shader_copper(false),
    m_shader_color(true),
    m_shader_noise(true),
    m_shader_postfx(true)
{
    text_render = new TextRender(m_caca, font_size);
}

void Render::TickGame(float seconds)
{
    Entity::TickGame(seconds);

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
        for (int i = 0; i < setup_h; i++)
        {
            int y = setup_p.y + 1 + i;
            int k = (setup_option_p + i) * (setup_item_n + 1);
            if (setup_option_i != setup_option_p + i || setup_switch)
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
        for (int i = 0; i < setup_h; i++)
        {
            int y = setup_p.y + 1 + i;
            int k = setup_option_i * (setup_item_n + 1) + 1 + setup_item_p + i;
            if (setup_item_i != setup_item_p + i || !setup_switch)
            {
                caca_set_color_argb(m_caca, setup_color.x, setup_color.y);
                caca_put_str(m_caca, setup_p.x + setup_size.z + 1, y, setup_text[k]);
            }
            else
            {
                caca_set_color_argb(m_caca, setup_color.y, setup_color.x);
                caca_draw_line(m_caca, setup_p.x + setup_size.z, y, setup_p.x + setup_size.x, y,' ');
                caca_put_str(m_caca, setup_p.x + setup_size.z + 1, y, setup_text[k]);
            }
        }
        /* display variable */
        int y = setup_p.y + setup_size.y;
        setup_item_key = setup_option_i * (setup_item_n + 1) + 1 + setup_item_i;
        caca_set_color_argb(m_caca, setup_color.y, setup_color.x);
        caca_draw_line(m_caca, setup_p.x, y, setup_p.x + setup_size.x, y,' ');
        if (setup_switch)
        {
            int x = setup_p.x + 1;
            int w = setup_size.x - 3 - 4;
            int bar_w = w / (setup_var[setup_item_key].y - setup_var[setup_item_key].x);
            int bar_x = bar_w * setup_var[setup_item_key].x;
            if ((setup_var[setup_item_key].y - setup_var[setup_item_key].x) / setup_var[setup_item_key].z > 1)
            {
                /* Work around a bug in libcaca */
                if (setup_p.x + setup_size.x - 4 < caca_get_canvas_width(m_caca)) caca_printf(m_caca, setup_p.x + setup_size.x - 4, y, "%4.2f", setup_var[setup_item_key].w);
                caca_draw_line(m_caca, x, y, x - bar_x + bar_w * setup_var[setup_item_key].y, y,'.');
                if (setup_var[setup_item_key].w != setup_var[setup_item_key].x) caca_draw_line(m_caca, x, y, x - bar_x + bar_w * setup_var[setup_item_key].w, y, 'x');
            }
            else
            {
                if (setup_var[setup_item_key] != vec4(0))
                {
                    caca_put_str(m_caca, setup_p.x + setup_size.x - 3, y, (setup_var[setup_item_key].w == setup_var[setup_item_key].y)?"YES":" NO");
                }
            }
        }
        else
        {
            caca_printf(m_caca, setup_p.x + 1, y, "%d/%d [%d]", setup_option_i, setup_n, setup_option_p);
        }

        /* informations */
        int w = caca_get_canvas_width(m_caca);
        int h = caca_get_canvas_height(m_caca);
        caca_set_color_argb(m_caca, 0xfff, 0x000);
        caca_printf(m_caca, 0, 0, "%i*%i", w, h);
    }
}

void Render::Pause()
{
    m_pause=!m_pause;
}

void Render::TickDraw(float seconds)
{
    /* keyboard manager */
    if (Input::WasReleased(Key::Escape))
    {
        Ticker::Shutdown();
    }
    if (Input::WasPressed(Key::F1))
    {
        m_setup = !m_setup;
        if (m_setup) setup_n = calc_item_length();
        sync_flag = true;
        sync_angle = main_angle;
    }
    if (Input::WasPressed(Key::F2))
    {
        m_shader_glow = !m_shader_glow;
        m_shader_blur = !m_shader_blur;
        m_shader_remanency = !m_shader_remanency;
        m_shader_copper = !m_shader_copper;
        m_shader_color = !m_shader_color;
        m_shader_noise = !m_shader_noise;
        m_shader_postfx = !m_shader_postfx;
        //m_polygon = !m_polygon;
        //polygon_fillmode = (m_polygon)?GL_FILL:GL_LINE;
        //glPolygonMode(GL_FRONT, polygon_fillmode);
    }
   if (Input::WasPressed(Key::Tab))
    {
        if (m_setup)
        {
            setup_switch = !setup_switch;
            setup_n = calc_item_length();
            setup_cursor = (!setup_switch?setup_option_i:setup_item_i) - (!setup_switch?setup_option_p:setup_item_p);
        }
    }
    if (Input::WasPressed(Key::Up))
    {
        if (m_setup)
        {
            if (!setup_switch)
            {
                if (setup_cursor > 0)
                {
                    setup_cursor--;
                }
                else
                {
                    if (setup_cursor == 0) setup_option_p--;
                }
                if (setup_option_i > 0)
                {
                    setup_option_i--;
                }
                else
                {
                    setup_option_i = setup_option_n - 1;
                    setup_option_p = setup_option_n - setup_h;
                    setup_cursor = setup_h - 1;
                }
                setup_item_i = 0;
                setup_item_p = 0;
            }
            else
            {
                if (setup_cursor > 0)
                {
                    setup_cursor--;
                }
                else
                {
                    if (setup_cursor == 0) setup_item_p--;
                }
                if (setup_item_i > 0)
                {
                    setup_item_i--;
                }
                else
                {
                    setup_item_i = setup_n;
                    setup_item_p = (setup_n < setup_h) ? 0 : setup_n - setup_h + 1;
                    setup_cursor = (setup_n < setup_h) ? setup_n : setup_h - 1;
                }
            }
        }
    }
    if (Input::WasPressed(Key::Down))
    {
        if (m_setup)
        {
            if (!setup_switch)
            {
                if (setup_cursor < setup_h - 1)
                {
                    setup_cursor++;
                }
                else
                {
                    if (setup_cursor == setup_h - 1) setup_option_p++;
                }
                if (setup_option_i < setup_option_n - 1)
                {
                    setup_option_i++;
                }
                else
                {
                    setup_option_i = 0;
                    setup_option_p = 0;
                    setup_cursor = 0;
                }
                setup_item_i = 0;
                setup_item_p = 0;
            }
            else
            {
                if (setup_cursor < setup_h - 1)
                {
                    setup_cursor++;
                }
                else
                {
                    if (setup_cursor == setup_h - 1) setup_item_p++;
                }
                if (setup_item_i < setup_n)
                {
                    setup_item_i++;
                }
                else
                {
                    setup_item_i = 0;
                    setup_item_p = 0;
                    setup_cursor = 0;
                }
            }
        }
    }
    if (Input::WasPressed(Key::PageUp))
    {
        if (m_setup)
        {
            if (!setup_switch)
            {
                if (setup_cursor > 0)
                {
                    setup_option_i -= setup_cursor;
                    setup_cursor = 0;
                }
                else
                {
                    if (setup_option_i > setup_h)
                    {
                        setup_option_i -= setup_h;
                        setup_option_p -= setup_h;
                    }
                else
                    {
                        setup_option_i = 0;
                        setup_option_p = 0;
                    }
                }
            setup_item_i = 0;
            }
            else
            {
                if (setup_cursor > 0)
                {
                    setup_item_i -= setup_cursor;
                    setup_cursor = 0;
                }
                else
                {
                    if (setup_item_i > setup_h)
                    {
                        setup_item_i -= setup_h;
                        setup_item_p -= setup_h;
                    }
                else
                    {
                        setup_item_i = 0;
                        setup_item_p = 0;
                    }
                }
            }
        }
    }
    if (Input::WasPressed(Key::PageDown))
    {
        if (m_setup)
        {
            if (!setup_switch)
            {
                if (setup_cursor < setup_h - 1)
                {
                    setup_option_i += setup_h - setup_cursor - 1;
                    setup_cursor = setup_h - 1;
                    setup_item_i = 0;
                }
                else
                {
                    if (setup_option_i < setup_option_n - setup_h - 1)
                    {
                        setup_option_i += setup_h;
                        setup_option_p += setup_h;
                    }
                else
                    {
                        setup_option_i = setup_option_n - 1;
                        setup_option_p = setup_option_n - setup_h;
                    }
                }
            }
            else
            {
                if (setup_cursor < setup_h - 1)
                {
                    setup_item_i += (setup_n < setup_h) ? setup_n - setup_cursor : setup_h - setup_cursor - 1;
                    setup_cursor = (setup_n < setup_h) ? setup_n : setup_h - 1;
                }
                else
                {
                    if (setup_item_i < setup_n - setup_h + 1)
                    {
                        setup_item_i += setup_h;
                        setup_item_p += setup_h;
                    }
                else
                    {
                        setup_item_i = setup_n;
                        setup_item_p = setup_n - setup_h + 1;
                    }
                }
            }
        }
    }
    if (Input::WasPressed(Key::Left))
    {
        if (m_setup && setup_switch)
        {
            setup_var[setup_item_key].w -= setup_var[setup_item_key].z;
            if (setup_var[setup_item_key].w < setup_var[setup_item_key].x) setup_var[setup_item_key].w = setup_var[setup_item_key].x;
            UpdateVar();
        }
    }
    if (Input::WasPressed(Key::Right))
    {
        if (m_setup && setup_switch)
        {
            setup_var[setup_item_key].w += setup_var[setup_item_key].z;
            if (setup_var[setup_item_key].w > setup_var[setup_item_key].y) setup_var[setup_item_key].w = setup_var[setup_item_key].y;
            UpdateVar();
        }
    }
    if (Input::WasPressed(Key::Home))
    {
        if (m_setup && setup_switch)
        {
            setup_var[setup_item_key].w = setup_var[setup_item_key].x;
            UpdateVar();
        }
    }
    if (Input::WasPressed(Key::End))
    {
        if (m_setup && setup_switch)
        {
            setup_var[setup_item_key].w = setup_var[setup_item_key].y;
            UpdateVar();
        }
    }
    if (Input::WasPressed(Key::Return))
    {
        beat_flag = true;
        beat_angle = main_angle;
        //flash_flag = true;
        //flash_angle = main_angle;
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
        angle = (main_angle - sync_angle) * sync_speed;
        sync_value = 1.0f - sinf(angle);
        if (angle > 90.0f * PID)
        {
            sync_value = 0;
            sync_flag = false;
        }
    }
    if (beat_flag)
    {
        angle = (main_angle - beat_angle) * beat_speed;
        beat_value = 1.0f - sinf(angle);
        if (angle > 90.0f * PID)
        {
            beat_value = 0;
            beat_flag = false;
        }
    }
    if (flash_flag)
    {
        angle = (main_angle - flash_angle) * flash_speed;
        flash_value = 1.0f - sinf(angle);
        if (angle > 90.0f * PID)
        {
            flash_value = 0;
            flash_flag = false;
        }
    }
    if (fade_flag)
    {
        angle = (main_angle - fade_angle) * fade_speed;
        fade_value = 1.0f - sinf(angle);
        if (angle > 90.0f * PID)
        {
            fade_value = 0;
            fade_flag = false;
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
    glBlendFunc(GL_SRC_COLOR, GL_DST_ALPHA);

    Video::SetClearColor(vec4(screen_color, 1.f));
    Video::SetClearDepth(1.0f); // set depth buffer
    Video::Clear(ClearMask::Color | ClearMask::Depth);

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

    if (m_shader_copper)
    {
        // shader copper
        fbo_tmp->Bind();
        shader_copper->Bind();
        shader_copper->SetUniform(shader_copper_texture, fbo_back->GetTexture(), 0);
        shader_copper->SetUniform(shader_copper_screen_size, (vec2)screen_size);
        shader_copper->SetUniform(shader_copper_time, fx_angle);
        shader_copper->SetUniform(shader_copper_copper, copper);
        TraceQuad();
        shader_color->Unbind();
        fbo_tmp->Unbind();
        // shader simple
        fbo_back->Bind();
        ShaderSimple(fbo_tmp, 0);
        fbo_back->Unbind();
    }

    if (m_shader_remanency)
    {
        // shader remanency
        fbo_tmp->Bind();
        shader_remanency->Bind();
        shader_remanency->SetUniform(shader_remanency_source, fbo_back->GetTexture(), 0);
        shader_remanency->SetUniform(shader_remanency_buffer, fbo_buffer->GetTexture(), 1);
        shader_remanency->SetUniform(shader_remanency_mix, remanency);
        TraceQuad();
        shader_remanency->Unbind();
        fbo_tmp->Unbind();
        // shader simple
        fbo_back->Bind();
        ShaderSimple(fbo_tmp, 0);
        fbo_back->Unbind();
        // save previous fbo
        fbo_tmp->Bind();
        shader_remanency->Bind();
        shader_remanency->SetUniform(shader_remanency_source, fbo_front->GetTexture(), 0);
        shader_remanency->SetUniform(shader_remanency_buffer, fbo_buffer->GetTexture(), 1);
        shader_remanency->SetUniform(shader_remanency_mix, buffer);
        TraceQuad();
        shader_remanency->Unbind();
        fbo_tmp->Unbind();
        // shader simple
        fbo_buffer->Bind();
        ShaderSimple(fbo_tmp, 0);
        fbo_buffer->Unbind();
    }

    // shader glow
    if (m_shader_glow)
    {
        // shader blur horizontal
        fbo_blur_h->Bind();
        shader_blur_h->Bind();
        shader_blur_h->SetUniform(shader_blur_h_texture, fbo_back->GetTexture(), 0);
        shader_blur_h->SetUniform(shader_blur_h_radius, glow_large / screen_size.x);
        TraceQuad();
        shader_blur_h->Unbind();
        fbo_blur_h->Unbind();
        // shader blur vertical
        fbo_blur_v->Bind();
        shader_blur_v->Bind();
        shader_blur_v->SetUniform(shader_blur_v_texture, fbo_blur_h->GetTexture(), 0);
        shader_blur_v->SetUniform(shader_blur_v_radius, glow_large / screen_size.y);
        TraceQuad();
        shader_blur_v->Unbind();
        fbo_blur_v->Unbind();
        // shader blur horizontal
        fbo_blur_h->Bind();
        shader_blur_h->Bind();
        shader_blur_h->SetUniform(shader_blur_h_texture, fbo_blur_v->GetTexture(), 0);
        shader_blur_h->SetUniform(shader_blur_h_radius, glow_small / screen_size.x);
        TraceQuad();
        shader_blur_h->Unbind();
        fbo_blur_h->Unbind();
        // shader blur vertical
        fbo_blur_v->Bind();
        shader_blur_v->Bind();
        shader_blur_v->SetUniform(shader_blur_v_texture, fbo_blur_h->GetTexture(), 0);
        shader_blur_v->SetUniform(shader_blur_v_radius, glow_small / screen_size.y);
        TraceQuad();
        shader_blur_v->Unbind();
        fbo_blur_v->Unbind();
        // shader glow
        fbo_front->Bind();
        shader_glow->Bind();
        shader_glow->SetUniform(shader_glow_glow, fbo_blur_v->GetTexture(), 0);
        shader_glow->SetUniform(shader_glow_source, fbo_back->GetTexture(), 1);
        shader_glow->SetUniform(shader_glow_mix, glow_mix);
        TraceQuad();
        shader_glow->Unbind();
        fbo_front->Unbind();
    }
    else
    {
        // shader simple
        fbo_front->Bind();
        ShaderSimple(fbo_back, 0);
        fbo_front->Unbind();
    }

    if (m_shader_color)
    {
        // shader color
        fbo_tmp->Bind();
        shader_color->Bind();
        shader_color->SetUniform(shader_color_texture, fbo_front->GetTexture(), 0);
        shader_color->SetUniform(shader_color_screen_size, (vec2)screen_size);
        shader_color->SetUniform(shader_color_filter, color_filter);
        shader_color->SetUniform(shader_color_color, color_color);
        shader_color->SetUniform(shader_color_flash, flash_value);
        TraceQuad();
        shader_color->Unbind();
        fbo_tmp->Unbind();
        // shader simple
        fbo_front->Bind();
        ShaderSimple(fbo_tmp, 0);
        fbo_front->Unbind();
    }

    if (m_shader_noise)
    {
        // shader noise
        fbo_tmp->Bind();
        shader_noise->Bind();
        shader_noise->SetUniform(shader_noise_texture, fbo_front->GetTexture(), 0);
        shader_noise->SetUniform(shader_noise_screen_size, (vec2)screen_size);
        shader_noise->SetUniform(shader_noise_time, fx_angle);
        shader_noise->SetUniform(shader_noise_offset, noise_offset);
        shader_noise->SetUniform(shader_noise_noise, noise_noise);
        shader_noise->SetUniform(shader_noise_retrace, noise_retrace);
        TraceQuad();
        shader_noise->Unbind();
        fbo_tmp->Unbind();
        // shader simple
        fbo_front->Bind();
        ShaderSimple(fbo_tmp, 0);
        fbo_front->Unbind();
    }

    if (m_shader_blur)
    {
        // shader blur horizontal
        fbo_tmp->Bind();
        shader_blur_h->Bind();
        shader_blur_h->SetUniform(shader_blur_h_texture, fbo_front->GetTexture(), 0);
        shader_blur_h->SetUniform(shader_blur_h_radius, blur / screen_size.x);
        TraceQuad();
        shader_blur_h->Unbind();
        fbo_tmp->Unbind();
        // shader blur vertical
        fbo_front->Bind();
        shader_blur_v->Bind();
        shader_blur_v->SetUniform(shader_blur_v_texture, fbo_tmp->GetTexture(), 0);
        shader_blur_v->SetUniform(shader_blur_v_radius, blur / screen_size.y);
        TraceQuad();
        shader_blur_v->Unbind();
        fbo_front->Unbind();
    }

    if (m_shader_postfx)
    {
        // shader postfx
        shader_postfx->Bind();
        shader_postfx->SetUniform(shader_postfx_texture, fbo_front->GetTexture(), 0);
        shader_postfx->SetUniform(shader_postfx_screen_size, (vec2)screen_size);
        shader_postfx->SetUniform(shader_postfx_time, fx_angle);
        shader_postfx->SetUniform(shader_postfx_deform, postfx_deform);
        shader_postfx->SetUniform(shader_postfx_ghost1, postfx_ghost1);
        shader_postfx->SetUniform(shader_postfx_ghost2, postfx_ghost2);
        shader_postfx->SetUniform(shader_postfx_vignetting, postfx_vignetting);
        shader_postfx->SetUniform(shader_postfx_aberration, postfx_aberration);
        shader_postfx->SetUniform(shader_postfx_moire_h, postfx_moire_h);
        shader_postfx->SetUniform(shader_postfx_moire_v, postfx_moire_v);
        shader_postfx->SetUniform(shader_postfx_scanline_h, postfx_scanline_h);
        shader_postfx->SetUniform(shader_postfx_scanline_v, postfx_scanline_v);
        shader_postfx->SetUniform(shader_postfx_corner, postfx_corner);
        shader_postfx->SetUniform(shader_postfx_sync, (float)fabs(sync_value*cosf((main_angle-sync_angle)*6.0f)));
        shader_postfx->SetUniform(shader_postfx_beat, (float)fabs(beat_value*cosf((main_angle-beat_angle)*6.0f)));
        TraceQuad();
        shader_postfx->Unbind();
    }
    else
    {
        // shader simple
        ShaderSimple(fbo_front, 0);
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

Render::~Render()
{
}
