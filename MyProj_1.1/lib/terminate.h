/* The source code comes from LearningPath library 
   https://github.com/MicrosoftDocs/Azure-Sphere-Developer-Learning-Path/tree/master/LearningPathLibrary */ 

#pragma once

#include "exit_codes.h"
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

// volatile sig_atomic_t terminationRequired = false;

void lp_registerTerminationHandler(void);
void lp_terminationHandler(int signalNumber);
void lp_terminate(int exitCode);
bool lp_isTerminationRequired(void);
int lp_getTerminationExitCode(void);