//////////////////////////////////////////////////////////////////////////////////////////
//
//   ESP32 Library for ADS1292R Shield/Breakout
//
//   Copyright (c) 2017 ProtoCentral
//   Heartrate and respiration computation based on original code from Texas Instruments
//   
//
//   This software is licensed under the MIT License(http://opensource.org/licenses/MIT).
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
//   NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//   Requires g4p_control graphing library for processing.  Built on V4.1
//   Downloaded from Processing IDE Sketch->Import Library->Add Library->G4P Install
//   If you have bought the breakout the connection with the ESP32 board is as follows:
//
//  |ads1292r pin label| ESP32 Connection   |Pin Function      |
//  |----------------- |:--------------------:|-----------------:|
//  | VDD              | +3.3V                |  Supply voltage  |
//  | PWDN/RESET       | GPIO 4               |  Reset           |
//  | START            | GPIO 33              |  Start Input     |
//  | DRDY             | GPIO 32              |  Data Ready Outpt|
//  | CS               | GPIO 5               |  Chip Select     |
//  | MOSI             | GPIO 23              |  Slave In        |
//  | MISO             | GPIO 19              |  Slave Out       |
//  | SCK              | GPIO 18              |  Serial Clock    |
//  | GND              | Gnd                  |  Gnd             |
//
/////////////////////////////////////////////////////////////////////////////////////////



// Load libraries
#include "BluetoothSerial.h"

// Check if Bluetooth configs are enabled
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
#include "protocentralAds1292r.h"
#include "ecgRespirationAlgo.h"
#include <SPI.h>

volatile uint8_t globalHeartRate;
volatile uint8_t globalRespirationRate=0;

const int ADS1292_DRDY_PIN = 32;
const int ADS1292_CS_PIN = 5;
const int ADS1292_START_PIN = 33;
const int ADS1292_PWDN_PIN = 4;

int16_t ecgWaveBuff, ecgFilterout;
int16_t resWaveBuff,respFilterout;

long timeElapsed=0;

ads1292r ADS1292R;
ecg_respiration_algorithm ECG_RESPIRATION_ALGORITHM;
BluetoothSerial SerialBT;

double current_millis;


void setup()
{
  delay(2000);

  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  //CPOL = 0, CPHA = 1
  SPI.setDataMode(SPI_MODE1);
  // Selecting 1Mhz clock for SPI
  SPI.setClockDivider(SPI_CLOCK_DIV16);

  pinMode(ADS1292_DRDY_PIN, INPUT);
  pinMode(ADS1292_CS_PIN, OUTPUT);
  pinMode(ADS1292_START_PIN, OUTPUT);
  pinMode(ADS1292_PWDN_PIN, OUTPUT);
  Serial.begin(115200);
  SerialBT.begin("ESP32");

  ADS1292R.ads1292Init(ADS1292_CS_PIN,ADS1292_PWDN_PIN,ADS1292_START_PIN);  //initalize ADS1292 slave
  Serial.println("Initiliziation is done");
}

void loop()
{
  ads1292OutputValues ecgRespirationValues;
  boolean ret = ADS1292R.getAds1292EcgAndRespirationSamples(ADS1292_DRDY_PIN,ADS1292_CS_PIN,&ecgRespirationValues);

  if (ret == true)
  {
    ecgWaveBuff = (int16_t)(ecgRespirationValues.sDaqVals[1] >> 8) ;  // ignore the lower 8 bits out of 24bits
    resWaveBuff = (int16_t)(ecgRespirationValues.sresultTempResp>>8) ;

    if(ecgRespirationValues.leadoffDetected == false)
    {
      ECG_RESPIRATION_ALGORITHM.ECG_ProcessCurrSample(&ecgWaveBuff, &ecgFilterout);   // filter out the line noise @40Hz cutoff 161 order
      ECG_RESPIRATION_ALGORITHM.QRS_Algorithm_Interface(ecgFilterout,&globalHeartRate);// calculate
      
      //disable below 2 lines if you want to run with arduino uno. (arduino uno does not have the memory to do all processing together)
      respFilterout = ECG_RESPIRATION_ALGORITHM.Resp_ProcessCurrSample(resWaveBuff);
      ECG_RESPIRATION_ALGORITHM.RESP_Algorithm_Interface(respFilterout,&globalRespirationRate);

    }else{

      ecgFilterout = 0;
      respFilterout = 0;
    }
        current_millis = millis();
       // Serial.print(current_millis);
       // Serial.print(",");
        Serial.println(ecgFilterout);
        SerialBT.print("E");
        SerialBT.print(ecgFilterout);
        SerialBT.print(",");
        //SerialBT.print(resWaveBuff);
        //SerialBT.print(",");
        SerialBT.println(globalHeartRate);
        //SerialBT.print(",");
        //SerialBT.println(globalRespirationRate);
     
  }
 }
