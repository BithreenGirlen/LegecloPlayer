#ifndef WIN_TIMER_H_
#define WIN_TIMER_H_

#include <Windows.h>

class CWinTimer
{
public:
	CWinTimer();
	~CWinTimer();

	void start();
	void end();

	void setCallback(void (*pFunc)(void*), void* pUserData);
	void setDerfaultInterval(long long nInterval);

	void setInterval(long long nInterval);
	long long getInterval() const;
	void resetInterval();
private:
	enum Constants { kDefaultInterval = 16 };

	long long m_nDefaultInterval = Constants::kDefaultInterval;
	long long m_nInterval = Constants::kDefaultInterval;
	PTP_TIMER m_pTpTimer = nullptr;

	void (*m_pCallback)(void*) = nullptr;
	void* m_pUserData = nullptr;

	void updateTimerInterval(PTP_TIMER timer);
	void onTide();
	static void CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_TIMER Timer);
};
#endif // !WIN_TIMER_H_
