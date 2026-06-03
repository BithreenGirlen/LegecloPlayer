

#include "win_timer.h"

CWinTimer::CWinTimer()
{
	DEVMODE devMode{};
	::EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devMode);
	setInterval(1000 / devMode.dmDisplayFrequency);
}

CWinTimer::~CWinTimer()
{
	end();
}

void CWinTimer::start()
{
	if (m_pTpTimer != nullptr)return;

	m_pTpTimer = ::CreateThreadpoolTimer(TimerCallback, this, nullptr);
	if (m_pTpTimer != nullptr)
	{
		updateTimerInterval(m_pTpTimer);
	}
}

void CWinTimer::end()
{
	if (m_pTpTimer != nullptr)
	{
		::SetThreadpoolTimer(m_pTpTimer, nullptr, 0, 0);
		::WaitForThreadpoolTimerCallbacks(m_pTpTimer, TRUE);
		::CloseThreadpoolTimer(m_pTpTimer);
		m_pTpTimer = nullptr;
	}
}

void CWinTimer::setCallback(void(*pFunc)(void*), void* pUserData)
{
	m_pCallback = pFunc;
	m_pUserData = pUserData;
}

void CWinTimer::setDerfaultInterval(long long nInterval)
{
	m_nDefaultInterval = nInterval;
	m_nInterval = nInterval;
}

void CWinTimer::setInterval(long long nInterval)
{
	m_nInterval = nInterval;
}

long long CWinTimer::getInterval() const
{
	return m_nInterval;
}

void CWinTimer::resetInterval()
{
	m_nInterval = m_nDefaultInterval;
}

void CWinTimer::updateTimerInterval(PTP_TIMER timer)
{
	if (timer != nullptr)
	{
		FILETIME sFileDueTime{};
		ULARGE_INTEGER ulDueTime{};
		ulDueTime.QuadPart = static_cast<ULONGLONG>(-(1LL * 10 * 1000 * m_nInterval));
		sFileDueTime.dwHighDateTime = ulDueTime.HighPart;
		sFileDueTime.dwLowDateTime = ulDueTime.LowPart;
		::SetThreadpoolTimer(timer, &sFileDueTime, 0, 0);
	}
}

void CWinTimer::onTide()
{
	if (m_pCallback != nullptr)
	{
		m_pCallback(m_pUserData);
	}
}

void CWinTimer::TimerCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_TIMER Timer)
{
	CWinTimer* pThis = static_cast<CWinTimer*>(Context);
	if (pThis != nullptr)
	{
		pThis->onTide();
		pThis->updateTimerInterval(Timer);
	}
}
