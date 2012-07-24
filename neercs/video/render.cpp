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
bool key_state = 0;        // key state
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
bool sync_flag = false;    // flag
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
ivec2 ratio_2d(1,4);       // 2d ratio
ivec2 map_size(256,256);   // texture map size
ivec2 font_size(8,8);      // font size
ivec2 canvas_char(0,0);    // canvas char number
ivec2 canvas_size(0,0);    // caca size
/* common variable */
float value, angle, radius, scale, speed;
/* shader variable */
float remanency_source_mix = 0.25f;
float remanency_buffer_mix = 0.75f;
float remanency_new_frame_mix = 0.75f;
float remanency_old_frame_mix = 0.25f;
float blur_radius = 0.25f;      // glow radius
float blur_radius_deform = 0.625f;// glow radius deform
int glow_fbo_size = 2;          // glow fbo size
float glow_smoothstep = 0.0f;   // glow smoothstep value (try 0.025f)
float glow_mix_ratio1 = 0.375f; // glow mixing ratio
float glow_mix_ratio2 = 0.625f; // source mixing ratio
float glow_radius1 = 2.0f;      // large glow radius
float glow_radius2 = 1.0f;      // small glow radius
float glow_radius_deform1 = 3.0f;// large glow radius deform
float glow_radius_deform2 = 1.5f;// small glow radius deform
float postfx_deform = 0.625f;   // deformation ratio
float postfx_noise = 0.125f;    // global noise
float postfx_noise_h = 3.0f;    // horizontal noise
float postfx_noise_v = 3.0f;    // vertical noise
float postfx_aberration = 3.0f; // chromatic aberration
bool postfx_scanline = true;    // scanline
float postfx_scanline_h_base = 0.75f;
float postfx_scanline_h_var = 0.25f;
float postfx_scanline_h_repeat_x = 1.0f;
float postfx_scanline_h_repeat_y = 1.0f;
float postfx_scanline_v_base = 0.75f;
float postfx_scanline_v_var = 0.5f;
float postfx_scanline_v_repeat_x = 0.0f;
float postfx_scanline_v_repeat_y = 1.0f;
//float radial_value1 = 2.0f;
//float radial_value2 = 0.8f;
//float radial_color = 0;

Shader *shader_simple;
Shader *shader_blur_h, *shader_blur_v;
Shader *shader_remanency, *shader_glow, *shader_radial, *shader_postfx;
// shader variables
ShaderUniform shader_simple_texture;
ShaderUniform shader_blur_h_texture,
              shader_blur_h_screen_size,
              shader_blur_h_blur,
              shader_blur_h_deform;
ShaderUniform shader_blur_v_texture,
              shader_blur_v_screen_size,
              shader_blur_v_blur,
              shader_blur_v_deform;
ShaderUniform shader_remanency_texture,
              shader_remanency_texture_buffer,
              shader_remanency_screen_size,
              shader_remanency_screen_color,
              shader_remanency_value1,
              shader_remanency_value2;
ShaderUniform shader_glow_texture,
              shader_glow_texture_prv,
              shader_glow_screen_size,
              shader_glow_time,
              shader_glow_step,
              shader_glow_value1,
              shader_glow_value2;
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
              shader_postfx_noise,
              shader_postfx_noise_h,
              shader_postfx_noise_v,
              shader_postfx_aberration,
              shader_postfx_scanline,
              shader_postfx_scanline_h_base,
              shader_postfx_scanline_h_var,
              shader_postfx_scanline_h_repeat_x,
              shader_postfx_scanline_h_repeat_y,
              shader_postfx_scanline_v_base,
              shader_postfx_scanline_v_var,
              shader_postfx_scanline_v_repeat_x,
              shader_postfx_scanline_v_repeat_y,
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
    fbo_blur_h = new FrameBuffer(screen_size / glow_fbo_size);
    fbo_blur_v = new FrameBuffer(screen_size / glow_fbo_size);
    fbo_ping = new FrameBuffer(screen_size);
    fbo_pong = new FrameBuffer(screen_size);
    // shader simple
    shader_simple = Shader::Create(lolfx_simple);
    shader_simple_texture = shader_simple->GetUniformLocation("texture");
    // shader blur horizontal
    shader_blur_h = Shader::Create(lolfx_blurh);
    shader_blur_h_texture = shader_blur_h->GetUniformLocation("texture");
    shader_blur_h_screen_size = shader_blur_h->GetUniformLocation("screen_size");
    shader_blur_h_blur = shader_blur_h->GetUniformLocation("blur");
    shader_blur_h_deform = shader_blur_h->GetUniformLocation("deform");
    // shader blur vertical
    shader_blur_v = Shader::Create(lolfx_blurv);
    shader_blur_v_texture = shader_blur_v->GetUniformLocation("texture");
    shader_blur_v_screen_size = shader_blur_v->GetUniformLocation("screen_size");
    shader_blur_v_blur = shader_blur_v->GetUniformLocation("blur");
    shader_blur_v_deform = shader_blur_v->GetUniformLocation("deform");
    // shader remanency
    shader_remanency = Shader::Create(lolfx_remanency);
    shader_remanency_texture = shader_remanency->GetUniformLocation("texture");
    shader_remanency_texture_buffer = shader_remanency->GetUniformLocation("texture_buffer");
    shader_remanency_screen_size = shader_remanency->GetUniformLocation("screen_size");
    shader_remanency_screen_color = shader_remanency->GetUniformLocation("screen_color");
    shader_remanency_value1 = shader_remanency->GetUniformLocation("value1");
    shader_remanency_value2 = shader_remanency->GetUniformLocation("value2");
    // shader glow
    shader_glow = Shader::Create(lolfx_glow);
    shader_glow_texture = shader_glow->GetUniformLocation("texture");
    shader_glow_texture_prv = shader_glow->GetUniformLocation("texture_prv");
    shader_glow_screen_size = shader_glow->GetUniformLocation("screen_size");
    shader_glow_time = shader_glow->GetUniformLocation("time");
    shader_glow_step = shader_glow->GetUniformLocation("step");
    shader_glow_value1 = shader_glow->GetUniformLocation("value1");
    shader_glow_value2 = shader_glow->GetUniformLocation("value2");
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
    shader_postfx_noise = shader_postfx->GetUniformLocation("noise");
    shader_postfx_noise_h = shader_postfx->GetUniformLocation("noise_h");
    shader_postfx_noise_v = shader_postfx->GetUniformLocation("noise_v");
    shader_postfx_aberration = shader_postfx->GetUniformLocation("aberration");
    shader_postfx_scanline = shader_postfx->GetUniformLocation("scanline");
    shader_postfx_scanline_h_base = shader_postfx->GetUniformLocation("scanline_h_base");
    shader_postfx_scanline_h_var = shader_postfx->GetUniformLocation("scanline_h_var");
    shader_postfx_scanline_h_repeat_x = shader_postfx->GetUniformLocation("scanline_h_repeat_x");
    shader_postfx_scanline_h_repeat_y = shader_postfx->GetUniformLocation("scanline_h_repeat_y");
    shader_postfx_scanline_v_base = shader_postfx->GetUniformLocation("scanline_v_base");
    shader_postfx_scanline_v_var = shader_postfx->GetUniformLocation("scanline_v_var");
    shader_postfx_scanline_v_repeat_x = shader_postfx->GetUniformLocation("scanline_v_repeat_x");
    shader_postfx_scanline_v_repeat_y = shader_postfx->GetUniformLocation("scanline_v_repeat_y");
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

    InitDraw();
    return true;
}

Render::Render(caca_canvas_t *caca)
  : m_caca(caca),
    m_ready(false),
    m_pause(false),
    m_polygon(true),
    m_shader(true),
    m_shader_remanency(true),
    m_shader_blur(true),
    m_shader_glow(true),
    m_shader_fx(true),
    m_shader_postfx(true),
    m_border(false)
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
    //if (Input::GetButtonState(282/*SDLK_F1*/))
    //    LEAULE();
    if (Input::GetButtonState(283/*SDLK_F2*/))
        {
        m_polygon=!m_polygon;
        polygon_fillmode=(m_polygon)?GL_FILL:GL_LINE;
        glPolygonMode(GL_FRONT,polygon_fillmode);
        }
    if (Input::GetButtonState(284/*SDLK_F3*/)&&key_state!=284)
        {
        m_shader=!m_shader;
        key_state=284;
        }
    if (Input::GetButtonState(285/*SDLK_F4*/))
        m_shader_postfx=!m_shader_postfx;
    if (Input::GetButtonState(286/*SDLK_F5*/))
        Pause();

    Entity::TickDraw(seconds);

    if (!m_ready)
    {
        CreateGLWindow();
        text_render->Init();
        m_ready = true;
    }

    // timer
    if (!m_pause)
        main_angle += seconds * 100.0f * PID;
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

    Draw2D();
    Draw3D();
}

void Render::Draw2D()
{
    /* Draw text in an offline buffer */
    text_render->Render();

    if (m_shader)
    {
        fbo_back->Bind();
    }

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
    // draw border
    if (m_border)
    {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glColor3f(1.0f, 1.0f, 1.0f);
        rectangle(border.x - ratio_2d.x, border.y - ratio_2d.y, canvas_size.x + ratio_2d.x * 2, ratio_2d.y);
        rectangle(border.x - ratio_2d.x, border.y, ratio_2d.x, canvas_size.y);
        rectangle(border.x + canvas_size.x, border.y, ratio_2d.x, canvas_size.y);
        rectangle(border.x - ratio_2d.x, border.y + canvas_size.y, canvas_size.x + ratio_2d.x * 2, ratio_2d.y);
        glEnable(GL_BLEND);
    }
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
        shader_remanency->SetTexture(shader_remanency_texture, fbo_back->GetTexture(), 0);
        shader_remanency->SetTexture(shader_remanency_texture_buffer, fbo_buffer->GetTexture(), 1);
        shader_remanency->SetUniform(shader_remanency_screen_size, vec2(1.0f));
        shader_remanency->SetUniform(shader_remanency_screen_color, screen_color);
        shader_remanency->SetUniform(shader_remanency_value1, remanency_source_mix);
        shader_remanency->SetUniform(shader_remanency_value2, remanency_buffer_mix);
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
        shader_remanency->SetTexture(shader_remanency_texture, fbo_front->GetTexture(), 0);
        shader_remanency->SetTexture(shader_remanency_texture_buffer, fbo_buffer->GetTexture(), 1);
        shader_remanency->SetUniform(shader_remanency_screen_size, vec2(1.0f));
        shader_remanency->SetUniform(shader_remanency_screen_color, screen_color);
        shader_remanency->SetUniform(shader_remanency_value1, remanency_new_frame_mix);
        shader_remanency->SetUniform(shader_remanency_value2, remanency_old_frame_mix);
        fs_quad();
        shader_remanency->Unbind();
        fbo_ping->Unbind();
        // shader simple
        fbo_buffer->Bind();
        draw_shader_simple(fbo_ping, 0);
        fbo_buffer->Unbind();
    }

    if (m_shader_fx && m_shader_blur)
    {
        // shader blur horizontal
        fbo_ping->Bind();
        shader_blur_h->Bind();
        shader_blur_h->SetTexture(shader_blur_h_texture, fbo_back->GetTexture(), 0);
        shader_blur_h->SetUniform(shader_blur_h_screen_size, vec2(1.0f));
        shader_blur_h->SetUniform(shader_blur_h_blur, blur_radius / screen_size.x);
        shader_blur_h->SetUniform(shader_blur_h_deform, blur_radius_deform / screen_size.x);
        fs_quad();
        shader_blur_h->Unbind();
        fbo_ping->Unbind();
        // shader blur vertical
        fbo_front->Bind();
        shader_blur_v->Bind();
        shader_blur_v->SetTexture(shader_blur_v_texture, fbo_ping->GetTexture(), 0);
        shader_blur_v->SetUniform(shader_blur_v_screen_size, vec2(1.0f));
        shader_blur_v->SetUniform(shader_blur_v_blur, blur_radius / screen_size.y);
        shader_blur_v->SetUniform(shader_blur_v_deform, blur_radius_deform / screen_size.y);
        fs_quad();
        shader_blur_v->Unbind();
    }
    else
    {
        // shader simple
        fbo_front->Bind();
        draw_shader_simple(fbo_back, 0);
    }
    fbo_front->Unbind();
    // shader glow
    if (m_shader_fx && m_shader_glow)
    {
        // shader blur horizontal
        fbo_blur_h->Bind();
        shader_blur_h->Bind();
        shader_blur_h->SetTexture(shader_blur_h_texture, fbo_ping->GetTexture(), 0);
        shader_blur_h->SetUniform(shader_blur_h_screen_size, vec2(1.0f / glow_fbo_size));
        shader_blur_h->SetUniform(shader_blur_h_blur, glow_radius1 / screen_size.x);
        shader_blur_h->SetUniform(shader_blur_h_deform, glow_radius_deform1 / screen_size.x);
        fs_quad();
        shader_blur_h->Unbind();
        fbo_blur_h->Unbind();
        // shader blur vertical
        fbo_blur_v->Bind();
        shader_blur_v->Bind();
        shader_blur_v->SetTexture(shader_blur_v_texture, fbo_blur_h->GetTexture(), 0);
        shader_blur_v->SetUniform(shader_blur_v_screen_size, vec2(1.0f / glow_fbo_size));
        shader_blur_v->SetUniform(shader_blur_v_blur, glow_radius1 / screen_size.y);
        shader_blur_v->SetUniform(shader_blur_v_deform, glow_radius_deform1 / screen_size.y);
        fs_quad();
        shader_blur_v->Unbind();
        fbo_blur_v->Unbind();
        // shader blur horizontal
        fbo_blur_h->Bind();
        shader_blur_h->Bind();
        shader_blur_h->SetTexture(shader_blur_h_texture, fbo_blur_v->GetTexture(), 0);
        shader_blur_h->SetUniform(shader_blur_h_screen_size, vec2(1.0f / glow_fbo_size));
        shader_blur_h->SetUniform(shader_blur_h_blur, glow_radius2 / screen_size.x);
        shader_blur_h->SetUniform(shader_blur_h_deform, glow_radius_deform2 / screen_size.x);
        fs_quad();
        shader_blur_h->Unbind();
        fbo_blur_h->Unbind();
        // shader blur vertical
        fbo_blur_v->Bind();
        shader_blur_v->Bind();
        shader_blur_v->SetTexture(shader_blur_v_texture, fbo_blur_h->GetTexture(), 0);
        shader_blur_v->SetUniform(shader_blur_v_screen_size, vec2(1.0f / glow_fbo_size));
        shader_blur_v->SetUniform(shader_blur_v_blur, glow_radius2 / screen_size.y);
        shader_blur_h->SetUniform(shader_blur_v_deform, glow_radius_deform2 / screen_size.y);
        fs_quad();
        shader_blur_v->Unbind();
        fbo_blur_v->Unbind();
        // shader glow
        fbo_pong->Bind();
        shader_glow->Bind();
        shader_glow->SetTexture(shader_glow_texture, fbo_blur_v->GetTexture(), 0);
        shader_glow->SetTexture(shader_glow_texture_prv, fbo_front->GetTexture(), 1);
        shader_glow->SetUniform(shader_glow_screen_size, vec2(1.0f));
        shader_glow->SetUniform(shader_glow_time, fx_angle);
        shader_glow->SetUniform(shader_glow_step, glow_smoothstep);
        shader_glow->SetUniform(shader_glow_value1, glow_mix_ratio1);
        shader_glow->SetUniform(shader_glow_value2, glow_mix_ratio2);
        fs_quad();
        shader_glow->Unbind();
    }
    if (!m_shader_fx)
    {
        // shader simple
        fbo_pong->Bind();
        draw_shader_simple(fbo_front, 0);
    }
    fbo_pong->Unbind();
    if (m_shader_postfx)
    {
        // shader postfx
        shader_postfx->Bind();
        shader_postfx->SetTexture(shader_postfx_texture, fbo_pong->GetTexture(), 0);
        shader_postfx->SetUniform(shader_postfx_screen_size, (vec2)screen_size);
        shader_postfx->SetUniform(shader_postfx_time, fx_angle);
        shader_postfx->SetUniform(shader_postfx_deform, postfx_deform);
        shader_postfx->SetUniform(shader_postfx_noise, postfx_noise);
        shader_postfx->SetUniform(shader_postfx_noise_h, postfx_noise_h);
        shader_postfx->SetUniform(shader_postfx_noise_v, postfx_noise_v);
        shader_postfx->SetUniform(shader_postfx_aberration, postfx_aberration);
        shader_postfx->SetUniform(shader_postfx_scanline, postfx_scanline);
        shader_postfx->SetUniform(shader_postfx_scanline_h_base, postfx_scanline_h_base);
        shader_postfx->SetUniform(shader_postfx_scanline_h_var, postfx_scanline_h_var);
        shader_postfx->SetUniform(shader_postfx_scanline_h_repeat_x, postfx_scanline_h_repeat_x);
        shader_postfx->SetUniform(shader_postfx_scanline_h_repeat_y, postfx_scanline_h_repeat_y);
        shader_postfx->SetUniform(shader_postfx_scanline_v_base, postfx_scanline_v_base);
        shader_postfx->SetUniform(shader_postfx_scanline_v_var, postfx_scanline_v_var);
        shader_postfx->SetUniform(shader_postfx_scanline_v_repeat_x, postfx_scanline_v_repeat_x);
        shader_postfx->SetUniform(shader_postfx_scanline_v_repeat_y, postfx_scanline_v_repeat_y);
        shader_postfx->SetUniform(shader_postfx_flash, flash_value);
        shader_postfx->SetUniform(shader_postfx_sync, (float)fabs(beat_value*cosf((main_angle-beat_angle)*8.0f)));
        fs_quad();
        shader_postfx->Unbind();
    }
    else
    {
        // shader simple
        draw_shader_simple(fbo_pong, 0);
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

Render::~Render()
{
}
