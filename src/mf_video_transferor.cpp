
#include <atlbase.h>
#include <dxgiformat.h>

#include "mf_video_transferor.h"

#pragma comment (lib,"Windowscodecs.lib")

CMfVideoTransferor::CMfVideoTransferor()
{

}

CMfVideoTransferor::~CMfVideoTransferor()
{
	ReleaseWicBitmap();
}
/*転送*/
bool CMfVideoTransferor::TransferVideoFrame(SVideoFrame* pVideoFrame, long long* currentFrameTime)
{
	if (pVideoFrame == nullptr)return false;

	if (TransferVideoFrameToWicBitmap())
	{
		if (currentFrameTime != nullptr)
		{
			*currentFrameTime = GetCurrentTimeInMilliSeconds();
		}

		unsigned int uiWidth = 0;
		unsigned int uiHeight = 0;
		HRESULT hr = m_pWicBitmap->GetSize(&uiWidth, &uiHeight);
		if (FAILED(hr))return false;

		pVideoFrame->iWidth = uiWidth;
		pVideoFrame->iHeight = uiHeight;

		CComPtr<IWICBitmapLock> pWicBitmapLock;
		WICRect wicRect{ 0, 0, pVideoFrame->iWidth, pVideoFrame->iHeight };
		hr = m_pWicBitmap->Lock(&wicRect, WICBitmapLockFlags::WICBitmapLockRead, &pWicBitmapLock);
		if (FAILED(hr))return false;

		hr = pWicBitmapLock->GetStride(&pVideoFrame->uiStride);
		if (FAILED(hr))return false;

		pVideoFrame->nPixelSize = static_cast<size_t>(pVideoFrame->uiStride * pVideoFrame->iHeight);
		pVideoFrame->pPixels = static_cast<unsigned char*>(malloc(pVideoFrame->nPixelSize));
		if (pVideoFrame->pPixels == nullptr)return false;

		hr = m_pWicBitmap->CopyPixels(nullptr, pVideoFrame->uiStride, static_cast<UINT>(pVideoFrame->nPixelSize), pVideoFrame->pPixels);
		if (FAILED(hr))
		{
			free(pVideoFrame->pPixels);
			return false;
		}

		return SUCCEEDED(hr);
	}

	return false;
}

bool CMfVideoTransferor::TransferVideoFrame(ID2D1DeviceContext* const pD2d1DeviceContext, ID2D1Bitmap** pD2d1Bitmap, long long* currentFrameTime)
{
	if (pD2d1DeviceContext == nullptr)return false;

	if (TransferVideoFrameToWicBitmap())
	{
		if (currentFrameTime != nullptr)
		{
			*currentFrameTime = GetCurrentTimeInMilliSeconds();
		}

		HRESULT hr = pD2d1DeviceContext->CreateBitmapFromWicBitmap(m_pWicBitmap, D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)), pD2d1Bitmap);

		return SUCCEEDED(hr);
	}

	return false;
}

bool CMfVideoTransferor::SetPlaybackWindow(HWND hWnd, UINT uMsg)
{
	m_hRetWnd = hWnd;
	m_uRetMsg = uMsg;

	HRESULT hr = m_pMfAttributes->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM);
	return SUCCEEDED(hr);
}

bool CMfVideoTransferor::ResizeBuffer()
{
	/*rendering mode only*/
	return false;
}

void CMfVideoTransferor::ReleaseWicBitmap()
{
	if (m_pWicBitmap != nullptr)
	{
		m_pWicBitmap->Release();
		m_pWicBitmap = nullptr;
	}
}

bool CMfVideoTransferor::CreateWicBitmap(unsigned long uiWidth, unsigned long uiHeight)
{
	CComPtr<IWICImagingFactory> pWicImageFactory;
	HRESULT hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX::CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pWicImageFactory));
	if (FAILED(hr))return false;

	ReleaseWicBitmap();

	hr = pWicImageFactory->CreateBitmap(uiWidth, uiHeight, GUID_WICPixelFormat32bppPBGRA, WICBitmapCreateCacheOption::WICBitmapCacheOnDemand, &m_pWicBitmap);

	return SUCCEEDED(hr);
}

bool CMfVideoTransferor::CheckWicBitmapSize(unsigned long uiWidth, unsigned long uiHeight)
{
	if (m_pWicBitmap == nullptr)
	{
		return CreateWicBitmap(uiWidth, uiHeight);
	}
	else
	{
		unsigned int uiCurrentWidth = 0;
		unsigned int uiCUrrenHeight = 0;
		m_pWicBitmap->GetSize(&uiCurrentWidth, &uiCUrrenHeight);
		if (uiWidth > uiCurrentWidth && uiHeight > uiCUrrenHeight)
		{
			return CreateWicBitmap(uiWidth, uiHeight);
		}
		else
		{
			return true;
		}
	}
	return false;
}

bool CMfVideoTransferor::TransferVideoFrameToWicBitmap()
{
	if (m_pMfEngineEx != nullptr)
	{
		BOOL iRet = m_pMfEngineEx->HasVideo();
		if (iRet)
		{
			long long llReadyFrame = 0;
			HRESULT hr = m_pMfEngineEx->OnVideoStreamTick(&llReadyFrame);
			if (SUCCEEDED(hr) && llReadyFrame >= 0)
			{
				unsigned long ulDestWidth = 0;
				unsigned long ulDestHeight = 0;
				bool bRet = GetVideoSize(&ulDestWidth, &ulDestHeight);
				if (!bRet)return false;

				bRet = CheckWicBitmapSize(ulDestWidth, ulDestHeight);
				if (!bRet)return false;

				RECT dstRect{ 0, 0, static_cast<LONG>(ulDestWidth), static_cast<LONG>(ulDestHeight) };
				MFARGB bg{ 0, 0, 0, 0 };
				MFVideoNormalizedRect normalisedRect{};
				hr = m_pMfEngineEx->TransferVideoFrame(m_pWicBitmap, &normalisedRect, &dstRect, &bg);

				return SUCCEEDED(hr);
			}
		}
	}

	return false;
}
