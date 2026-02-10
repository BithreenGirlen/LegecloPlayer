#ifndef MF_VIDEO_TRANSFEROR_H_
#define MF_VIDEO_TRANSFEROR_H_

/*
* MF Media Engine in frame-server mode.
* 
* For fundamental features, rendering mode is easier to use.
* But in rendering mode, the video frame is always drawn on topmost of the target window that
* it is impossible to draw text on video frame.
* Thus, frame-server mode is used to controll drawring order.
* 
* Transfer to DXGI surface seems not to be useful because it is basically the same as rendering mode.
* 
*/

#include <wincodec.h>
#include <d2d1_1.h>

#include "mf_media_player.h"

class CMfVideoTransferor : public CMfMediaPlayer
{
public:
	CMfVideoTransferor();
	~CMfVideoTransferor();

	struct SVideoFrame
	{
		int iWidth = 0;
		int iHeight = 0;
		unsigned int uiStride = 0;
		unsigned char* pPixels = nullptr; /* should be freed on user's side. */
		size_t nPixelSize = 0;
	};
	/// @brief Transfer as CPU image
	bool TransferVideoFrame(SVideoFrame* pVideoFrame, long long *currentFrameTime);
	/// @brief Transfer as GPU resource
	bool TransferVideoFrame(ID2D1DeviceContext* const pD2d1DeviceContext, ID2D1Bitmap** pD2d1Bitmap, long long *currentFrameTime);

	bool SetPlaybackWindow(HWND hWnd, UINT uMsg) override;
	bool ResizeBuffer() override;
private:
	IWICBitmap *m_pWicBitmap = nullptr;

	void ReleaseWicBitmap();
	bool CreateWicBitmap(unsigned long uiWidth, unsigned long uiHeight);
	bool CheckWicBitmapSize(unsigned long uiWidth, unsigned long uiHeight);

	bool TransferVideoFrameToWicBitmap();
};

#endif // !MF_VIDEO_TRANSFEROR_H_
