//
// Neercs
//

#pragma once

class TextRender
{
public:
    TextRender(caca_canvas_t *caca, ivec2 font_size);
    void Init();
    void Render();
    void Blit(ivec2 pos, ivec2 size);

private:
    void CreateBuffers();

    caca_canvas_t *m_caca;
    ivec2 m_font_size, m_canvas_size, m_fbo_size;
    int m_cells;

    TileSet *m_font;

    Shader *m_shader;
    ShaderAttrib m_color, m_char, m_vertexid;
    ShaderUniform m_texture, m_transform, m_datasize;
    VertexDeclaration *m_vdecl;
    VertexBuffer *m_vbo1, *m_vbo2, *m_vbo3;

    Framebuffer *m_fbo;
};

