/* The source code comes from LearningPath library 
   https://github.com/MicrosoftDocs/Azure-Sphere-Developer-Learning-Path/tree/master/LearningPathLibrary */ 

#include "timer.h"
#include "eventloop_timer_utilities.h"

static EventLoop* eventLoop = NULL;

EventLoop* lp_timerGetEventLoop(void) {
	if (eventLoop == NULL) {
		eventLoop = EventLoop_Create();
	}
	return eventLoop;
}

bool lp_timerChange(LP_TIMER* timer, const struct timespec* period) {
	if (timer->eventLoopTimer == NULL) { return false; }
	timer->period.tv_nsec = period->tv_nsec;
	timer->period.tv_sec = period->tv_sec;
	int result = SetEventLoopTimerPeriod(timer->eventLoopTimer, period);

	return result == 0 ? true : false;
}

bool lp_timerStart(LP_TIMER* timer) {
	EventLoop* eventLoop = lp_timerGetEventLoop();
	if (eventLoop == NULL) {
		return false;
	}

	if (timer->eventLoopTimer != NULL) {
		return true;
	}

	if (timer->period.tv_nsec == 0 && timer->period.tv_sec == 0) {  
		// Set up a disabled LP_TIMER for oneshot or change timer
		timer->eventLoopTimer = CreateEventLoopDisarmedTimer(eventLoop, timer->handler);
		if (timer->eventLoopTimer == NULL) {
			return false;
		}
	}
	else {
		timer->eventLoopTimer = CreateEventLoopPeriodicTimer(eventLoop, timer->handler, &timer->period);
		if (timer->eventLoopTimer == NULL) {
			return false;
		}
	}
	
	return true;
}

void lp_timerStop(LP_TIMER* timer) {
	if (timer->eventLoopTimer != NULL) {
		DisposeEventLoopTimer(timer->eventLoopTimer);
		timer->eventLoopTimer = NULL;
	}
}

void lp_timerSetStart(LP_TIMER* timerSet[], size_t timerCount) {
	for (int i = 0; i < timerCount; i++) {
		if (!lp_timerStart(timerSet[i])) {
			break;
		};
	}
	Log_Debug("Timers init successfully.\n"); 
}

void lp_timerSetStop(LP_TIMER* timerSet[], size_t timerCount) {
	for (int i = 0; i < timerCount; i++) {
		lp_timerStop(timerSet[i]);
	}
}

void lp_timerEventLoopStop(void) {
	EventLoop* eventLoop = lp_timerGetEventLoop();
	if (eventLoop != NULL) {
		EventLoop_Close(eventLoop);
	}
}

bool lp_timerOneShotSet(LP_TIMER* timer, const struct timespec* period) {
	EventLoop* eventLoop = lp_timerGetEventLoop();
	if (eventLoop == NULL) {
		return false;
	}

	if (timer->eventLoopTimer == NULL) {
		return false;
	}

	if (SetEventLoopTimerOneShot(timer->eventLoopTimer, period) != 0) {
		return false;
	}

	return true;
}

bool lp_timerOneShotSetOnce(LP_TIMER* timer, const struct timespec* period) {
	EventLoop* eventLoop = lp_timerGetEventLoop();
	if (eventLoop == NULL) {
		return false;
	}

	if (timer->eventLoopTimer == NULL) {
		return false;
	}

	const struct timespec nullTimeSpec = {.tv_sec = 0, .tv_nsec = 0}; 
	struct itimerspec specBuffer = { .it_value = nullTimeSpec, .it_interval = nullTimeSpec }; 
    if (timerfd_gettime(GetEventLoopTimerfd(timer->eventLoopTimer), &specBuffer) < 0) {
        Log_Debug("ERROR: Could not get timer period.\n"); 
        return false; 
    }

	if (specBuffer.it_value.tv_nsec !=0 || specBuffer.it_value.tv_sec != 0) {
		return true; 
	}

	if (SetEventLoopTimerOneShot(timer->eventLoopTimer, period) != 0) {
		return false;
	}

	return true;
}