//
// Neercs
//

#if defined HAVE_CONFIG_H
#   include "config.h"
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

extern char const *lolfx_text;

/*
 * Text rendering interface
 */

TextRender::TextRender(caca_canvas_t *caca, ivec2 font_size)
  : m_caca(caca),
    m_font_size(font_size),
    m_canvas_size(caca_get_canvas_width(m_caca),
                  caca_get_canvas_height(m_caca)),
    m_fbo_size(m_font_size * m_canvas_size),
    m_cells(m_canvas_size.x * m_canvas_size.y)
{
}

void TextRender::Init()
{
    m_font = new TileSet("tools/neercs/video/resource/charset_p0t-noodle.png",
                         ivec2(256, 256), ivec2(1));

    m_shader = Shader::Create(lolfx_text);
    m_coord = m_shader->GetAttribLocation("in_Position",
                                          VertexUsage::Position, 0);
    m_color = m_shader->GetAttribLocation("in_Attr",
                                          VertexUsage::Color, 0);
    m_char = m_shader->GetAttribLocation("in_Char",
                                         VertexUsage::Color, 1);
    m_texture = m_shader->GetUniformLocation("in_Texture");
    m_transform = m_shader->GetUniformLocation("in_Transform");
    m_vdecl
      = new VertexDeclaration(VertexStream<vec2>(VertexUsage::Position),
                              VertexStream<uint32_t>(VertexUsage::Color),
                              VertexStream<uint32_t>(VertexUsage::Color));

    CreateBuffers();
}

void TextRender::CreateBuffers()
{
    m_vbo1 = new VertexBuffer(m_cells * sizeof(vec2));
    vec2 *vertices = (vec2 *)m_vbo1->Lock(0, 0);
    for (int j = 0; j < m_canvas_size.y; j++)
    for (int i = 0; i < m_canvas_size.x; i++)
        vertices[j * m_canvas_size.x + i] = vec2(i, j);
    m_vbo1->Unlock();

    m_vbo2 = new VertexBuffer(m_cells * sizeof(int32_t));
    m_vbo3 = new VertexBuffer(m_cells * sizeof(int32_t));

    m_fbo = new FrameBuffer(m_fbo_size);
}

void TextRender::Render()
{
    /* Handle canvas size changes */
    ivec2 current_size(caca_get_canvas_width(m_caca),
                       caca_get_canvas_height(m_caca));
    if (current_size != m_canvas_size)
    {
        delete m_vbo1;
        delete m_vbo2;
        delete m_vbo3;
        delete m_fbo;

        m_canvas_size = current_size;
        m_fbo_size = m_font_size * m_canvas_size;
        m_cells = m_canvas_size.x * m_canvas_size.y;

        CreateBuffers();
    }

    /* Transform matrix for the scene:
     *  - translate to the centre of the glyph
     *  - scale by 2.f * font_size / fbo_size
     *  - translate to the lower left corner */
    mat4 xform = mat4::translate(-1.f, -1.f + 2.0f * m_font_size.y
                                        * m_canvas_size.y / m_fbo_size.y, 0.f)
               * mat4::scale(vec3(2.f * m_font_size / m_fbo_size, 1.f)
                              * vec3(1.f, -1.f, 1.f))
               * mat4::translate(0.5f, 0.5f, 0.f);

    /* Upload libcaca canvas contents to the vertex buffers */
    uint32_t *colors = (uint32_t *)m_vbo2->Lock(0, 0);
    for (int j = 0; j < m_canvas_size.y; j++)
    for (int i = 0; i < m_canvas_size.x; i++)
    {
        uint32_t attr = caca_get_attr(m_caca, i, j);
        uint16_t fg = caca_attr_to_rgb12_fg(attr);
        uint16_t bg = caca_attr_to_rgb12_bg(attr);
        caca_set_color_argb(m_caca, fg, bg);
        attr = caca_get_attr(m_caca, -1, -1);
        caca_put_attr(m_caca, i, j, attr);
    }
    memcpy(colors, caca_get_canvas_attrs(m_caca),
           m_cells * sizeof(uint32_t));
    m_vbo2->Unlock();

    uint32_t *chars = (uint32_t *)m_vbo3->Lock(0, 0);
    memcpy(chars, caca_get_canvas_chars(m_caca),
           m_cells * sizeof(uint32_t));
    m_vbo3->Unlock();

    m_fbo->Bind();
    glViewport(0, 0, m_fbo_size.x, m_fbo_size.y);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_POINT_SPRITE);
    //glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glDisable(GL_POINT_SMOOTH);
    glPointSize((float)max(m_font_size.x, m_font_size.y));
    m_shader->Bind();
    m_font->Bind();
    m_shader->SetUniform(m_texture, 0);
    m_shader->SetUniform(m_transform, xform);
    m_vdecl->SetStream(m_vbo1, m_coord);
    m_vdecl->SetStream(m_vbo2, m_color);
    m_vdecl->SetStream(m_vbo3, m_char);
    m_vdecl->Bind();
    m_vdecl->DrawElements(MeshPrimitive::Points, 0, m_cells);
    m_vdecl->Unbind();
    m_font->Unbind();
    m_shader->Unbind();
    glDisable(GL_POINT_SPRITE);
    m_fbo->Unbind();
}

void TextRender::Blit(ivec2 pos, ivec2 size)
{
    /* FIXME: this is ugly! But we will get rid of it when we
     * do the Direct3D port, so don't worry too much. */
    ShaderTexture t = m_fbo->GetTexture();
    uint64_t const &x = *(uint64_t const *)&t;

    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, (int)x);
    glColor3f(1.0f, 1.0f, 1.0f);

    vec2 tc = (vec2)m_canvas_size * m_font_size / m_fbo_size;

    glLoadIdentity();
    glBegin(GL_QUADS);
        glTexCoord2f(tc.x, tc.y);
        glVertex2i(pos.x + size.x, pos.y);
        glTexCoord2f(0.0f, tc.y);
        glVertex2i(pos.x, pos.y);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2i(pos.x, pos.y + size.y);
        glTexCoord2f(tc.x, 0.0f);
        glVertex2i(pos.x + size.x, pos.y + size.y);
    glEnd();
}

