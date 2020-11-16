#pragma once
#include "BinArray.h"
#include "ToolStrings.h"
#include "DataTypes.h"
#include "SoftwareSerial.h"

typedef CommStt(*onRfcRead)(BinArray& pArr);

class RfcIO
{
	const int  MAX_FALSE_READ_ITERATIONS = 50;
	const int  SIZE_CHUNK_SEND = 20;

public:

	RfcIO(
		onRfcRead onRead,
		uint32_t baud,
		SoftwareSerialConfig config,
		int8_t rxPin,
		int8_t txPin,
		bool invert,
		int bufCapacity);

	~RfcIO() {
		delete Rf;
		Rf = nullptr;
		OnRead = nullptr;
	}

	CommStt ReadRF();
	void ClearRF();
	bool SendRF(BinArray& arr);
	int Available() {
		return Rf->available();
	}

	//size_t Read(uint8_t* buffer, size_t size) {
	//	return Rf.read(buffer, size);
	//}
	
	size_t ReadBytes(uint8_t* buffer, size_t size) {
		return Rf->readBytes(buffer, size);
	}

	size_t Write(const uint8_t* buffer, size_t size) {
		return Rf->write(buffer, size);
	}


private:
	SoftwareSerial *Rf = nullptr;
	onRfcRead OnRead = nullptr;
	
};

