/* The source code comes from LearningPath library 
   https://github.com/MicrosoftDocs/Azure-Sphere-Developer-Learning-Path/tree/master/LearningPathLibrary */  

// #include "inter_core.h"

// static void SocketEventHandler(EventLoop* el, int fd, EventLoop_IoEvents events, void* context);
// static bool ProcessMsg(void);
// static void (*_interCoreCallback)(LP_INTER_CORE_BLOCK*);
// static const char* _rtAppComponentId = NULL;
// static int sockFd = -1;
// static EventRegistration* socketEventReg = NULL;

// static bool initialise_inter_core_communications(void)
// {
// 	if (sockFd != -1) // Already initialised
// 	{
// 		return true;
// 	}

// 	if (_rtAppComponentId == NULL)
// 	{
// 		lp_terminate(ExitCode_MissingRealTimeComponentId);
// 		return false;
// 	}

// 	// Open connection to real-time capable application.
// 	sockFd = Application_Connect(_rtAppComponentId);
// 	if (sockFd == -1)
// 	{
// 		Log_Debug("ERROR: Unable to create socket: %d (%s)\n", errno, strerror(errno));
// 		return false;
// 	}

// 	// Set timeout, to handle case where real-time capable application does not respond.
// 	static const struct timeval recvTimeout = { .tv_sec = 5, .tv_usec = 0 };
// 	int result = setsockopt(sockFd, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout));
// 	if (result == -1)
// 	{
// 		Log_Debug("ERROR: Unable to set socket timeout: %d (%s)\n", errno, strerror(errno));
// 		return false;
// 	}

// 	// Register handler for incoming messages from real-time capable application.
// 	socketEventReg = EventLoop_RegisterIo(lp_timerGetEventLoop(), sockFd, EventLoop_Input, SocketEventHandler,
// 		/* context */ NULL);
// 	if (socketEventReg == NULL)
// 	{
// 		Log_Debug("ERROR: Unable to register socket event: %d (%s)\n", errno, strerror(errno));
// 		return false;
// 	}

// 	return true;
// }

// /// <summary>
// ///     Nonblocking send intercore message
// ///		https://linux.die.net/man/2/send - Nonblocking = MSG_DONTWAIT.
// /// </summary>
// bool lp_interCoreSendMessage(LP_INTER_CORE_BLOCK* control_block, size_t len)
// {
// 	initialise_inter_core_communications();

// 	if (sockFd == -1)
// 	{
// 		Log_Debug("Socket not initialized");
// 		return false;
// 	}

// 	// https://linux.die.net/man/2/send - Nonblocking.
// 	// Returns EAGAIN if socket is full
// 	int bytesSent = send(sockFd, (void*)control_block, len, MSG_DONTWAIT);
// 	if (bytesSent == -1)
// 	{
// 		Log_Debug("ERROR: Unable to send message: %d (%s)\n", errno, strerror(errno));
// 		return false;
// 	}

// 	return true;
// }

// int lp_interCoreCommunicationsEnable(const char* rtAppComponentId, void (*interCoreCallback)(LP_INTER_CORE_BLOCK*))
// {
// 	_interCoreCallback = interCoreCallback;
// 	_rtAppComponentId = rtAppComponentId;

// 	return 0;
// }

// /// <summary>
// ///     Handle socket event by reading incoming data from real-time capable application.
// /// </summary>
// static void SocketEventHandler(EventLoop* el, int fd, EventLoop_IoEvents events, void* context)
// {
// 	if (!ProcessMsg())
// 	{
// 		lp_terminate(ExitCode_InterCoreHandler);
// 	}
// }

// /// <summary>
// ///     Handle socket event by reading incoming data from real-time capable application.
// /// </summary>
// static bool ProcessMsg()
// {
// 	LP_INTER_CORE_BLOCK ic_control_block;

// 	int bytesReceived = recv(sockFd, (void*)&ic_control_block, sizeof(ic_control_block), 0);

// 	if (bytesReceived == -1)
// 	{
// 		lp_terminate(ExitCode_InterCoreReceiveFailed);
// 		return false;
// 	}

// 	_interCoreCallback(&ic_control_block);

// 	return true;
// }