
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
	case WM_SIZE:
		return onSize(wParam, lParam);
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
	window_menu::SetMenuCheckState(window_menu::GetMenuInBar(m_hWnd, MenuBar::kImage), Menu::kSyncImage, m_sceneState.isImageSynced);

	static constexpr wchar_t s_defualtFontFilePath[] = L"C:\\Windows\\Fonts\\yumindb.ttf";

	static const auto TimerCallback = [](void* pUserData)
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

	m_pSceneTextWriter = new CD2TextWriter(m_pD2ImageDrawer->getD2Factory(), m_pD2ImageDrawer->getD2DeviceContext());
	m_pSceneTextWriter->setupOutLinedDrawing(s_defualtFontFilePath);
	m_pSceneTextWriter->onDpiChanged(::GetDpiForWindow(m_hWnd));

	m_audioPlayer.setPlaybackWindow(m_hWnd, EventMessage::kAudioPlayer);

	m_videoTransferor.setPlaybackWindow(m_hWnd, EventMessage::kVideoPlayer);
	m_videoTransferor.setLoop(true);

	m_pHelpTextWriter = new CD2TextWriter(m_pD2ImageDrawer->getD2Factory(), m_pD2ImageDrawer->getD2DeviceContext());
	m_pHelpTextWriter->setupOutLinedDrawing(s_defualtFontFilePath, false, false, m_pSceneTextWriter->getFontSize() / 2.f);
	m_pHelpTextWriter->onDpiChanged(::GetDpiForWindow(m_hWnd));
	recreateHelpTextBitmap();

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

	if (m_pHelpTextWriter != nullptr)
	{
		delete m_pHelpTextWriter;
		m_pHelpTextWriter = nullptr;
	}

	if (m_pSceneTextWriter != nullptr)
	{
		delete m_pSceneTextWriter;
		m_pSceneTextWriter = nullptr;
	}

	if (m_pD2ImageDrawer != nullptr)
	{
		delete m_pD2ImageDrawer;
		m_pD2ImageDrawer = nullptr;
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

	if (m_pD2ImageDrawer == nullptr || m_nPaintIndex >= m_paintData.size())
	{
		::EndPaint(m_hWnd, &ps);
		return 0;
	}

	m_pD2ImageDrawer->clear();

	const adv::PaintDatum* pPaintDatum = getCurrentPaintData();
	if (pPaintDatum != nullptr)
	{
		/* 動画フレームを取得できなかった場合、そのフレームの表示を行わない。 */
		bool hasDrawn = false;
		if (pPaintDatum->isVideo) /* 動画 */
		{
			CComPtr<ID2D1Bitmap> pVideoFrame = getCurrentVideoFrame();
			if (pVideoFrame != nullptr)
			{
				const D2D1_SIZE_U sceneSize = pVideoFrame->GetPixelSize();
				const D2D1_MATRIX_3X2_F transformMatrix = calculateTransformMatrix(sceneSize);

				m_pD2ImageDrawer->getD2DeviceContext()->SetTransform(transformMatrix);
				hasDrawn = m_pD2ImageDrawer->draw(pVideoFrame);
				m_pD2ImageDrawer->getD2DeviceContext()->SetTransform(D2D1::Matrix3x2F::Identity());
			}
		}
		else /* 静止画 */
		{
			const auto& iter = m_imageMap.find(pPaintDatum->wstrFilePath);
			if (iter != m_imageMap.cend())
			{
				const D2D1_SIZE_U sceneSize = iter->second->GetPixelSize();
				const D2D1_MATRIX_3X2_F transformMatrix = calculateTransformMatrix(sceneSize);

				m_pD2ImageDrawer->getD2DeviceContext()->SetTransform(transformMatrix);
				hasDrawn = m_pD2ImageDrawer->draw(iter->second);
				m_pD2ImageDrawer->getD2DeviceContext()->SetTransform(D2D1::Matrix3x2F::Identity());
			}
		}

		if (hasDrawn)
		{
			if (!m_sceneState.isHelpTextHidden)
			{
				if (m_pHelpTextBitmap != nullptr)
				{
					/* 
					 * This results in retrieving window size twice because
					 * calculateTransformMatrix has already called it.
					 */
					RECT rc;
					::GetClientRect(m_hWnd, &rc);

					int targetWidth = rc.right - rc.left;
					int targetHeight = rc.bottom - rc.top;

					D2D1_SIZE_U helpTextSize = m_pHelpTextBitmap->GetPixelSize();
					D2D_POINT_2F textOffset{ 0, static_cast<float>(targetHeight - helpTextSize.height) };
					m_pD2ImageDrawer->draw(m_pHelpTextBitmap, &textOffset);
				}
			}

			if (!m_sceneState.isTextHidden)
			{
				if (m_pSceneTextBitmap != nullptr)
				{
					m_pD2ImageDrawer->draw(m_pSceneTextBitmap);
				}
			}

			m_pD2ImageDrawer->display();
		}
	}

	::EndPaint(m_hWnd, &ps);

	return 0;
}
/*WM_SIZE*/
LRESULT CMainWindow::onSize(WPARAM wParam, LPARAM lParam)
{
	if (m_pD2ImageDrawer != nullptr)
	{
		m_pD2ImageDrawer->onResize();
	}

	recreateSceneTextBitmap();

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
		if (m_pSceneTextWriter != nullptr)
		{
			m_pSceneTextWriter->toggleTextColour();
			recreateSceneTextBitmap();
		}

		if (m_pHelpTextWriter != nullptr)
		{
			m_pHelpTextWriter->toggleTextColour();
			recreateHelpTextBitmap();
		}

		updateScreen();
		break;
	case 'H':
		m_sceneState.isHelpTextHidden ^= true;
		updateScreen();
		break;
	case 'T':
		m_sceneState.isTextHidden ^= true;
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
		if (m_audioPlayer.isEnded())
		{
			autoTexting();
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
	WORD pressedKeyState = LOWORD(wParam);
	if (pressedKeyState == MK_LBUTTON)
	{
		POINT pt{};
		::GetCursorPos(&pt);

		if (m_mouseState.hasLeftBeenDragged)
		{
			int iX = m_mouseState.lastCursorPos.x - pt.x;
			int iY = m_mouseState.lastCursorPos.y - pt.y;

			m_viewManager.addOffset(iX, iY);
		}

		m_mouseState.lastCursorPos = pt;
		m_mouseState.hasLeftBeenDragged = true;
	}

	return 0;
}
/*WM_MOUSEWHEEL*/
LRESULT CMainWindow::onMouseWheel(WPARAM wParam, LPARAM lParam)
{
	short usDelta = static_cast<short>(HIWORD(wParam));
	int iScroll = -usDelta / WHEEL_DELTA;

	WORD pressedKeyState = LOWORD(wParam);
	if (pressedKeyState == MK_LBUTTON)
	{

	}
	else if (pressedKeyState == MK_RBUTTON)
	{
		shiftScene(iScroll > 0);

		m_mouseState.wasRightCombined = true;
	}
	else
	{
		m_viewManager.rescale(iScroll > 0);
	}

	return 0;
}
/*WM_LBUTTONDOWN*/
LRESULT CMainWindow::onLButtonDown(WPARAM wParam, LPARAM lParam)
{
	::GetCursorPos(&m_mouseState.lastCursorPos);

	m_mouseState.wasLeftPressed = true;

	return 0;
}
/*WM_LBUTTONUP*/
LRESULT CMainWindow::onLButtonUp(WPARAM wParam, LPARAM lParam)
{
	if (m_mouseState.hasLeftBeenDragged)
	{
		m_mouseState.hasLeftBeenDragged = false;
		m_mouseState.wasLeftPressed = false;

		return 0;
	}

	WORD pressedKeyState = LOWORD(wParam);
	if (pressedKeyState == MK_RBUTTON)
	{
		if (m_windowStyle.isFrameless)
		{
			::PostMessage(m_hWnd, WM_SYSCOMMAND, SC_MOVE, 0);
			INPUT input{};
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = VK_DOWN;
			::SendInput(1, &input, sizeof(input));

			m_mouseState.wasRightCombined = true;
		}
	}
	else if (pressedKeyState == 0)
	{
		if (m_mouseState.wasLeftPressed)
		{
			POINT pt{};
			::GetCursorPos(&pt);
			int iX = m_mouseState.lastCursorPos.x - pt.x;
			int iY = m_mouseState.lastCursorPos.y - pt.y;

			if (iX == 0 && iY == 0)
			{
				if (m_videoTransferor.isPaused())
				{
					m_videoTransferor.frameStep(true);
				}
				else
				{
					shiftPaintData();
				}
			}
		}

	}

	m_mouseState.wasLeftPressed = false;

	return 0;
}
/*WM_RBUTTONUP*/
LRESULT CMainWindow::onRButtonUp(WPARAM wParam, LPARAM lParam)
{
	if (m_mouseState.wasRightCombined)
	{
		m_mouseState.wasRightCombined = false;

		return 0;
	}

	WORD pressedKeyState = LOWORD(wParam);
	if (pressedKeyState == 0)
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
	WORD pressedKeyState = LOWORD(wParam);
	if (pressedKeyState == 0)
	{
		m_viewManager.resetScale();
	}
	else if (pressedKeyState == MK_RBUTTON)
	{
		toggleWindowFrameStyle();

		m_mouseState.wasRightCombined = true;
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
	std::wstring selectedFilePath = win_dialogue::SelectOpenFile(L"script file", fileFilter, L"Select evsc file under gamedata/adv/scenario fodler", m_hWnd);
	if (!selectedFilePath.empty())
	{
		bool bRet = setupScenario(selectedFilePath);
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
	setupScenario(m_scriptFilePaths[m_nScriptFilePathIndex]);
}
/*前ファイルに移動*/
void CMainWindow::menuOnForeFile()
{
	if (m_scriptFilePaths.empty())return;

	--m_nScriptFilePathIndex;
	if (m_nScriptFilePathIndex >= m_scriptFilePaths.size())m_nScriptFilePathIndex = m_scriptFilePaths.size() - 1;
	setupScenario(m_scriptFilePaths[m_nScriptFilePathIndex]);
}
/*音声設定画面呼び出し*/
void CMainWindow::menuOnAudioSetting()
{
	CMediaSettingDialogue mediaSettingDialogue;
	mediaSettingDialogue.open(m_hInstance, m_hWnd, &m_audioPlayer, L"Audio");
}
/*動画設定画面呼び出し*/
void CMainWindow::menuOnVideoSetting()
{
	CMediaSettingDialogue mediaSettingDialogue;
	mediaSettingDialogue.open(m_hInstance, m_hWnd, &m_videoTransferor, L"Video");
}

void CMainWindow::menuOnFontSetting()
{
	if (m_fontSettingDiallogue.getHwnd() == nullptr)
	{
		static const auto FontChangeCallback = [](void* pUserDatum, CFontSettingDialogue::FontCallbackDatum* pFontCallbackDatum)
			-> void
			{
				CMainWindow* pThis = static_cast<CMainWindow*>(pUserDatum);
				if (pThis != nullptr)
				{
					pThis->m_pHelpTextWriter->setupOutLinedDrawing(pFontCallbackDatum->fontFilePath, false, false, pFontCallbackDatum->fontSize / 2.f);

					pThis->recreateSceneTextBitmap();
					pThis->recreateHelpTextBitmap();
					pThis->updateScreen();
				}
			};

		HWND hWnd = m_fontSettingDiallogue.open(m_hInstance, m_hWnd, L"Font", m_pSceneTextWriter, FontChangeCallback, this);
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
	bool toBePaused = !m_videoTransferor.isPaused();
	bool bRet = m_videoTransferor.setPause(toBePaused);
	if (bRet)
	{
		window_menu::SetMenuCheckState(window_menu::GetMenuInBar(m_hWnd, MenuBar::kImage), Menu::kPauseVideo, toBePaused);
	}
}
void CMainWindow::menuOnSyncImage()
{
	bool bRet = window_menu::SetMenuCheckState(window_menu::GetMenuInBar(m_hWnd, MenuBar::kImage), Menu::kSyncImage, !m_sceneState.isImageSynced);
	if (bRet)
	{
		m_sceneState.isImageSynced ^= true;
		updatePaintData();
	}
}
/*標題変更*/
void CMainWindow::changeWindowTitle(const wchar_t* windowTitle)
{
	const wchar_t* truncatedWindowTitle = windowTitle;
	if (truncatedWindowTitle != nullptr)
	{
		for (;;)
		{
			const wchar_t* pPos = wcspbrk(truncatedWindowTitle, L"\\/");
			if (pPos == nullptr)break;
			truncatedWindowTitle = pPos + 1;
		}
	}

	::SetWindowTextW(m_hWnd, truncatedWindowTitle == nullptr ? m_defaultWindowName : truncatedWindowTitle);
}
/*表示形式変更*/
void CMainWindow::toggleWindowFrameStyle()
{
	if (!isPlayReady())return;

	LONG lStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);

	m_windowStyle.isFrameless ^= true;

	if (m_windowStyle.isFrameless)
	{
		MONITORINFO monitorInfo{.cbSize = sizeof(MONITORINFO) };
		if (HMONITOR hMonitor = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST); hMonitor != nullptr)
		{
			/* Nothing can be done even if failed. */
			::GetMonitorInfoW(hMonitor, &monitorInfo);
		}

		RECT rect;
		::GetWindowRect(m_hWnd, &rect);

		::SetWindowLong(m_hWnd, GWL_STYLE, lStyle & ~WS_CAPTION & ~WS_SYSMENU);
		/* If GetMonitorInfoW failed, rcMonitor.left and rcMonitor.top remains zero, namely the origin of primary monitor. */
		::SetWindowPos(m_hWnd, nullptr, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);
		::SetMenu(m_hWnd, nullptr);
	}
	else
	{
		::SetWindowLong(m_hWnd, GWL_STYLE, lStyle | WS_CAPTION | WS_SYSMENU);
		::SetMenu(m_hWnd, m_hMenuBar);
	}

	m_viewManager.onStyleChanged();
}

void CMainWindow::updateMenuItemState() const
{
	constexpr unsigned int fileMenuIndices[] = { Menu::kNextFile, Menu::kForeFile };
	bool toEnable = isPlayReady();

	window_menu::EnableMenuItems(window_menu::GetMenuInBar(m_hWnd, MenuBar::kFile), fileMenuIndices, toEnable);
}
/*寸劇構築*/
bool CMainWindow::setupScenario(const std::wstring& scriptFilePath)
{
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

	changeWindowTitle(bRet ? scriptFilePath.data() : nullptr);
	if (hadBeenReady != bRet) updateMenuItemState();

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

	m_sceneState.hasFirstPaintDataBeenLoaded = false;

	m_formattedText.clear();
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
	if (!m_sceneState.isImageSynced)
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

	const adv::PaintDatum* pPaintDatum = getCurrentPaintData();
	if (pPaintDatum == nullptr)return;

	if (pPaintDatum->isVideo)
	{
		if (m_nLastVideoIndex != m_nPaintIndex)
		{
			clearStoeredVideoFrame();

			m_videoTransferor.play(pPaintDatum->wstrFilePath.c_str());

			m_videoTimer.start();

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

			m_formattedText.assign(t.wstrText);
			if (!m_formattedText.empty() && m_formattedText.back() != L'\n')
			{
				m_formattedText.push_back(L'\n');
			}

			wchar_t buffer[64]{};
			::swprintf_s(buffer, L"%zu/%zu", nTextIndex + 1, m_textData.size());
			m_formattedText += buffer;

			recreateSceneTextBitmap();

			if (!t.wstrVoicePath.empty())
			{
				m_audioPlayer.play(t.wstrVoicePath.c_str());
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
		if (m_sceneState.isImageSynced)
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
/* 変形行列計算 */
D2D1_MATRIX_3X2_F CMainWindow::calculateTransformMatrix(const D2D1_SIZE_U& sceneSize)
{
	RECT rc;
	::GetClientRect(m_hWnd, &rc);

	int targetWidth = rc.right - rc.left;
	int targetHeight = rc.bottom - rc.top;

	const float fScale = m_viewManager.getScale();
	const float fX = (sceneSize.width * fScale - targetWidth) / 2 + m_viewManager.offsetX() / 2;
	const float fY = (sceneSize.height * fScale - targetHeight) / 2 + m_viewManager.offsetY() / 2;

	const D2D1_MATRIX_3X2_F scaleMatrix = D2D1::Matrix3x2F::Scale(fScale, fScale);
	const D2D1_MATRIX_3X2_F translateMatrix = D2D1::Matrix3x2F::Translation(-fX, -fY);
	const D2D1_MATRIX_3X2_F transformMatrix = scaleMatrix * translateMatrix;

	return transformMatrix;
}

CComPtr<ID2D1Bitmap> CMainWindow::getCurrentVideoFrame()
{
	/* 最大バッファ時間までの取得フレームは保存しておき、ループ再生時にフレーム待ちが発生しないにしておく */

	CComPtr<ID2D1Bitmap> pD2d1Bitmap;
	long long frameTime = 0;
	bool bRet = m_videoTransferor.transferVideoFrame(m_pD2ImageDrawer->getD2DeviceContext(), &pD2d1Bitmap, &frameTime);
	if (bRet)
	{
		static constexpr long long kMaxBufferMilliSeconds = 200;
		if (frameTime < kMaxBufferMilliSeconds)
		{
			const auto& storedFramePair = m_storedVideoFrames.insert({ frameTime, std::move(pD2d1Bitmap) });
			return storedFramePair.first->second;
		}
	}
	else
	{
		frameTime = m_videoTransferor.getCurrentTimeInMilliSeconds();
		const auto& storedFrame = m_storedVideoFrames.find(frameTime);
		if (storedFrame != m_storedVideoFrames.cend())
		{
			return storedFrame->second;
		}
	}

	return pD2d1Bitmap;
}
/* 溜め置き動画消去 */
void CMainWindow::clearStoeredVideoFrame()
{
	m_storedVideoFrames.clear();
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
						if (!m_sceneState.hasFirstPaintDataBeenLoaded)
						{
							const D2D1_SIZE_F& size = pD2d1Bitmap->GetSize();
							m_viewManager.setBaseSize(m_hWnd, static_cast<unsigned int>(size.width), static_cast<unsigned int>(size.height));
							m_viewManager.resetScale();

							m_sceneState.hasFirstPaintDataBeenLoaded = true;
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

void CMainWindow::recreateSceneTextBitmap()
{
	if (!isPlayReady())return;

	RECT rc;
	::GetClientRect(m_hWnd, &rc);
	const float wrapWidth = static_cast<float>(rc.right - rc.left);

	m_pSceneTextBitmap.Release();
	drawTextOnBitmap(m_pSceneTextWriter, m_formattedText.c_str(), m_formattedText.size(), &m_pSceneTextBitmap, wrapWidth);
}

void CMainWindow::recreateHelpTextBitmap()
{
	static constexpr wchar_t s_helpText[] =
	{
		L"[H] Hide/show help\n"
		L"[T] Hide/show scene text\n"
		L"[C] Toggle text colour\n"
		L"[Wheel] Scale up/down\n"
		L"[L-drag] Move view-point\n"
		L"[M-click] Reset scale and view-point\n"
		L"[R-click] Show context menu to jump scene\n"
		L"[R-pressed + M-click] Hide/show the border of window\n"
		L"[R-pressed + L-click] Move borderless window\n"
		L"[←|→; R-pressed + wheel] Rewind/fast-forward the scene text\n"
		L"[↑|↓] Open the previous/next folder\n"
	};
	static constexpr size_t helpTextLength = sizeof(s_helpText) / sizeof(wchar_t) - 1;

	m_pHelpTextBitmap.Release();
	drawTextOnBitmap(m_pHelpTextWriter, s_helpText, helpTextLength, &m_pHelpTextBitmap);
}

void CMainWindow::drawTextOnBitmap(CD2TextWriter* pTextWriter, const wchar_t* text, size_t textLength, ID2D1Bitmap1** targetBitmap, float wrapWidth)
{
	if (m_pD2ImageDrawer == nullptr || pTextWriter == nullptr || targetBitmap == nullptr)return;

	D2D1_SIZE_F textBounds = pTextWriter->calculateTextBounds(text, textLength, wrapWidth);
	const D2D_POINT_2F textOffset = { pTextWriter->getThickness(), pTextWriter->getThickness() };
	textBounds.width += textOffset.x;
	textBounds.height += textOffset.y;
	const D2D1_SIZE_U bitmapSize{ static_cast<UINT>(textBounds.width), static_cast<UINT>(textBounds.height) };

	/* Do not specify DPI here. */
	const D2D1_BITMAP_PROPERTIES1 bitmapProperties1 = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS::D2D1_BITMAP_OPTIONS_TARGET,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

	HRESULT hr = m_pD2ImageDrawer->getD2DeviceContext()->CreateBitmap(bitmapSize, nullptr, 0, bitmapProperties1, targetBitmap);
	if (SUCCEEDED(hr))
	{
		CComPtr<ID2D1Image> pPreviousRendererTarget;
		m_pD2ImageDrawer->getD2DeviceContext()->GetTarget(&pPreviousRendererTarget);

		m_pD2ImageDrawer->getD2DeviceContext()->SetTarget(*targetBitmap);
		pTextWriter->outLinedDraw(text, textLength, wrapWidth, textOffset);
		m_pD2ImageDrawer->getD2DeviceContext()->SetTarget(pPreviousRendererTarget);
	}
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
		if (!m_sceneState.hasFirstPaintDataBeenLoaded)
		{
			unsigned long ulWidth = 0;
			unsigned long ulHeight = 0;
			bool bRet = m_videoTransferor.getVideoSize(&ulWidth, &ulHeight);
			if (bRet)
			{
				m_viewManager.setBaseSize(m_hWnd, ulWidth, ulHeight);
				m_viewManager.resetScale();

				m_sceneState.hasFirstPaintDataBeenLoaded = true;
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