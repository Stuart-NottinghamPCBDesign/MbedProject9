#include <events/mbed_events.h>
#include "ble/BLE.h"
#include "rtos/Thread.h"
#include "rtos/rtos_idle.h"
#include "stdio.h"
#include <mbed.h>
#include "BleData.h"



#define DEBUG	1
#define CLOCKWISE 0
#define ANTICLOCKWISE 1

/*
 The 28BYJ-48 Motor has a Gear ratio of 64 , and Stride Angle 5.625°  so this motor has a 4096 Steps .
 steps = Number of steps in One Revolution  * Gear ratio   .
 steps= (360°/5.625°)*64"Gear ratio" = 64 * 64 =4096/4 1024 for a Quater turn 

*/
#define SINGLE_STEP	12
#define QUATER	1024
#define HALF	2048


int _step = 0; 
bool dir = 0; // 


DigitalOut LED_1(p4); 			// Test LED
DigitalOut LED_2(p9); 			// Test LED
DigitalOut LED_LOWBATT(p10); 	// Low Battery LED
DigitalOut INA(p13); 			// Motor Step_1
DigitalOut INB(p12); 			// Motor Step_2
DigitalOut INC(p11); 			// Motor Step_3
DigitalOut IND(p8); 			// Motor Step_4
DigitalOut _5VENABLE(p14); 		// 5v Enable to boost 3.3v to 5v for the Motor IC

DigitalIn SWITCH(p3);			// Header for a switch input if needed

AnalogIn BattLevel(p2);			// VCC in to measure battery 

Serial pc(p6, p5);  // tx, rx
 	
const static char     DEVICE_NAME[] = "BLELOCK";
static const uint16_t uuid16_list[] = { LEDService::LED_SERVICE_UUID };

//	BLE classes
static EventQueue eventQueue(/* event count */ 10 * EVENTS_EVENT_SIZE);

LEDService *ledServicePtr;


void BatteryCheck(void)
{
	

	float voltage = BattLevel.read();
	printf("Battery percentage: %3.3f%% \r\n", voltage * 100.0f);
	float TrueVoltage = (voltage *(3.3)); 
	printf("value voltage %f \r\n", TrueVoltage);
	printf("Battery normalized: 0x%04X  \r\n", BattLevel.read_u16());  

	if (BattLevel > 0.9f)  // If the Battery Voltage is greater than 2.97v Keep the Low battery LED off 
		{
			LED_LOWBATT = 0;
		}
	else {
		LED_LOWBATT = 1;
	}
}
void TurnMotor(bool dir,uint Length)
{
	_5VENABLE = 1;       // Turn on the 5v Enable to the motor Driver IC
	for (uint x = 0; x < Length; x++)
	{
		switch (_step) { 
   
		case 0: 
			INA = 0;  
			INB = 0;
			INC = 0;
			IND = 1;
			break;  
		case 1: 
			INA = 0;  
			INB = 0;
			INC = 1;
			IND = 1;
			break;  
		case 2: 
			INA = 0;  
			INB = 0;
			INC = 1;
			IND = 0;
			break;  
		case 3: 
			INA = 0;  
			INB = 1;
			INC = 1;
			IND = 0;
			break;  
		case 4: 
			INA = 0;  
			INB = 1;
			INC = 0;
			IND = 0;
			break;  
		case 5: 
			INA = 1;  
			INB = 1;
			INC = 0;
			IND = 0;
			break;  
		case 6: 
			INA = 1;  
			INB = 0;
			INC = 0;
			IND = 0;
			break;  
		case 7: 
			INA = 1;  
			INB = 0;
			INC = 0;
			IND = 1;
			break;  
		default: 
			INA = 0;  
			INB = 0;
			INC = 0;
			IND = 0;
			break;  
		} 
		wait(0.001); 
		if (dir) { 
			_step++; 
		}
		else { 
			_step--; 
		} 
		if (_step > 7) { 
			_step = 0; 
		} 
		if (_step < 0) { 
			_step = 7; 
		} 
	}
	_5VENABLE = 0;       // Turn off the 5v Enable to the motor Driver IC
	INA = 0;			
	INB = 0;
	INC = 0;
	IND = 0;
}
	
void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params)
{
	(void) params;
	BLE::Instance().gap().startAdvertising();
}

void blinkCallback(void)
{
	char ch = 0;
	LED_1 = !LED_1; /* Do blinky on LED1 to indicate system aliveness. */
	//BatteryCheck();
	if (pc.readable()) {
		ch = pc.getc();
		
		if (ch == 's')
		{
			printf("\r\n\r\n");
			printf("Welcome to the Config software \r\n");		
			printf("Press 'c' To turn Clockwise a single step \r\n");
			printf("Press 'C' To turn Clockwise a Quater turn \r\n");
			printf("Press 'a' To turn AntiClockwise a single step \r\n");
			printf("Press 'A' To turn AntiClockwise a Quater turn \r\n");
			printf("*************************************\r\n");
			printf("Press 'e' To Exit \r\n");
			printf("*************************************\r\n");
			printf("*************************************\r\n\r\n\r\n");
			
			while (ch != 'e')
			{
				ch = pc.getc();
			switch (ch)
				{
					case 'a':
							printf("Motor Turning AntiClockwise Single Step \r\n");
							TurnMotor(ANTICLOCKWISE, SINGLE_STEP);
					break;
				case 'A':
					printf("Motor Turning AntiClockwise Quater Turn \r\n");
					TurnMotor(ANTICLOCKWISE, QUATER);
					break;
					case 'c':
							printf("Motor Turning Clockwise Single Step\r\n");
							TurnMotor(CLOCKWISE, SINGLE_STEP);

					break;
				case 'C':
					printf("Motor Turning Clockwise Quater Turn\r\n");
					TurnMotor(CLOCKWISE, QUATER);

					break;
					default:
					if (ch != 'e')
					{
						printf("Incorrect Key Pressed please try again \r\n");

					}
					break;
				} 
			}
			printf("GoodBye \r\n");		
			
		}
	
	}
	
}

/**
 * This callback allows the LEDService to receive updates to the ledState Characteristic.
 *
 * @param[in] params
 *     Information about the characterisitc being updated.
 */
void onDataWrittenCallback(const GattWriteCallbackParams *params) {
	uint Direction = 0xFF;
	if ((params->handle == ledServicePtr->getValueHandle()) && (params->len == 1)) {
		Direction = *(params->data);
		if (Direction == 0x01)
		{
			TurnMotor(CLOCKWISE, QUATER);
			LED_2 = 1;
			
		}
		else if (Direction == 0x00)
		{
			TurnMotor(ANTICLOCKWISE, QUATER);
			LED_2 = 0;
		}	
		
	}
}

/**
 * This function is called when the ble initialization process has failled
 */
void onBleInitError(BLE &ble, ble_error_t error)
{
	/* Initialization error handling should go here */
}

void printMacAddress()
{
	/* Print out device MAC address to the console*/
	Gap::AddressType_t addr_type;
	Gap::Address_t address;
	BLE::Instance().gap().getAddress(&addr_type, address);
	printf("DEVICE MAC ADDRESS: ");
	for (int i = 5; i >= 1; i--) {
		printf("%02x:", address[i]);
	}
	printf("%02x\r\n", address[0]);
}

/**
 * Callback triggered when the ble initialization process has finished
 */
void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
	BLE&        ble   = params->ble;
	ble_error_t error = params->error;

	if (error != BLE_ERROR_NONE) {
		/* In case of error, forward the error handling to onBleInitError */
		onBleInitError(ble, error);
		return;
	}

	/* Ensure that it is the default instance of BLE */
	if (ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
		return;
	}

	ble.gap().onDisconnection(disconnectionCallback);
	ble.gattServer().onDataWritten(onDataWrittenCallback);

	bool initialValueForLEDCharacteristic = true;
	ledServicePtr = new LEDService(ble, initialValueForLEDCharacteristic);

	/* setup advertising */
	ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
	ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
	ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
	ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
	ble.gap().setAdvertisingInterval(1000); /* 1000ms. */
	ble.gap().startAdvertising();

	 printMacAddress();
}

void scheduleBleEventsProcessing(BLE::OnEventsToProcessCallbackContext* context) {
	BLE &ble = BLE::Instance();
	eventQueue.call(Callback<void()>(&ble, &BLE::processEvents));
}



int main() 
{
	pc.baud(115200);
	INA = 0;
	INB = 0;
	INC = 0;
	IND = 0;
	_5VENABLE = 0;      // Turn off the 5v Enable to the motor Driver IC
	
	
	eventQueue.call_every(500, blinkCallback);

	BLE &ble = BLE::Instance();
	ble.onEventsToProcess(scheduleBleEventsProcessing);
	ble.init(bleInitComplete);
	
	
	eventQueue.dispatch_forever();

	return 0;
		
}	
	

	
