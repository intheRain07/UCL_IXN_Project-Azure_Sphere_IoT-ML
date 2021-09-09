/* The source code comes from LearningPath library 
   https://github.com/MicrosoftDocs/Azure-Sphere-Developer-Learning-Path/tree/master/LearningPathLibrary */ 

#pragma once

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code.  They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
	ExitCode_Success = 0,
	ExitCode_TermHandler_SigTerm = 1,
	ExitCode_Main_EventLoopFail = 2,
	ExitCode_Missing_ID_Scope = 3,

	ExitCode_Open_Peripheral = 10,
	ExitCode_OpenDeviceTwin = 11,
	ExitCode_AzureCloudToDeviceHandler = 12,
	ExitCode_InterCoreHandler = 13,
	ExitCode_ConsumeEventLoopTimeEvent = 14,
	ExitCode_Gpio_Read = 15,
	ExitCode_InterCoreReceiveFailed = 16,

	ExitCode_IsButtonPressed = 20,
	ExitCode_ButtonPressCheckHandler = 21,
	ExitCode_Led2OffHandler = 22,

	ExitCode_MissingRealTimeComponentId = 23,

	ExitCode_Validate_ConnectionType = 30,
    ExitCode_Validate_ScopeId = 31,
    ExitCode_Validate_IotHubConnectionString = 32,

	ExitCode_I2C_Open = 33,
	ExitCode_I2C_Set_Speed = 34,
	ExitCode_I2C_Set_Timeout = 35,

} ExitCode;