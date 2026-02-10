/*=========================  Media player setting dialogue  =========================
 * Dialogue-box-like behavior window; modal only.
 *===================================================================================*/

#include "media_setting_dialogue.h"

#include "../mf_media_player.h"

CMediaSettingDialogue::CMediaSettingDialogue()
{
	m_hFont = ::CreateFontW(Constants::kFontSize, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, EASTEUROPE_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"DFKai-SB");
}

CMediaSettingDialogue::~CMediaSettingDialogue()
{
	if (m_hFont != nullptr)
	{
		::DeleteObject(m_hFont);
	}
}

bool CMediaSettingDialogue::Open(HINSTANCE hInstance, HWND hWnd, void* pMediaPlayer, const wchar_t* windowName)
{
	WNDCLASSEXW wcex{};

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = ::GetSysColorBrush(COLOR_BTNFACE);
	wcex.lpszClassName = m_className;

	if (::RegisterClassExW(&wcex))
	{
		m_pMediaPlayer = pMediaPlayer;

		UINT uiDpi = ::GetDpiForSystem();
		int iWindowWidth = ::MulDiv(100, uiDpi, USER_DEFAULT_SCREEN_DPI);
		int iWindowHeight = ::MulDiv(200, uiDpi, USER_DEFAULT_SCREEN_DPI);

		RECT rect{};
		::GetClientRect(hWnd, &rect);
		POINT parentClientPos{ rect.left, rect.top };
		::ClientToScreen(hWnd, &parentClientPos);

		m_hWnd = ::CreateWindowW(m_className, windowName, WS_OVERLAPPEDWINDOW & ~WS_MINIMIZEBOX & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			parentClientPos.x, parentClientPos.y, iWindowWidth, iWindowHeight, hWnd, nullptr, hInstance, this);
		if (m_hWnd != nullptr)
		{
			MessageLoop();
			return true;
		}
	}

	return false;
}

int CMediaSettingDialogue::MessageLoop()
{
	MSG msg;

	for (;;)
	{
		BOOL bRet = ::GetMessageW(&msg, 0, 0, 0);
		if (bRet > 0)
		{
			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
		}
		else if (bRet == 0)
		{
			return static_cast<int>(msg.wParam);
		}
		else
		{
			return -1;
		}
	}

	return 0;
}
/*C CALLBACK*/
LRESULT CMediaSettingDialogue::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CMediaSettingDialogue* pThis = nullptr;
	if (uMsg == WM_NCCREATE)
	{
		LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		pThis = reinterpret_cast<CMediaSettingDialogue*>(pCreateStruct->lpCreateParams);
		::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
	}

	pThis = reinterpret_cast<CMediaSettingDialogue*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if (pThis != nullptr)
	{
		return pThis->HandleMessage(hWnd, uMsg, wParam, lParam);
	}

	return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
/*メッセージ処理*/
LRESULT CMediaSettingDialogue::HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		return OnCreate(hWnd);
	case WM_DESTROY:
		return OnDestroy();
	case WM_CLOSE:
		return OnClose();
	case WM_PAINT:
		return OnPaint();
	case WM_SIZE:
		return OnSize();
	case WM_NOTIFY:
		return OnNotify(wParam, lParam);
	case WM_COMMAND:
		return OnCommand(wParam, lParam);
	case WM_VSCROLL:
		return OnVScroll(wParam, lParam);
	default:
		break;
	}

	return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
/*WM_CREATE*/
LRESULT CMediaSettingDialogue::OnCreate(HWND hWnd)
{
	m_hWnd = hWnd;

	CreateSliders();
	m_volumeStatic.Create(L"Volume", m_hWnd);
	m_rateStatic.Create(L"Rate", m_hWnd);

	::ShowWindow(hWnd, SW_NORMAL);

	::EnableWindow(::GetWindow(m_hWnd, GW_OWNER), FALSE);

	::EnumChildWindows(m_hWnd, SetFontCallback, reinterpret_cast<LPARAM>(m_hFont));

	return 0;
}
/*WM_DESTROY*/
LRESULT CMediaSettingDialogue::OnDestroy()
{
	::PostQuitMessage(0);
	return 0;
}
/*WM_CLOSE*/
LRESULT CMediaSettingDialogue::OnClose()
{
	HWND hOwnerWnd = ::GetWindow(m_hWnd, GW_OWNER);
	::EnableWindow(hOwnerWnd, TRUE);
	::BringWindowToTop(hOwnerWnd);

	::DestroyWindow(m_hWnd);
	::UnregisterClassW(m_className, ::GetModuleHandleW(nullptr));

	return 0;
}
/*WM_PAINT*/
LRESULT CMediaSettingDialogue::OnPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = ::BeginPaint(m_hWnd, &ps);

	::EndPaint(m_hWnd, &ps);

	return 0;
}
/*WM_SIZE*/
LRESULT CMediaSettingDialogue::OnSize()
{
	RECT rect;
	::GetClientRect(m_hWnd, &rect);
	long clientWidth = rect.right - rect.left;
	long clientHeight = rect.bottom - rect.top;

	long spaceX = clientWidth / 96 * 10;
	long spaceY = clientHeight / 96;

	long textSpace = Constants::kFontSize;

	::MoveWindow(m_volumeIntSlider.GetHwnd(), spaceX, spaceY + textSpace, clientWidth / 2 - spaceX * 2, clientHeight - spaceY * 2 - textSpace, TRUE);
	::MoveWindow(m_volumeStatic.GetHwnd(), spaceX, spaceY, Constants::kTextWidth, Constants::kFontSize, TRUE);

	::MoveWindow(m_rateFloatSlider.GetHwnd(), clientWidth / 2 + spaceX, spaceY + textSpace, clientWidth / 2 - spaceX * 2, clientHeight - spaceY * 2 - textSpace, TRUE);
	::MoveWindow(m_rateStatic.GetHwnd(), clientWidth / 2 + spaceX, spaceY, Constants::kTextWidth, Constants::kFontSize, TRUE);

	return 0;
}
/*WM_NOTIFY*/
LRESULT CMediaSettingDialogue::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pNmhdr = reinterpret_cast<LPNMHDR>(lParam);
	if (pNmhdr != nullptr)
	{
		if (pNmhdr->code == TTN_NEEDTEXT)
		{
			if (pNmhdr->hwndFrom == m_rateFloatSlider.GetToolTipHandle())
			{
				m_rateFloatSlider.OnToolTipNeedText(reinterpret_cast<LPTOOLTIPTEXTW>(lParam));
			}
		}
	}
	return 0;
}
/*WM_COMMAND*/
LRESULT CMediaSettingDialogue::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int wmId = LOWORD(wParam);
	int wmKind = LOWORD(lParam);
	if (wmKind == 0)
	{
		/*Menus*/
	}
	else
	{
		/*Controls*/
	}

	return 0;
}
/*WM_VSCROLL*/
LRESULT CMediaSettingDialogue::OnVScroll(WPARAM wParam, LPARAM lParam)
{
	CMfMediaPlayer* pPlayer = static_cast<CMfMediaPlayer*>(m_pMediaPlayer);
	if (pPlayer != nullptr)
	{
		HANDLE hScroll = reinterpret_cast<HANDLE>(lParam);

		if (hScroll == m_volumeIntSlider.GetHwnd())
		{
			double dbVolume = m_volumeIntSlider.GetPosition() / 100.0;
			if (dbVolume != pPlayer->GetCurrentVolume())
			{
				pPlayer->SetCurrentVolume(dbVolume);
			}
		}

		if (hScroll == m_rateFloatSlider.GetHwnd())
		{
			double dbRate = m_rateFloatSlider.GetPosition();
			if (dbRate != pPlayer->GetCurrentRate())
			{
				pPlayer->SetCurrentRate(dbRate);
			}
		}
	}

	return 0;
}
/*音量調整・再生速度変更スライダ作成*/
void CMediaSettingDialogue::CreateSliders()
{
	m_volumeIntSlider.Create(L"", m_hWnd, reinterpret_cast<HMENU>(Controls::kVolumeSlider), 0, 100, 20, true);
	m_rateFloatSlider.Create(L"", m_hWnd, reinterpret_cast<HMENU>(Controls::kRateSkuder), 0.5f, 2.5f, 0.1f, 20, true);

	SetSliderPosition();
}
/*現在値取得・表示*/
void CMediaSettingDialogue::SetSliderPosition()
{
	CMfMediaPlayer* pPlayer = static_cast<CMfMediaPlayer*>(m_pMediaPlayer);
	if (pPlayer != nullptr)
	{
		if (m_volumeIntSlider.GetHwnd() != nullptr)
		{
			double dbVolume = pPlayer->GetCurrentVolume() * 100.0;
			m_volumeIntSlider.SetPosition(static_cast<long long>(dbVolume));
		}

		if (m_rateFloatSlider.GetHwnd() != nullptr)
		{
			double dbRate = pPlayer->GetCurrentRate();
			m_rateFloatSlider.SetPosition(static_cast<float>(dbRate));
		}
	}
}
/*EnumChildWindows CALLBACK*/
BOOL CMediaSettingDialogue::SetFontCallback(HWND hWnd, LPARAM lParam)
{
	::SendMessage(hWnd, WM_SETFONT, static_cast<WPARAM>(lParam), 0);
	/*TRUE: 続行, FALSE: 終了*/
	return TRUE;
}
