//
// Neercs
//

#if !defined __VIDEO_RENDER_H__
#define __VIDEO_RENDER_H__

class Render : public WorldEntity
{
public:
    Render(caca_canvas_t *caca);
    virtual ~Render();

    char const *GetName() { return "<title>"; }

protected:
    virtual void TickGame(float seconds);
    virtual void TickDraw(float seconds, Scene &scene);

    void Draw2D();
    void Draw3D();

private:
    int InitDrawResources();
    void ShaderSimple(Framebuffer *fbo_output, int n);
    void TraceQuad();
    void Pause();
    void Shader();
    void InitShaderVar();
    void UpdateVar();
    void UpdateSize();

    caca_canvas_t *m_cv_screen, *m_cv_setup;
    class TextRender *m_txt_screen, *m_txt_setup;
    VertexDeclaration *m_vdecl;
    VertexBuffer *m_vbo;

    Entity *m_fps_debug;

    struct
    {
        Framebuffer *back, *screen, *front, *buffer,
                    *blur_h, *blur_v, *tmp;
    }
    m_fbos;

    bool m_ready;
    bool m_pause;

    struct
    {
        bool shader, remanence, glow, blur, postfx,
             copper, color, noise, mirror, radial;
    }
    m_flags;
};

#endif // __VIDEO_RENDER_H__
