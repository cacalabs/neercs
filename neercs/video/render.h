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
    caca_canvas_t *m_caca;
    bool m_ready;
};

#endif // __VIDEO_RENDER_H__

