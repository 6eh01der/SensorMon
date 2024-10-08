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
float GetFloatFromString(String & str);
void TestModem();
void InitialModem();
void GetNewSMS();
void InitialSensors();
void InitialMacros();
void InitialEeprom();
void SetDingDong();
void GetIncomingCall();

#endif