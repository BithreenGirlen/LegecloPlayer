

#include "view_manager.h"


/*基準長設定*/
void CViewManager::setBaseSize(HWND hRenderTargetWindow, unsigned int width, unsigned int height)
{
	m_hRenderTargetWnd = hRenderTargetWindow;
	m_baseWidth = width;
	m_baseHeight = height;

	workOutDefaultScale();
}
/*尺度変更*/
void CViewManager::rescale(bool toUpscale)
{
	constexpr float fScaleMin = 0.25f;
	constexpr float fScalePortion = 0.05f;
	if (toUpscale)
	{
		m_fScale += fScalePortion;
	}
	else
	{
		m_fScale -= fScalePortion;
		if (m_fScale < fScaleMin) m_fScale = fScaleMin;
	}
	resizeWindow();
}
/*原点位置移動*/
void CViewManager::setOffset(int iX, int iY)
{
	m_fOffsetX += iX;
	m_fOffsetY += iY;
	adjustOffset();
	requestRedraw();
}
/*原寸表示*/
void CViewManager::resetZoom()
{
	m_fScale = m_fDefaultScale;
	m_fOffsetX = 0;
	m_fOffsetY = 0;

	resizeWindow();
}
/*表示形式変更通知*/
void CViewManager::onStyleChanged()
{
	resizeWindow();
}
/*基準尺度算出*/
void CViewManager::workOutDefaultScale()
{
	/* 基準長がモニタ解像度より大きい場合には予め縮小する */

	unsigned int uiMonitorWidth = static_cast<unsigned int>(::GetSystemMetrics(SM_CXSCREEN));
	unsigned int uiMonitorHeight = static_cast<unsigned int>(::GetSystemMetrics(SM_CYSCREEN));
	if (m_baseWidth > uiMonitorWidth || m_baseHeight > uiMonitorHeight)
	{
		if (uiMonitorWidth > uiMonitorHeight)
		{
			m_fDefaultScale = static_cast<float>(uiMonitorHeight) / m_baseHeight;
		}
		else
		{
			m_fDefaultScale = static_cast<float>(uiMonitorWidth) / m_baseWidth;
		}
	}
	else
	{
		m_fDefaultScale = ::GetDpiForWindow(m_hRenderTargetWnd) / 96.f;
	}

	m_fScale = m_fDefaultScale;
}
/*窓寸法調整*/
void CViewManager::resizeWindow()
{
	if (m_hRenderTargetWnd != nullptr)
	{
		const auto IsWidowBarHidden = [this]()
			-> bool
			{
				if (m_hRenderTargetWnd != nullptr)
				{
					LONG lStyle = ::GetWindowLong(m_hRenderTargetWnd, GWL_STYLE);
					return !((lStyle & WS_CAPTION) && (lStyle & WS_SYSMENU));
				}
				return false;
			};

		RECT rect;
		::GetWindowRect(m_hRenderTargetWnd, &rect);
		int iX = static_cast<int>(m_baseWidth * m_fScale);
		int iY = static_cast<int>(m_baseHeight * m_fScale);

		rect.right = iX + rect.left;
		rect.bottom = iY + rect.top;
		LONG lStyle = ::GetWindowLong(m_hRenderTargetWnd, GWL_STYLE);
		bool bBarHidden = IsWidowBarHidden();
		::AdjustWindowRect(&rect, lStyle, bBarHidden ? FALSE : TRUE);
		::SetWindowPos(m_hRenderTargetWnd, HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
	}

	adjustOffset();
	requestRedraw();
}
/*原点位置調整*/
void CViewManager::adjustOffset()
{
	if (m_hRenderTargetWnd != nullptr)
	{
		int iScaledWidth = static_cast<int>(m_baseWidth * m_fScale);
		int iScaledHeight = static_cast<int>(m_baseHeight * m_fScale);

		RECT rc;
		::GetClientRect(m_hRenderTargetWnd, &rc);

		int iClientWidth = rc.right - rc.left;
		int iClientHeight = rc.bottom - rc.top;

		int iXOffsetMax = iScaledWidth > iClientWidth ? static_cast<int>((iScaledWidth - iClientWidth) / m_fScale) : 0;
		int iYOffsetMax = iScaledHeight > iClientHeight ? static_cast<int>((iScaledHeight - iClientHeight) / m_fScale) : 0;

		if (m_fOffsetX < 0) m_fOffsetX = 0;
		if (m_fOffsetY < 0) m_fOffsetY = 0;

		if (m_fOffsetX > iXOffsetMax)m_fOffsetX = static_cast<float>(iXOffsetMax);
		if (m_fOffsetY > iYOffsetMax)m_fOffsetY = static_cast<float>(iYOffsetMax);
	}
}
/*再描画要求*/
void CViewManager::requestRedraw() const
{
	if (m_hRenderTargetWnd != nullptr)
	{
		::InvalidateRect(m_hRenderTargetWnd, nullptr, FALSE);
	}
}