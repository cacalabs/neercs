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
#include "loldebug.h"

using namespace std;
using namespace lol;

#include "../neercs.h"
#include "render.h"
#include "text-render.h"

extern char const *lolfx_simple;
extern char const *lolfx_blurh;
extern char const *lolfx_blurv;
extern char const *lolfx_glow;
extern char const *lolfx_remanence;
extern char const *lolfx_copper;
extern char const *lolfx_color;
extern char const *lolfx_noise;
extern char const *lolfx_postfx;
extern char const *lolfx_mirror;
extern char const *lolfx_radial;

#define PID M_PI/180.0f    // pi ratio

/*
 * Global variable -- ugly (<rez> sam: c'est toi qui est ugly! \:D/)
 */
bool g_setup = false;

/*
 * Various variables
 */

int active = true;         // window active flag
float nearplane = 0.1f;    // nearplane
float farplane = 1000.0f;  // farplane
/* timer variable */
float timer = 0;           // timer
/* window variable */
ivec2 screen_size;         // screen size
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
/* text variable */
ivec2 map_size(256,256);        // texture map size
ivec2 canvas_char(0,0);         // canvas char number
ivec2 canvas_size(0,0);         // caca size
ivec2 font_size(8,8);           // font size
ivec2 ratio_2d(1,1);            // 2d ratio
ivec2 border(0,0);              // border width
/* shader variable */
vec2 buffer(1.0f,0.0f);         // [new frame mix,old frame mix]
vec2 remanence(0.0f,0.0f);      // remanence [source mix,buffer mix]
vec2 glow_mix(0.5f,0.0f);       // glow mix [source mix,glow mix]
vec2 glow_large(0.0f,0.0f);     // large glow radius [center,corner]
vec2 glow_small(0.0f,0.0f);     // small glow radius [center,corner]
vec2 blur(0.0f,0.0f);           // blur radius [center,corner]
vec4 copper_copper(0.75f,0.25f,0.42f,4.0f);     // copper [base,variable,repeat,color cycle]
vec3 copper_mask_color(4.0f,4.0f,4.0f);         // color [red,green,blue]
vec3 color_filter(1.0f,1.0f,1.0f);              // color filter [red,green,blue]
vec4 color_color(1.0f,1.0f,0.0f,0.0f);          // color modifier [brightness,contrast,level,grayscale]
vec2 noise_offset(0.0f,0.0f);                   // random line [horizontal,vertical]
float noise_noise = 0.0f;                       // noise
vec3 noise_retrace(0.0f,0.0f,0.0f);             // retrace [strength,length,speed]
vec2 postfx_deform(0.0f,0.5f);                  // deformation [ratio,zoom]
float postfx_vignetting = 0.0f;                 // vignetting strength
float postfx_aberration = 0.0f;                 // chromatic aberration
vec4 postfx_ghost1(0.0f,0.0f,0.0f,0.0f);        // ghost picture 1 [position x,position y,position z,strength]
vec4 postfx_ghost2(0.0f,0.0f,0.0f,0.0f);        // ghost picture 2 [position x,position y,position z,strength]
vec4 postfx_glass(8.0f,0.0f,0.0f,0.0f);         // glass [depth,thickness,strength,deform ratio]
vec4 postfx_moire_h(1.0f,0.0f,0.0f,0.0f);       // vertical moire [base,variable,repeat,shift]
vec4 postfx_moire_v(1.0f,0.0f,0.0f,0.0f);       // horizontal moire [base,variable,repeat,shift]
vec4 postfx_scanline_h(1.0f,0.0f,0.0f,0.0f);    // vertical scanline [base,variable,repeat,shift]
vec4 postfx_scanline_v(1.0f,0.0f,0.0f,0.0f);    // horizontal scanline [base,variable,repeat,shift]
vec3 postfx_corner(0.0f,1.0f,1.0f);             // corner [width,radius,blur]
vec4 mirror(0.0f,0.0f,0.0f,1.0f);               // mirror [width,height,strength,ratio]
vec4 radial(4.0f,0.9f,16,0.0f);                 // radial [distance,fade ratio,iteration,strength]
/* theme variable */
int theme_i = 1;                // current theme
const int theme_n = 2;          // theme number
const int theme_var_n = 79;     // var number
/* setup variable */
bool setup_switch = false;      // switch [option/item]
int setup_n = 0;                // item/option number
int setup_h = 8;                // height
int setup_cursor = 0;           // cursor position
int setup_option_i = 0;         // selected option
int setup_option_n = 14;        // option number
int setup_option_p = 0;         // option position
int setup_item_i = 0;           // selected item
int setup_item_n = 8;           // item number
int setup_item_p = 0;           // item position
int setup_item_key = 0;         // item array key
ivec3 setup_size(29,9,12);      // size [w,h,split]
ivec2 setup_canvas_size(ivec2(setup_size.x + 1, setup_size.y + 1) * font_size * ivec2(2,4));
ivec2 setup_color(0xaaa,0x222); // color [foreground,background] 0x678,0x234
char const *setup_text[]={
    "main",
        "2d ratio w",
        "2d ratio h",
        "border w",
        "border h",
        "",
        "",
        "",
        "",
    "remanence",
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
    "copper",
        "enable",
        "base",
        "variable",
        "repeat",
        "color cycle",
        "color r",
        "color g",
        "color b",
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
    "glass",
        "depth",
        "thickness",
        "strength",
        "deform ratio",
        "",
        "",
        "",
        "",
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
        "v shift",
    "mirror",
        "enable",
        "width",
        "height",
        "strength",
        "ratio",
        "",
        "",
        "",
    "radial",
        "enable",
        "distance",
        "fade ratio",
        "iteration",
        "strength",
        "",
        "",
        ""
    };

vec4 setup_var[]={ // setup variable [start,end,step,value]
vec4(0), /* main */
    vec4(1,  8, 1, ratio_2d.x),
    vec4(1,  8, 1, ratio_2d.y),
    vec4(0, 16, 1, border.x),
    vec4(0, 16, 1, border.y),
    vec4(0),
    vec4(0),
    vec4(0),
    vec4(0),
vec4(0), /* remanence */
    vec4(0, 1, 1, 1),
    vec4(0.0f, 1.0f, 0.1f, buffer.x),
    vec4(0.0f, 1.0f, 0.1f, buffer.y),
    vec4(0.0f, 1.0f, 0.1f, remanence.x),
    vec4(0.0f, 1.0f, 0.1f, remanence.y),
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
    vec4(0, 2, 0.05f, blur.x),
    vec4(0, 2, 0.05f, blur.y),
    vec4(0),
    vec4(0),
    vec4(0),
    vec4(0),
    vec4(0),
vec4(0), /* screen */
    vec4(0, 1, 1, 1),
    vec4(0.0f, 1.0f, 0.05f, postfx_deform.x),
    vec4(0.2f, 0.7f, 0.01f, postfx_deform.y),
    vec4(0.0f, 4.0f, 0.10f, postfx_corner.x),
    vec4(0.0f, 1.0f, 0.05f, postfx_corner.y),
    vec4(0.8f, 1.0f, 0.01f, postfx_corner.z),
    vec4(0.0f, 1.0f, 0.10f, postfx_vignetting),
    vec4(0),
vec4(0), /* copper */
    vec4(0, 1, 1, 1),
    vec4(0.0f, 1.0f, 0.05f, copper_copper.x),
    vec4(0.0f, 1.0f, 0.05f, copper_copper.y),
    vec4(0.0f, 1.0f, 0.02f, copper_copper.z),
    vec4(1.0f, 8.0f, 0.25f, copper_copper.w),
    vec4(0.0f, 4.0f, 0.25f, copper_mask_color.x),
    vec4(0.0f, 4.0f, 0.25f, copper_mask_color.y),
    vec4(0.0f, 4.0f, 0.25f, copper_mask_color.z),
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
    vec4(0, 1, 1, 1),
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
vec4(0), /* glass */
    vec4(0.0f, 16.0f, 0.50f, postfx_glass.x),
    vec4(0.0f,  1.0f, 0.05f, postfx_glass.y),
    vec4(0.0f,  2.0f, 0.05f, postfx_glass.z),
    vec4(0.0f,  1.0f, 0.05f, postfx_glass.w),
    vec4(0),
    vec4(0),
    vec4(0),
    vec4(0),
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
vec4(0), /* mirror */
    vec4(0, 1, 1, 1),
    vec4(0.0f, 2.0f, 0.05f, mirror.x),
    vec4(0.0f, 2.0f, 0.05f, mirror.y),
    vec4(0.0f, 1.0f, 0.05f, mirror.z),
    vec4(1.0f, 8.0f, 1.00f, mirror.w),
    vec4(0),
    vec4(0),
    vec4(0),
vec4(0), /* radial blur */
    vec4(0, 1, 1, 1),
    vec4(2.0f, 8.0f, 0.25f, radial.x),
    vec4(0.8f, 1.0f, 0.01f, radial.y),
    vec4(2.0f,32.0f, 2.00f, radial.z),
    vec4(0.0f, 1.0f, 0.05f, radial.w),
    vec4(0),
    vec4(0),
    vec4(0),
vec4(0) /* ? */
};

float theme_var[]={
    // DO LIKE SETUP TO KEEP KEY
    /* default */
    1,1,                      // ratio_2d
    0,0,                      // border
    1.0f,0.8f,                // buffer
    0.0f,0.0f,                // remanence
    0.5f,0.0f,                // glow_mix
    0.0f,0.0f,                // glow_large
    0.0f,0.0f,                // glow_small
    0.0f,0.0f,                // blur
    0.0f,0.5f,                // postfx_deform
    0.0f,1.0f,1.0f,           // postfx_corner
    0.0f,                     // postfx_vignetting
    0.75f,0.25f,0.42f,4.0f,   // copper_copper
    4.0f,4.0f,4.0f,           // copper_mask_color
    1.0f,1.0f,1.0f,           // color_filter
    1.0f,1.0f,0.0f,0.0f,      // color_color
    0.0f,                     // postfx_aberration
    0.0f,0.0f,                // noise_offset
    0.0f,                     // noise_noise
    0.0f,0.0f,0.0f,           // noise_retrace
    0.0f,0.0f,0.0f,0.0f,      // postfx_ghost1
    0.0f,0.0f,0.0f,0.0f,      // postfx_ghost2
    8.0f,0.0f,0.0f,0.0f,      // postfx_glass
    1.0f,0.0f,0.0f,0.0f,      // postfx_moire_h
    1.0f,0.0f,0.0f,0.0f,      // postfx_moire_v
    1.0f,0.0f,0.0f,0.0f,      // postfx_scanline_h
    1.0f,0.0f,0.0f,0.0f,      // postfx_scanline_v
    0.0f,0.0f,0.0f,1.0f,      // mirror
    4.0f,0.9f,16,0.0f,        // radial
    /* crt */
    2,3,                      // ratio_2d
    2,1,                      // border
    0.2f,0.8f,                // buffer
    0.6f,0.4f,                // remanence
    0.7f,0.3f,                // glow_mix
    3.0f,0.0f,                // glow_large
    1.5f,0.0f,                // glow_small
    0.5f,0.0f,                // blur
    0.8f,0.48f,               // postfx_deform
    0.0f,0.8f,0.96f,          // postfx_corner
    0.5f,                     // postfx_vignetting
    0.75f,0.25f,0.42f,4.0f,   // copper_copper
    4.0f,4.0f,4.0f,           // copper_mask_color
    0.9f,0.95f,0.85f,         // color_filter
    1.0f,1.25f,0.1f,0.4f,     // color_color
    4.0f,                     // postfx_aberration
    1.0f,1.0f,                // noise_offset
    0.15f,                    // noise_noise
    1.0f,1.0f,0.5f,           // noise_retrace
    1.0f,0.0f,0.0f,-0.25f,    // postfx_ghost1
    1.5f,0.0f,0.0f,0.25f,     // postfx_ghost2
    8.0f,0.25f,0.75f,0.2f,    // postfx_glass
    0.75f,-0.25f,0.0f,1.0f,   // postfx_moire_h
    0.75f,-0.25f,1.0f,2.0f,   // postfx_moire_v
    1.0f,0.0f,0.0f,0.0f,      // postfx_scanline_h
    0.75f,-0.25f,2.0f,0.0f,   // postfx_scanline_v
    0.95f,0.9f,0.4f,4.0f,     // mirror
    4.0f,0.9f,16,0.25f,       // radial
    /* green screen */
    /* granpa tv */
    0.0f
    };

void Render::InitVar()
{
    int k = theme_i * theme_var_n;
    ratio_2d = vec2(theme_var[k], theme_var[k + 1]);
    border = vec2(theme_var[k + 2], theme_var[k + 3]);
    buffer = vec2(theme_var[k + 4], theme_var[k + 5]);
    remanence = vec2(theme_var[k + 6], theme_var[k + 7]);
    glow_mix = vec2(theme_var[k + 8], theme_var[k + 9]);
    glow_large = vec2(theme_var[k + 10], theme_var[k + 11]);
    glow_small = vec2(theme_var[k + 12], theme_var[k + 13]);
    blur = vec2(theme_var[k + 14], theme_var[k + 15]);
    postfx_deform = vec2(theme_var[k + 16], theme_var[k + 17]);
    postfx_corner = vec3(theme_var[k + 18], theme_var[k + 19], theme_var[k + 20]);
    postfx_vignetting = theme_var[k + 21],
    copper_copper = vec4(theme_var[k + 22], theme_var[k + 23], theme_var[k + 24], theme_var[k + 25]);
    copper_mask_color = vec3(theme_var[k + 26], theme_var[k + 27], theme_var[k + 28]);
    color_filter = vec3(theme_var[k + 29], theme_var[k + 30], theme_var[k + 31]);
    color_color = vec4(theme_var[k + 32], theme_var[k + 33], theme_var[k + 34], theme_var[k + 35]);
    postfx_aberration = theme_var[k + 36];
    noise_offset = vec2(theme_var[k + 37], theme_var[k + 38]);
    noise_noise = theme_var[k + 39];
    noise_retrace = vec3(theme_var[k + 40], theme_var[k + 41], theme_var[k + 42]);
    postfx_ghost1 = vec4(theme_var[k + 43], theme_var[k + 44], theme_var[k + 45], theme_var[k + 46]);
    postfx_ghost2 = vec4(theme_var[k + 47], theme_var[k + 48], theme_var[k + 49], theme_var[k + 50]);
    postfx_glass = vec4(theme_var[k + 51], theme_var[k + 52], theme_var[k + 53], theme_var[k + 54]);
    postfx_moire_h = vec4(theme_var[k + 55], theme_var[k + 56], theme_var[k + 57], theme_var[k + 58]);
    postfx_moire_v = vec4(theme_var[k + 59], theme_var[k + 60], theme_var[k + 61], theme_var[k + 62]);
    postfx_scanline_h = vec4(theme_var[k + 63], theme_var[k + 64], theme_var[k + 65], theme_var[k + 66]);
    postfx_scanline_v = vec4(theme_var[k + 67], theme_var[k + 68], theme_var[k + 69], theme_var[k + 70]);
    mirror = vec4(theme_var[k + 71], theme_var[k + 72], theme_var[k + 73], theme_var[k + 74]);
    radial = vec4(theme_var[k + 75], theme_var[k + 76], theme_var[k + 77], theme_var[k + 78]);
}

void Render::SetupVar()
{
}

void Render::UpdateVar()
{
    int k = 1; /* main */
    ratio_2d = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    border = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    k += 5; /* remanence */
    m_shader_remanence = (setup_var[k].w == 1) ? true : false; k++;
    buffer = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
    remanence = vec2(setup_var[k].w, setup_var[k + 1].w); k += 2;
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
    k += 2; /* copper */
    m_shader_copper = (setup_var[k].w == 1) ? true : false; k++;
    copper_copper = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;
    copper_mask_color = vec3(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w); k += 3;
    k += 1; /* color */
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
    k += 1; /* glass */
    postfx_glass = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;
    k += 5; /* moire */
    postfx_moire_h = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;
    postfx_moire_v = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;
    k += 1; /* scanline */
    postfx_scanline_h = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;
    postfx_scanline_v = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;
    k += 1; /* mirror */
    m_shader_mirror = (setup_var[k].w == 1) ? true : false; k++;
    mirror = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;
    k += 4; /* radial blur */
    m_shader_radial = (setup_var[k].w == 1) ? true : false; k++;
    radial = vec4(setup_var[k].w, setup_var[k + 1].w, setup_var[k + 2].w, setup_var[k + 3].w); k += 4;
}

void Render::UpdateSize()
{
    screen_size = Video::GetSize();
    canvas_char = (screen_size - border * ratio_2d * font_size * 2) / (font_size * ratio_2d);
    canvas_char = max(canvas_char, ivec2(1, 1));
    canvas_size = canvas_char * font_size * ratio_2d;

    border = (screen_size - canvas_size) / 2;

    caca_set_canvas_size(m_cv_screen, canvas_char.x, canvas_char.y);
    caca_set_canvas_size(m_cv_setup, setup_size.x + 1, setup_size.y + 1);
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
Shader *shader_remanence, *shader_copper, *shader_color;
Shader *shader_noise, *shader_postfx, *shader_mirror, *shader_radial;
// shader variables
ShaderUniform shader_simple_texture;
ShaderUniform shader_blur_h_texture,
              shader_blur_h_radius,
              shader_blur_v_texture,
              shader_blur_v_radius;
ShaderUniform shader_glow_glow,
              shader_glow_source,
              shader_glow_mix;
ShaderUniform shader_remanence_source,
              shader_remanence_buffer,
              shader_remanence_mix;
ShaderUniform shader_copper_texture,
              shader_copper_screen_size,
              shader_copper_time,
              shader_copper_copper,
              shader_copper_mask_color;
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
              shader_postfx_ratio_2d,
              shader_postfx_time,
              shader_postfx_deform,
              shader_postfx_ghost1,
              shader_postfx_ghost2,
              shader_postfx_glass,
              shader_postfx_vignetting,
              shader_postfx_aberration,
              shader_postfx_moire_h,
              shader_postfx_moire_v,
              shader_postfx_scanline_h,
              shader_postfx_scanline_v,
              shader_postfx_corner,
              shader_postfx_sync,
              shader_postfx_beat;
ShaderUniform shader_mirror_texture,
              shader_mirror_screen_size,
              shader_mirror_mirror;
ShaderUniform shader_radial_texture,
              shader_radial_screen_size,
              shader_radial_radial;

FrameBuffer *fbo_back, *fbo_front, *fbo_screen;
FrameBuffer *fbo_blur_h, *fbo_blur_v;
FrameBuffer *fbo_tmp, *fbo_buffer;

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
    fbo_screen = new FrameBuffer(screen_size);
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
    // shader remanence
    shader_remanence = Shader::Create(lolfx_remanence);
    shader_remanence_source = shader_remanence->GetUniformLocation("source");
    shader_remanence_buffer = shader_remanence->GetUniformLocation("buffer");
    shader_remanence_mix = shader_remanence->GetUniformLocation("mix");
    // shader copper
    shader_copper = Shader::Create(lolfx_copper);
    shader_copper_texture = shader_copper->GetUniformLocation("texture");
    shader_copper_screen_size = shader_copper->GetUniformLocation("screen_size");
    shader_copper_time = shader_copper->GetUniformLocation("time");
    shader_copper_copper = shader_copper->GetUniformLocation("copper");
    shader_copper_mask_color = shader_copper->GetUniformLocation("mask_color");
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
    shader_postfx_ratio_2d = shader_postfx->GetUniformLocation("ratio_2d");
    shader_postfx_time = shader_postfx->GetUniformLocation("time");
    shader_postfx_deform = shader_postfx->GetUniformLocation("deform");
    shader_postfx_ghost1 = shader_postfx->GetUniformLocation("ghost1");
    shader_postfx_ghost2 = shader_postfx->GetUniformLocation("ghost2");
    shader_postfx_glass = shader_postfx->GetUniformLocation("glass");
    shader_postfx_vignetting = shader_postfx->GetUniformLocation("vignetting");
    shader_postfx_aberration = shader_postfx->GetUniformLocation("aberration");
    shader_postfx_moire_h = shader_postfx->GetUniformLocation("moire_h");
    shader_postfx_moire_v = shader_postfx->GetUniformLocation("moire_v");
    shader_postfx_scanline_h = shader_postfx->GetUniformLocation("scanline_h");
    shader_postfx_scanline_v = shader_postfx->GetUniformLocation("scanline_v");
    shader_postfx_corner = shader_postfx->GetUniformLocation("corner");
    shader_postfx_sync = shader_postfx->GetUniformLocation("sync");
    shader_postfx_beat = shader_postfx->GetUniformLocation("beat");
    // shader mirror
    shader_mirror = Shader::Create(lolfx_mirror);
    shader_mirror_texture = shader_mirror->GetUniformLocation("texture");
    shader_mirror_screen_size = shader_mirror->GetUniformLocation("screen_size");
    shader_mirror_mirror = shader_mirror->GetUniformLocation("mirror");
    // shader radial blur
    shader_radial = Shader::Create(lolfx_radial);
    shader_radial_texture = shader_radial->GetUniformLocation("texture");
    shader_radial_screen_size = shader_radial->GetUniformLocation("screen_size");
    shader_radial_radial = shader_radial->GetUniformLocation("radial");
    // initialize setup
    setup_n = calc_item_length();
    return true;
}

int Render::CreateGLWindow()
{
    InitVar();
    SetupVar();
    UpdateVar();
    UpdateSize();
    InitDraw();
    return true;
}

Render::Render(caca_canvas_t *caca)
  : m_cv_screen(caca),
    m_cv_setup(caca_create_canvas(1, 1)),
    m_fps_debug(0),
    m_ready(false),
    m_pause(false),
    m_shader(true),
    m_shader_glow(true),
    m_shader_blur(true),
    m_shader_remanence(true),
    m_shader_copper(true),
    m_shader_color(true),
    m_shader_noise(true),
    m_shader_postfx(true),
    m_shader_mirror(true),
    m_shader_radial(true)
{
    m_txt_screen = new TextRender(m_cv_screen, font_size);
    m_txt_setup = new TextRender(m_cv_setup, font_size);
}

void Render::TickGame(float seconds)
{
    Entity::TickGame(seconds);
    /* draw setup */
    if (g_setup)
    {
        /* background */
        caca_set_color_argb(m_cv_setup, setup_color.x, setup_color.y);
        caca_fill_box(m_cv_setup, 0, 0, setup_size.x + 1, setup_size.y,' ');
        caca_draw_line(m_cv_setup, setup_size.z - 1, 1, setup_size.z - 1, setup_size.y - 1,'|');
        /* title */
        caca_set_color_argb(m_cv_setup, setup_color.y, setup_color.x);
        caca_draw_line(m_cv_setup, 0, 0, setup_size.x, 0, ' ');
        caca_put_str(m_cv_setup, setup_size.x / 2 - 3, 0, "SETUP");
        /* informations */
        /*
        int w = caca_get_canvas_width(m_cv_screen);
        int h = caca_get_canvas_height(m_cv_screen);
        caca_set_color_argb(m_cv_setup, setup_color.y, setup_color.x);
        caca_printf(m_cv_setup, 1, 0, "%i*%i", w, h);
        */
        /* display option */
        for (int i = 0; i < setup_h; i++)
        {
            int y = 1 + i;
            int k = (setup_option_p + i) * (setup_item_n + 1);
            if (setup_option_i != setup_option_p + i || setup_switch)
            {
                caca_set_color_argb(m_cv_setup, setup_color.x, setup_color.y);
                caca_put_str(m_cv_setup, 1, y, setup_text[k]);
            }
            else
            {
                caca_set_color_argb(m_cv_setup, setup_color.y, 0xfff);//setup_color.x);
                caca_draw_line(m_cv_setup, 0, y, setup_size.z - 2, y,' ');
                caca_put_str(m_cv_setup, 1, y, setup_text[k]);
            }
        }
        /* display item */
        for (int i = 0; i < setup_h; i++)
        {
            int y = 1 + i;
            int k = setup_option_i * (setup_item_n + 1) + 1 + setup_item_p + i;
            if (setup_item_i != setup_item_p + i || !setup_switch)
            {
                caca_set_color_argb(m_cv_setup, setup_color.x, setup_color.y);
                caca_put_str(m_cv_setup, setup_size.z + 1, y, setup_text[k]);
            }
            else
            {
                caca_set_color_argb(m_cv_setup, setup_color.y, 0xfff);//setup_color.x);
                caca_draw_line(m_cv_setup, setup_size.z, y, setup_size.x, y,' ');
                caca_put_str(m_cv_setup, setup_size.z + 1, y, setup_text[k]);
            }
        }
        /* display variable */
        int y = setup_size.y;
        setup_item_key = setup_option_i * (setup_item_n + 1) + 1 + setup_item_i;
        caca_set_color_argb(m_cv_setup, setup_color.y, setup_color.x);
        caca_draw_line(m_cv_setup, 0, y, setup_size.x, y,' ');
        if (setup_switch)
        {
            int x = 1;
            int w = setup_size.x - 3 - 4;
            float bar_w = w / (setup_var[setup_item_key].y - setup_var[setup_item_key].x);
            int bar_x = bar_w * setup_var[setup_item_key].x;
            if ((setup_var[setup_item_key].y - setup_var[setup_item_key].x) / setup_var[setup_item_key].z > 1)
            {
                /* Work around a bug in libcaca */
                if (setup_size.x - 4 < caca_get_canvas_width(m_cv_setup)) caca_printf(m_cv_setup, setup_size.x - 4, y, "%4.2f", setup_var[setup_item_key].w);
                caca_draw_line(m_cv_setup, x, y, x - bar_x + int(bar_w * setup_var[setup_item_key].y), y,'.');
                if (setup_var[setup_item_key].w != setup_var[setup_item_key].x) caca_draw_line(m_cv_setup, x, y, x - bar_x + int(bar_w * setup_var[setup_item_key].w), y, 'x');
            }
            else
            {
                if (setup_var[setup_item_key] != vec4(0))
                {
                    caca_put_str(m_cv_setup, setup_size.x - 3, y, (setup_var[setup_item_key].w == setup_var[setup_item_key].y)?"YES":" NO");
                }
            }
        }
        else
        {
            caca_printf(m_cv_setup, 1, y, "%d/%d", setup_option_i, setup_n);
        }
    }

}

void Render::Pause()
{
    m_pause=!m_pause;
}

void Render::TickDraw(float seconds)
{
    /* keyboard manager */
    if (Input::WasPressed(Key::F1))
    {
        g_setup = !g_setup;
        if (g_setup) setup_n = calc_item_length();
        sync_flag = true;
        sync_angle = main_angle;
        if (m_fps_debug)
        {
            Ticker::Unref(m_fps_debug);
            m_fps_debug = NULL;
        }
        if (g_setup)
        {
            m_fps_debug = new DebugFps(2, 2);
            Ticker::Ref(m_fps_debug);
        }
    }
    if (Input::WasPressed(Key::F2))
    {
        m_shader_glow = !m_shader_glow;
        m_shader_blur = !m_shader_blur;
        m_shader_remanence = !m_shader_remanence;
        //m_shader_copper = !m_shader_copper;
        m_shader_color = !m_shader_color;
        m_shader_noise = !m_shader_noise;
        m_shader_postfx = !m_shader_postfx;
        m_shader_mirror = !m_shader_mirror;
        m_shader_radial = !m_shader_radial;
    }
    if (Input::WasPressed(Key::F4))
    {
        theme_i--;
        if(theme_i < 0) theme_i = theme_n - 1;
        InitVar();
        SetupVar();
        UpdateVar();
        UpdateSize();
    }
    if (Input::WasPressed(Key::F5))
    {
        theme_i++;
        if(theme_i > theme_n) theme_i = 0;
        InitVar();
        SetupVar();
        UpdateVar();
        UpdateSize();
    }
    if (Input::WasPressed(Key::Tab))
    {
        if (g_setup)
        {
            setup_switch = !setup_switch;
            setup_n = calc_item_length();
            setup_cursor = (!setup_switch?setup_option_i:setup_item_i) - (!setup_switch?setup_option_p:setup_item_p);
        }
    }
    if (Input::WasPressed(Key::Up))
    {
        if (g_setup)
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
        if (g_setup)
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
        if (g_setup)
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
        if (g_setup)
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
        if (g_setup && setup_switch)
        {
            setup_var[setup_item_key].w -= setup_var[setup_item_key].z;
            if (setup_var[setup_item_key].w < setup_var[setup_item_key].x) setup_var[setup_item_key].w = setup_var[setup_item_key].x;
            UpdateVar();
            UpdateSize();
        }
    }
    if (Input::WasPressed(Key::Right))
    {
        if (g_setup && setup_switch)
        {
            setup_var[setup_item_key].w += setup_var[setup_item_key].z;
            if (setup_var[setup_item_key].w > setup_var[setup_item_key].y) setup_var[setup_item_key].w = setup_var[setup_item_key].y;
            UpdateVar();
            UpdateSize();
        }
    }
    if (Input::WasPressed(Key::Home))
    {
        if (g_setup && setup_switch)
        {
            setup_var[setup_item_key].w = setup_var[setup_item_key].x;
            UpdateVar();
            UpdateSize();
        }
    }
    if (Input::WasPressed(Key::End))
    {
        if (g_setup && setup_switch)
        {
            setup_var[setup_item_key].w = setup_var[setup_item_key].y;
            UpdateVar();
            UpdateSize();
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
        m_txt_screen->Init();
        m_txt_setup->Init();
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
    m_txt_screen->Render();

    if (g_setup)
        m_txt_setup->Render();

    if (m_shader)
        fbo_back->Bind();

    glViewport(0, 0, screen_size.x, screen_size.y);

    /* Clear the back buffer */
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_COLOR, GL_DST_ALPHA);

    Video::SetClearColor(vec4(0.0f, 0.0f, 0.0f, 1.0f));
    Video::SetClearDepth(1.0f); // set depth buffer
    Video::Clear(ClearMask::Color | ClearMask::Depth);

    m_txt_screen->Blit(border, canvas_size);
    if (g_setup)
        m_txt_setup->Blit((screen_size - setup_canvas_size) / 2, setup_canvas_size);

    glMatrixMode(GL_PROJECTION);
    mat4 m = mat4::ortho(0, screen_size.x, screen_size.y, 0, -1.f, 1.f);
    glLoadMatrixf(&m[0][0]);
    glMatrixMode(GL_MODELVIEW);

    fx_angle=main_angle-part_angle;
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
        shader_copper->SetUniform(shader_copper_copper, vec4(copper_copper.x, copper_copper.y, copper_copper.z * 16.0f, copper_copper.w * 16.0f));
        shader_copper->SetUniform(shader_copper_mask_color, copper_mask_color / 4.0f);
        TraceQuad();
        shader_color->Unbind();
        fbo_tmp->Unbind();
        // shader simple
        fbo_back->Bind();
        ShaderSimple(fbo_tmp, 0);
        fbo_back->Unbind();
    }

    if (m_shader_remanence)
    {
        // shader remanence
        fbo_tmp->Bind();
        shader_remanence->Bind();
        shader_remanence->SetUniform(shader_remanence_source, fbo_back->GetTexture(), 0);
        shader_remanence->SetUniform(shader_remanence_buffer, fbo_buffer->GetTexture(), 1);
        shader_remanence->SetUniform(shader_remanence_mix, remanence);
        TraceQuad();
        shader_remanence->Unbind();
        fbo_tmp->Unbind();
        // shader simple
        fbo_back->Bind();
        ShaderSimple(fbo_tmp, 0);
        fbo_back->Unbind();
        // save previous fbo
        fbo_tmp->Bind();
        shader_remanence->Bind();
        shader_remanence->SetUniform(shader_remanence_source, fbo_screen->GetTexture(), 0);
        shader_remanence->SetUniform(shader_remanence_buffer, fbo_buffer->GetTexture(), 1);
        shader_remanence->SetUniform(shader_remanence_mix, buffer);
        TraceQuad();
        shader_remanence->Unbind();
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
        fbo_screen->Bind();
        shader_glow->Bind();
        shader_glow->SetUniform(shader_glow_glow, fbo_blur_v->GetTexture(), 0);
        shader_glow->SetUniform(shader_glow_source, fbo_back->GetTexture(), 1);
        shader_glow->SetUniform(shader_glow_mix, glow_mix);
        TraceQuad();
        shader_glow->Unbind();
        fbo_screen->Unbind();
    }
    else
    {
        // shader simple
        fbo_screen->Bind();
        ShaderSimple(fbo_back, 0);
        fbo_screen->Unbind();
    }

    if (m_shader_color)
    {
        // shader color
        fbo_tmp->Bind();
        shader_color->Bind();
        shader_color->SetUniform(shader_color_texture, fbo_screen->GetTexture(), 0);
        shader_color->SetUniform(shader_color_screen_size, (vec2)screen_size);
        shader_color->SetUniform(shader_color_filter, color_filter);
        shader_color->SetUniform(shader_color_color, color_color);
        shader_color->SetUniform(shader_color_flash, flash_value);
        TraceQuad();
        shader_color->Unbind();
        fbo_tmp->Unbind();
        // shader simple
        fbo_screen->Bind();
        ShaderSimple(fbo_tmp, 0);
        fbo_screen->Unbind();
    }

    if (m_shader_noise)
    {
        // shader noise
        fbo_tmp->Bind();
        shader_noise->Bind();
        shader_noise->SetUniform(shader_noise_texture, fbo_screen->GetTexture(), 0);
        shader_noise->SetUniform(shader_noise_screen_size, (vec2)screen_size);
        shader_noise->SetUniform(shader_noise_time, fx_angle);
        shader_noise->SetUniform(shader_noise_offset, noise_offset);
        shader_noise->SetUniform(shader_noise_noise, noise_noise);
        shader_noise->SetUniform(shader_noise_retrace, noise_retrace);
        TraceQuad();
        shader_noise->Unbind();
        fbo_tmp->Unbind();
        // shader simple
        fbo_screen->Bind();
        ShaderSimple(fbo_tmp, 0);
        fbo_screen->Unbind();
    }

    if (m_shader_blur)
    {
        // shader blur horizontal
        fbo_tmp->Bind();
        shader_blur_h->Bind();
        shader_blur_h->SetUniform(shader_blur_h_texture, fbo_screen->GetTexture(), 0);
        shader_blur_h->SetUniform(shader_blur_h_radius, blur / screen_size.x);
        TraceQuad();
        shader_blur_h->Unbind();
        fbo_tmp->Unbind();
        // shader blur vertical
        fbo_screen->Bind();
        shader_blur_v->Bind();
        shader_blur_v->SetUniform(shader_blur_v_texture, fbo_tmp->GetTexture(), 0);
        shader_blur_v->SetUniform(shader_blur_v_radius, blur / screen_size.y);
        TraceQuad();
        shader_blur_v->Unbind();
        fbo_screen->Unbind();
    }

    if (m_shader_postfx)
    {
        // shader postfx
        fbo_front->Bind();
        shader_postfx->Bind();
        shader_postfx->SetUniform(shader_postfx_texture, fbo_screen->GetTexture(), 0);
        shader_postfx->SetUniform(shader_postfx_screen_size, (vec2)screen_size);
        shader_postfx->SetUniform(shader_postfx_ratio_2d, (vec2)ratio_2d / 2);
        shader_postfx->SetUniform(shader_postfx_time, fx_angle);
        shader_postfx->SetUniform(shader_postfx_deform, postfx_deform);
        shader_postfx->SetUniform(shader_postfx_ghost1, vec4(postfx_ghost1.x * 0.01, postfx_ghost1.y * 0.01, postfx_ghost1.z * 0.01, postfx_ghost1.w));
        shader_postfx->SetUniform(shader_postfx_ghost2, vec4(postfx_ghost2.x * 0.01, postfx_ghost2.y * 0.01, postfx_ghost2.z * 0.01, postfx_ghost2.w));
        shader_postfx->SetUniform(shader_postfx_glass, vec4(postfx_glass.x * 0.01, postfx_glass.y * 0.01, postfx_glass.z * 0.1, postfx_glass.w));
        shader_postfx->SetUniform(shader_postfx_vignetting, postfx_vignetting);
        shader_postfx->SetUniform(shader_postfx_aberration, postfx_aberration);
        shader_postfx->SetUniform(shader_postfx_moire_h, postfx_moire_h);
        shader_postfx->SetUniform(shader_postfx_moire_v, postfx_moire_v);
        shader_postfx->SetUniform(shader_postfx_scanline_h, postfx_scanline_h);
        shader_postfx->SetUniform(shader_postfx_scanline_v, postfx_scanline_v);
        shader_postfx->SetUniform(shader_postfx_corner, postfx_corner);
        shader_postfx->SetUniform(shader_postfx_sync, (float)fabs(sync_value * cosf((main_angle - sync_angle) * 6.0f)));
        shader_postfx->SetUniform(shader_postfx_beat, (float)fabs(beat_value * cosf((main_angle - beat_angle) * 6.0f)));
        TraceQuad();
        shader_postfx->Unbind();
        fbo_front->Unbind();
    }
    else
    {
        // shader simple
        fbo_front->Bind();
        ShaderSimple(fbo_screen, 0);
        fbo_front->Unbind();
    }

    if (m_shader_mirror)
    {
        // shader mirror
        fbo_tmp->Bind();
        shader_mirror->Bind();
        shader_mirror->SetUniform(shader_mirror_texture, fbo_front->GetTexture(), 0);
        shader_mirror->SetUniform(shader_mirror_screen_size, (vec2)screen_size);
        shader_mirror->SetUniform(shader_mirror_mirror, vec4(mirror.x * 0.1f, mirror.y * 0.1f, mirror.z, mirror.w));
        TraceQuad();
        shader_mirror->Unbind();
        fbo_tmp->Unbind();
        // shader simple
        fbo_front->Bind();
        ShaderSimple(fbo_tmp, 0);
        fbo_front->Unbind();
    }

    if (m_shader_radial)
    {
        // shader radial blur
        fbo_tmp->Bind();
        shader_radial->Bind();
        shader_radial->SetUniform(shader_radial_texture, fbo_front->GetTexture(), 0);
        shader_radial->SetUniform(shader_radial_screen_size, (vec2)screen_size);
        shader_radial->SetUniform(shader_radial_radial, vec4(radial.x, radial.y, radial.z, radial.w * 0.1f));
        TraceQuad();
        shader_radial->Unbind();
        fbo_tmp->Unbind();
        // shader simple
        fbo_front->Bind();
        ShaderSimple(fbo_tmp, 0);
        fbo_front->Unbind();
    }

    // shader simple
    ShaderSimple(fbo_front, 0);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

Render::~Render()
{
    if (m_fps_debug)
        Ticker::Unref(m_fps_debug);
}