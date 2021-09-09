/* The source code comes from LearningPath library 
   https://github.com/MicrosoftDocs/Azure-Sphere-Developer-Learning-Path/tree/master/LearningPathLibrary */ 

#pragma once

#include "eventloop_timer_utilities.h"
#include "stdbool.h"
#include <applibs/eventloop.h>
#include <applibs/log.h>
#include <sys/timerfd.h>

typedef struct {
	void (*handler)(EventLoopTimer* timer);
	struct timespec period;
	EventLoopTimer* eventLoopTimer;
	const char* name;
} LP_TIMER;

EventLoop* lp_timerGetEventLoop(void);
bool lp_timerChange(LP_TIMER* timer, const struct timespec* period);
bool lp_timerOneShotSet(LP_TIMER* timer, const struct timespec* delay);
bool lp_timerOneShotSetOnce(LP_TIMER* timer, const struct timespec* period);
bool lp_timerStart(LP_TIMER* timer);
void lp_timerSetStart(LP_TIMER* timerSet[], size_t timerCount);
void lp_timerSetStop(LP_TIMER* timerSet[], size_t timerCount);
void lp_timerStop(LP_TIMER* timer);
void lp_timerEventLoopStop(void);