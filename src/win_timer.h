#ifndef WIN_TIMER_H_
#define WIN_TIMER_H_

#include <functional>

#include <Windows.h>

class CWinTimer
{
public:
	CWinTimer();
	~CWinTimer();

	void Start();
	void End();

	void SetCallback(std::function<void()> pFunc);
	void SetInterval(long long nInterval);
private:
	long long m_nInterval = 16;
	PTP_TIMER m_pTpTimer = nullptr;

	std::function<void()> m_pPeriodicFunc = nullptr;

	void UpdateTimerInterval(PTP_TIMER timer);
	void OnTide();
	static void CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_TIMER Timer);
};
#endif // !WIN_TIMER_H_
