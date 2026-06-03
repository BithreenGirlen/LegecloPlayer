
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

bool CMainWindow::create(HINSTANCE hInstance)
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

int CMainWindow::messageLoop()
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
		return pThis->handleMessage(hWnd, uMsg, wParam, lParam);
	}

	return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
/*メッセージ処理*/
LRESULT CMainWindow::handleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		return onCreate(hWnd);
	case WM_DESTROY:
		return onDestroy();
	case WM_CLOSE:
		return onClose();
	case WM_PAINT:
		return onPaint();
	case WM_ERASEBKGND:
		return 1;
	case WM_KEYDOWN:
		return onKeyDown(wParam, lParam);
	case WM_KEYUP:
		return onKeyUp(wParam, lParam);
	case WM_COMMAND:
		return onCommand(wParam, lParam);
	case WM_TIMER:
		return onTimer(wParam);
	case WM_MOUSEMOVE:
		return onMouseMove(wParam, lParam);
	case WM_MOUSEWHEEL:
		return onMouseWheel(wParam, lParam);
	case WM_LBUTTONDOWN:
		return onLButtonDown(wParam, lParam);
	case WM_LBUTTONUP:
		return onLButtonUp(wParam, lParam);
	case WM_RBUTTONUP:
		return onRButtonUp(wParam, lParam);
	case WM_MBUTTONUP:
		return onMButtonUp(wParam, lParam);
	case EventMessage::kAudioPlayer:
		onAudioPlayerEvent(static_cast<unsigned long>(lParam), wParam);
		break;
	case EventMessage::kVideoPlayer:
		onVideoPlayerEvent(static_cast<unsigned long>(lParam), wParam);
		break;
	default:

		break;
	}

	return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
/*WM_CREATE*/
LRESULT CMainWindow::onCreate(HWND hWnd)
{
	m_hWnd = hWnd;

	initialiseMenuBar();
	updateMenuItemState();
	window_menu::SetMenuCheckState(window_menu::GetMenuInBar(m_hWnd, MenuBar::kImage), Menu::kSyncImage, m_isImageSynced);

	const auto TimerCallback = [](void* pUserData)
		-> void
		{
			CMainWindow* pThis = static_cast<CMainWindow*>(pUserData);
			if (pThis != nullptr)
			{
				pThis->updateScreen();
			}
		};

	m_videoTimer.setCallback(TimerCallback, this);

	m_pD2ImageDrawer = new CD2ImageDrawer(m_hWnd);

	m_pAudioPlayer = new CMfMediaPlayer();
	m_pAudioPlayer->setPlaybackWindow(m_hWnd, EventMessage::kAudioPlayer);

	m_pVideoTransferor = new CMfVideoTransferor();
	m_pVideoTransferor->setPlaybackWindow(m_hWnd, EventMessage::kVideoPlayer);
	m_pVideoTransferor->setLoop(true);

	m_pD2TextWriter = new CD2TextWriter(m_pD2ImageDrawer->getD2Factory(), m_pD2ImageDrawer->getD2DeviceContext());
	m_pD2TextWriter->setupOutLinedDrawing(L"C:\\Windows\\Fonts\\yumindb.ttf");

	m_pViewManager = new CViewManager(m_hWnd);

	return 0;
}
/*WM_DESTROY*/
LRESULT CMainWindow::onDestroy()
{
	::PostQuitMessage(0);

	return 0;
}
/*WM_CLOSE*/
LRESULT CMainWindow::onClose()
{
	m_videoTimer.end();

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
LRESULT CMainWindow::onPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = ::BeginPaint(m_hWnd, &ps);

	if (m_pD2ImageDrawer == nullptr || m_pVideoTransferor == nullptr || m_pD2TextWriter == nullptr
		|| m_pViewManager == nullptr || m_nPaintIndex >= m_paintData.size())
	{
		::EndPaint(m_hWnd, &ps);
		return 0;
	}

	m_pD2ImageDrawer->clear();

	bool bRet = false;
	const adv::PaintDatum* pPaintDatum = getCurrentPaintData();
	if (pPaintDatum != nullptr)
	{
		if (pPaintDatum->isVideo)
		{
			CComPtr<ID2D1Bitmap> d2d1Bitmap;
			long long frameTime = 0;
			bRet = m_pVideoTransferor->transferVideoFrame(m_pD2ImageDrawer->getD2DeviceContext(), &d2d1Bitmap, &frameTime);
			if (bRet)
			{
				bRet = m_pD2ImageDrawer->draw(d2d1Bitmap.p, { m_pViewManager->getOffsetX(), m_pViewManager->getOffsetY() }, m_pViewManager->getScale());
				if (bRet)
				{
					storeVideoFrame(frameTime, d2d1Bitmap);
				}
			}
			else
			{
				long long llCurrentTime = m_pVideoTransferor->getCurrentTimeInMilliSeconds();
				ID2D1Bitmap* p = restoreVideoFrame(llCurrentTime);
				if (p != nullptr)
				{
					bRet = m_pD2ImageDrawer->draw(p, { m_pViewManager->getOffsetX(), m_pViewManager->getOffsetY() }, m_pViewManager->getScale());
				}
			}
		}
		else /* 静止画 */
		{
			const auto& iter = m_imageMap.find(pPaintDatum->wstrFilePath);
			if (iter != m_imageMap.cend())
			{
				bRet = m_pD2ImageDrawer->draw(iter->second.p, { m_pViewManager->getOffsetX(), m_pViewManager->getOffsetY() }, m_pViewManager->getScale());
			}
		}
	}

	if (bRet)
	{
		if (!m_isTextHidden && m_pD2TextWriter != nullptr)
		{
			const std::wstring wstr = formatCurrentText();
			m_pD2TextWriter->outLinedDraw(wstr.c_str(), wstr.size());
		}
		m_pD2ImageDrawer->display();
	}

	::EndPaint(m_hWnd, &ps);

	return 0;
}
/*WM_SIZE*/
LRESULT CMainWindow::onSize()
{

	return 0;
}
/*WM_KEYDOWN*/
LRESULT CMainWindow::onKeyDown(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case VK_RIGHT:
		autoTexting();
		break;
	case VK_LEFT:
		shiftScene(false);
		break;
	default:

		break;
	}

	return 0;
}
/*WM_KEYUP*/
LRESULT CMainWindow::onKeyUp(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case VK_ESCAPE:
		::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
		break;
	case VK_UP:
		menuOnForeFile();
		break;
	case VK_DOWN:
		menuOnNextFile();
		break;
	case 'C':
		if (m_pD2TextWriter != nullptr)
		{
			m_pD2TextWriter->toggleTextColour();
			updateScreen();
		}
		break;
	case 'T':
		m_isTextHidden ^= true;
		updateScreen();
		break;
	}
	return 0;
}
/*WM_COMMAND*/
LRESULT CMainWindow::onCommand(WPARAM wParam, LPARAM lParam)
{
	int wmId = LOWORD(wParam);
	int wmKind = LOWORD(lParam);
	if (wmKind == 0)
	{
		/*Menus*/
		switch (wmId)
		{
		case Menu::kOpenFile:
			menuOnOpenFile();
			break;
		case Menu::kNextFile:
			menuOnNextFile();
			break;
		case Menu::kForeFile:
			menuOnForeFile();
			break;
		case Menu::kAudioSetting:
			menuOnAudioSetting();
			break;
		case Menu::kVideoSetting:
			menuOnVideoSetting();
			break;
		case Menu::kFontSetting:
			menuOnFontSetting();
			break;
		case Menu::kPauseVideo:
			menuOnPauseVideo();
			break;
		case Menu::kSyncImage:
			menuOnSyncImage();
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
LRESULT CMainWindow::onTimer(WPARAM wParam)
{
	switch (wParam)
	{
	case Timer::kText:
		if (m_pAudioPlayer != nullptr)
		{
			if (m_pAudioPlayer->isEnded())
			{
				autoTexting();
			}
		}
		break;
	default:
		break;
	}
	return 0;
}
/* WM_MOUSEMOVE */
LRESULT CMainWindow::onMouseMove(WPARAM wParam, LPARAM lParam)
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

				m_pViewManager->setOffset(iX, iY);
				updateScreen();
			}
		}

		m_lastCursorPos = pt;
		m_hasLeftBeenDragged = true;
	}

	return 0;
}
/*WM_MOUSEWHEEL*/
LRESULT CMainWindow::onMouseWheel(WPARAM wParam, LPARAM lParam)
{
	short usDelta = static_cast<short>(HIWORD(wParam));
	int iScroll = -usDelta / WHEEL_DELTA;
	WORD usKey = LOWORD(wParam);

	if (usKey == MK_LBUTTON)
	{

	}
	else if (usKey == MK_RBUTTON)
	{
		shiftScene(iScroll > 0);

		m_wasRightCombined = true;
	}
	else
	{
		if (m_pViewManager != nullptr)
		{
			m_pViewManager->rescale(iScroll > 0);
		}
	}

	return 0;
}
/*WM_LBUTTONDOWN*/
LRESULT CMainWindow::onLButtonDown(WPARAM wParam, LPARAM lParam)
{
	::GetCursorPos(&m_lastCursorPos);

	m_wasLeftPressed = true;

	return 0;
}
/*WM_LBUTTONUP*/
LRESULT CMainWindow::onLButtonUp(WPARAM wParam, LPARAM lParam)
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

		m_wasRightCombined = true;
	}

	if (usKey == 0 && m_wasLeftPressed)
	{
		POINT pt{};
		::GetCursorPos(&pt);
		int iX = m_lastCursorPos.x - pt.x;
		int iY = m_lastCursorPos.y - pt.y;

		if (iX == 0 && iY == 0)
		{
			if (m_pVideoTransferor->isPaused())
			{
				m_pVideoTransferor->frameStep(true);
			}
			else
			{
				shiftPaintData();
			}
		}
	}

	m_wasLeftPressed = false;

	return 0;
}
/*WM_RBUTTONUP*/
LRESULT CMainWindow::onRButtonUp(WPARAM wParam, LPARAM lParam)
{
	if (m_wasRightCombined)
	{
		m_wasRightCombined = false;

		return 0;
	}

	WORD usKey = LOWORD(wParam);

	if (usKey == 0)
	{
		if (isPlayReady() && !m_labelData.empty())
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
					updateScene();
				}
				::DestroyMenu(hPopupMenu);
			}
		}
	}

	return 0;
}
/*WM_MBUTTONUP*/
LRESULT CMainWindow::onMButtonUp(WPARAM wParam, LPARAM lParam)
{
	WORD usKey = LOWORD(wParam);
	if (usKey == 0)
	{
		if (m_pViewManager != nullptr)
		{
			m_pViewManager->resetZoom();
		}
	}

	if (usKey == MK_RBUTTON)
	{
		toggleWindowFrameStyle();

		m_wasRightCombined = true;
	}

	return 0;
}
/*操作欄作成*/
void CMainWindow::initialiseMenuBar()
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
				}).get()
			},
			{0, L"Setting", window_menu::MenuBuilder(
				{
					{Menu::kAudioSetting, L"Audio"},
					{Menu::kVideoSetting, L"Video"},
					{Menu::kFontSetting, L"Font"}
				}).get()
			},
			{0, L"Image", window_menu::MenuBuilder(
				{
					{
						{Menu::kPauseVideo, L"Pause"},
						{Menu::kSyncImage, L"Sync"}
					}
				}).get()
			}
		}
	).get();

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
void CMainWindow::menuOnOpenFile()
{
	constexpr wchar_t fileFilter[] = L"cs_*2.evsc;cs_*3.evsc";
	std::wstring selectedFilePath = win_dialogue::SelectOpenFile(L"script file", fileFilter, L"Select EVSC script", m_hWnd);
	if (!selectedFilePath.empty())
	{
		bool bRet = setupScenario(selectedFilePath.c_str());
		if (bRet)
		{
			m_scriptFilePaths.clear();
			m_nScriptFilePathIndex = 0;
			win_filesystem::GetFilePathListAndIndex(selectedFilePath, fileFilter, m_scriptFilePaths, m_nScriptFilePathIndex);
		}
	}
}
/*次ファイルに移動*/
void CMainWindow::menuOnNextFile()
{
	if (m_scriptFilePaths.empty())return;

	++m_nScriptFilePathIndex;
	if (m_nScriptFilePathIndex >= m_scriptFilePaths.size())m_nScriptFilePathIndex = 0;
	setupScenario(m_scriptFilePaths[m_nScriptFilePathIndex].c_str());
}
/*前ファイルに移動*/
void CMainWindow::menuOnForeFile()
{
	if (m_scriptFilePaths.empty())return;

	--m_nScriptFilePathIndex;
	if (m_nScriptFilePathIndex >= m_scriptFilePaths.size())m_nScriptFilePathIndex = m_scriptFilePaths.size() - 1;
	setupScenario(m_scriptFilePaths[m_nScriptFilePathIndex].c_str());
}
/*音声設定画面呼び出し*/
void CMainWindow::menuOnAudioSetting()
{
	if (m_pAudioPlayer != nullptr)
	{
		CMediaSettingDialogue mediaSettingDialogue;
		mediaSettingDialogue.open(m_hInstance, m_hWnd, m_pAudioPlayer, L"Audio");
	}
}
/*動画設定画面呼び出し*/
void CMainWindow::menuOnVideoSetting()
{
	if (m_pVideoTransferor != nullptr)
	{
		CMediaSettingDialogue mediaSettingDialogue;
		mediaSettingDialogue.open(m_hInstance, m_hWnd, m_pVideoTransferor, L"Video");
	}
}

void CMainWindow::menuOnFontSetting()
{
	if (m_fontSettingDiallogue.getHwnd() == nullptr)
	{
		HWND hWnd = m_fontSettingDiallogue.open(m_hInstance, m_hWnd, L"Font", m_pD2TextWriter);
		::ShowWindow(hWnd, SW_SHOWNORMAL);
	}
	else
	{
		::SetFocus(m_fontSettingDiallogue.getHwnd());
	}
}
/*動画一時停止*/
void CMainWindow::menuOnPauseVideo()
{
	if (m_pVideoTransferor != nullptr)
	{
		bool toBePaused = !m_pVideoTransferor->isPaused();
		bool bRet = m_pVideoTransferor->setPause(toBePaused);
		if (bRet)
		{
			window_menu::SetMenuCheckState(window_menu::GetMenuInBar(m_hWnd, MenuBar::kImage), Menu::kPauseVideo, toBePaused);
		}
	}
}
void CMainWindow::menuOnSyncImage()
{
	bool bRet = window_menu::SetMenuCheckState(window_menu::GetMenuInBar(m_hWnd, MenuBar::kImage), Menu::kSyncImage, !m_isImageSynced);
	if (bRet)
	{
		m_isImageSynced ^= true;
		updatePaintData();
	}
}
/*標題変更*/
void CMainWindow::changeWindowTitle(const wchar_t* pzTitle)
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
void CMainWindow::toggleWindowFrameStyle()
{
	if (!isPlayReady())return;

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
		m_pViewManager->onStyleChanged();
	}
}

void CMainWindow::updateMenuItemState() const
{
	constexpr unsigned int fileMenuIndices[] = { Menu::kNextFile, Menu::kForeFile };
	bool toEnable = isPlayReady();

	window_menu::EnableMenuItems(window_menu::GetMenuInBar(m_hWnd, MenuBar::kFile), fileMenuIndices, toEnable);
}
/*寸劇構築*/
bool CMainWindow::setupScenario(const wchar_t* scriptFilePath)
{
	if (scriptFilePath == nullptr)return false;

	clearScenarioData();

	bool hadBeenReady = isPlayReady();
	bool bRet = legeclo::LoadScenario(scriptFilePath, m_textData, m_paintData, m_sceneData, m_labelData);
	if (!bRet)
	{
		::MessageBoxW(m_hWnd, L"Failed to load scenario.", L"Error", MB_ICONERROR);
	}
	else
	{
		createImageMap();
		updateText();
		updatePaintData();
	}

	changeWindowTitle(bRet ? scriptFilePath : nullptr);
	if(hadBeenReady != bRet) updateMenuItemState();

	return bRet;
}
/*寸劇情報消去*/
void CMainWindow::clearScenarioData()
{
	m_textData.clear();

	m_paintData.clear();
	m_nPaintIndex = 0;
	m_nLastVideoIndex = 0;

	m_sceneData.clear();
	m_nSceneIndex = 0;

	m_labelData.clear();

	clearImageMap();

	m_videoTimer.end();
	clearStoeredVideoFrame();

	m_hasFirstPaintDataBeenLoaded = false;
}
/*再描画要求*/
void CMainWindow::updateScreen() const
{
	::InvalidateRect(m_hWnd, nullptr, FALSE);
}

bool CMainWindow::isPlayReady() const
{
	return !m_textData.empty() && !m_paintData.empty();
}
/*表示図画送り・戻し*/
void CMainWindow::shiftPaintData()
{
	if (!m_isImageSynced)
	{
		++m_nPaintIndex;
		if (m_nPaintIndex >= m_paintData.size())m_nPaintIndex = 0;
	}

	updatePaintData();
}
/*図画データ更新*/
void CMainWindow::updatePaintData()
{
	if (m_nPaintIndex >= m_paintData.size())return;

	const adv::PaintDatum *pPaintDatum = getCurrentPaintData();
	if (pPaintDatum == nullptr)return;

	if (pPaintDatum->isVideo)
	{
		if (m_nLastVideoIndex != m_nPaintIndex)
		{
			clearStoeredVideoFrame();

			if (m_pVideoTransferor != nullptr)
			{
				m_pVideoTransferor->play(pPaintDatum->wstrFilePath.c_str());

				m_videoTimer.start();
			}

			m_nLastVideoIndex = m_nPaintIndex;
		}
	}
	else
	{
		m_nLastVideoIndex = 0;
		clearStoeredVideoFrame();
		m_videoTimer.end();
	}

	updateScreen();
}
/*文章送り・戻し*/
void CMainWindow::shiftScene(bool forward)
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

	updateScene();
}

void CMainWindow::updateScene()
{
	updateText();
	updatePaintData();
}
/*文章更新*/
void CMainWindow::updateText()
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
					m_pAudioPlayer->play(t.wstrVoicePath.c_str());
				}
			}
			constexpr unsigned int kTimerInterval = 2000;
			::SetTimer(m_hWnd, Timer::kText, kTimerInterval, nullptr);
		}
	}
}
/*自動送り*/
void CMainWindow::autoTexting()
{
	if (m_nSceneIndex < m_sceneData.size() - 1)shiftScene(true);
}
/* 現在の図画受け渡し */
const adv::PaintDatum* CMainWindow::getCurrentPaintData()
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
std::wstring CMainWindow::formatCurrentText()
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
void CMainWindow::storeVideoFrame(long long llCurrentTime, CComPtr<ID2D1Bitmap> pD2D1Bitmap)
{
	constexpr int kMaxBufferMilliSeconds = 200;
	if (llCurrentTime < kMaxBufferMilliSeconds)
	{
		m_storedVideoFrames.insert({ llCurrentTime, std::move(pD2D1Bitmap) });
	}
}
/*溜め置き動画消去*/
void CMainWindow::clearStoeredVideoFrame()
{
	m_storedVideoFrames.clear();
}
/*溜め置き動画取り出し*/
ID2D1Bitmap* CMainWindow::restoreVideoFrame(long long llCurrentTime)
{
	const auto& iter = m_storedVideoFrames.find(llCurrentTime);
	if (iter != m_storedVideoFrames.cend())
	{
		return iter->second.p;
	}
	return nullptr;
}
/*静画メモリ取り込み*/
void CMainWindow::createImageMap()
{
	if (m_pD2ImageDrawer == nullptr)return;
	ID2D1DeviceContext* const pD2d1DeviceContext = m_pD2ImageDrawer->getD2DeviceContext();
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
							m_pViewManager->setBaseSize(static_cast<unsigned int>(size.width), static_cast<unsigned int>(size.height));
							m_pViewManager->resetZoom();

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
void CMainWindow::clearImageMap()
{
	m_imageMap.clear();
}
/*IMFMediaEngineNotify::EventNotify*/
void CMainWindow::onAudioPlayerEvent(unsigned long ulEvent, DWORD_PTR param1)
{
	switch (ulEvent)
	{
	case MF_MEDIA_ENGINE_EVENT_LOADEDMETADATA:

		break;
	case MF_MEDIA_ENGINE_EVENT_ENDED:
		autoTexting();
		break;
	default:
		break;
	}
}

void CMainWindow::onVideoPlayerEvent(unsigned long ulEvent, DWORD_PTR param1)
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
				bool bRet = m_pVideoTransferor->getVideoSize(&ulWidth, &ulHeight);
				if (bRet)
				{
					if (m_pViewManager != nullptr)
					{
						m_pViewManager->setBaseSize(ulWidth, ulHeight);
						m_pViewManager->resetZoom();
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