#include "pch.h"
/*
The perfect timer :) code (C) by E}I{  ej@tut.by
this part of gpuE}I{soft can be redistributed and modified
in either source or binary form as long as this notice isn't changed or removed.

Code requires x98 MMX CPU and Win32 VC5.0 compatible compiler.
*/


// Timer () will return time passed from previous call in mksec as uint32_t

static LARGE_INTEGER oldtimer,newtimer;
static LARGE_INTEGER kf;

// Prepare data structures for timers, call at least once
void TimerInit(void) {
	QueryPerformanceFrequency(&kf);
}

// reset Timer
void Timer(void) {
	QueryPerformanceCounter(&oldtimer);
}

// Get timer, no reset
uint32_t GetTimer(void) {
	QueryPerformanceCounter(&newtimer);

	LARGE_INTEGER diff{};
	diff.QuadPart = newtimer.QuadPart - oldtimer.QuadPart;
	diff.QuadPart *= 1000000;
	diff.QuadPart /= kf.QuadPart;

	return diff.LowPart;
}

// Get timer and reset
uint32_t GetTimerR(void) {
	uint32_t t = GetTimer();
	oldtimer.QuadPart = newtimer.QuadPart;
	return t;
}

void TimerTest() {
	Timer();
	uint32_t stamp1 = GetTimer();
	::Sleep(1000);
	uint32_t stamp2 = GetTimer();
	printf("TimerTest, ticks between Sleep(1000), should be around 1'000'000: %d\n", stamp2 - stamp1);
}