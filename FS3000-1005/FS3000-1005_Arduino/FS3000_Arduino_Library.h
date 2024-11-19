#ifndef __SparkFun_FS3000_H__
#define __SparkFun_FS3000_H__

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
  #include "pins_arduino.h"
#endif

#include "Wire.h"

#define FS3000_TO_READ 5        // Number of Bytes Read:
                                // [0]Checksum, [1]data high, [2]data low, 
                                // [3]generic checksum data, [4]generic checksum data

#define FS3000_DEVICE_ADDRESS 0x28     // Note, the FS3000 does not have an adjustable address.
#define AIRFLOW_RANGE_7_MPS 0x00       // FS3000-1005 has a range of 0-7.23 meters per second
#define AIRFLOW_RANGE_15_MPS 0x01      // FS3000-1015 has a range of 0-15 meters per second

class FS3000
{
public:
	// FS3000 class constructor
	FS3000();
		
	// begin() -- Initialize the sensor
	// INPUTS:
	// - i2C port (Note, only on "begin()" funtion, for use with I2C com interface)
	//   defaults to Wire, but if hardware supports it, can use other TwoWire ports.
	bool begin(TwoWire &wirePort = Wire); //By default use Wire port
	bool isConnected();
  uint16_t readRaw();
  float readMetersPerSecond();
  float readMilesPerHour();
  void setRange(uint8_t range);
	
private:
	TwoWire *_i2cPort;
  uint8_t _buff[5] ;		//	5 Bytes Buffer
	void readData(uint8_t* buffer_in);
  bool checksum(uint8_t* data_in, bool debug = false);
  void printHexByte(uint8_t x);
  uint8_t _range = AIRFLOW_RANGE_7_MPS; // defaults to FS3000-1005 range
  float _mpsDataPoint[13] = {0, 1.07, 2.01, 3.00, 3.97, 4.96, 5.98, 6.99, 7.23}; // defaults to FS3000-1005 datapoints
  int _rawDataPoint[13] =  {409, 915, 1522, 2066, 2523, 2908, 3256, 3572, 3686}; // defaults to FS3000-1005 datapoints
};

#endif // __SparkFun_FS3000_H__ //
