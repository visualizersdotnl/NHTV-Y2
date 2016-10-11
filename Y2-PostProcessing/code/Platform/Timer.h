
/*
	Simple Win32 timer (uses performance timer if available).
*/

#pragma once

class Timer
{
public:
	Timer() :
		m_isHighFreq(QueryPerformanceFrequency(&m_pcFrequency) != 0)
	{
		Reset();
	}

	void Reset()
	{
		if (!m_isHighFreq)
		{
			m_offset.LowPart = GetTickCount();
			m_oneOverFreq = 0.001f;
		}
		else
		{
			VERIFY(QueryPerformanceCounter(&m_offset));
			m_oneOverFreq = 1.f / (float) m_pcFrequency.QuadPart;
		}
	}

	float Get() const 
	{
		if (!m_isHighFreq)
		{
			return (float) (GetTickCount() - m_offset.LowPart) * m_oneOverFreq;
		}
		else
		{
			LARGE_INTEGER curCount;
			VERIFY(QueryPerformanceCounter(&curCount)); 
			return (float) (curCount.QuadPart - m_offset.QuadPart) * m_oneOverFreq;
		}
	}

private:
	const bool m_isHighFreq;
	LARGE_INTEGER m_pcFrequency; 
	LARGE_INTEGER m_offset;
	float m_oneOverFreq;
};
