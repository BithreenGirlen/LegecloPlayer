#ifndef VIEW_MANAGER_H_
#define VIEW_MANAGER_H_

#include <Windows.h>

class CViewManager
{
public:
    CViewManager() = default;
    ~CViewManager() = default;

    void setBaseSize(HWND hRenderTargetWindow, unsigned int width, unsigned int height);
    void rescale(bool toUpscale);
    void setOffset(int iX, int iY);
    void resetZoom();
    void onStyleChanged();

    float getScale() const { return m_fScale; };
    float getOffsetX() const { return m_fOffsetX; };
    float getOffsetY() const { return m_fOffsetY; };
private:
    enum Constants { kBaseWidth = 1280, kBaseHeight = 720 };

    HWND m_hRenderTargetWnd = nullptr;

    unsigned int m_baseWidth = Constants::kBaseWidth;
    unsigned int m_baseHeight = Constants::kBaseHeight;
    float m_fDefaultScale = 1.f;

    float m_fScale = 1.f;
    float m_fOffsetX = 0;
    float m_fOffsetY = 0;

    void workOutDefaultScale();
    void resizeWindow();
    void adjustOffset();
    void requestRedraw() const;
};

#endif // !VIEW_MANAGER_H_
