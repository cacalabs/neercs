//
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
    void Pause();
    void Shader();

    caca_canvas_t *m_caca;
    bool m_ready;
    bool m_pause;
    bool m_polygon;
    bool m_shader;
    bool m_shader_blur;
    bool m_shader_glow;
    bool m_shader_fx;
    bool m_shader_postfx;
};

#endif // __VIDEO_RENDER_H__

