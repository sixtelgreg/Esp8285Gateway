/*
 Name:		Terminal_2560.ino
 Created:	10/06/2019
 Author:	Greg Sixtel
*/

#include "BinArray.h"
#include "ToolStrings.h"
#include "DataTypes.h"
#include "DateTime.h"
#include "SimpleTimer.h"

#include <SPI.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "RfcIO.h"


Users CurrentUser = Users::USER_3;
uint8_t SendToCtrlUsr = (uint8_t)(Users::TO_CONTROLLER | CurrentUser);

SimpleTimer SmplTimer;
//String LastReadTime;


#define GREEN_LED 13

#define SSID "MxP024"
#define PSWD "alonaalona"

#define BBT "mqtt.beebotte.com" // Domain name of Beebotte MQTT service
#define TOKEN "token_wpa5EXxaUXbxT1yM" // Set your channel token here
#define CHANNEL "boiler" // Replace with your channel name
#define CLIENTID "Controller22"
#define ResourceTelemetry "Telemetry"  
#define TELEMETRY_TOPIC "boiler/Telemetry"  
#define HISTORYT_TOPIC "boiler/HistoryT"  
#define HISTORYH_TOPIC "boiler/HistoryH"  
#define SCHEDULE_TOPIC "boiler/Schedule"  
#define RX_TOPIC "boiler/RX" 

extern DateTime RtcNow();
extern bool MqttReconnect(bool firstTime);
extern bool WiFiReconnect();
extern void MqttRxCallback(char* pTopic, uint8_t* pPayload, unsigned int length);
extern void PublishMqtt(BinArray& arr, const char* topic);
extern void Publish_RT(BinArray& arr);
extern CommStt RfcOnRead(BinArray& arr);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
//PubSubClient mqttClientRx(wifiClient);
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
RfcIO RFC(RfcOnRead, 9600UL, SWSERIAL_8N1, 12, 2, false, 256);
//begin(115200, SWSERIAL_8N1, 12, 12, false, 256);

//extern void OnPingRf();
//#define COMMUNICATION_LOST_INTERVAL 100000UL // 100 sec
//int CommunicationLostTimerID = -1;
//extern void NoCommunication();

//#define COMMUNICATION_SEARCH_INTERVAL 3000UL // 3 sec
//int CommunicationSearchTimerID = -1;
//extern void RequestUpdate();


//#define USER_TESTREQUEST_INTERVAL 17000UL // 17 sec
//int UserTestRequestTimerID = -1;

//extern void UpdateFromController(BinArray* arr);

#pragma region Logic
//extern void LogicSelector(BinArray* pArr);
//extern void LogicRX(BinArray* pArr);
//extern void LogicTX(BinArray* pArr);
//extern void OutputSelector(BinArray* pArr);
#pragma endregion

// the setup function runs once when you press reset or power the board
void setup()
{
	delay(1000UL); // power-up safety delay
	//LastReadTime = F("....starting....");
	//xxxx-xx-xx xx:xx

	pinMode(GREEN_LED, OUTPUT);
	digitalWrite(GREEN_LED, LOW);

	//Rf.begin(9600UL);
	//RFC.Init(RfcOnRead, 9600UL, SWSERIAL_8N1, 12, 2, false, 256);
	//Rf.begin(9600UL, SWSERIAL_8N1, 12, 2, false, 256);
	//Rf.enableIntTx(false);
	//Rf.listen();

#ifdef DBG
	//Dbg.begin(115200);
	Dbg.begin(115200);
	while (!Dbg);
	Dbg.println();
	Dbg.println(F("GATEWAY SETUP STARTED"));
#endif // DBG

	delay(1000UL); // additional delay

#ifdef DBG
	Dbg.println("WiFi Connecting");
#endif // DBG
	WiFiReconnect();
	delay(500);

#ifdef DBG
	Dbg.println("TimeClient Begin");
#endif // DBG

	// Initialize a NTPClient to get time
	timeClient.begin();
	// Set offset time in seconds to adjust for your timezone, for example:
	// GMT +1 = 3600
	// GMT +8 = 28800
	// GMT -1 = -3600
	// GMT +2 = 7200
	// GMT +3 = 10800
	// GMT 0 = 0
	timeClient.setTimeOffset(10800);
	timeClient.setUpdateInterval(1000UL * 60UL * 60UL); // Ones per hour

#ifdef DBG
	Dbg.println("TimeClient finished settings");
#endif // DBG

#ifdef DBG
	Dbg.println("MQTT Connecting");
#endif // DBG
	mqttClient.setServer(BBT, 1883);
	//mqttClientRx.setServer(BBT, 1883);
	mqttClient.setCallback(MqttRxCallback);
	delay(100);
	MqttReconnect(true);
	delay(100);

	//CommunicationLostTimerID = SmplTimer.setInterval(COMMUNICATION_LOST_INTERVAL, NoCommunication);
	//SmplTimer.disable(CommunicationLostTimerID);

	//CommunicationSearchTimerID = SmplTimer.setInterval(COMMUNICATION_SEARCH_INTERVAL, RequestUpdate);
	//SmplTimer.disable(CommunicationSearchTimerID);

	//UserTestRequestTimerID = SmplTimer.setInterval(USER_TESTREQUEST_INTERVAL, RequestUpdate);
	//SmplTimer.enable(UserTestRequestTimerID);

	// Try found communicaton
	//NoCommunication();

#ifdef DBG
	Dbg.println();
	Dbg.println(F("GATEWAY SETUP SUCCESSED"));
#endif // DBG

	delay(500UL);
	//RequestUpdate();
	digitalWrite(GREEN_LED, HIGH);
}

DateTime RtcNow()
{
	auto epochTime = timeClient.getEpochTime();
	auto dt = DateTime(epochTime, true);
//#ifdef DBG
//	Dbg.print("RtcNow() => ");
//	Dbg.println(dt.timestamp());
//#endif // DBG

	return dt;
}

bool WiFiReconnect() {

	if (WiFi.status() != WL_CONNECTED) {
#ifdef DBG
		Dbg.print("Connecting to ");
		Dbg.print(SSID);
		Dbg.println("...");
#endif // DBG

		WiFi.begin(SSID, PSWD);

		if (WiFi.waitForConnectResult(/*20000UL*/) != WL_CONNECTED) {
#ifdef DBG
			Dbg.println("WiFi is NOT connected");
#endif // DBG
			return false;
		}
#ifdef DBG
		Dbg.println("WiFi is Connected");
#endif // DBG
	}
	return true;
}

bool MqttReconnect(bool firstTime) {

	auto connected = mqttClient.connected();

	if (!connected) {
		//auto clientId = "myxtap"; // generateID();
#ifdef DBG
		Dbg.print("Client ID: ");
		Dbg.print(CLIENTID);

		if (firstTime) {
			Dbg.print(" is connecting to Beebotte MQTT");
		}
		else {
			Dbg.print(" is reconnecting to Beebotte MQTT");
		}
		Dbg.println();
#endif // DBG

		mqttClient.connect(CLIENTID, TOKEN, "");
		connected = mqttClient.connected();
		if (connected) {
			mqttClient.subscribe(RX_TOPIC);
#ifdef DBG
			Dbg.println("Successfully Connected to Beebotte MQTT");
#endif // DBG
		}
		else {
#ifdef DBG
			Dbg.println("NOT Connected to Beebotte MQTT");
#endif // DBG
		}
	}

	mqttClient.loop();

	return connected;
}

// the loop function runs over and over again until power down or reset
void loop()
{
	while (!timeClient.update()) {
		Serial.println("TimeClient ForceUpdate Update");
		timeClient.forceUpdate();
	}

	SmplTimer.run();

	RFC.ReadRF();

	if (!WiFiReconnect()) {
		delay(100);
		//return;
	}

	if (!MqttReconnect(false)) {
		delay(100);
		//return;
	}	
}

CommStt RfcOnRead(BinArray& arr)
{
	auto currUsr = CurrentUser;
	auto id = arr.GetVal<uint32_t>(_ID_INDEX);
	auto readUsr = arr.GetVal<Users>(_USERS_INDEX);
	auto opcode = arr.GetVal<Opcode>(_OPCODE_INDEX);

	if (id != _ID_)
	{
#ifdef DBG
		Dbg.println();
		Dbg.println(F("RF ID is invalid"));
#endif // DBG
		return CommStt::IdErr;
	}

	// Message sent for specific user   
	if (Opcode::CMD_NONE == (currUsr & readUsr))
	{
#ifdef DBG
		Dbg.println();
		Dbg.print(F("Invalid User. Received: "));
		Dbg.print(readUsr & Users::USER_All);
		Dbg.print(F(", instead: "));
		Dbg.println(CurrentUser);
#endif // DBG
		return CommStt::InvalidUser;
	}

	if (0 == opcode) {
#ifdef DBG
		Dbg.println(F("Undefined Opcode"));
#endif // DBG
		return CommStt::UnknownData;
	}

#ifdef DBG
	Dbg.println();
	Dbg.print(F("Request: "));
	Dbg.println(ToolStr::CmdGet((Opcode)opcode));
	auto dt = RtcNow();
	char dtstr[] = {"Time: hh:mm:ss DDD DD.MMM.YYYY "};
	Dbg.println(dt.toString(dtstr));
	
#endif // DBG

	digitalWrite(GREEN_LED, LOW);

	switch (opcode)
	{
	case CMD_HT:
	{
		PublishMqtt(arr, HISTORYT_TOPIC);
		break;
	}
	case CMD_HH:
	{
		PublishMqtt(arr, HISTORYH_TOPIC);
		break;
	}
	case CMD_GS:
	{
		PublishMqtt(arr, SCHEDULE_TOPIC);
		break;
	}
	case CMD_RT:
	case CMD_BR:
	{
		Publish_RT(arr);
		break;
	}
	case CMD_ST:
	case CMD_GT:
	case CMD_AH:
	case CMD_MH:
	case CMD_US:
	case CMD_AS:
	case CMD_PH:
	case CMD_RS:
	case CMD_PS:
	case CMD_DS:
	case CMD_GA:
		break;
	default:
		break;
	}

	//OutputSelector(pArr);
	SmplTimer.setTimeout(1000UL, LedOff);
	return CommStt::ReadFull;
}

void Publish_RT(BinArray& arr)
{
	static bool publishedTelemetry = false;
	if (!publishedTelemetry) {
		publishedTelemetry = true;

#ifdef DBG
		Dbg.println();
		arr.SetNextIndex(_DATA_START_INDEX);
		uint16_t sttFlags = arr.NextVal<uint16_t>();
		uint8_t heatDestTemp = arr.NextVal<uint8_t>();
		uint16_t pause = arr.NextVal<uint16_t>();
		uint8_t pauseSchedule = arr.NextVal<uint8_t>();
		uint32_t temperature = arr.NextVal<uint32_t>();
		DateTime dt = arr.NextVal<DateTime>();
		auto t = reinterpret_cast<byte*>(&temperature);
		char tmpStr[] = {" hh:mm:ss DDD DD.MMM.YYYY "};
		Dbg.print(F("Broadcast Time: "));
		Dbg.println(dt.toString(tmpStr));
		Dbg.print("Heat Dest Temp: ");
		Dbg.println(heatDestTemp);
		Dbg.print("STTs: ");
		Dbg.println(sttFlags);
		Dbg.print("Pause: ");
		Dbg.println(pause);
		Dbg.print("Pause Schedule: ");
		Dbg.println(pauseSchedule);
		sprintf(tmpStr, "Temp => %d  %d  %d  %d", t[0], t[1], t[2], t[3]);
		Dbg.print("Temp: ");
		Dbg.println(tmpStr);
#endif // DBG
		PublishMqtt(arr, TELEMETRY_TOPIC);
	}
	else {
		publishedTelemetry = false;
	}
}

void PublishMqtt(BinArray& arr, const char* topic)
{
	auto toSend = arr.GetUsedSize();
	auto arrPtr = arr.GetPtr();

	// Now publish the char buffer to Beebotte
	auto rc = mqttClient.publish_P(topic, arrPtr, toSend, true);
	auto plannedSend = strlen(topic) + 4 + toSend;

#ifdef DBG
	Dbg.print("Published: ");
	Dbg.println(topic);

	if (plannedSend == rc) {
		Dbg.println("Successfully Published");
	}
	else {
		if (plannedSend > rc) {
			Dbg.println("Too Long message");
		}

		Dbg.print("Published: ");
		Dbg.print(rc);
		Dbg.print("  Planned: ");
		Dbg.println(plannedSend);
	}
#endif // DBG
}

void LedOff()
{
	digitalWrite(GREEN_LED, HIGH);
//#ifdef DBG
//	Dbg.println("GREEN LED OFF");
//#endif // DBG
	//RequestUpdate();
}

void MqttRxCallback(char* pTopic, uint8_t* pPayload, unsigned int length)
{
	digitalWrite(GREEN_LED, LOW);
#ifdef DBG
	Dbg.println();
	Dbg.print("Received Topic: ");
	Dbg.println(pTopic);
	Serial.print("Message length: ");
	Serial.println(length);
#endif // DBG
	BinArray arr(pPayload, length);

	auto msgSz = arr.GetVal<uint8_t>(_DATA_SIZE_INDEX);
	auto appID = arr.GetVal<uint32_t>(_ID_INDEX);
	auto opcode = arr.GetVal<Opcode>(_OPCODE_INDEX);
	
#ifdef DBG
	Dbg.print("App ID: ");
	Dbg.println(appID); // 0x3A505041 // 978,341,953
	Dbg.print("Opcode: ");
	Dbg.println(ToolStr::CmdGet(opcode));
	Dbg.print("Data Size: ");
	Dbg.println(msgSz);
	Dbg.print("Message to send Size: ");
	Dbg.println(arr.GetUsedSize());
#endif // DBG

	uint32_t id = _ID_;
	uint8_t user = SendToCtrlUsr;
	uint16_t route = (uint16_t)Route::T_RQST_MQTT;

	arr.Set(id, _ID_INDEX);
	arr.Set(user, _USERS_INDEX);
	arr.Set(route, _ROUTE_INDEX);

//#ifdef DBG
//	Dbg.println();
//	auto tsID = arr.GetVal<uint32_t>(_ID_INDEX);
//	Dbg.print("2Send ID: ");
//	Dbg.println(tsID);
//	auto tsOpcode = arr.GetVal<Opcode>(_OPCODE_INDEX);
//	Dbg.print("2Send Opcode: ");
//	Dbg.println(ToolStr::CmdGet(tsOpcode));
//	auto tsUser = arr.GetVal<uint8_t>(_USERS_INDEX);
//	Dbg.print("2Send User: ");
//	Dbg.println(tsUser);
//	Dbg.print("2Send Size: ");
//	Dbg.println(arr.GetUsedSize());
//#endif // DBG


	//arr.Set(&id, sizeof(uint32_t), _ID_INDEX);
	//arr.Set(&user, sizeof(uint8_t), _USERS_INDEX);
	//arr.Set(&route, sizeof(uint16_t), _ROUTE_INDEX);

	RFC.SendRF(arr);
	SmplTimer.setTimeout(800UL, LedOff);
}


#pragma region Logic
//void LogicSelector(BinArray* pArr)
//{
//	if (nullptr == pArr) {
//		return;
//	}
//
//	BinArray& arr = *pArr;
//	auto opcode = (Opcode)arr[_OPCODE_INDEX];
//
//	// Message is RESPONSE [TX]
//	if (pArr->IsDataMask((uint16_t)Route::TX, _ROUTE_INDEX))
//	{
//#ifdef DBG
//		Dbg.print("LogicSelector [TX]=> Opcode: ");
//		Dbg.println(ToolStr::CmdGet(opcode));
//#endif // DBG
//
//		// Message reqests Logic Tx of Terminal
//		if (pArr->IsDataMask((uint16_t)Route::TLgTx, _ROUTE_INDEX))
//		{
//			LogicTX(pArr);
//		}
//	}
//	else // Message is REQUEST [RX]
//	{
//#ifdef DBG
//		Dbg.print("LogicSelector [RX]=> Opcode: ");
//		Dbg.println(ToolStr::CmdGet(opcode));
//#endif // DBG
//		// Message reqests Logic Rx of Terminal
//		if (pArr->IsDataMask((uint16_t)Route::TLgRx, _ROUTE_INDEX))
//		{
//			LogicRX(pArr);
//		}
//	}
//}

//void LogicRX(BinArray* pArr)
//{
//	BinArray& arr = *pArr;
//	auto opcode = (Opcode)arr[_OPCODE_INDEX];
//
//#ifdef DBG
//	Dbg.print("LogicRX=> Opcode: ");
//	Dbg.println(ToolStr::CmdGet(opcode));
//#endif // DBG
//
//	switch (opcode)
//	{
//	case CMD_RT:
//	case CMD_GT:
//	case CMD_HT:
//	case CMD_HH:
//	case CMD_AH:
//	case CMD_MH:
//	case CMD_PH:
//	case CMD_GS:
//	case CMD_US:
//	case CMD_AS:
//	case CMD_DS:
//	case CMD_RS:
//	case CMD_PS:
//	case CMD_ST:
//	{
//		arr.SetDataMask(SendToCtrlUsr, _USERS_INDEX);
//		break;
//	}
//	case CMD_GA:
//	{
//		// Still not implemented
//		return;
//	}
//
//	default:
//		return;
//	}
//
//	if (pArr->IsDataMask((uint16_t)Route::TRxTx, _ROUTE_INDEX))
//	{
//		pArr->SetDataMask((uint16_t)Route::TX, _ROUTE_INDEX);
//	}
//
//	OutputSelector(pArr);
//}
//void OutputSelector(BinArray* pArr)
//{
//	if (nullptr == pArr)
//	{
//		return;
//	}
//
//	// Message is RESPONSE [TX]
//	if (pArr->IsDataMask((uint16_t)Route::TX, _ROUTE_INDEX))
//	{
//		// Message reqests TX RF Sender of Terminal
//		if (pArr->IsDataMask((uint16_t)Route::TTxRf, _ROUTE_INDEX))
//		{
//			RFC.SendRF(pArr);
//		}
//	}
//	else // Message is REQUEST [RX]
//	{
//		// Message reqests RX RF Sender of Terminal
//		if (pArr->IsDataMask((uint16_t)Route::TRxRf, _ROUTE_INDEX))
//		{
//			RFC.SendRF(pArr);
//		}
//	}
//}
#pragma endregion

//void NoCommunication()
//{
//#ifdef DBG
//	Dbg.println();
//	Dbg.println(F("Searching Communication..."));
//#endif // DBG
//
//	SmplTimer.disable(CommunicationLostTimerID);
//	SmplTimer.restartTimer(CommunicationSearchTimerID);
//	SmplTimer.enable(CommunicationSearchTimerID);
//	RequestUpdate();
//}

//void RequestUpdate()
//{
//	static BinArray arr(_PROTOCOL_HEADER_SZ + 5);
//
//	arr.Clear(0);
//
//	uint32_t id = _ID_;
//	uint8 user = (uint8_t)SendToCtrlUsr;
//	uint16_t route = (uint16_t)Route::T_RQST_MQTT;
//	uint8 opcode = (uint8_t)(Opcode::CMD_RT);
//	uint8_t dummy = 0;
//
//	arr.Add(&dummy, sizeof(uint8_t)); // Message size == 0.
//	arr.Add(&id, sizeof(uint32_t));
//	arr.Add(&user, sizeof(uint8_t));
//	arr.Add(&route, sizeof(uint16_t));
//	arr.Add(&opcode, sizeof(uint8_t));
//	arr.Add(&dummy, sizeof(uint8_t));
//	
//	LogicSelector(&arr);
//
//#ifdef DBG
//	auto dt = RtcNow();
//	char dtstr[] = {" hh:mm:ss "};
//	Dbg.print(F("Request Update Sent: "));
//	Dbg.println(dt.toString(dtstr));
//	Dbg.println();
//#endif // DBG
//}

