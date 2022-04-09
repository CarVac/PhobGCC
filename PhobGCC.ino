//This software uses bits of code from GoodDoge's Dogebawx project, which was the initial starting point: https://github.com/DogeSSBM/DogeBawx

#include <math.h>
#include <curveFitting.h>
#include <EEPROM.h>
#include <eigen.h>
#include <Eigen/LU>
#include <ADC.h>
#include <VREF.h>
#include <Bounce2.h>
#include "TeensyTimerTool.h"

using namespace Eigen;


TeensyTimerTool::OneShotTimer timer1;



/////defining which pin is what on the teensy
const int _pinLa = 16;
const int _pinRa = 23;
const int _pinL = 12;
const int _pinR = 3;
const int _pinAx = 15;
const int _pinAy = 14;
//const int _pinCx = 21;
//const int _pinCy = 22;
const int _pinCx = 22;
const int _pinCy = 21;
const int _pinRX = 7;
const int _pinTX = 8;
const int _pinDr = 6;
const int _pinDu = 18;
const int _pinDl = 17;
const int _pinDd = 11;
const int _pinX = 1;
const int _pinY = 2;
const int _pinA = 4;
const int _pinB = 20;
const int _pinZ = 0;
const int _pinS = 19;
int _pinZSwappable = _pinZ;
int _pinXSwappable = _pinX;
int _pinYSwappable = _pinY;
int _jumpConfig = 0;
int _cycleTime = 0;
int _cycleStep = 0;

///// Values used for dealing with snapback in the Kalman Filter, a 6th power relationship between distance to center and ADC/acceleration variance is used, this was arrived at by trial and error
//const float _accelVarFast = 0.5; //governs the acceleration variation around the edge of the gate, higher value means less filtering
const float _accelVarFast = 0.1;
const float _accelVarSlow = 0.01; //governs the acceleration variation with the stick centered, higher value means less filtering
const float _ADCVarFast = 0.1; //governs the ADC variation around the edge of the gate, higher vaule means more filtering
const float _ADCVarMax = 10; //maximum allowable value for the ADC variation
const float _ADCVarMin = 0.1; //minimum allowable value for the ADC variation
const float _varScale = 0.1;
const float _varOffset = 0;
const float _x1 = 100; //maximum stick distance from center
const float _x6 = _x1*_x1*_x1*_x1*_x1*_x1; //maximum distance to the 6th power
const float _aAccelVar = (_accelVarFast - _accelVarSlow)/_x6; //first coefficient used to calculate the actual acceleration variation
const float _bAccelVar = _accelVarSlow; //Second coefficient used to calculate the actual acceleration variation
const float _damping = 1; //amount of damping used in the stick model
//ADC variance is used to compensate for differing amounts of snapback, so it is not a constant, can be set by user
float _ADCVarSlowX = 0.2; //default value
float _ADCVarSlowY = 0.2; //default value
float _aADCVarX = (_ADCVarFast - _ADCVarSlowX)/_x6; //first coefficient used to calculate the actual ADC variation
float _bADCVarX = _ADCVarSlowX; //second coefficient used to calculate the actual ADC variation
float _aADCVarY = (_ADCVarFast - _ADCVarSlowY)/_x6; //first coefficient used to calculate the actual ADC variation
float _bADCVarY = _ADCVarSlowY; //second coefficient used to calculate the actual ADC variation
float _podeThreshX = 0.5;
float _podeThreshY = 0.5;
float _velFilterX = 0;
float _velFilterY = 0;


//////values used for calibration
const int _noOfNotches = 16;
const int _noOfCalibrationPoints = _noOfNotches * 2;
float _ADCScale = 1;
float _ADCScaleFactor = 1;
const int _notCalibrating = -1;
const float _maxStickAngle = 0.67195176201;
bool	_calAStick = true; //determines which stick is being calibrated (if false then calibrate the c-stick)
int _currentCalStep; //keeps track of which caliblration step is active, -1 means calibration is not running
bool _notched = false; //keeps track of whether or not the controller has firefox notches
const int _calibrationPoints = _noOfNotches+1; //number of calibration points for the c-stick and a-stick for a controller without notches
float _cleanedPointsX[_noOfNotches+1]; //array to hold the x coordinates of the stick positions for calibration
float _cleanedPointsY[_noOfNotches+1]; //array to hold the y coordinates of the stick positions for calibration
float _notchPointsX[_noOfNotches+1]; //array to hold the x coordinates of the notches for calibration
float _notchPointsY[_noOfNotches+1]; //array to hold the x coordinates of the notches for calibration
const float _cDefaultCalPointsX[_noOfCalibrationPoints] = {0.507073712,0.9026247224,0.5072693007,0.5001294236,0.5037118952,0.8146074226,0.5046028951,0.5066508636,0.5005339326,0.5065670067,0.5006805723,0.5056853599,0.5058308703,0.1989667596,0.5009560613,0.508400395,0.507729394,0.1003568119,0.5097473849,0.5074989796,0.5072406293,0.2014042034,0.5014653263,0.501119675,0.502959011,0.5032433665,0.5018446562,0.5085523857,0.5099732513,0.8100862401,0.5089320995,0.5052066109};
const float _cDefaultCalPointsY[_noOfCalibrationPoints] = {0.5006151799,0.5025356503,0.501470528,0.5066983468,0.5008275958,0.8094667357,0.5008874968,0.5079207909,0.5071239815,0.9046004275,0.5010136589,0.5071086316,0.5058914031,0.8076523013,0.5078213507,0.5049117887,0.5075638281,0.5003774649,0.504562192,0.50644895,0.5074859854,0.1983865682,0.5074515232,0.5084323402,0.5015846608,0.1025902875,0.5043605453,0.5070589342,0.5073953693,0.2033337702,0.5005351734,0.5056548782};
const float _aDefaultCalPointsX[_noOfCalibrationPoints] = {0.3010610568,0.3603937084,0.3010903951,0.3000194135,0.3005567843,0.3471911134,0.3006904343,0.3009976295,0.3000800899,0.300985051,0.3001020858,0.300852804,0.3008746305,0.2548450139,0.3001434092,0.3012600593,0.3011594091,0.2400535218,0.3014621077,0.3011248469,0.3010860944,0.2552106305,0.3002197989,0.3001679513,0.3004438517,0.300486505,0.3002766984,0.3012828579,0.3014959877,0.346512936,0.3013398149,0.3007809916};
const float _aDefaultCalPointsY[_noOfCalibrationPoints] = {0.300092277,0.3003803475,0.3002205792,0.301004752,0.3001241394,0.3464200104,0.3001331245,0.3011881186,0.3010685972,0.3606900641,0.3001520488,0.3010662947,0.3008837105,0.3461478452,0.3011732026,0.3007367683,0.3011345742,0.3000566197,0.3006843288,0.3009673425,0.3011228978,0.2547579852,0.3011177285,0.301264851,0.3002376991,0.2403885431,0.3006540818,0.3010588401,0.3011093054,0.2555000655,0.300080276,0.3008482317};
const float _notchAngleDefaults[_noOfNotches] = {0,M_PI/8.0,M_PI*2/8.0,M_PI*3/8.0,M_PI*4/8.0,M_PI*5/8.0,M_PI*6/8.0,M_PI*7/8.0,M_PI*8/8.0,M_PI*9/8.0,M_PI*10/8.0,M_PI*11/8.0,M_PI*12/8.0,M_PI*13/8.0,M_PI*14/8.0,M_PI*15/8.0};
const float _notchRange[_noOfNotches] = {0,M_PI*1/16.0,M_PI/16.0,M_PI*1/16.0,0,M_PI*1/16.0,M_PI/16.0,M_PI*1/16.0,0,M_PI*1/16.0,M_PI/16.0,M_PI*1/16.0,0,M_PI*1/16.0,M_PI/16.0,M_PI*1/16.0};
const int _notchStatusDefaults[_noOfNotches] = {3,1,2,1,3,1,2,1,3,1,2,1,3,1,2,1};
float _aNotchAngles[_noOfNotches] = {0,M_PI/8.0,M_PI*2/8.0,M_PI*3/8.0,M_PI*4/8.0,M_PI*5/8.0,M_PI*6/8.0,M_PI*7/8.0,M_PI*8/8.0,M_PI*9/8.0,M_PI*10/8.0,M_PI*11/8.0,M_PI*12/8.0,M_PI*13/8.0,M_PI*14/8.0,M_PI*15/8.0};
int _aNotchStatus[_noOfNotches] = {3,1,2,1,3,1,2,1,3,1,2,1,3,1,2,1};
float _cNotchAngles[_noOfNotches];
int _cNotchStatus[_noOfNotches] = {3,1,2,1,3,1,2,1,3,1,2,1,3,1,2,1};
const int _cardinalNotch = 3;
const int _secondaryNotch = 2;
const int _tertiaryNotchActive = 1;
const int _tertiaryNotchInactive = 0;
const int _fitOrder = 3; //fit order used in the linearization step
float _aFitCoeffsX[_fitOrder+1]; //coefficients for linearizing the X axis of the a-stick
float _aFitCoeffsY[_fitOrder+1]; //coefficients for linearizing the Y axis of the a-stick
float _cFitCoeffsX[_fitOrder+1]; //coefficients for linearizing the Y axis of the c-stick
float _cFitCoeffsY[_fitOrder+1]; //coefficients for linearizing the Y axis of the c-stick
float _aAffineCoeffs[_noOfNotches][6]; //affine transformation coefficients for all regions of the a-stick
float _cAffineCoeffs[_noOfNotches][6]; //affine transformation coefficients for all regions of the c-stick
float _aBoundaryAngles[_noOfNotches]; //angles at the boundaries between regions of the a-stick
float _cBoundaryAngles[_noOfNotches]; //angles at the boundaries between regions of the c-stick
float _tempCalPointsX[(_noOfNotches)*2]; //temporary storage for the x coordinate points collected during calibration before the are cleaned and put into _cleanedPointsX
float _tempCalPointsY[(_noOfNotches)*2]; //temporary storage for the y coordinate points collected during calibration before the are cleaned and put into _cleanedPointsY

//index values to store data into eeprom
const int _bytesPerFloat = 4;
const int _eepromAPointsX = 0;
const int _eepromAPointsY = _eepromAPointsX+_noOfCalibrationPoints*_bytesPerFloat;
const int _eepromCPointsX = _eepromAPointsY+_noOfCalibrationPoints*_bytesPerFloat;
const int _eepromCPointsY = _eepromCPointsX+_noOfCalibrationPoints*_bytesPerFloat;
const int _eepromADCVarX = _eepromCPointsY+_noOfCalibrationPoints*_bytesPerFloat;
const int _eepromADCVarY = _eepromADCVarX+_bytesPerFloat;
const int _eepromJump = _eepromADCVarY+_bytesPerFloat;
const int _eepromANotchAngles = _eepromJump+_bytesPerFloat;
const int _eepromCNotchAngles = _eepromANotchAngles+_noOfNotches*_bytesPerFloat;


Bounce bounceDr = Bounce();
Bounce bounceDu = Bounce(); 
Bounce bounceDl = Bounce(); 
Bounce bounceDd = Bounce();

ADC *adc = new ADC();

union Buttons{
	uint8_t arr[10];
	struct {
		
				// byte 0
		uint8_t A : 1;
		uint8_t B : 1;
		uint8_t X : 1;
		uint8_t Y : 1;
		uint8_t S : 1;
		uint8_t orig : 1;
		uint8_t errL : 1;
		uint8_t errS : 1;
		
		// byte 1
		uint8_t Dl : 1;
		uint8_t Dr : 1;
		uint8_t Dd : 1;
		uint8_t Du : 1;
		uint8_t Z : 1;
		uint8_t R : 1;
		uint8_t L : 1;
		uint8_t high : 1;

		//byte 2-7
		uint8_t Ax : 8;
		uint8_t Ay : 8;
		uint8_t Cx : 8;
		uint8_t Cy : 8;
		uint8_t La : 8;
		uint8_t Ra : 8;

		// magic byte 8 & 9 (only used in origin cmd)
		// have something to do with rumble motor status???
		// ignore these, they are magic numbers needed
		// to make a cmd response work
		uint8_t magic1 : 8;
		uint8_t magic2 : 8;
	};
}btn;

float _aStickX;
float _aStickLastX;
float _aStickY;
float _aStickLastY;
float _cStickX;
float _cStickY;

unsigned int _lastMicros;
float _dT;
bool _running = false;

VectorXf _xState(2);
VectorXf _yState(2);
MatrixXf _xP(2,2);
MatrixXf _yP(2,2);

////Serial settings
bool _writing = false;
bool _waiting = false;
int _bitQueue = 8;
int _waitQueue = 0;
int _writeQueue = 0;
uint8_t _cmdByte = 0;
const int _fastBaud = 2500000;
const int _slowBaud = 2000000;
const int _probeLength = 25;
const int _originLength = 81;
const int _pollLength = 65;
static char _serialBuffer[128];
int _errorCount = 0;
int _reportCount = 0;

const char _probeResponse[_probeLength] = {
	0,0,0,0, 1,0,0,1,
	0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,1,1,
	1};
const char _originResponse[_originLength] = {
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,1,1,1,1,1,1,1,
0,1,1,1,1,1,1,1,
0,1,1,1,1,1,1,1,
0,1,1,1,1,1,1,1,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
1};
volatile char _pollResponse[_originLength] = {
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,1,1,1,1,1,1,1,
0,1,1,1,1,1,1,1,
0,1,1,1,1,1,1,1,
0,1,1,1,1,1,1,1,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
1};


/* volatile char _bitCount = 0;
volatile bool _probe = false;
volatile bool _pole = false;
volatile int _commStatus = 0;
static int _commIdle = 0;
static int _commRead = 1;
static int _commPoll = 2;
static int _commWrite = 3; */

void setup() {
	
	//start USB serial
	Serial.begin(115200);
	//Serial.println("Software version b1.1s0.1 (hopefully Phobos remembered to update this message)");
	Serial.println("This is not a stable version");
	delay(1000);

	readEEPROM();
	
	//set some of the unused values in the message response
	btn.errS = 0;
	btn.errL = 0;
	btn.orig = 0;
	btn.high = 1;
	
	_currentCalStep = _notCalibrating;

	
	_xState << 0,0;
	_yState << 0,0;
	_xP << 1000,0,0,1000;
	_yP << 1000,0,0,1000;
	_lastMicros = micros();
	
	//analogReference(1);
	
	//adc->adc0->setReference(ADC_REFERENCE::REF_EXT);
	adc->adc0->setAveraging(8); // set number of averages
  adc->adc0->setResolution(12); // set bits of resolution
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::MED_SPEED ); // change the conversion speed
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_LOW_SPEED ); // change the sampling speed
/* 
  adc->adc1->setAveraging(32); // set number of averages
  adc->adc1->setResolution(16); // set bits of resolution
  adc->adc1->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED ); // change the conversion speed
  adc->adc1->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_LOW_SPEED ); // change the sampling speed
   */
	setPinModes();
	attachInterrupt(7,commInt,RISING);
	
	
/* 
	VREF::start();
		
	double refVoltage = 0;
	for(int i = 0; i < 512; i++){
		int value = adc->adc1->analogRead(ADC_INTERNAL_SOURCE::VREF_OUT);
		double volts = value*3.3/(float)adc->adc1->getMaxValue();
		refVoltage += volts;
	}
	refVoltage = refVoltage/512.0;
	

	Serial.print("the reference voltage read was:");
	Serial.println(refVoltage,8);
	_ADCScale = 1.2/refVoltage;
	
	adc->adc1->setAveraging(4); // set number of averages
  adc->adc1->setResolution(12); // set bits of resolution
  adc->adc1->setConversionSpeed(ADC_CONVERSION_SPEED::MED_SPEED ); // change the conversion speed
  adc->adc1->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_LOW_SPEED ); // change the sampling speed
	
	_ADCScaleFactor = 0.001*1.2*adc->adc1->getMaxValue()/3.3; */
	
	//_slowBaud = findFreq();
  //serialFreq = 950000;
	//Serial.print("starting hw serial at freq:");
	//Serial.println(_slowBaud);
	//start hardware serial
	Serial2.addMemoryForRead(_serialBuffer,128);
	Serial2.begin(_slowBaud,SERIAL_HALF_DUPLEX);
	//UART1_C2 &= ~UART_C2_RE;
	//attach the interrupt which will call the communicate function when the data line transitions from high to low

	timer1.begin(resetSerial);
	//timer2.begin(checkCmd);
	//timer3.begin(writePole);
	//digitalWriteFast(12,HIGH);
	//ARM_DEMCR |= ARM_DEMCR_TRCENA;
	//ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
	//attachInterrupt(_pinRX, bitCounter, FALLING);
	//NVIC_SET_PRIORITY(IRQ_GPIO6789, 0);
	_cycleTime = millis();
}

void loop() {
	//read the controllers buttons
	readButtons();
	//read the analog inputs
	//digitalWriteFast(13,HIGH);
	readSticks();
	//digitalWriteFast(13,LOW);
	//check to see if we are calibrating
	if(_currentCalStep >= 0){
		if(_calAStick){
			adjustNotch(_currentCalStep,_dT,btn.Y,btn.X,true,_aNotchAngles,_aNotchStatus);
		}
		else{
			adjustNotch(_currentCalStep,_dT,btn.Y,btn.X,false,_cNotchAngles,_cNotchStatus);
		}
			
	}
	//check if we should be reporting values yet
	if(btn.B && !_running){
		Serial.println("Starting to report values");
		_running=true;
	}
	//update the pole message so new data will be sent to the gamecube
	//if(_running){
	//cycleInputs();
	setPole();
	//}

}

void cycleInputs(){
	unsigned int timeSinceStep = millis() - _cycleTime;
	
	btn.Ax = _cycleStep*10;
	btn.Ay = _cycleStep*10;
	btn.Cx = _cycleStep*10;
	btn.Cy = _cycleStep*10;
	btn.Ra = _cycleStep*10;
	btn.La = _cycleStep*10;
		
	btn.A = _cycleStep % 2;
	btn.B = _cycleStep % 2;
	btn.X = _cycleStep % 2;
	btn.Y = _cycleStep % 2;
	btn.Z = _cycleStep % 2;
	btn.L = _cycleStep % 2;
	btn.R = _cycleStep % 2;
	btn.S = _cycleStep % 2;
		
	if(timeSinceStep > 500){
		
		//Serial.println("cycleStep");
		
		_cycleTime =  millis();
		_cycleStep ++;
		if(_cycleStep > 25){
			_cycleStep = 0;
		}
	}
}
//commInt() will be called on every rising edge of a pulse that we receive
//we will check if we have the expected amount of serial data yet, if we do we will do something with it, if we don't we will do nothing and wait for the next rising edge to check again
void commInt() {
	digitalWriteFast(13,LOW);
	//check to see if we have the expected amount of data yet
	if(Serial2.available() >= _bitQueue){
		//check to see if we were waiting for a poll command to finish
		//if we are, we need to clear the data and send our poll response
		if(_waiting){
			//wait for the stop bit to be received
			while(Serial2.available() < _bitQueue){}
			//check to see if we just reset reportCount to 0, if we have then we will report the remainder of the poll response to the PC over serial
			if(_reportCount == 0){
				Serial.print("Poll: ");
				char myBuffer[128];
				for(int i = 0; i < _bitQueue; i++){
					myBuffer[i] = (Serial2.read() > 0b11110000)+48;
				}
				Serial.write(myBuffer,_bitQueue);
				Serial.println();
			}

			//clear any remaining data, set the waiting flag to false, and set the serial port to high speed to be ready to send our poll response
			Serial2.clear();
			_waiting = false;
			Serial2.begin(_fastBaud,SERIAL_HALF_DUPLEX);
			
			//set the writing flag to true, set our expected bit queue to the poll response length -1 (to account for the stop bit)
			//_writing = true;
			//_bitQueue = _pollLength-1;
			
			
			
			//write the poll response
			for(int i = 0; i<_pollLength; i++){
				if(_pollResponse[i]){
					//short low period = 1
					Serial2.write(0b11111100);
				}
				else{
					//long low period = 0
					Serial2.write(0b11000000);
				}
			}
			//start the timer to reset the the serial port when the response has been sent
			timer1.trigger(175);
			_bitQueue = 8;
			
		}
		else{
			//We are not writing a response or waiting for a poll response to finish, so we must have received the start of a new command	
			//increment the report count, will be used to only send a report every 64 commands to not overload the PC serial connection
			_reportCount++;
			if(_reportCount > 64){
				_reportCount = 0;
			}
			
			//clear the command byte of previous data
			_cmdByte = 0;
			
			//write the new data from the serial buffer into the command byte
			for(int i = 0; i<8; i++){
				_cmdByte = (_cmdByte<<1) | (Serial2.read() > 0b11110000);

			}
			
			//if we just reset reportCount, report the command we received and the number of strange commands we've seen so far over serial
			if(_reportCount==0){
				Serial.print("Received: ");
				Serial.println(_cmdByte,BIN);
				Serial.print("Error Count:");
				Serial.println(_errorCount);
			}
			
			//if the command byte is all 0s it is probe command, we will send a probe response
			if(_cmdByte == 0b00000000){
				//wait for the stop bit to be received and clear it
				while(!Serial2.available()){}
				Serial2.clear();
				
				//switch the hardware serial to high speed for sending the response, set the _writing flag to true, and set the expected bit queue length to the probe response length minus 1 (to account for the stop bit)
				Serial2.begin(_fastBaud,SERIAL_HALF_DUPLEX);
				//_writing = true;
				//_bitQueue = _probeLength-1;
				
				//write the probe response
				for(int i = 0; i<_probeLength; i++){
					if(_probeResponse[i]){
						//short low period = 1
						Serial2.write(0b11111100);
					}
					else{
						//long low period = 0
						Serial2.write(0b11000000);
					}
				}
				resetSerial();
			}
			//if the command byte is 01000001 it is an origin command, we will send an origin response
			else if(_cmdByte == 0b01000001){
				//wait for the stop bit to be received and clear it
				while(!Serial2.available()){}
				Serial2.clear();
				
				//switch the hardware serial to high speed for sending the response, set the _writing flag to true, and set the expected bit queue length to the origin response length minus 1 (to account for the stop bit)
				Serial2.begin(_fastBaud,SERIAL_HALF_DUPLEX);
				//_writing = true;
				//_bitQueue = _originLength-1;
				
				//write the origin response
				for(int i = 0; i<_originLength; i++){
					if(_originResponse[i]){
						//short low period = 1
						Serial2.write(0b11111100);
					}
					else{
						//long low period = 0
						Serial2.write(0b11000000);
					}
				}
				resetSerial();
			}
			
			//if the command byte is 01000000 it is an poll command, we need to wait for the poll command to finish then send our poll response
			//to do this we will set our expected bit queue to the remaining length of the poll command, and wait until it is finished
			else if(_cmdByte == 0b01000000){
				_waiting = true;
				_bitQueue = 16;
			}
			//if we got something else then something went wrong, print the command we got and increase the error count
			else{
				Serial.print("error: ");
				Serial.println(_cmdByte,BIN);
				_errorCount ++;
				
				//we don't know for sure what state things are in, so clear, flush, and restart the serial port at low speed to be ready to receive a command
				resetSerial();
				//set our expected bit queue to 8, which will collect the first byte of any command we receive
				_bitQueue = 8;
			}
		}
	}
	//turn the LED back on to indicate we are not stuck
	digitalWriteFast(13,HIGH);
}
void resetSerial(){
	digitalWriteFast(13,!digitalReadFast(13));
	Serial2.clear();
	Serial2.flush();
	Serial2.begin(_slowBaud,SERIAL_HALF_DUPLEX);
	digitalWriteFast(13,!digitalReadFast(13));
}
void readEEPROM(){
	//get the jump setting
	EEPROM.get(_eepromJump, _jumpConfig);
	if(std::isnan(_jumpConfig)){
		_jumpConfig = 0;
	}
	setJump(_jumpConfig);
	
	//get the x-axis snapback filter settings
	EEPROM.get(_eepromADCVarX, _ADCVarSlowX);
	Serial.print("the _ADCVarSlowX value from eeprom is:");
	Serial.println(_ADCVarSlowX);
	if(std::isnan(_ADCVarSlowX)){
		_ADCVarSlowX = _ADCVarMin;
		Serial.print("the _ADCVarSlowX value was adjusted to:");
		Serial.println(_ADCVarSlowX);
	}
	if(_ADCVarSlowX >_ADCVarMax){
		_ADCVarSlowX = _ADCVarMax;
	}
	else if(_ADCVarSlowX < _ADCVarMin){
		_ADCVarSlowX = _ADCVarMin;
	}
	
	//get the y-axis snapback fitler settings
	EEPROM.get(_eepromADCVarY, _ADCVarSlowY);
	Serial.print("the _ADCVarSlowY value from eeprom is:");
	Serial.println(_ADCVarSlowY);
	if(std::isnan(_ADCVarSlowY)){
		_ADCVarSlowY = _ADCVarMin;
		Serial.print("the _ADCVarSlowY value was adjusted to:");
		Serial.println(_ADCVarSlowY);
	}
	if(_ADCVarSlowY >_ADCVarMax){
		_ADCVarSlowY = _ADCVarMax;
	}
	else if(_ADCVarSlowY < _ADCVarMin){
		_ADCVarSlowY = _ADCVarMin;
	}
	
	//set the snapback filtering
	setADCVar(&_aADCVarX, &_bADCVarX, _ADCVarSlowX);
	setADCVar(&_aADCVarY, &_bADCVarY, _ADCVarSlowY);
	
	//get the calibration points collected during the last A stick calibration
	EEPROM.get(_eepromAPointsX, _tempCalPointsX);
	EEPROM.get(_eepromAPointsY, _tempCalPointsY);
	EEPROM.get(_eepromANotchAngles, _aNotchAngles);
	cleanCalPoints(_tempCalPointsX,_tempCalPointsY,_aNotchAngles,_cleanedPointsX,_cleanedPointsY,_notchPointsX,_notchPointsY);
	Serial.println("calibration points cleaned");
	linearizeCal(_cleanedPointsX,_cleanedPointsY,_cleanedPointsX,_cleanedPointsY,_aFitCoeffsX,_aFitCoeffsY);
	Serial.println("A stick linearized");
	notchCalibrate(_cleanedPointsX,_cleanedPointsY, _notchPointsX, _notchPointsY, _noOfNotches, _aAffineCoeffs, _aBoundaryAngles);
	//stickCal(_cleanedPointsX,_cleanedPointsY,_aNotchAngles,_aFitCoeffsX,_aFitCoeffsY,_aAffineCoeffs,_aBoundaryAngles);
	
	//get the calibration points collected during the last A stick calibration
	EEPROM.get(_eepromCPointsX, _tempCalPointsX);
	EEPROM.get(_eepromCPointsY, _tempCalPointsY);
	EEPROM.get(_eepromCNotchAngles, _cNotchAngles);
	cleanCalPoints(_tempCalPointsX,_tempCalPointsY,_cNotchAngles,_cleanedPointsX,_cleanedPointsY,_notchPointsX,_notchPointsY);
	Serial.println("calibration points cleaned");
	linearizeCal(_cleanedPointsX,_cleanedPointsY,_cleanedPointsX,_cleanedPointsY,_cFitCoeffsX,_cFitCoeffsY);
	Serial.println("C stick linearized");
	notchCalibrate(_cleanedPointsX,_cleanedPointsY, _notchPointsX, _notchPointsY, _noOfNotches, _cAffineCoeffs, _cBoundaryAngles);
	//stickCal(_cleanedPointsX,_cleanedPointsY,_cNotchAngles,_cFitCoeffsX,_cFitCoeffsY,_cAffineCoeffs,_cBoundaryAngles);
}
void resetDefaults(){
	Serial.println("RESETTING ALL DEFAULTS");
	
	_jumpConfig = 0;
	setJump(_jumpConfig);
	EEPROM.put(_eepromJump,_jumpConfig);
	
	_ADCVarSlowX = _ADCVarMin;
	EEPROM.put(_eepromADCVarX,_ADCVarSlowX);
	_ADCVarSlowY = _ADCVarMin;
	EEPROM.put(_eepromADCVarY,_ADCVarSlowY);
	
	setADCVar(&_aADCVarX, &_bADCVarX, _ADCVarSlowX);
	setADCVar(&_aADCVarY, &_bADCVarY, _ADCVarSlowY);
	
	for(int i = 0; i < _noOfNotches; i++){
		_aNotchAngles[i] = _notchAngleDefaults[i];
		_cNotchAngles[i] = _notchAngleDefaults[i];
	}
	EEPROM.put(_eepromANotchAngles,_aNotchAngles);
	EEPROM.put(_eepromCNotchAngles,_cNotchAngles);
	
	for(int i = 0; i < _noOfCalibrationPoints; i++){
		_tempCalPointsX[i] = _aDefaultCalPointsX[i];
		_tempCalPointsY[i] = _aDefaultCalPointsY[i];
	}
	EEPROM.put(_eepromAPointsX,_tempCalPointsX);
	EEPROM.put(_eepromAPointsY,_tempCalPointsY);

	Serial.println("A calibration points stored in EEPROM");
	cleanCalPoints(_tempCalPointsX,_tempCalPointsY,_aNotchAngles,_cleanedPointsX,_cleanedPointsY,_notchPointsX,_notchPointsY);
	Serial.println("A calibration points cleaned");
	linearizeCal(_cleanedPointsX,_cleanedPointsY,_cleanedPointsX,_cleanedPointsY,_aFitCoeffsX,_aFitCoeffsY);
	Serial.println("A stick linearized");
	notchCalibrate(_cleanedPointsX,_cleanedPointsY, _notchPointsX, _notchPointsY, _noOfNotches, _aAffineCoeffs, _aBoundaryAngles);
	
	for(int i = 0; i < _noOfCalibrationPoints; i++){
		_tempCalPointsX[i] = _cDefaultCalPointsX[i];
		_tempCalPointsY[i] = _cDefaultCalPointsY[i];
	}
	EEPROM.put(_eepromCPointsX,_tempCalPointsX);
	EEPROM.put(_eepromCPointsY,_tempCalPointsY);
	
	Serial.println("C calibration points stored in EEPROM");
	cleanCalPoints(_tempCalPointsX,_tempCalPointsY,_cNotchAngles,_cleanedPointsX,_cleanedPointsY,_notchPointsX,_notchPointsY);
	Serial.println("C calibration points cleaned");
	linearizeCal(_cleanedPointsX,_cleanedPointsY,_cleanedPointsX,_cleanedPointsY,_cFitCoeffsX,_cFitCoeffsY);
	Serial.println("C stick linearized");
	notchCalibrate(_cleanedPointsX,_cleanedPointsY, _notchPointsX, _notchPointsY, _noOfNotches, _cAffineCoeffs, _cBoundaryAngles);
	
}
void setPinModes(){
	
	pinMode(_pinL,INPUT_PULLUP);
	pinMode(_pinR,INPUT_PULLUP);
	pinMode(_pinDr,INPUT_PULLUP);
	pinMode(_pinDu,INPUT_PULLUP);
	pinMode(_pinDl,INPUT_PULLUP);
	pinMode(_pinDd,INPUT_PULLUP);
	pinMode(_pinX,INPUT_PULLUP);
	pinMode(_pinY,INPUT_PULLUP);
	pinMode(_pinA,INPUT_PULLUP);
	pinMode(_pinB,INPUT_PULLUP);
	pinMode(_pinZ,INPUT_PULLUP);
	pinMode(_pinS,INPUT_PULLUP);
	pinMode(9,INPUT_PULLUP);
	pinMode(_pinS,INPUT_PULLUP);
	pinMode(13,OUTPUT);
	
	//pinMode(12,OUTPUT);

	
	bounceDr.attach(_pinDr);
	bounceDr.interval(1000);
	bounceDu.attach(_pinDu);
	bounceDu.interval(1000);
	bounceDl.attach(_pinDl);
	bounceDl.interval(1000);
	bounceDd.attach(_pinDd);
	bounceDd.interval(1000);
}
void readButtons(){
	btn.A = !digitalRead(_pinA);
	btn.B = !digitalRead(_pinB);
	btn.X = !digitalRead(_pinXSwappable);
	btn.Y = !digitalRead(_pinYSwappable);
	btn.Z = !digitalRead(_pinZSwappable);
	btn.S = !digitalRead(_pinS);
	btn.L = !digitalRead(_pinL);
	btn.R = !digitalRead(_pinR);
	btn.Du = !digitalRead(_pinDu);
	btn.Dd = !digitalRead(_pinDd);
	btn.Dl = !digitalRead(_pinDl);
	btn.Dr = !digitalRead(_pinDr);
	
	bounceDr.update();
	bounceDu.update();
	bounceDl.update();
	bounceDd.update();
	
	
	//check the dpad buttons to change the controller settings
	if(bounceDr.fell()){
		if(_currentCalStep == -1){
			Serial.println("Calibrating the C stick");
			_calAStick = false;
			_currentCalStep ++;
		}
		else if (!_calAStick){
			collectCalPoints(_calAStick, _currentCalStep,_tempCalPointsX,_tempCalPointsY);
			_currentCalStep ++;
			
			if(_currentCalStep >= _noOfNotches*2){
				Serial.println("finished collecting the calibration points for the C stick");
				EEPROM.put(_eepromCPointsX,_tempCalPointsX);
				EEPROM.put(_eepromCPointsY,_tempCalPointsY);
				EEPROM.put(_eepromCNotchAngles,_cNotchAngles);
				Serial.println("calibration points stored in EEPROM");
				cleanCalPoints(_tempCalPointsX,_tempCalPointsY,_cNotchAngles,_cleanedPointsX,_cleanedPointsY,_notchPointsX,_notchPointsY);
				Serial.println("calibration points cleaned");
				linearizeCal(_cleanedPointsX,_cleanedPointsY,_cleanedPointsX,_cleanedPointsY,_cFitCoeffsX,_cFitCoeffsY);
				Serial.println("C stick linearized");
				notchCalibrate(_cleanedPointsX,_cleanedPointsY, _notchPointsX, _notchPointsY, _noOfNotches, _cAffineCoeffs, _cBoundaryAngles);
				_currentCalStep = -1;
			}
		}
	}
	else if(bounceDl.fell()){
		if(_currentCalStep == -1){
			if(btn.S == true){
				resetDefaults();
			}
			else{
				Serial.println("Calibrating the A stick");
				_calAStick = true;
				_currentCalStep ++;
			}
		}
		else if (_calAStick){
			collectCalPoints(_calAStick, _currentCalStep,_tempCalPointsX,_tempCalPointsY);
			_currentCalStep ++;
			
			if(_currentCalStep >= _noOfNotches*2){
				Serial.println("finished collecting the calibration points for the A stick");
				EEPROM.put(_eepromAPointsX,_tempCalPointsX);
				EEPROM.put(_eepromAPointsY,_tempCalPointsY);
				EEPROM.put(_eepromANotchAngles,_aNotchAngles);
				Serial.println("calibration points stored in EEPROM");
				cleanCalPoints(_tempCalPointsX,_tempCalPointsY,_aNotchAngles,_cleanedPointsX,_cleanedPointsY,_notchPointsX,_notchPointsY);
				Serial.println("calibration points cleaned");
				linearizeCal(_cleanedPointsX,_cleanedPointsY,_cleanedPointsX,_cleanedPointsY,_aFitCoeffsX,_aFitCoeffsY);
				Serial.println("A stick linearized");
				notchCalibrate(_cleanedPointsX,_cleanedPointsY, _notchPointsX, _notchPointsY, _noOfNotches, _aAffineCoeffs, _aBoundaryAngles);
				_currentCalStep = -1;
			}
		}
	}
	else if(bounceDu.fell()){
		adjustSnapback(btn.Cx,btn.Cy);
	}
	else if(bounceDd.fell()){
		readJumpConfig();
	}
	/* 
	bool dPad = (btn.Dl || btn.Dr);
	
	if(dPad && !_lastDPad){
		_dPadSince = millis();
		_watchingDPad = true;
	}
	else if(dPad && _watchingDPad){
		int startTimer = millis()- _dPadSince;
		if(startTimer > 1000){
			if(_currentCalStep == -1){
				if(btn.Dl){
					_calAStick = true;
				}
				else{
					_calAStick = false;
				}
			}
			_currentCalStep ++;
			_watchingDPad = false;
			Serial.println("calibrating");
			Serial.println(_currentCalStep);
		}
	}
	_lastDPad = dPad; */
}
void adjustSnapback(int cStickX, int cStickY){
	Serial.println("adjusting snapback filtering");
	if(cStickX > 127+50){
		_ADCVarSlowX = _ADCVarSlowX*1.1;
		Serial.print("X filtering increased to:");
		Serial.println(_ADCVarSlowX);
	}
	else if(cStickX < 127-50){
		_ADCVarSlowX = _ADCVarSlowX*0.90909090909;
		Serial.print("X filtering decreased to:");
		Serial.println(_ADCVarSlowX);
	}
	if(_ADCVarSlowX >_ADCVarMax){
		_ADCVarSlowX = _ADCVarMax;
	}
	else if(_ADCVarSlowX < _ADCVarMin){
		_ADCVarSlowX = _ADCVarMin;
	}
	
	if(cStickY > 127+50){
		_ADCVarSlowY = _ADCVarSlowY*1.1;
		Serial.print("Y filtering increased to:");
		Serial.println(_ADCVarSlowY);
	}
	else if(cStickY < 127-50){
		_ADCVarSlowY = _ADCVarSlowY*0.9;
		Serial.print("Y filtering decreased to:");
		Serial.println(_ADCVarSlowY);
	}
	if(_ADCVarSlowY >_ADCVarMax){
		_ADCVarSlowY = _ADCVarMax;
	}
	else if(_ADCVarSlowY < _ADCVarMin){
		_ADCVarSlowY = _ADCVarMin;
	}
	
	Serial.println("Var scale parameters");
	Serial.println(_varScale);
	Serial.println(_varOffset);
	
	float xVarDisplay = 10*(log( _varScale*_ADCVarSlowX + _varOffset)+4.60517);
	float yVarDisplay = 10*(log( _varScale*_ADCVarSlowY + _varOffset)+4.60517);
	
	Serial.println("Var display results");
		Serial.println(xVarDisplay);
	Serial.println(yVarDisplay);
	
	
	btn.Cx = (uint8_t) (xVarDisplay + 127.5);
	btn.Cy = (uint8_t) (yVarDisplay + 127.5);
	
	setPole();
	
	int startTime = millis();
	int delta = 0;
	while(delta < 2000){
		delta = millis() - startTime;
	}
	
	setADCVar(&_aADCVarX, &_bADCVarX, _ADCVarSlowX);
	setADCVar(&_aADCVarY, &_bADCVarY, _ADCVarSlowY);
	
	EEPROM.put(_eepromADCVarX,_ADCVarSlowX);
	EEPROM.put(_eepromADCVarY,_ADCVarSlowY);
}
void readJumpConfig(){
	Serial.print("setting jump to: ");
	if(!digitalRead(_pinX)){
		_jumpConfig = 1;
		Serial.println("X<->Z");
	}
	else if(!digitalRead(_pinY)){
		_jumpConfig = 2;
		Serial.println("Y<->Z");
	}
	else{
		Serial.println("normal");
		_jumpConfig = 0;
	}
	EEPROM.put(_eepromJump,_jumpConfig);
	setJump(_jumpConfig);
}
void setJump(int jumpConfig){
	switch(jumpConfig){
			case 1:
				_pinZSwappable = _pinX;
				_pinXSwappable = _pinZ;
				_pinYSwappable = _pinY;
				break;				
			case 2:
				_pinZSwappable = _pinY;
				_pinXSwappable = _pinX;
				_pinYSwappable = _pinZ;
				break;
			default:
				_pinZSwappable = _pinZ;
				_pinXSwappable = _pinX;
				_pinYSwappable = _pinY;
	}
}
void setADCVar(float* aADCVar,float* bADCVar, float ADCVarSlow){
	*aADCVar = (_ADCVarFast - ADCVarSlow)/_x6;
	*bADCVar = ADCVarSlow;
}
void readSticks(){
	 //_ADCScale = _ADCScale*0.999 + _ADCScaleFactor/adc->adc1->analogRead(ADC_INTERNAL_SOURCE::VREF_OUT);

	//read the analog stick, scale it down so that we don't get huge values when we linearize
	//_aStickX = adc->adc0->analogRead(_pinAx)/4096.0*_ADCScale;
	//_aStickY = adc->adc0->analogRead(_pinAy)/4096.0*_ADCScale;
	_aStickX = adc->adc0->analogRead(_pinAx)/4096.0;
	_aStickY = adc->adc0->analogRead(_pinAy)/4096.0;
/* 	Serial.print(_aStickX);
	Serial.print(",");
	Serial.print(_aStickY);
	Serial.print(",");
	Serial.println(_ADCScale); */
	
	
	//read the L and R sliders
	btn.La = adc->adc0->analogRead(_pinLa)>>4;
	btn.Ra = adc->adc0->analogRead(_pinRa)>>4;
	
	//read the C stick
	//btn.Cx = adc->adc0->analogRead(pinCx)>>4;
	//btn.Cy = adc->adc0->analogRead(pinCy)>>4;

	
	//read the c stick, scale it down so that we don't get huge values when we linearize 
	_cStickX = (_cStickX + adc->adc0->analogRead(_pinCx)/4096.0)*0.5;
	_cStickY = (_cStickY + adc->adc0->analogRead(_pinCy)/4096.0)*0.5;
	
	//Serial.print(_cStickX);
	//Serial.print(",");
	//Serial.println(_cStickY);

	
	//create the measurement vector to be used in the kalman filter
	VectorXf xZ(1);
	VectorXf yZ(1);
	
	//linearize the analog stick inputs by multiplying by the coefficients found during calibration (3rd order fit)
	//store in the measurement vectors
	//xZ << (_aFitCoeffsX[0]*(_aStickX*_aStickX*_aStickX) + _aFitCoeffsX[1]*(_aStickX*_aStickX) + _aFitCoeffsX[2]*_aStickX + _aFitCoeffsX[3]);
	//yZ << (_aFitCoeffsY[0]*(_aStickY*_aStickY*_aStickY) + _aFitCoeffsY[1]*(_aStickY*_aStickY) + _aFitCoeffsY[2]*_aStickY + _aFitCoeffsY[3]);
	xZ << linearize(_aStickX,_aFitCoeffsX);
	yZ << linearize(_aStickY,_aFitCoeffsY);
	
	//float posCx = (_cFitCoeffsX[0]*(_cStickX*_cStickX*_cStickX) + _cFitCoeffsX[1]*(_cStickX*_cStickX) + _cFitCoeffsX[2]*_cStickX + _cFitCoeffsX[3]);
	//float posCy = (_aFitCoeffsY[1]*(_cStickY*_cStickY*_cStickY) + _aFitCoeffsY[1]*(_cStickY*_cStickY) + _aFitCoeffsY[2]*_cStickY + _aFitCoeffsY[3]);
  float posCx = linearize(_cStickX,_cFitCoeffsX);
	float posCy = linearize(_cStickY,_cFitCoeffsY);
	
	//Run the kalman filter to eliminate snapback
	runKalman(xZ,yZ);
/* 	Serial.println();
	Serial.print(xZ[0]);
	Serial.print(",");
	Serial.print(yZ[0]);
	Serial.print(",");
	Serial.print(_xState[0]);
	Serial.print(",");
	Serial.print(_yState[0]);
 */
	float posAx;
	float posAy;
	//float posCx;
	//float posCy;

	notchRemap(_xState[0],_yState[0], &posAx,  &posAy, _aAffineCoeffs, _aBoundaryAngles,_noOfNotches);
	notchRemap(posCx,posCy, &posCx,  &posCy, _cAffineCoeffs, _cBoundaryAngles,_noOfNotches);
	
	float filterWeight = 0.8;
	_velFilterX = filterWeight*_velFilterX + (1-filterWeight)*(posAx-_aStickLastX)/_dT;
	_velFilterY = filterWeight*_velFilterY + (1-filterWeight)*(posAy-_aStickLastY)/_dT;
	//assign the remapped values to the button struct
	if(_running){
		
/* 		if((_velFilterX < _podeThreshX) && (_velFilterX > -_podeThreshX)){
			btn.Ax = (uint8_t) (posAx+127.5);
		}
		
		if((_velFilterY < _podeThreshY) && (_velFilterY > -_podeThreshY)){
			btn.Ay = (uint8_t) (posAy+127.5);
		} */
	
		btn.Ax = (uint8_t) (posAx+127.5);
		btn.Ay = (uint8_t) (posAy+127.5);
		btn.Cx = (uint8_t) (posCx+127.5);
		btn.Cy = (uint8_t) (posCy+127.5);
	}
	else
	{
		btn.Ax = 127;
		btn.Ay = 127;
		btn.Cx = 127;
		btn.Cy = 127;
	}
	
	_aStickLastX = posAx;
	_aStickLastY = posAy;
	
	//Serial.println();
	//Serial.print(_dT/16.7);
	//Serial.print(",");
	//Serial.print(xZ[0],8);
	//Serial.print(",");
	//Serial.print(_velFilterX*10,8);
	//Serial.print(",");
	//Serial.print(yZ[0],8);
	//Serial.print(",");
	//Serial.print((posAx+127.5),8);
	//Serial.print(",");
	//Serial.print((posAy+127.5),8);
	//Serial.print(",");
	//Serial.print(btn.Cx);
	//Serial.print(",");
	//Serial.println(btn.Cy);
}
/*******************
	notchRemap
	Remaps the stick position using affine transforms generated from the notch positions
*******************/
void notchRemap(float xIn, float yIn, float* xOut, float* yOut, float affineCoeffs[][6], float regionAngles[], int regions){
	//determine the angle between the x unit vector and the current position vector
	float angle = atan2f(yIn,xIn);
	
	//unwrap the angle based on the first region boundary
	if(angle < regionAngles[0]){
		angle += M_PI*2;
	}
	
	//go through the region boundaries from lowest angle to highest, checking if the current position vector is in that region
	//if the region is not found then it must be between the first and the last boundary, ie the last region
	int region = regions-1;
	for(int i = 1; i < regions; i++){
		if(angle < regionAngles[i]){
			region = i-1;
			break;
		}
	}

	//Apply the affine transformation using the coefficients found during calibration
	*xOut = affineCoeffs[region][0]*xIn + affineCoeffs[region][1]*yIn + affineCoeffs[region][2];
	*yOut = affineCoeffs[region][3]*xIn + affineCoeffs[region][4]*yIn + affineCoeffs[region][5];
	
	if((abs(*xOut)<5) && (abs(*yOut)>95)){
		*xOut = 0;
	}
	if((abs(*yOut)<5) && (abs(*xOut)>95)){
		*yOut = 0;
	}
}
/*******************
	setPole
	takes the values that have been put into the button struct and translates them in the serial commands ready
	to be sent to the gamecube/wii
*******************/
void setPole(){
	for(int i = 0; i < 8; i++){
		//write all of the data in the button struct (taken from the dogebawx project, thanks to GoodDoge)
		for(int j = 0; j < 8; j++){
			_pollResponse[i*8+j] = btn.arr[i]>>(7-j) & 1;
		}
	}
}
/*******************
	communicate
	try to communicate with the gamecube/wii
*******************/
/* 
void bitCounter(){
	_bitCount ++;
	//digitalWriteFast(12,!(_bitCount%2));
	//Serial.println('b');
	if(_bitCount == 1){
		timer1.trigger((CMD_LENGTH_SHORT-1)*20);
		_commStatus = _commRead;
	}
}
void communicate(){
	//Serial.println(_commStatus,DEC);
	digitalWriteFast(12,LOW);
	Serial.println(Serial2.read(),HEX);
	Serial2.write(0xEF);
	delayMicroseconds(10);
	digitalWriteFast(12,HIGH);
 	if(_commStatus == _commRead){
		
		//Serial.println(Serial2.available(),DEC);
		while(Serial2.available() < (CMD_LENGTH_SHORT-1)){}
			//digitalWriteFast(12,HIGH);
			cmdByte = 0;
			for(int i = 0; i < CMD_LENGTH_SHORT-1; i++){
				cmd[i] = Serial2.read();
				//Serial.println(cmd[i],BIN);
				switch(cmd[i]){
				case 0x08:
					cmdByte = (cmdByte<<2);
					break;
				case 0xE8:
					cmdByte = (cmdByte<<2)+1;
					break;
 				case 0xC8:
					cmdByte = (cmdByte<<2)+1;
					break;
				case 0x0F:
					cmdByte = (cmdByte<<2)+2;
					break;
				case 0x0E:
					cmdByte = (cmdByte<<2)+2;
					break;
				case 0xEF:
					cmdByte = (cmdByte<<2)+3;
					break;
				case 0xCF:
					cmdByte = (cmdByte<<2)+3;
					break;
				case 0xEE:
					cmdByte = (cmdByte<<2)+3;
					break;
				case 0xCE:
					cmdByte = (cmdByte<<2)+3;
					break;
				default:
					//got garbage data or a stop bit where it shouldn't be
					Serial.println(cmd[i],BIN);
					cmdByte = -1;
					//Serial.println('o');
				}
				if(cmdByte == -1){
					Serial.println('b');
					break;
				}
			}
		
		Serial.println(cmdByte,HEX);
		//setSerialFast();
		//digitalWriteFast(12,HIGH);
		Serial2.begin(_fastBaud);
		
		switch(cmdByte){
		case 0x00:
			//digitalWriteFast(12,LOW);
			timer1.trigger(PROBE_LENGTH*8);
			//Serial2.write(probeResponse,PROBE_LENGTH);
			for(int i = 0; i< PROBE_LENGTH; i++){
				Serial2.write(probeResponse[i]);
			}
			Serial.println("probe");
			_writeQueue = 9+(PROBE_LENGTH-1)*2+1;
			_commStatus = _commWrite;
			//digitalWriteFast(12,HIGH);
		break;
		case 0x41:
			timer1.trigger(ORIGIN_LENGTH*8);
			//Serial2.write(originResponse,ORIGIN_LENGTH);
			for(int i = 0; i< ORIGIN_LENGTH; i++){
				Serial2.write(originResponse[i]);
			}
			Serial.println("origin");
			_writeQueue = 9+(ORIGIN_LENGTH-1)*2+1;
			_commStatus = _commWrite;
		  break;
		case 0x40:
			timer1.trigger(56);
			_commStatus = _commPoll;
			break;
		default:
		  //got something strange, try waiting for a stop bit to syncronize
			//resetFreq();
			//digitalWriteFast(12,LOW);
			Serial.println("error");
			Serial.println(_bitCount,DEC);
			
			//setSerialSlow();
			Serial2.begin(_slowBaud);
			
			
			uint8_t thisbyte = 0;
		  while(thisbyte != 0xFF){
				while(!Serial2.available());
				thisbyte = Serial2.read();
				Serial.println(thisbyte,BIN);
			}
			//Serial2.clear();
			_commStatus = _commIdle;
			_bitCount = 0;
			_writeQueue = 0;
			//digitalWriteFast(12,HIGH);
	  }
		//digitalWriteFast(12,HIGH);
	}
	else if(_commStatus == _commPoll){
		//digitalWriteFast(12,LOW);
		while(_bitCount<25){}
		//Serial2.write((const char*)pollResponse,POLL_LENGTH);
		for(int i = 0; i< POLL_LENGTH; i++){
			Serial2.write(pollResponse[i]);
		}
		timer1.trigger(135);
		_writeQueue = 25+(POLL_LENGTH-1)*2+1;
		_commStatus = _commWrite;
		//digitalWriteFast(12,HIGH);
	}
	else if(_commStatus == _commWrite){
		//digitalWriteFast(12,LOW);
 		while(_writeQueue > _bitCount){}
		
		//setSerialSlow();
		Serial2.begin(_slowBaud);
		
		_bitCount = 0;
		_commStatus = _commIdle;
		_writeQueue = 0;
		//digitalWriteFast(12,HIGH);
	}
	else{
		Serial.println('a');
	} 
	//Serial.println(_commStatus,DEC);
}
 void setSerialFast(){
	UART1_BDH = _fastBDH;
	UART1_BDL = _fastBDL;
	UART1_C4 = _fastC4;
	UART1_C2 &= ~UART_C2_RE;
}
void setSerialSlow(){
	UART1_BDH = _slowBDH;
	UART1_BDL = _slowBDL;
	UART1_C4 = _slowC4;
	UART1_C2 |= UART_C2_RE;
	Serial2.clear();
}
 */
/*******************
	cleanCalPoints
	take the x and y coordinates and notch angles collected during the calibration procedure, and generate the cleaned x an y stick coordinates and the corresponding x and y notch coordinates
*******************/
void cleanCalPoints(float calPointsX[], float  calPointsY[], float notchAngles[], float cleanedPointsX[], float cleanedPointsY[], float notchPointsX[], float notchPointsY[]){
	
	Serial.println("The raw calibration points (x,y) are:");
	for(int i = 0; i< _noOfCalibrationPoints; i++){
		Serial.print(calPointsX[i]);
		Serial.print(",");
		Serial.println(calPointsY[i]);
	}
	
	Serial.println("The notch angles are:");
	for(int i = 0; i< _noOfNotches; i++){
		Serial.println(notchAngles[i]);
	}
	
	notchPointsX[0] = 0;
	notchPointsY[0] = 0;
	cleanedPointsX[0] = 0;
	cleanedPointsY[0] = 0;
	
	for(int i = 0; i < _noOfNotches; i++){
			//add the origin values toe the first x,y point
			cleanedPointsX[0] += calPointsX[i*2];
			cleanedPointsY[0] += calPointsY[i*2];
			
			//set the notch point
			cleanedPointsX[i+1] = calPointsX[i*2+1];
			cleanedPointsY[i+1] = calPointsY[i*2+1];
			
			calcStickValues(notchAngles[i], notchPointsX+i+1, notchPointsY+i+1);
		}
		

		//divide by the total number of calibration steps/2 to get the average origin value
		cleanedPointsX[0] = cleanedPointsX[0]/((float)_noOfNotches);
		cleanedPointsY[0] = cleanedPointsY[0]/((float)_noOfNotches);
		
	for(int i = 0; i < _noOfNotches; i++){
		float deltaX = cleanedPointsX[i+1] - cleanedPointsX[0];
		float deltaY = cleanedPointsY[i+1] - cleanedPointsY[0];
		float mag = sqrt(deltaX*deltaX + deltaY*deltaY);
		if(mag < 0.02){
			int prevIndex = (i-1+_noOfNotches) % _noOfNotches+1;
			int nextIndex = (i+1) % _noOfNotches+1;
			
			cleanedPointsX[i+1] = (cleanedPointsX[prevIndex] + cleanedPointsX[nextIndex])/2.0;
			cleanedPointsY[i+1] = (cleanedPointsY[prevIndex] + cleanedPointsY[nextIndex])/2.0;
			
			notchPointsX[i+1] = (notchPointsX[prevIndex] + notchPointsX[nextIndex])/2.0;
			notchPointsY[i+1] = (notchPointsY[prevIndex] + notchPointsY[nextIndex])/2.0;
			
			Serial.print("no input was found for notch: ");
			Serial.println(i+1);
		}
	}
	
	Serial.println("The cleaned calibration points are:");
	for(int i = 0; i< (_noOfNotches+1); i++){
		Serial.print(cleanedPointsX[i]);
		Serial.print(",");
		Serial.println(cleanedPointsY[i]);
	}
	
	Serial.println("The corresponding notch points are:");
	for(int i = 0; i< (_noOfNotches+1); i++){
		Serial.print(notchPointsX[i]);
		Serial.print(",");
		Serial.println(notchPointsY[i]);
	}
}
void adjustNotch(int currentStep, float loopDelta, bool CW, int CCW, bool calibratingAStick, float notchAngles[], int notchStatus[]){
	float X = 0;
	float Y = 0;
	//don't run on center steps
	if(currentStep%2){
		int notchIndex = currentStep/2;
		//Serial.println(notchAngles[notchIndex]);
		if(notchStatus[notchIndex] != _cardinalNotch){
			if(CW){
				notchAngles[notchIndex] += loopDelta*0.00005;
			}
			else if(CCW){
				notchAngles[notchIndex] -= loopDelta*0.00005;
			}
		}
		if(notchAngles[notchIndex] > _notchAngleDefaults[notchIndex]+_notchRange[notchIndex]){
			notchAngles[notchIndex] = _notchAngleDefaults[notchIndex]+_notchRange[notchIndex];
		}
		else if(notchAngles[notchIndex] < _notchAngleDefaults[notchIndex]-_notchRange[notchIndex]){
			notchAngles[notchIndex] = _notchAngleDefaults[notchIndex]-_notchRange[notchIndex];
		}
		calcStickValues(notchAngles[notchIndex], &X, &Y);
	}
	if(calibratingAStick){
		btn.Cx = (uint8_t) (X + 127.5);
		btn.Cy = (uint8_t) (Y + 127.5);
	}
	else{
		btn.Ax = (uint8_t) (X + 127.5);
		btn.Ay = (uint8_t) (Y + 127.5);
	}
}
void collectCalPoints(bool aStick, int currentStep, float calPointsX[], float calPointsY[]){
	
	Serial.print("Collecting cal point for step: ");
	Serial.println(currentStep);
	float X = 0;
	float Y = 0;
	
	for(int i = 0; i < 128; i++){
		if(aStick){
			//_ADCScale = _ADCScale*0.999 + _ADCScaleFactor/adc->adc1->analogRead(ADC_INTERNAL_SOURCE::VREF_OUT);
			//X += adc->adc0->analogRead(_pinAx)/4096.0*_ADCScale;
			//Y += adc->adc0->analogRead(_pinAy)/4096.0*_ADCScale;
			X += adc->adc0->analogRead(_pinAx)/4096.0;
			Y += adc->adc0->analogRead(_pinAy)/4096.0;
		}
		else{
			X += adc->adc0->analogRead(_pinCx)/4096.0;
			Y += adc->adc0->analogRead(_pinCy)/4096.0;
		}
	}
	
	calPointsX[currentStep] = X/128.0;
	calPointsY[currentStep] = Y/128.0;
	
	Serial.println("The collected coordinates are: ");
	Serial.println(calPointsX[currentStep],8);
	Serial.println(calPointsY[currentStep],8);
}
/*******************
	linearizeCal
	calibrate a stick so that its response will be linear
	Inputs:
		cleaned points X and Y, (must be 17 points for each of these, the first being the center, the others starting at 3 oclock and going around counterclockwise)
	Outputs:
		linearization fit coefficients X and Y
*******************/
void linearizeCal(float inX[],float inY[],float outX[], float outY[], float fitCoeffsX[],float fitCoeffsY[]){
	Serial.println("beginning linearization");
	
	//do the curve fit first
	//generate all the notched/not notched specific cstick values we will need
	
	double fitPointsX[5];
	double fitPointsY[5];
	
	fitPointsX[0] = inX[8+1];
	fitPointsX[1] = (inX[6+1] + inX[10+1])/2.0;
	fitPointsX[2] = inX[0];
	fitPointsX[3] = (inX[2+1] + inX[14+1])/2.0;
	fitPointsX[4] = inX[0+1];
	
	fitPointsY[0] = inY[12+1];
	fitPointsY[1] = (inY[10+1] + inY[14+1])/2.0;
	fitPointsY[2] = inY[0];
	fitPointsY[3] = (inY[6+1] + inY[2+1])/2.0;
	fitPointsY[4] = inY[4+1];
	
	
	//////determine the coefficients needed to linearize the stick
	//create the expected output, what we want our curve to be fit too
	//this is hard coded because it doesn't depend on the notch adjustments
	double x_output[5] = {27.5,53.2537879754,127.5,201.7462120246,227.5};
	double y_output[5] = {27.5,53.2537879754,127.5,201.7462120246,227.5};
	
	Serial.println("The fit input points are (x,y):");
	for(int i = 0; i < 5; i++){
		Serial.print(fitPointsX[i],8);
		Serial.print(",");
		Serial.println(fitPointsY[i],8);
	}
	
	Serial.println("The corresponding fit output points are (x,y):");
	for(int i = 0; i < 5; i++){
		Serial.print(x_output[i]);
		Serial.print(",");
		Serial.println(y_output[i]);
	}
	
	//perform the curve fit, order is 3
	double tempCoeffsX[_fitOrder+1];
	double tempCoeffsY[_fitOrder+1];

	fitCurve(_fitOrder, 5, fitPointsX, x_output, _fitOrder+1,  tempCoeffsX);
	fitCurve(_fitOrder, 5, fitPointsY, y_output, _fitOrder+1, tempCoeffsY);
	
		//write these coefficients to the array that was passed in, this is our first output
	for(int i = 0; i < (_fitOrder+1); i++){
		fitCoeffsX[i] = tempCoeffsX[i];
		fitCoeffsY[i] = tempCoeffsY[i];
	}
	
	//we will now take out the offset, making the range -100 to 100 isntead of 28 to 228
	//calculate the offset
	float xZeroError = linearize((float)fitPointsX[2],fitCoeffsX);
	float yZeroError = linearize((float)fitPointsY[2],fitCoeffsY);
	
	//Adjust the fit so that the stick zero position is 0
	fitCoeffsX[3] = fitCoeffsX[3] - xZeroError;
	fitCoeffsY[3] = fitCoeffsY[3] - yZeroError;
	
	Serial.println("The fit coefficients are  are (x,y):");
	for(int i = 0; i < 4; i++){
		Serial.print(fitCoeffsX[i]);
		Serial.print(",");
		Serial.println(fitCoeffsY[i]);
	}
	
	Serial.println("The linearized points are:");
	for(int i = 0; i <= _noOfNotches; i++){
		outX[i] = linearize(inX[i],fitCoeffsX);
		outY[i] = linearize(inY[i],fitCoeffsY);
		Serial.print(outX[i],8);
		Serial.print(",");
		Serial.println(outY[i],8);
	}
	
}

void notchCalibrate(float xIn[], float yIn[], float xOut[], float yOut[], int regions, float allAffineCoeffs[][6], float regionAngles[]){
	for(int i = 1; i <= regions; i++){
      Serial.print("calibrating region: ");
      Serial.println(i);
      
		MatrixXf pointsIn(3,3);

    MatrixXf pointsOut(3,3);
    
    if(i == (regions)){
      Serial.println("final region");
      pointsIn << xIn[0],xIn[i],xIn[1],
                yIn[0],yIn[i],yIn[1],
                1,1,1;
      pointsOut << xOut[0],xOut[i],xOut[1],
                   yOut[0],yOut[i],yOut[1],
                   1,1,1;
    }
    else{
		pointsIn << xIn[0],xIn[i],xIn[i+1],
								yIn[0],yIn[i],yIn[i+1],
								1,1,1;
		pointsOut << xOut[0],xOut[i],xOut[i+1],
								 yOut[0],yOut[i],yOut[i+1],
								 1,1,1;
    }
    
    Serial.println("In points:");
    print_mtxf(pointsIn);
    Serial.println("Out points:");
    print_mtxf(pointsOut);
    
		MatrixXf A(3,3);
    
		A = pointsOut*pointsIn.inverse();
    //A = pointsOut.colPivHouseholderQr().solve(pointsIn);
   

    Serial.println("The transform matrix is:");
    print_mtxf(A);
    
    Serial.println("The affine transform coefficients for this region are:");
    
		for(int j = 0; j <2;j++){
			for(int k = 0; k<3;k++){
				allAffineCoeffs[i-1][j*3+k] = A(j,k);
				Serial.print(allAffineCoeffs[i-1][j*3+k]);
				Serial.print(",");
			}
		}
		
		Serial.println();
		Serial.println("The angle defining this  regions is:");
		regionAngles[i-1] = atan2f((yIn[i]-yIn[0]),(xIn[i]-xIn[0]));
		//unwrap the angles so that the first has the smallest value
		if(regionAngles[i-1] < regionAngles[0]){
			regionAngles[i-1] += M_PI*2;
		}
		Serial.println(regionAngles[i-1]);
	}
}
float linearize(float point, float coefficients[]){
	return (coefficients[0]*(point*point*point) + coefficients[1]*(point*point) + coefficients[2]*point + coefficients[3]);
}
void runKalman(VectorXf& xZ,VectorXf& yZ){
	//Serial.println("Running Kalman");
	
	
	//get the time delta since the kalman filter was last run
	unsigned int thisMicros = micros();
	_dT = (thisMicros-_lastMicros)/1000.0;
	_lastMicros = thisMicros;
	//Serial.print("loop time: ");
	//Serial.println(_dT);
	
	//generate the state transition matrix, note the damping term
	MatrixXf Fmat(2,2);
	Fmat << 1,_dT-_damping/2*_dT*_dT,
			 0,1-_damping*_dT;

	//generate the acceleration variance for this filtering step, the further from the origin the higher it will be
  float R2 = xZ[0]*xZ[0]+yZ[0]*yZ[0];
  if(R2 > 10000){
    R2 = 10000;
  }
  float accelVar = _aAccelVar*(R2*R2*R2) + _bAccelVar;
	
	MatrixXf Q(2,2);
  //Serial.print(accelVar,10);
  //Serial.print(',');
	Q << (_dT*_dT*_dT*_dT/4), (_dT*_dT*_dT/2),
			 (_dT*_dT*_dT/2), (_dT*_dT);
	Q = Q * accelVar;
	
	MatrixXf sharedH(1,2);
	sharedH << 1,0;
	
	//Serial.println('H');
	//print_mtxf(H)
	
	MatrixXf xR(1,1);
	MatrixXf yR(1,1);
	
	//generate the measurement variance (i've called it ADC var) for this time step, the further from the origin the lower it will be
  xR << _aADCVarX*(R2*R2*R2) + _bADCVarX;
	yR << _aADCVarY*(R2*R2*R2) + _bADCVarY;

  //Serial.println(xR(0,0),10);
	//_dT = micros()-lastMicros;
	//Serial.println(_dT);
	//print_mtxf(_xP);
	
	//run the prediciton step for the x-axis
	kPredict(_xState,Fmat,_xP,Q);
	//_dT = micros()-lastMicros;
	//Serial.println(_dT);
	
	//run the prediciton step for the y-axis
	kPredict(_yState,Fmat,_yP,Q);
	//_dT = micros()-lastMicros;
	//Serial.println(_dT);
	
	//run the update step for the x-axis
	kUpdate(_xState,xZ,_xP,sharedH,xR);
	//_dT = micros()-lastMicros;
	//Serial.println(_dT);
	
	//run the update step for the y-axis
	kUpdate(_yState,yZ,_yP,sharedH,yR);
	//_dT = micros()-lastMicros;
	//Serial.println(_dT);
}
void kPredict(VectorXf& X, MatrixXf& F, MatrixXf& P, MatrixXf& Q){
	//Serial.println("Predicting Kalman");
	
	X = F*X;
	P = F*P*F.transpose() + Q;
	
}
void kUpdate(VectorXf& X, VectorXf& Z, MatrixXf& P, MatrixXf& H,  MatrixXf& R){
//void kUpdate(VectorXf& X, float measX, MatrixXf& P, MatrixXf& H,  MatrixXf& R){
	//Serial.println("Updating Kalman");
	
	int sizeState = X.size();
	//int sizeMeas = Z.size();
	MatrixXf A(1,2);
	A = P*H.transpose();
	MatrixXf B(1,1);
	B = H*A+R;
	MatrixXf K(2,1);
	K = A*B.inverse();
	//print_mtxf(K);
	//K = MatrixXf::Identity(sizeState,sizeState);
	MatrixXf C(1,1); 
	//C = Z - H*X;
	X = X + K*(Z - H*X);
	//X = X + K*(measX-X[0]);
	
	MatrixXf D = MatrixXf::Identity(sizeState,sizeState) - K*H;
	P = D*P*D.transpose() + K*R*K.transpose();
	
	//print_mtxf(P);
}

void print_mtxf(const Eigen::MatrixXf& X){
   int i, j, nrow, ncol;
   nrow = X.rows();
   ncol = X.cols();
   Serial.print("nrow: "); Serial.println(nrow);
   Serial.print("ncol: "); Serial.println(ncol);       
   Serial.println();
   for (i=0; i<nrow; i++)
   {
       for (j=0; j<ncol; j++)
       {
           Serial.print(X(i,j), 6);   // print 6 decimal places
           Serial.print(", ");
       }
       Serial.println();
   }
   Serial.println();
}
void calcStickValues(float angle, float* x, float* y){
	*x = 100*atan2f((sinf(_maxStickAngle)*cosf(angle)),cosf(_maxStickAngle))/_maxStickAngle;
	*y = 100*atan2f((sinf(_maxStickAngle)*sinf(angle)),cosf(_maxStickAngle))/_maxStickAngle;
}