//
// Neercs
//

#if !defined __TEXT_RENDER_H__
#define __TEXT_RENDER_H__

struct TextRender
{
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
    ShaderAttrib m_coord, m_color, m_char;
    ShaderUniform m_texture, m_transform;
    VertexDeclaration *m_vdecl;
    VertexBuffer *m_vbo1, *m_vbo2, *m_vbo3;
    FrameBuffer *m_fbo;
};

#endif // __TEXT_RENDER_H__

