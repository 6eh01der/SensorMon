#ifndef FUNCTION_H
#define FUNCTION_H

/////////////////////////////// Прототипы функций ////////////////////////////////////////

String SendATCommand(String cmd, bool waiting);
String WaitResponse();
void GetSensors();
void AlarmMessages();
void ParseSMS(String & msg);
void SetLedState(String & result, String & msgphone);
void SendSMS(String message);
float GetBalans(String & code);
float GetFloatFromString(String & str);
void GetBalanceSim();
void TestModem();
void InitialModem();
void GetNewSMS();
void InitialSensors();
void InitialEeprom();
void SetDingDong();
void GetIncomingCall();

#endif