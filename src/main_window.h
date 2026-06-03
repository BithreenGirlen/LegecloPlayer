#ifndef MAIN_WINDOW_H_
#define MAIN_WINDOW_H_

#include <Windows.h>
#include <atlbase.h>

#include <string>
#include <vector>
#include <map>

#include "d2_image_drawer.h"
#include "d2_text_writer.h"
#include "mf_media_player.h"
#include "mf_video_transferor.h"
#include "view_manager.h"
#include "adv.h"
#include "win_timer.h"

#include "native-ui/font_setting_dialogue.h"

class CMainWindow
{
public:
	CMainWindow();
	~CMainWindow();

	bool create(HINSTANCE hInstance);
	int messageLoop();

	HWND GetHwnd()const { return m_hWnd;}
private:
	const wchar_t* m_className = L"Legeclo player window";
	const wchar_t* m_defaultWindowName = L"Legeclo player";
	HINSTANCE m_hInstance = nullptr;
	HWND m_hWnd = nullptr;

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT handleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT onCreate(HWND hWnd);
	LRESULT onDestroy();
	LRESULT onClose();
	LRESULT onPaint();
	LRESULT onSize();
	LRESULT onKeyDown(WPARAM wParam, LPARAM lParam);
	LRESULT onKeyUp(WPARAM wParam, LPARAM lParam);
	LRESULT onCommand(WPARAM wParam, LPARAM lParam);
	LRESULT onTimer(WPARAM wParam);
	LRESULT onMouseMove(WPARAM wParam, LPARAM lParam);
	LRESULT onMouseWheel(WPARAM wParam, LPARAM lParam);
	LRESULT onLButtonDown(WPARAM wParam, LPARAM lParam);
	LRESULT onLButtonUp(WPARAM wParam, LPARAM lParam);
	LRESULT onRButtonUp(WPARAM wParam, LPARAM lParam);
	LRESULT onMButtonUp(WPARAM wParam, LPARAM lParam);

	struct Menu
	{
		enum
		{
			kOpenFile = 1, kNextFile, kForeFile,
			kAudioSetting, kVideoSetting, kFontSetting,
			kPauseVideo, kTickVideo, kSyncImage
		};
	};
	struct MenuBar { enum { kFile, kSetting, kImage }; };
	struct EventMessage { enum { kAudioPlayer = WM_USER + 1, kVideoPlayer }; };
	struct Timer { enum { kText = 1}; };

	POINT m_lastCursorPos{};
	bool m_wasLeftPressed = false;
	bool m_hasLeftBeenDragged = false;
	bool m_wasRightCombined = false;

	HMENU m_hMenuBar = nullptr;
	bool m_isFramelessWindow = false;

	std::vector<std::wstring> m_scriptFilePaths;
	size_t m_nScriptFilePathIndex = 0;

	void initialiseMenuBar();

	void menuOnOpenFile();
	void menuOnNextFile();
	void menuOnForeFile();

	void menuOnAudioSetting();
	void menuOnVideoSetting();
	void menuOnFontSetting();

	void menuOnPauseVideo();
	void menuOnSyncImage();

	void changeWindowTitle(const wchar_t* pzTitle);
	void toggleWindowFrameStyle();
	void updateMenuItemState() const;

	bool setupScenario(const wchar_t* scriptFilePath);
	void clearScenarioData();

	void updateScreen() const;

	CD2ImageDrawer* m_pD2ImageDrawer = nullptr;
	CD2TextWriter* m_pD2TextWriter = nullptr;
	CMfMediaPlayer* m_pAudioPlayer = nullptr;
	CMfVideoTransferor* m_pVideoTransferor = nullptr;
	CViewManager* m_pViewManager = nullptr;
	CFontSettingDialogue m_fontSettingDiallogue;

	std::vector<adv::TextDatum> m_textData;

	std::vector<adv::PaintDatum> m_paintData;
	size_t m_nPaintIndex = 0;
	size_t m_nLastVideoIndex = 0;

	std::vector<adv::SceneDatum> m_sceneData;
	size_t m_nSceneIndex = 0;

	std::vector<adv::LabelDatum> m_labelData;

	bool m_hasFirstPaintDataBeenLoaded = false;
	bool m_isTextHidden = false;
	bool m_isImageSynced = true;

	bool isPlayReady() const;

	void shiftPaintData();
	void updatePaintData();

	void shiftScene(bool forward);
	void updateScene();

	void updateText();
	void autoTexting();

	const adv::PaintDatum* getCurrentPaintData();
	std::wstring formatCurrentText();

	std::map<long long, CComPtr<ID2D1Bitmap>> m_storedVideoFrames;
	void storeVideoFrame(long long llCurrentTime, CComPtr<ID2D1Bitmap> pD2D1Bitmap);
	void clearStoeredVideoFrame();
	ID2D1Bitmap* restoreVideoFrame(long long llCurrentTime);

	std::map<std::wstring, CComPtr<ID2D1Bitmap>> m_imageMap;
	void createImageMap();
	void clearImageMap();

	void onAudioPlayerEvent(unsigned long ulEvent, DWORD_PTR param1);
	void onVideoPlayerEvent(unsigned long ulEvent, DWORD_PTR param1);

	CWinTimer m_videoTimer;
};

#endif //MAIN_WINDOW_H_