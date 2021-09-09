// KGTB3, UCL IXN project ver1.1 

// system library
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>

// azsphere library
#include <applibs/log.h>
#include <applibs/gpio.h>

// learning path library 
#include "lib/azure_iot.h" 
#include "lib/peripheral_gpio.h"
#include "lib/timer.h" 
#include "lib/exit_codes.h" 
#include "lib/utilities.h" 
#include "lib/terminate.h"
#include "lib/config.h" 

// VL53L1X library
#include "VL53L1X/VL53L1X.h" 
#include "VL53L1X/i2c.h" 
#include "VL53L1X/utils.h" 

// hardeare definitions 
#include <hw/template_appliance.h>

// Number of bytes to allocate for the JSON telemetry message for IoT Central
#define JSON_MESSAGE_BYTES 256 

// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play 
// #define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:com:example:azuresphere:labmonitor;1" 
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:com:example:azuresphere:labmonitor;1" 

// Functions 
static bool CheckTransferSize(const char *desc, size_t expectedBytes, ssize_t actualBytes);

static void ToggleLED1Handler(EventLoopTimer* eventLoopTimer); 
static void AzureIoTConnectionHandler(EventLoopTimer* eventLoopTimer); 
static void SendDatatoAzureIoTCentralHandler(EventLoopTimer* eventLoopTimer);
static void ReadToFHandler(EventLoopTimer* eventLoopTimer); 
static void EnterLeaveTimeoutHandler(EventLoopTimer* eventLoopTimer); 
static void TurnOffEnterLeaveLEDHandler(EventLoopTimer* eventLoopTimer); 

static LP_DIRECT_METHOD_RESPONSE_CODE OverflowWarningHandler(JSON_Value* json, LP_DIRECT_METHOD_BINDING* directMethodBinding, char** responseMsg);
static LP_DIRECT_METHOD_RESPONSE_CODE CancelWarningHandler(JSON_Value* json, LP_DIRECT_METHOD_BINDING* directMethodBinding, char** responseMsg);

static void DeviceTwinSetMaxCapability(LP_DEVICE_TWIN_BINDING* deviceTwinBinding);

static void InitPeripheralAndHandlers(void);
static void ClosePeripheralAndHandlers(void);


// Variables 
static char msgBuffer[JSON_MESSAGE_BYTES] = { 0 }; 

static int msgId = 0;
static int numOfPeople = 0; 

static uint8_t obj_detect[3] = { 0 }; 

static bool enter_leave_clearflag = true; 

LP_USER_CONFIG lp_config; 

enum LEDS { RED, GREEN, BLUE, UNKNOWN };
static enum LEDS current_led = GREEN; 
static const char* capabilityState[] = { "overflow", "under capability", "at boundary" }; 

static const char* msgTemplate = "{ \"NumOfPeople\":%d, \"MaxCapability\":%d, \"MsgID\":%d }"; 

// GPIO 
static LP_GPIO RedCapabilityLED = {
    .pin = LED1_RED, 
    .direction = LP_OUTPUT, 
    .initialState = GPIO_Value_Low, 
    .invertPin = true,
    .name = "red led1"
};

static LP_GPIO GreenCapabilityLED = {
    .pin = LED1_GREEN, 
    .direction = LP_OUTPUT, 
    .initialState = GPIO_Value_Low, 
    .invertPin = true,
    .name = "green led1"
};

static LP_GPIO BlueCapabilityLED = {
    .pin = LED1_BLUE, 
    .direction = LP_OUTPUT, 
    .initialState = GPIO_Value_Low, 
    .invertPin = true,
    .name = "blue led1"
};

static LP_GPIO RedELLED = {
    .pin = LED2_RED, 
    .direction = LP_OUTPUT, 
    .initialState = GPIO_Value_Low, 
    .invertPin = true,
    .name = "red led1"
};

static LP_GPIO GreenELLED = {
    .pin = LED2_GREEN, 
    .direction = LP_OUTPUT, 
    .initialState = GPIO_Value_Low, 
    .invertPin = true,
    .name = "green led1"
};

static LP_GPIO BlueELLED = {
    .pin = LED2_BLUE, 
    .direction = LP_OUTPUT, 
    .initialState = GPIO_Value_Low, 
    .invertPin = true,
    .name = "blue led1"
};

static LP_GPIO OverflowWarningLED = {
    .pin = LED3_RED, 
    .direction = LP_OUTPUT, 
    .initialState = GPIO_Value_Low, 
    .invertPin = true,
    .name = "overflow warning led"
};

static LP_GPIO networkConnectedLed = {
    .pin = NETWORK_CONNECTED_LED,
    .direction = LP_OUTPUT,
    .initialState = GPIO_Value_Low,
    .invertPin = true,
    .name = "network connected led" 
};

// GPIO set

// LED1 inidcates the capability status of the current room
// green: under, blue: equal, red: overflow 

// LED2: enter/leave indicator
// green: enter, blue: leave, red: re-enter/re-leave 

// LED3: overflow warning LED 

LP_GPIO* gpioSet[] = 
{ &BlueCapabilityLED, &GreenCapabilityLED, &RedCapabilityLED,
  &RedELLED, &GreenELLED, &BlueELLED,
  &OverflowWarningLED, 
  &networkConnectedLed 
}; 

static LP_GPIO* rgbSet[] = { &RedCapabilityLED, &GreenCapabilityLED, &BlueCapabilityLED }; 

//Timer
static LP_TIMER toggleLED1Timer = {
    .period = { 2, 0 },
	.name = "toggleLED1Timer",
	.handler = ToggleLED1Handler 
};

static LP_TIMER readToFTimer = {
    .period = { 0, 3e7 },
	.name = "readToFTimer",
	.handler = ReadToFHandler
};

static LP_TIMER azureIoTConnectionTimer = {
    .period = { 3, 0 }, 
	.name = "azureIoTConnectionTimer",
	.handler = AzureIoTConnectionHandler 
};

static LP_TIMER azureIoTMsgSendTimer = {
    .period = { 10, 0 }, 
	.name = "azureIoTMsgSendTimer",
	.handler =  SendDatatoAzureIoTCentralHandler 
};

// oneshot timer
static LP_TIMER enterLeaveTimeoutTimer = {
    .period = { 0, 0 }, 
	.name = "enterLeaveTimeoutTimer",
	.handler =  EnterLeaveTimeoutHandler  
};

// oneshot timer 
static LP_TIMER turnOffEnterLeaveLEDTimer = {
    .period = { 0, 0 }, 
	.name = "turnOffEnterLeaveLEDTimer",
	.handler =  TurnOffEnterLeaveLEDHandler 
};

// Timer set
LP_TIMER* timerSet[] = { &toggleLED1Timer, &readToFTimer, &turnOffEnterLeaveLEDTimer, 
                         &azureIoTConnectionTimer, &azureIoTMsgSendTimer }; 

// Azure IoT Device Twins
static LP_DEVICE_TWIN_BINDING dt_desiredMaxCapability = {
	.twinProperty = "DesiredMaxCapability", // property name, which should be same with name on IoT central 
	.twinType = LP_TYPE_INT, 
	.handler = DeviceTwinSetMaxCapability // handler function to be triggered when property is received 
};

static LP_DEVICE_TWIN_BINDING dt_reportedCapabilityState = {
	.twinProperty = "ReportedCapabilityState",
	.twinType = LP_TYPE_STRING 
}; 

// Device Binding set 
LP_DEVICE_TWIN_BINDING* deviceTwinBindingSet[] = { &dt_desiredMaxCapability, &dt_reportedCapabilityState }; 

// Azure IoT Direct Methods
static LP_DIRECT_METHOD_BINDING dm_overflowWarning = {
    .methodName = "OverflowWarning",
    .handler = OverflowWarningHandler 
};

static LP_DIRECT_METHOD_BINDING dm_cancelWarning = {
    .methodName = "CancelWarning", // command name 
    .handler = CancelWarningHandler // handler function 
}; 

// Direct Method set 
LP_DIRECT_METHOD_BINDING* directMethodBindingSet[] = { &dm_overflowWarning, &dm_cancelWarning }; 


/// <summary>
///    Checks the number of transferred bytes for I2C functions and prints an error
///    message if the functions failed or if the number of bytes is different than
///    expected number of bytes to be transferred.
/// </summary>
/// <returns>true on success, or false on failure</returns>
static bool CheckTransferSize(const char *desc, size_t expectedBytes, ssize_t actualBytes)
{
    if (actualBytes < 0) {
        Log_Debug("ERROR: %s: errno=%d (%s)\n", desc, errno, strerror(errno));
        return false;
    }

    if (actualBytes != (ssize_t)expectedBytes) {
        Log_Debug("ERROR: %s: transferred %zd bytes; expected %zd\n", desc, actualBytes,
                  expectedBytes);
        return false;
    }

    return true;
}

static void AzureIoTConnectionHandler(EventLoopTimer* eventLoopTimer) {
    static bool toggleConnectionStatusLed = true;

	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
		lp_terminate(ExitCode_ConsumeEventLoopTimeEvent);
	}
	else {
		if (lp_azureConnect()) {
			lp_gpioStateSet(&networkConnectedLed, toggleConnectionStatusLed);
			toggleConnectionStatusLed = !toggleConnectionStatusLed;
		}
		else {
			lp_gpioStateSet(&networkConnectedLed, false); 
            Log_Debug("Network not ready...\n");
		}
	}
}

static void SendDatatoAzureIoTCentralHandler(EventLoopTimer* eventLoopTimer) {
	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0)
	{
		lp_terminate(ExitCode_ConsumeEventLoopTimeEvent);
	}
	else {
        int maxCapability = *(int*)dt_desiredMaxCapability.twinState; 
        //  msgTemplate is defined as "{ \"NumOfPeople\":%d, \"MaxCapability\":%d, \"MsgID\":%d }"; 
		if (snprintf(msgBuffer, JSON_MESSAGE_BYTES, msgTemplate, numOfPeople, maxCapability, msgId++) > 0) 
		{
			Log_Debug("%s\n", msgBuffer);
			lp_azureMsgSend(msgBuffer); // send data to cloud 
            lp_deviceTwinReportState(&dt_reportedCapabilityState, (void*)capabilityState[(int)current_led]); 
		}
	}
}

static void ToggleLED1Handler(EventLoopTimer* eventLoopTimer) {
    static bool toggleflag = true; 

    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
		lp_terminate(ExitCode_ConsumeEventLoopTimeEvent);
	} 
    else {
        if (dt_desiredMaxCapability.twinStateUpdated) {
            int maxCapability = *(int*)dt_desiredMaxCapability.twinState; 
            current_led = numOfPeople == maxCapability ? BLUE : (numOfPeople > maxCapability ? RED : GREEN);
        } 

        if (toggleflag) {
            lp_gpioOn(rgbSet[(int)current_led]); 
            // Log_Debug("people num: %d\n", numOfPeople);
            // Log_Debug("%d %d %d\n", obj_detect[0], obj_detect[1], obj_detect[2]); 
            // Log_Debug("flag: %d\n", enter_leave_clearflag); 
        } else {
            lp_gpioOff(rgbSet[(int)RED]); 
            lp_gpioOff(rgbSet[(int)GREEN]); 
            lp_gpioOff(rgbSet[(int)BLUE]); 
        }
        toggleflag = !toggleflag; 
	}
}

static void ReadToFHandler(EventLoopTimer* eventLoopTimer) {
    uint16_t ToF_res1; 
    uint16_t ToF_res2; 
    struct timespec delay; 

    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
		lp_terminate(ExitCode_ConsumeEventLoopTimeEvent);
	} 
    else {
        // read ToF sensor
        ToF_res1 = VL53L1X_read(GetI2CHandle(I2C0), true); 
        ToF_res2 = VL53L1X_read(GetI2CHandle(I2C1), true); 

        // Log_Debug("1: %d, 2: %d\n", ToF_res1, ToF_res2); 

        if (ToF_res1<=OBJ_BOUNDARY_MM && ToF_res2>OBJ_BOUNDARY_MM) {
            obj_detect[0] = (obj_detect[0]==0 ? 1 : obj_detect[0]); 
            obj_detect[1] = (obj_detect[0]==2 ? 1 : 0); 
        } else if (ToF_res1>OBJ_BOUNDARY_MM && ToF_res2<=OBJ_BOUNDARY_MM) {
            obj_detect[0] = (obj_detect[0]==0 ? 2 : obj_detect[0]); 
            obj_detect[1] = (obj_detect[0]==1 ? 2: 0); 
        } else if (ToF_res1>OBJ_BOUNDARY_MM && ToF_res2>OBJ_BOUNDARY_MM) {
            obj_detect[2] = (obj_detect[1]==0 ? 0 : 3); 
        }

        // entering/leaving finish 
        if (obj_detect[2] == 3) {
            enter_leave_clearflag = true; 
            switch (obj_detect[0]) {
                case 1: //entering 
                    lp_timerStop(&enterLeaveTimeoutTimer); // stop timer used to clear obj_detect
                    numOfPeople++; 
                    memset(obj_detect, 0, sizeof(uint8_t)*3); // clear the obj_detect buffer 

                    // trun on blue led2 and turn it off 1s later
                    lp_gpioOn(&BlueELLED);
                    delay.tv_sec = 1, delay.tv_nsec = 0; 
                    lp_timerOneShotSet(&turnOffEnterLeaveLEDTimer, &delay); // start counting 

                    Log_Debug("Enter...\n"); 
                    break;
                
                case 2: // leaving 
                    lp_timerStop(&enterLeaveTimeoutTimer); // stop timer used to clear obj_detect
                    numOfPeople--; 
                    memset(obj_detect, 0, sizeof(uint8_t)*3); // clear the obj_detect buffer 

                    // trun on green led2 and turn it off 1s later
                    lp_gpioOn(&GreenELLED);
                    delay.tv_sec = 1, delay.tv_nsec = 0; 
                    lp_timerOneShotSet(&turnOffEnterLeaveLEDTimer, &delay); // start counting 

                    Log_Debug("Exit...\n"); 
                    break; 
                
                default:
                    Log_Debug("Read ToF Error!\n"); 
                    break;
            }
            Log_Debug("people num: %d\n", numOfPeople); 
        } 
        // entering/leaving started but not finished: start timeout counting, refresh if timeout occurs 
        else if (obj_detect[0]!=0 && obj_detect[2]==0 && enter_leave_clearflag) {
            // clear the flag, re-start timer until the last EnterLeaveTimeoutHandler complete  
            enter_leave_clearflag = false; 
            delay.tv_sec = 2, delay.tv_nsec = 5e8; 
            lp_timerStart(&enterLeaveTimeoutTimer); // init timer
            lp_timerOneShotSet(&enterLeaveTimeoutTimer, &delay); // start counting 
        }
	}
}

static void EnterLeaveTimeoutHandler(EventLoopTimer* eventLoopTimer) {
    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
		lp_terminate(ExitCode_ConsumeEventLoopTimeEvent);
	} 
    else {
        enter_leave_clearflag = true; 
        Log_Debug("Clear obj_detect...\n"); 
        lp_timerStop(&readToFTimer); 
        memset(obj_detect, 0, sizeof(uint8_t)*3); // clear the obj_detect buffer 
        lp_gpioOn(&RedELLED);
        delay(3000); 
        lp_timerStart(&readToFTimer);
        lp_gpioOff(&RedELLED); 
    }
}

static void TurnOffEnterLeaveLEDHandler(EventLoopTimer* eventLoopTimer) {
    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
		lp_terminate(ExitCode_ConsumeEventLoopTimeEvent);
	} 
    else {
        lp_gpioOff(&GreenELLED);
        lp_gpioOff(&BlueELLED);
    }
}

static void DeviceTwinSetMaxCapability(LP_DEVICE_TWIN_BINDING* deviceTwinBinding) {
    // validate data is sensible range before applying
	if (deviceTwinBinding->twinType == LP_TYPE_INT) 
	{
		lp_deviceTwinAckDesiredState(deviceTwinBinding, deviceTwinBinding->twinState, LP_DEVICE_TWIN_COMPLETED);
        Log_Debug("New max capability: %d\n", *(int*)deviceTwinBinding->twinState); 
	}
	else {
		lp_deviceTwinAckDesiredState(deviceTwinBinding, deviceTwinBinding->twinState, LP_DEVICE_TWIN_ERROR);
	}
}

static LP_DIRECT_METHOD_RESPONSE_CODE OverflowWarningHandler(JSON_Value* json, LP_DIRECT_METHOD_BINDING* directMethodBinding, char** responseMsg)
{
    // static bool toggle_flag = true; 
    // Allocate and initialize a response message buffer. The calling function is responsible for the freeing memory
    const size_t responseLen = 100; 

    *responseMsg = (char*)malloc(responseLen);
    memset(*responseMsg, 0, responseLen);

    Log_Debug( "json type: %i\n", (int)json_value_get_type(json) ); 

    // Create Direct Method Response
    snprintf(*responseMsg, responseLen, "%s called. Light led3 red...", directMethodBinding->methodName); 

    // Toggle the overflow led (LED_RED_2)
    lp_gpioStateSet(&OverflowWarningLED, true); 
    // toggle_flag = !toggle_flag; 

    return LP_METHOD_SUCCEEDED;
}

static LP_DIRECT_METHOD_RESPONSE_CODE CancelWarningHandler(JSON_Value* json, LP_DIRECT_METHOD_BINDING* directMethodBinding, char** responseMsg) {
    // Allocate and initialize a response message buffer. The calling function is responsible for the freeing memory
    const size_t responseLen = 100; 

    *responseMsg = (char*)malloc(responseLen);
    memset(*responseMsg, 0, responseLen);

    Log_Debug( "json type: %i\n", (int)json_value_get_type(json) ); 

    // Create Direct Method Response
    snprintf(*responseMsg, responseLen, "%s called. Turn off led3 red...", directMethodBinding->methodName); 

    // Toggle the overflow led (LED_RED_2)
    lp_gpioStateSet(&OverflowWarningLED, false); 
    // toggle_flag = !toggle_flag; 

    return LP_METHOD_SUCCEEDED;
}

static void InitPeripheralAndHandlers(void) {
    lp_azureInitialize(lp_config.scopeId, IOT_PLUG_AND_PLAY_MODEL_ID); 

    lp_gpioSetOpen(gpioSet, NELEMS(gpioSet));

    if (VL53L1X_init(true)) { 
        VL53L1X_startContinuous(25); // ranging frequency = 40 hz
        Log_Debug("ToF init successfully.\n"); 
    } else {
        Log_Debug("Failed to init ToF.\n"); 
    }

    lp_timerSetStart(timerSet, NELEMS(timerSet)); 

    lp_deviceTwinSetOpen(deviceTwinBindingSet, NELEMS(deviceTwinBindingSet));
    
    lp_directMethodSetOpen(directMethodBindingSet, NELEMS(directMethodBindingSet));

    lp_azureToDeviceStart();
}


static void ClosePeripheralAndHandlers(void) {
    lp_timerSetStop(timerSet, NELEMS(timerSet));
    lp_gpioSetClose(gpioSet, NELEMS(gpioSet));

    lp_azureToDeviceStop();
    lp_deviceTwinSetClose();
    lp_directMethodSetClose();

    lp_timerEventLoopStop();
}

int main(int argc, char* argv[])
{
	lp_registerTerminationHandler();

	lp_configParseCmdLineArguments(argc, argv, &lp_config);
	if (!lp_configValidate(&lp_config)) {
		return lp_getTerminationExitCode();
	}
    Log_Debug("\nCOMP0073 Project, Yuxin Guo, version 1.0 testing...\n"); 
    
    InitPeripheralAndHandlers(); 

    // Main loop
	while (!lp_isTerminationRequired())
	{
		int result = EventLoop_Run(lp_timerGetEventLoop(), -1, true);
		// Continue if interrupted by signal, e.g. due to breakpoint being set.
		if (result == -1 && errno != EINTR)
		{
			lp_terminate(ExitCode_Main_EventLoopFail);
		}
	}

	ClosePeripheralAndHandlers();

	Log_Debug("Application exiting.\n");
	return lp_getTerminationExitCode();

} 
