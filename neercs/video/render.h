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
    virtual void TickDraw(float seconds);

    void Draw2D();
    void Draw3D();

private:
    int CreateGLWindow();
    int InitDraw();
    void ShaderSimple(FrameBuffer *fbo_output, int n);
    void TraceQuad();
    void Pause();
    void Shader();
    void UpdateVar();
    void UpdateSize();

    caca_canvas_t *m_cv_screen, *m_cv_setup;
    class TextRender *m_txt_screen, *m_txt_setup;

    Entity *m_fps_debug;

    bool m_ready;
    bool m_pause;
    bool m_polygon;
    bool m_shader;
    bool m_shader_glow;
    bool m_shader_blur;
    bool m_shader_remanence;
    bool m_shader_copper;
    bool m_shader_color;
    bool m_shader_noise;
    bool m_shader_postfx;
    bool m_shader_mirror;
};

#endif // __VIDEO_RENDER_H__

