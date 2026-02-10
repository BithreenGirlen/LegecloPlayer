
#include <Windows.h>
#include <CommCtrl.h>

#include "main_window.h"
#include "win_filesystem.h"
#include "win_dialogue.h"
#include "win_image.h"
#include "legeclo.h"

#include "native-ui/window_menu.h"
#include "native-ui/media_setting_dialogue.h"

#pragma comment(lib, "Comctl32.lib")

CMainWindow::CMainWindow()
{

}

CMainWindow::~CMainWindow()
{

}

bool CMainWindow::Create(HINSTANCE hInstance)
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
		m_hInstance = hInstance;

		UINT uiDpi = ::GetDpiForSystem();
		int iWindowWidth = ::MulDiv(200, uiDpi, USER_DEFAULT_SCREEN_DPI);
		int iWindowHeight = ::MulDiv(200, uiDpi, USER_DEFAULT_SCREEN_DPI);

		m_hWnd = ::CreateWindowW(m_className, m_defaultWindowName, WS_OVERLAPPEDWINDOW & ~WS_MINIMIZEBOX & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			CW_USEDEFAULT, CW_USEDEFAULT, iWindowWidth, iWindowHeight, nullptr, nullptr, hInstance, this);
	}

	return m_hWnd != nullptr;
}

int CMainWindow::MessageLoop()
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
			/*ループ終了*/
			return static_cast<int>(msg.wParam);
		}
		else
		{
			/*ループ異常*/
			return -1;
		}
	}
	return 0;
}
/*C CALLBACK*/
LRESULT CMainWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CMainWindow* pThis = nullptr;
	if (uMsg == WM_NCCREATE)
	{
		LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		pThis = reinterpret_cast<CMainWindow*>(pCreateStruct->lpCreateParams);
		::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
	}

	pThis = reinterpret_cast<CMainWindow*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if (pThis != nullptr)
	{
		return pThis->HandleMessage(hWnd, uMsg, wParam, lParam);
	}

	return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
/*メッセージ処理*/
LRESULT CMainWindow::HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
	case WM_ERASEBKGND:
		return 1;
	case WM_KEYDOWN:
		return OnKeyDown(wParam, lParam);
	case WM_KEYUP:
		return OnKeyUp(wParam, lParam);
	case WM_COMMAND:
		return OnCommand(wParam, lParam);
	case WM_TIMER:
		return OnTimer(wParam);
	case WM_MOUSEMOVE:
		return OnMouseMove(wParam, lParam);
	case WM_MOUSEWHEEL:
		return OnMouseWheel(wParam, lParam);
	case WM_LBUTTONDOWN:
		return OnLButtonDown(wParam, lParam);
	case WM_LBUTTONUP:
		return OnLButtonUp(wParam, lParam);
	case WM_RBUTTONUP:
		return OnRButtonUp(wParam, lParam);
	case WM_MBUTTONUP:
		return OnMButtonUp(wParam, lParam);
	case EventMessage::kAudioPlayer:
		OnAudioPlayerEvent(static_cast<unsigned long>(lParam), wParam);
		break;
	case EventMessage::kVideoPlayer:
		OnVideoPlayerEvent(static_cast<unsigned long>(lParam), wParam);
		break;
	default:

		break;
	}

	return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
/*WM_CREATE*/
LRESULT CMainWindow::OnCreate(HWND hWnd)
{
	m_hWnd = hWnd;

	InitialiseMenuBar();
	UpdateMenuItemState();
	window_menu::SetMenuCheckState(window_menu::GetMenuInBar(m_hWnd, MenuBar::kImage), Menu::kSyncImage, m_isImageSynced);

	m_videoTimer.SetCallback(std::bind(&CMainWindow::UpdateScreen, this));

	m_pD2ImageDrawer = new CD2ImageDrawer(m_hWnd);

	m_pAudioPlayer = new CMfMediaPlayer();
	m_pAudioPlayer->SetPlaybackWindow(m_hWnd, EventMessage::kAudioPlayer);

	m_pVideoTransferor = new CMfVideoTransferor();
	m_pVideoTransferor->SetPlaybackWindow(m_hWnd, EventMessage::kVideoPlayer);
	m_pVideoTransferor->SetLoop(true);

	m_pD2TextWriter = new CD2TextWriter(m_pD2ImageDrawer->GetD2Factory(), m_pD2ImageDrawer->GetD2DeviceContext());
	m_pD2TextWriter->SetupOutLinedDrawing(L"C:\\Windows\\Fonts\\yumindb.ttf");

	m_pViewManager = new CViewManager(m_hWnd);

	return 0;
}
/*WM_DESTROY*/
LRESULT CMainWindow::OnDestroy()
{
	::PostQuitMessage(0);

	return 0;
}
/*WM_CLOSE*/
LRESULT CMainWindow::OnClose()
{
	m_videoTimer.End();

	::KillTimer(m_hWnd, Timer::kText);

	if (m_pD2TextWriter != nullptr)
	{
		delete m_pD2TextWriter;
		m_pD2TextWriter = nullptr;
	}

	if (m_pD2ImageDrawer != nullptr)
	{
		delete m_pD2ImageDrawer;
		m_pD2ImageDrawer = nullptr;
	}

	if (m_pAudioPlayer != nullptr)
	{
		delete m_pAudioPlayer;
		m_pAudioPlayer = nullptr;
	}

	if (m_pVideoTransferor != nullptr)
	{
		delete m_pVideoTransferor;
		m_pVideoTransferor = nullptr;
	}

	if (m_pViewManager != nullptr)
	{
		delete m_pViewManager;
		m_pViewManager = nullptr;
	}

	::DestroyWindow(m_hWnd);
	::UnregisterClassW(m_className, m_hInstance);

	return 0;
}
/*WM_PAINT*/
LRESULT CMainWindow::OnPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = ::BeginPaint(m_hWnd, &ps);

	if (m_pD2ImageDrawer == nullptr || m_pVideoTransferor == nullptr || m_pD2TextWriter == nullptr
		|| m_pViewManager == nullptr || m_nPaintIndex >= m_paintData.size())
	{
		::EndPaint(m_hWnd, &ps);
		return 0;
	}

	m_pD2ImageDrawer->Clear();

	bool bRet = false;
	const adv::PaintDatum* pPaintDatum = GetCurrentPaintData();
	if (pPaintDatum != nullptr)
	{
		if (pPaintDatum->isVideo)
		{
			CComPtr<ID2D1Bitmap> d2d1Bitmap;
			long long frameTime = 0;
			bRet = m_pVideoTransferor->TransferVideoFrame(m_pD2ImageDrawer->GetD2DeviceContext(), &d2d1Bitmap, &frameTime);
			if (bRet)
			{
				bRet = m_pD2ImageDrawer->Draw(d2d1Bitmap.p, { m_pViewManager->GetXOffset(), m_pViewManager->GetYOffset() }, m_pViewManager->GetScale());
				if (bRet)
				{
					StoreVideoFrame(frameTime, d2d1Bitmap);
				}
			}
			else
			{
				long long llCurrentTime = m_pVideoTransferor->GetCurrentTimeInMilliSeconds();
				ID2D1Bitmap* p = RestoreVideoFrame(llCurrentTime);
				if (p != nullptr)
				{
					bRet = m_pD2ImageDrawer->Draw(p, { m_pViewManager->GetXOffset(), m_pViewManager->GetYOffset() }, m_pViewManager->GetScale());
				}
			}
		}
		else /* 静止画 */
		{
			const auto& iter = m_imageMap.find(pPaintDatum->wstrFilePath);
			if (iter != m_imageMap.cend())
			{
				bRet = m_pD2ImageDrawer->Draw(iter->second.p, { m_pViewManager->GetXOffset(), m_pViewManager->GetYOffset() }, m_pViewManager->GetScale());
			}
		}
	}

	if (bRet)
	{
		if (!m_isTextHidden && m_pD2TextWriter != nullptr)
		{
			const std::wstring wstr = FormatCurrentText();
			m_pD2TextWriter->OutLinedDraw(wstr.c_str(), static_cast<unsigned long>(wstr.size()));
		}
		m_pD2ImageDrawer->Display();
	}

	::EndPaint(m_hWnd, &ps);

	return 0;
}
/*WM_SIZE*/
LRESULT CMainWindow::OnSize()
{

	return 0;
}
/*WM_KEYDOWN*/
LRESULT CMainWindow::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case VK_RIGHT:
		AutoTexting();
		break;
	case VK_LEFT:
		ShiftScene(false);
		break;
	default:

		break;
	}

	return 0;
}
/*WM_KEYUP*/
LRESULT CMainWindow::OnKeyUp(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case VK_ESCAPE:
		::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
		break;
	case VK_UP:
		MenuOnForeFile();
		break;
	case VK_DOWN:
		MenuOnNextFile();
		break;
	case 'C':
		if (m_pD2TextWriter != nullptr)
		{
			m_pD2TextWriter->SwitchTextColour();
			UpdateScreen();
		}
		break;
	case 'T':
		m_isTextHidden ^= true;
		UpdateScreen();
		break;
	}
	return 0;
}
/*WM_COMMAND*/
LRESULT CMainWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int wmId = LOWORD(wParam);
	int wmKind = LOWORD(lParam);
	if (wmKind == 0)
	{
		/*Menus*/
		switch (wmId)
		{
		case Menu::kOpenFile:
			MenuOnOpenFile();
			break;
		case Menu::kNextFile:
			MenuOnNextFile();
			break;
		case Menu::kForeFile:
			MenuOnForeFile();
			break;
		case Menu::kAudioSetting:
			MenuOnAudioSetting();
			break;
		case Menu::kVideoSetting:
			MenuOnVideoSetting();
			break;
		case Menu::kFontSetting:
			MenuOnFontSetting();
			break;
		case Menu::kPauseVideo:
			MenuOnPauseVideo();
			break;
		case Menu::kSyncImage:
			MenuOnSyncImage();
			break;
		default:

			break;
		}
	}
	else
	{
		/*Controls*/
	}

	return 0;
}
/*WM_TIMER*/
LRESULT CMainWindow::OnTimer(WPARAM wParam)
{
	switch (wParam)
	{
	case Timer::kText:
		if (m_pAudioPlayer != nullptr)
		{
			if (m_pAudioPlayer->IsEnded())
			{
				AutoTexting();
			}
		}
		break;
	default:
		break;
	}
	return 0;
}
/* WM_MOUSEMOVE */
LRESULT CMainWindow::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
	WORD usKey = LOWORD(wParam);
	if (usKey == MK_LBUTTON)
	{
		POINT pt{};
		::GetCursorPos(&pt);

		if (m_hasLeftBeenDragged)
		{
			if (m_pViewManager != nullptr)
			{
				int iX = m_lastCursorPos.x - pt.x;
				int iY = m_lastCursorPos.y - pt.y;

				m_pViewManager->SetOffset(iX, iY);
				UpdateScreen();
			}
		}

		m_lastCursorPos = pt;
		m_hasLeftBeenDragged = true;
	}

	return 0;
}
/*WM_MOUSEWHEEL*/
LRESULT CMainWindow::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
	short usDelta = static_cast<short>(HIWORD(wParam));
	int iScroll = -usDelta / WHEEL_DELTA;
	WORD usKey = LOWORD(wParam);

	if (usKey == MK_LBUTTON)
	{

	}
	else if (usKey == MK_RBUTTON)
	{
		ShiftScene(iScroll > 0);

		m_wasRightCombinated = true;
	}
	else
	{
		if (m_pViewManager != nullptr)
		{
			m_pViewManager->Rescale(iScroll > 0);
		}
	}

	return 0;
}
/*WM_LBUTTONDOWN*/
LRESULT CMainWindow::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
	::GetCursorPos(&m_lastCursorPos);

	m_wasLeftPressed = true;

	return 0;
}
/*WM_LBUTTONUP*/
LRESULT CMainWindow::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
	if (m_hasLeftBeenDragged)
	{
		m_hasLeftBeenDragged = false;
		m_wasLeftPressed = false;

		return 0;
	}

	WORD usKey = LOWORD(wParam);

	if (usKey == MK_RBUTTON && m_isFramelessWindow)
	{
		::PostMessage(m_hWnd, WM_SYSCOMMAND, SC_MOVE, 0);
		INPUT input{};
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = VK_DOWN;
		::SendInput(1, &input, sizeof(input));

		m_wasRightCombinated = true;
	}

	if (usKey == 0 && m_wasLeftPressed)
	{
		POINT pt{};
		::GetCursorPos(&pt);
		int iX = m_lastCursorPos.x - pt.x;
		int iY = m_lastCursorPos.y - pt.y;

		if (iX == 0 && iY == 0)
		{
			if (m_pVideoTransferor->IsPaused())
			{
				m_pVideoTransferor->FrameStep(true);
			}
			else
			{
				ShiftPaintData();
			}
		}
	}

	m_wasLeftPressed = false;

	return 0;
}
/*WM_RBUTTONUP*/
LRESULT CMainWindow::OnRButtonUp(WPARAM wParam, LPARAM lParam)
{
	if (m_wasRightCombinated)
	{
		m_wasRightCombinated = false;

		return 0;
	}

	WORD usKey = LOWORD(wParam);

	if (usKey == 0)
	{
		if (IsPlayReady() && !m_labelData.empty())
		{
			HMENU hPopupMenu = ::CreatePopupMenu();
			if (hPopupMenu != nullptr)
			{
				for (size_t i = 0; i < m_labelData.size(); ++i)
				{
					::AppendMenuW(hPopupMenu, MF_STRING, i + 1, m_labelData[i].wstrCaption.c_str());
				}

				POINT point{};
				::GetCursorPos(&point);
				BOOL menuIndex = ::TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, 0, m_hWnd, nullptr);
				if (menuIndex > 0)
				{
					size_t labelIndex = static_cast<size_t>(menuIndex - 1);
					m_nSceneIndex = m_labelData[labelIndex].nSceneIndex;
					UpdateScene();
				}
				::DestroyMenu(hPopupMenu);
			}
		}
	}

	return 0;
}
/*WM_MBUTTONUP*/
LRESULT CMainWindow::OnMButtonUp(WPARAM wParam, LPARAM lParam)
{
	WORD usKey = LOWORD(wParam);
	if (usKey == 0)
	{
		if (m_pViewManager != nullptr)
		{
			m_pViewManager->ResetZoom();
		}
	}

	if (usKey == MK_RBUTTON)
	{
		ToggleWindowFrameStyle();

		m_wasRightCombinated = true;
	}

	return 0;
}
/*操作欄作成*/
void CMainWindow::InitialiseMenuBar()
{
	if (m_hMenuBar != nullptr)return;

	HMENU hMenu = window_menu::MenuBuilder(
		{
			{0, L"File", window_menu::MenuBuilder(
				{
					{Menu::kOpenFile, L"Open"},
					{},
					{Menu::kNextFile, L"Next"},
					{Menu::kForeFile, L"Previous"}
				}).Get()
			},
			{0, L"Setting", window_menu::MenuBuilder(
				{
					{Menu::kAudioSetting, L"Audio"},
					{Menu::kVideoSetting, L"Video"},
					{Menu::kFontSetting, L"Font"}
				}).Get()
			},
			{0, L"Image", window_menu::MenuBuilder(
				{
					{
						{Menu::kPauseVideo, L"Pause"},
						{Menu::kSyncImage, L"Sync"}
					}
				}).Get()
			}
		}
	).Get();

	if (::IsMenu(hMenu))
	{
		if (::SetMenu(m_hWnd, hMenu) != 0)
		{
			m_hMenuBar = hMenu;
		}
		else
		{
			::DestroyMenu(hMenu);
		}
	}
}
/*ファイル選択*/
void CMainWindow::MenuOnOpenFile()
{
	constexpr wchar_t fileFilter[] = L"cs_*2.evsc;cs_*3.evsc";
	std::wstring selectedFilePath = win_dialogue::SelectOpenFile(L"script file", fileFilter, L"Select EVSC script", m_hWnd);
	if (!selectedFilePath.empty())
	{
		bool bRet = SetupScenario(selectedFilePath.c_str());
		if (bRet)
		{
			m_scriptFilePaths.clear();
			m_nScriptFilePathIndex = 0;
			win_filesystem::GetFilePathListAndIndex(selectedFilePath, fileFilter, m_scriptFilePaths, &m_nScriptFilePathIndex);
		}
	}
}
/*次ファイルに移動*/
void CMainWindow::MenuOnNextFile()
{
	if (m_scriptFilePaths.empty())return;

	++m_nScriptFilePathIndex;
	if (m_nScriptFilePathIndex >= m_scriptFilePaths.size())m_nScriptFilePathIndex = 0;
	SetupScenario(m_scriptFilePaths[m_nScriptFilePathIndex].c_str());
}
/*前ファイルに移動*/
void CMainWindow::MenuOnForeFile()
{
	if (m_scriptFilePaths.empty())return;

	--m_nScriptFilePathIndex;
	if (m_nScriptFilePathIndex >= m_scriptFilePaths.size())m_nScriptFilePathIndex = m_scriptFilePaths.size() - 1;
	SetupScenario(m_scriptFilePaths[m_nScriptFilePathIndex].c_str());
}
/*音声設定画面呼び出し*/
void CMainWindow::MenuOnAudioSetting()
{
	if (m_pAudioPlayer != nullptr)
	{
		CMediaSettingDialogue mediaSettingDialogue;
		mediaSettingDialogue.Open(m_hInstance, m_hWnd, m_pAudioPlayer, L"Audio");
	}
}
/*動画設定画面呼び出し*/
void CMainWindow::MenuOnVideoSetting()
{
	if (m_pVideoTransferor != nullptr)
	{
		CMediaSettingDialogue mediaSettingDialogue;
		mediaSettingDialogue.Open(m_hInstance, m_hWnd, m_pVideoTransferor, L"Video");
	}
}

void CMainWindow::MenuOnFontSetting()
{
	if (m_fontSettingDiallogue.GetHwnd() == nullptr)
	{
		HWND hWnd = m_fontSettingDiallogue.Open(m_hInstance, m_hWnd, L"Font", m_pD2TextWriter);
		::ShowWindow(hWnd, SW_SHOWNORMAL);
	}
	else
	{
		::SetFocus(m_fontSettingDiallogue.GetHwnd());
	}
}
/*動画一時停止*/
void CMainWindow::MenuOnPauseVideo()
{
	if (m_pVideoTransferor != nullptr)
	{
		bool toBePaused = !m_pVideoTransferor->IsPaused();
		bool bRet = m_pVideoTransferor->SetPause(toBePaused);
		if (bRet)
		{
			window_menu::SetMenuCheckState(window_menu::GetMenuInBar(m_hWnd, MenuBar::kImage), Menu::kPauseVideo, toBePaused);
		}
	}
}
void CMainWindow::MenuOnSyncImage()
{
	bool bRet = window_menu::SetMenuCheckState(window_menu::GetMenuInBar(m_hWnd, MenuBar::kImage), Menu::kSyncImage, !m_isImageSynced);
	if (bRet)
	{
		m_isImageSynced ^= true;
		UpdatePaintData();
	}
}
/*標題変更*/
void CMainWindow::ChangeWindowTitle(const wchar_t* pzTitle)
{
	const wchar_t* windowTitle = pzTitle;
	if (windowTitle != nullptr)
	{
		for (;;)
		{
			const wchar_t* pPos = wcspbrk(windowTitle, L"\\/");
			if (pPos == nullptr)break;
			windowTitle = pPos + 1;
		}
	}

	::SetWindowTextW(m_hWnd, windowTitle == nullptr ? m_defaultWindowName : windowTitle);
}
/*表示形式変更*/
void CMainWindow::ToggleWindowFrameStyle()
{
	if (!IsPlayReady())return;

	LONG lStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);

	m_isFramelessWindow ^= true;

	if (m_isFramelessWindow)
	{
		RECT rect;
		::GetWindowRect(m_hWnd, &rect);

		::SetWindowLong(m_hWnd, GWL_STYLE, lStyle & ~WS_CAPTION & ~WS_SYSMENU);
		::SetWindowPos(m_hWnd, nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);
		::SetMenu(m_hWnd, nullptr);
	}
	else
	{
		::SetWindowLong(m_hWnd, GWL_STYLE, lStyle | WS_CAPTION | WS_SYSMENU);
		::SetMenu(m_hWnd, m_hMenuBar);
	}

	if (m_pViewManager != nullptr)
	{
		m_pViewManager->OnStyleChanged();
	}
}

void CMainWindow::UpdateMenuItemState() const
{
	constexpr unsigned int fileMenuIndices[] = { Menu::kNextFile, Menu::kForeFile };
	bool toEnable = IsPlayReady();

	window_menu::EnableMenuItems(window_menu::GetMenuInBar(m_hWnd, MenuBar::kFile), fileMenuIndices, toEnable);
}
/*寸劇構築*/
bool CMainWindow::SetupScenario(const wchar_t* scriptFilePath)
{
	if (scriptFilePath == nullptr)return false;

	ClearScenarioData();

	bool hadBeenReady = IsPlayReady();
	bool bRet = legeclo::LoadScenario(scriptFilePath, m_textData, m_paintData, m_sceneData, m_labelData);
	if (!bRet)
	{
		::MessageBoxW(m_hWnd, L"Failed to load scenario.", L"Error", MB_ICONERROR);
	}
	else
	{
		CreateImageMap();
		UpdateText();
		UpdatePaintData();
	}

	ChangeWindowTitle(bRet ? scriptFilePath : nullptr);
	if(hadBeenReady != bRet) UpdateMenuItemState();

	return bRet;
}
/*寸劇情報消去*/
void CMainWindow::ClearScenarioData()
{
	m_textData.clear();

	m_paintData.clear();
	m_nPaintIndex = 0;
	m_nLastVideoIndex = 0;

	m_sceneData.clear();
	m_nSceneIndex = 0;

	m_labelData.clear();

	ClearImageMap();

	m_videoTimer.End();
	ClearStoeredVideoFrame();

	m_hasFirstPaintDataBeenLoaded = false;
}
/*再描画要求*/
void CMainWindow::UpdateScreen() const
{
	::InvalidateRect(m_hWnd, nullptr, FALSE);
}

bool CMainWindow::IsPlayReady() const
{
	return !m_textData.empty() && !m_paintData.empty();
}
/*表示図画送り・戻し*/
void CMainWindow::ShiftPaintData()
{
	if (!m_isImageSynced)
	{
		++m_nPaintIndex;
		if (m_nPaintIndex >= m_paintData.size())m_nPaintIndex = 0;
	}

	UpdatePaintData();
}
/*図画データ更新*/
void CMainWindow::UpdatePaintData()
{
	if (m_nPaintIndex >= m_paintData.size())return;

	const adv::PaintDatum *pPaintDatum = GetCurrentPaintData();
	if (pPaintDatum == nullptr)return;

	if (pPaintDatum->isVideo)
	{
		if (m_nLastVideoIndex != m_nPaintIndex)
		{
			ClearStoeredVideoFrame();

			if (m_pVideoTransferor != nullptr)
			{
				m_pVideoTransferor->Play(pPaintDatum->wstrFilePath.c_str());

				m_videoTimer.Start();
			}

			m_nLastVideoIndex = m_nPaintIndex;
		}
	}
	else
	{
		m_nLastVideoIndex = 0;
		ClearStoeredVideoFrame();
		m_videoTimer.End();
	}

	UpdateScreen();
}
/*文章送り・戻し*/
void CMainWindow::ShiftScene(bool forward)
{
	if (forward)
	{
		++m_nSceneIndex;
		if (m_nSceneIndex >= m_sceneData.size())m_nSceneIndex = 0;
	}
	else
	{
		--m_nSceneIndex;
		if (m_nSceneIndex >= m_sceneData.size())m_nSceneIndex = m_sceneData.size() - 1;
	}

	UpdateScene();
}

void CMainWindow::UpdateScene()
{
	UpdateText();
	UpdatePaintData();
}
/*文章更新*/
void CMainWindow::UpdateText()
{
	if (m_nSceneIndex < m_sceneData.size())
	{
		const size_t nTextIndex = m_sceneData[m_nSceneIndex].nTextIndex;
		if (nTextIndex < m_textData.size())
		{
			const adv::TextDatum& t = m_textData[nTextIndex];
			if (!t.wstrVoicePath.empty())
			{
				if (m_pAudioPlayer != nullptr)
				{
					m_pAudioPlayer->Play(t.wstrVoicePath.c_str());
				}
			}
			constexpr unsigned int kTimerInterval = 2000;
			::SetTimer(m_hWnd, Timer::kText, kTimerInterval, nullptr);
		}
	}
}
/*自動送り*/
void CMainWindow::AutoTexting()
{
	if (m_nSceneIndex < m_sceneData.size() - 1)ShiftScene(true);
}
/* 現在の図画受け渡し */
const adv::PaintDatum* CMainWindow::GetCurrentPaintData()
{
	if (m_nSceneIndex < m_sceneData.size())
	{
		if (m_isImageSynced)
		{
			m_nPaintIndex = m_sceneData[m_nSceneIndex].nPaintIndex;
		}

		if (m_nPaintIndex < m_paintData.size())
		{
			return &m_paintData[m_nPaintIndex];
		}
	}

	return nullptr;
}
/*表示文作成*/
std::wstring CMainWindow::FormatCurrentText()
{
	if (m_nSceneIndex < m_sceneData.size())
	{
		const size_t textIndex = m_sceneData[m_nSceneIndex].nTextIndex;
		if (textIndex < m_textData.size())
		{
			std::wstring wstr = m_textData[textIndex].wstrText;
			if (!wstr.empty() && wstr.back() != L'\n')wstr.push_back(L'\n');
			wstr += std::to_wstring(textIndex + 1).append(L"/").append(std::to_wstring(m_textData.size()));
			return wstr;
		}
	}

	return {};
}
/*転送動画溜め置き*/
void CMainWindow::StoreVideoFrame(long long llCurrentTime, CComPtr<ID2D1Bitmap> pD2D1Bitmap)
{
	constexpr int kMaxBufferMilliSeconds = 200;
	if (llCurrentTime < kMaxBufferMilliSeconds)
	{
		m_storedVideoFrames.insert({ llCurrentTime, std::move(pD2D1Bitmap) });
	}
}
/*溜め置き動画消去*/
void CMainWindow::ClearStoeredVideoFrame()
{
	m_storedVideoFrames.clear();
}
/*溜め置き動画取り出し*/
ID2D1Bitmap* CMainWindow::RestoreVideoFrame(long long llCurrentTime)
{
	const auto& iter = m_storedVideoFrames.find(llCurrentTime);
	if (iter != m_storedVideoFrames.cend())
	{
		return iter->second.p;
	}
	return nullptr;
}
/*静画メモリ取り込み*/
void CMainWindow::CreateImageMap()
{
	if (m_pD2ImageDrawer == nullptr)return;
	ID2D1DeviceContext* const pD2d1DeviceContext = m_pD2ImageDrawer->GetD2DeviceContext();
	for (const auto& paintDatum : m_paintData)
	{
		if (!paintDatum.isVideo)
		{
			const auto& iter = m_imageMap.find(paintDatum.wstrFilePath);
			if (iter == m_imageMap.cend())
			{
				CComPtr<IWICBitmap> pWicBitmap;
				bool bRet = win_image::LoadImageToWicBitmap(paintDatum.wstrFilePath.c_str(), reinterpret_cast<void**>(&pWicBitmap));
				if (bRet)
				{
					CComPtr<ID2D1Bitmap> pD2d1Bitmap;
					HRESULT hr = pD2d1DeviceContext->CreateBitmapFromWicBitmap(pWicBitmap, D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)), &pD2d1Bitmap);
					if (SUCCEEDED(hr))
					{
						if (!m_hasFirstPaintDataBeenLoaded)
						{
							const D2D1_SIZE_F& size = pD2d1Bitmap->GetSize();
							m_pViewManager->SetBaseSize(static_cast<unsigned int>(size.width), static_cast<unsigned int>(size.height));
							m_pViewManager->ResetZoom();

							m_hasFirstPaintDataBeenLoaded = true;
						}

						m_imageMap.insert({ paintDatum.wstrFilePath, std::move(pD2d1Bitmap) });
					}
				}
			}
		}
	}
}
/*静画メモリ消去*/
void CMainWindow::ClearImageMap()
{
	m_imageMap.clear();
}
/*IMFMediaEngineNotify::EventNotify*/
void CMainWindow::OnAudioPlayerEvent(unsigned long ulEvent, DWORD_PTR param1)
{
	switch (ulEvent)
	{
	case MF_MEDIA_ENGINE_EVENT_LOADEDMETADATA:

		break;
	case MF_MEDIA_ENGINE_EVENT_ENDED:
		AutoTexting();
		break;
	default:
		break;
	}
}

void CMainWindow::OnVideoPlayerEvent(unsigned long ulEvent, DWORD_PTR param1)
{
	switch (ulEvent)
	{
	case MF_MEDIA_ENGINE_EVENT_LOADEDMETADATA:
		if (m_pVideoTransferor != nullptr)
		{
			if (!m_hasFirstPaintDataBeenLoaded)
			{
				unsigned long ulWidth = 0;
				unsigned long ulHeight = 0;
				bool bRet = m_pVideoTransferor->GetVideoSize(&ulWidth, &ulHeight);
				if (bRet)
				{
					if (m_pViewManager != nullptr)
					{
						m_pViewManager->SetBaseSize(ulWidth, ulHeight);
						m_pViewManager->ResetZoom();
					}
					m_hasFirstPaintDataBeenLoaded = true;
				}
			}
		}
		break;
	case MF_MEDIA_ENGINE_EVENT_TIMEUPDATE:

		break;
	case MF_MEDIA_ENGINE_EVENT_ENDED:

		break;
	default:
		break;
	}
}