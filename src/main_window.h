#ifndef MAIN_WINDOW_H_
#define MAIN_WINDOW_H_

#include <Windows.h>
#include <atlbase.h>

#include <string>
#include <vector>
#include <unordered_map>

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

	bool Create(HINSTANCE hInstance);
	int MessageLoop();

	HWND GetHwnd()const { return m_hWnd;}
private:
	const wchar_t* m_className = L"Legeclo player window";
	const wchar_t* m_defaultWindowName = L"Legeclo player";
	HINSTANCE m_hInstance = nullptr;
	HWND m_hWnd = nullptr;

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnCreate(HWND hWnd);
	LRESULT OnDestroy();
	LRESULT OnClose();
	LRESULT OnPaint();
	LRESULT OnSize();
	LRESULT OnKeyDown(WPARAM wParam, LPARAM lParam);
	LRESULT OnKeyUp(WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
	LRESULT OnTimer(WPARAM wParam);
	LRESULT OnMouseMove(WPARAM wParam, LPARAM lParam);
	LRESULT OnMouseWheel(WPARAM wParam, LPARAM lParam);
	LRESULT OnLButtonDown(WPARAM wParam, LPARAM lParam);
	LRESULT OnLButtonUp(WPARAM wParam, LPARAM lParam);
	LRESULT OnRButtonUp(WPARAM wParam, LPARAM lParam);
	LRESULT OnMButtonUp(WPARAM wParam, LPARAM lParam);

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
	bool m_wasRightCombinated = false;

	HMENU m_hMenuBar = nullptr;
	bool m_isFramelessWindow = false;

	std::vector<std::wstring> m_scriptFilePaths;
	size_t m_nScriptFilePathIndex = 0;

	void InitialiseMenuBar();

	void MenuOnOpenFile();
	void MenuOnNextFile();
	void MenuOnForeFile();

	void MenuOnAudioSetting();
	void MenuOnVideoSetting();
	void MenuOnFontSetting();

	void MenuOnPauseVideo();
	void MenuOnSyncImage();

	void ChangeWindowTitle(const wchar_t* pzTitle);
	void ToggleWindowFrameStyle();
	void UpdateMenuItemState() const;

	bool SetupScenario(const wchar_t* scriptFilePath);
	void ClearScenarioData();

	void UpdateScreen() const;

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

	bool IsPlayReady() const;

	void ShiftPaintData();
	void UpdatePaintData();

	void ShiftScene(bool forward);
	void UpdateScene();

	void UpdateText();
	void AutoTexting();

	const adv::PaintDatum* GetCurrentPaintData();
	std::wstring FormatCurrentText();

	std::unordered_map<long long, CComPtr<ID2D1Bitmap>> m_storedVideoFrames;
	void StoreVideoFrame(long long llCurrentTime, CComPtr<ID2D1Bitmap> pD2D1Bitmap);
	void ClearStoeredVideoFrame();
	ID2D1Bitmap* RestoreVideoFrame(long long llCurrentTime);

	std::unordered_map<std::wstring, CComPtr<ID2D1Bitmap>> m_imageMap;
	void CreateImageMap();
	void ClearImageMap();

	void OnAudioPlayerEvent(unsigned long ulEvent, DWORD_PTR param1);
	void OnVideoPlayerEvent(unsigned long ulEvent, DWORD_PTR param1);

	CWinTimer m_videoTimer;
};

#endif //MAIN_WINDOW_H_