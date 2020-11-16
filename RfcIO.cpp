#include "RfcIO.h"

RfcIO::RfcIO(
	onRfcRead onRead,
	uint32_t baud,
	SoftwareSerialConfig config,
	int8_t rxPin,
	int8_t txPin,
	bool invert,
	int bufCapacity)
{
	Rf = new SoftwareSerial(rxPin, txPin, invert);
	Rf->enableIntTx(false);
	Rf->begin(baud);
	OnRead = onRead;
}

CommStt RfcIO::ReadRF() // ==================== READ RF ===================
{
	static BinArray arr(COMMUNICATION_ARR_SIZE);
	// Checking, if data is available in the RF receiver buffer
	auto avail = Rf->available();
	if (0 == avail) {
		return CommStt::AvailEmpty;
	}

	uint8_t readOneIter = 0;
	uint8_t readAll = 0;
	uint8_t msgSz = 0;
	uint16_t iFalseRead = 0;
	arr.Clear();

#ifdef DBG
	auto befRead = millis();
	int iRead = 0;
#endif // DBG

	do {
		if (avail = Rf->available()) {
			if (readOneIter = Rf->readBytes(arr.GetNextPtr(), avail)) {
				// First iteration, try to read message size
				if (!msgSz) {
					msgSz = arr.GetVal<uint8_t>(_DATA_SIZE_INDEX) + _PROTOCOL_HEADER_SZ;
				}

				arr.AddNextIndex(readOneIter);
				arr.AddUsedSize(readOneIter);
				readAll += readOneIter;
				delay(5);
			}
		}
		else {
			++iFalseRead;
			delay(10);
		}

#ifdef DBG
		++iRead;
#endif // DBG

	} while (iFalseRead < MAX_FALSE_READ_ITERATIONS &&
		readAll < msgSz);

	if (iFalseRead >= MAX_FALSE_READ_ITERATIONS) {
#ifdef DBG
		Dbg.println();
		Dbg.println(F("RF Read iteration count reached max value MAX_READ_ITERATIONS"));
#endif // DBG
		return CommStt::TooManyRdItr;
	}

	if (readAll < msgSz) {
#ifdef DBG
		auto aftRead = millis();
		Dbg.println();
		Dbg.println(F("RF No whole mesage is read"));
		Dbg.print(F("Read: "));
		Dbg.print(readAll);
		Dbg.print(F(" from: "));
		Dbg.print(msgSz);
		Dbg.print(F(" bytes. In: "));
		Dbg.print(aftRead - befRead);
		Dbg.print(F("ms. "));
		Dbg.print(iRead);
		Dbg.println(F(" iter"));
#endif // DBG
		return CommStt::NoWholeMsgRead;
	}

	if (readAll > msgSz)
	{
		ClearRF();
#ifdef DBG
		Dbg.println();
		Dbg.println(F("RF read buffer is overloaded and cleaned"));
#endif // DBG
	}

#ifdef DBG
	auto aftRead = millis();
	Dbg.println();
	Dbg.print(F("RF Read: "));
	Dbg.print(readAll);
	Dbg.print(F(" bytes. Time: "));
	Dbg.print(aftRead - befRead);
	Dbg.print(F("ms. Iter: "));
	Dbg.print(iRead);
#endif // DBG

	if (nullptr != OnRead) {
		return OnRead(arr);
	}

	return CommStt::ReadFull;
}

bool RfcIO::SendRF(BinArray& arr)
{
	auto toSend = arr.GetUsedSize();
	uint16_t sent = 0;

#ifdef DBG
	int iterSend = 0;
#endif // DBG

	do {
		sent += Rf->write(arr.GetPtr(sent), __MIN(SIZE_CHUNK_SEND, toSend - sent));
#ifdef DBG
		++iterSend;
#endif // DBG

		//delay(5);

	} while (sent < toSend);

#ifdef DBG
	//Dbg.println();
	Dbg.print(F("RF Sent: "));
	Dbg.print(sent);
	Dbg.print(F(" bytes."));
	Dbg.print(F(" Iter: "));
	Dbg.println(iterSend);
#endif // DBG

	return (sent == toSend);
}

void RfcIO::ClearRF()
{
	static BinArray arr(24);
	for (int it = 0; Rf->available() > 0 && it < 10; ++it) { 
		auto read = Rf->readBytes(arr.GetPtr(), arr.GetArrSize()); 
	}
}
