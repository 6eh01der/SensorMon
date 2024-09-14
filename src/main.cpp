/////////////////////////// Охранно-аварийная gsm-сигнализация с sms-управляемыми реле ////////////////////////////////

#include  <SoftwareSerial.h>                                
#include  <EEPROM.h> 
#include  <CyberLib.h> 
#include  "SecurityCircuit.h"
#include  "function.h"

//#define _SS_MAX_RX_BUFF 128                             // регулируем размер RX буфера SoftwareSerial
                        
#define SENSOR_1 11         // Первый датчик
#define SENSOR_2 12         // Второй датчик
#define BUTTON_PIN 16     // На плате                                    
#define GSM_RX 1  // Arduino pin 1 to URX
#define GSM_TX 0  // Arduino pin 0 to UTX

SoftwareSerial sim(GSM_RX, GSM_TX);

String operator_code = "";                                      // Переменная кода оператора сим
String _response     =  "";                                     // Переменная для хранения ответа модуля
String phones[5] = {"","","","",""};                            // Телефон админа
String temp = "";
int AT_counter = 1;
unsigned long timer_ring = millis();                            // Инициализируем таймер для входящего звонка
unsigned long timer_delay = millis();                           // Таймер задержки срабатывания
unsigned long last_update  =  millis();                         // Время последнего обновления      
unsigned long update_period =  20000;                           // Проверять каждые 20 секунд
unsigned int timer_delay_off;                                   // Время задержки сработки тревоги в миллисекундах.
unsigned int timer_delay_on;                                    // Время задержки постановки на охрану в миллисекундах.
long counter_errors = 0;                                        // Счетчик итераций при ошибке обработки входящего смс 
unsigned long alarm_timer = 0;
bool flag_loop = false;
bool flag_call = false;                                         // Флаг входящего звонка  
bool mode = true;                                              // Режим отслеживания вкл/выкл
bool flag_timer_sms = false;                                    // Флаг обработки входящего смс
bool hasmsg  =  false;                                          // Флаг наличия сообщений к удалению
bool flag_alarm_timer = 0;                                          // Переменная хранения макросов
byte triggered[2] = {9,9};                              // Массив адресов сработавших датчиков                           
byte counter_triggered = 0;                                     // Счетчик сработок датчиков
byte counter_admins = 2;                                        // Количество администраторов
byte read_pin_count = 0;
SecurityCircuit sensor[2];                                      

///////////////////////////////////////// Функция настроек контроллера /////////////////////////////////////////

void setup() 
{
  D11_In;
  D12_In;
  D13_Out;
  D16_In;
  D17_In;
  D18_In;

  pinMode(SENSOR_1, INPUT_PULLUP);
  pinMode(SENSOR_2, INPUT_PULLUP);
  sensor[0].pin_ = SENSOR_1;
  sensor[1].pin_ = SENSOR_2;

  Serial.begin(9600);                                  // Скорость обмена данными с компьютером
  sim.begin(9600);                                  // Скорость обмена данными с сим модулем                                                          

  InitialSensors();
  InitialEeprom();
  
  temp = F("System v5.3 Status OFF. Phones - ");      //
  for(int i = 0; i < counter_admins; i ++ )            //
  {                                                    //
    if(phones[i][0] == '+')                            //
    {                                                  //
      temp += phones[i];                               //
      if(phones[i + 1][0] == '+')                      //
      {                                                //
        temp += F(", ");                               //
      }                                                //      
    }                                                  //   Формируем сообщение о перезагрузке контроллера
  }                                                    //
  temp += F(". ");                                     //
  temp += F("Time ON ");                               //
  temp += timer_delay_on/1000;                         //
  temp += F(". Time OFF ");                            //  
  temp += timer_delay_off/1000;                        //
  temp += F(".Code ");                                 //
  temp += operator_code;                               //
  temp += F(".");                                      ////////////////////////////////////////////////////////////////

  TestModem();                                         // Проверяем работоспособность сим-модуля
  InitialModem();                                      // Инициализируем сим-модуль                                       
  delay(1000);
  SendSMS(temp);                                       // Отправляем смс уведомление о перезагрузке контроллера администратору
  D13_High;
  delay(1000);
  D13_Low;  
  last_update  =  millis();                            // Обнуляем таймер    
}

/////////////////////////////////////////////// Главная функция ////////////////////////////////////////////

void loop() 
{ 
  flag_loop = true;
  SetDingDong();                                      // Свето-звуковая индикация входящего звонка 
  GetNewSMS();                                        // Получаем входящие смс
  
  if (sim.available())   {                         // Если модем, что-то отправил...
    _response  =  WaitResponse();                     // Получаем ответ от модема для анализа
    _response.trim();                                 // Убираем лишние пробелы в начале и конце
    if (_response.indexOf("+CMTI:") > -1)             // Если пришло сообщение об отправке SMS
    {    
     flag_timer_sms = true;      
    }
    if (_response.startsWith("RING"))                // Есть входящий вызов
    { 
      GetIncomingCall();                             // Обрабатываем его
    }  
    else                                             // Если нет
    {
      flag_call = false;                             // Опускаем флаг входящего вызова
    }
  }
  if(Serial.available())                             // Ожидаем команды по Serial...
  {                         
    sim.write(Serial.read());                     // ...и отправляем полученную команду модему
  }
  if(sim.available())                             // Ожидаем команды от модема ...
  {                         
    Serial.write(sim.read());                     // ...и отправляем полученную команду в Serial    
  }
  if(!mode)                                          // Если снято с охраны (режим выкл)
  {   
    flag_alarm_timer = false; 
    counter_triggered = 0;
    for(int i = 0; i < 2; i ++ )
    {
      sensor[i].alarm_ = false;
      sensor[i].send_alarm_ = false;
      triggered[i] = 9;
    }                         
    D13_Low;                                        // Выключаем светодиод
  }
  else                                                // Если стоит на охране (режим вкл)
  {           
    GetSensors();                                     // Смотрим на датчики
    AlarmMessages();                                  // Отправляем необходимые уведомления
  }
}

////////////////////////////////////////////// Конец главной функции //////////////////////////////////////////////

//////////////////////////////////////// Функция отправки команды сим модулю /////////////////////////////////////

String SendATCommand(String cmd, bool waiting) 
{
  String _resp  =  "";                                            // Переменная для хранения результата
  sim.println(cmd);                                            // Отправляем команду модулю
  if (waiting)                                                    // Если необходимо дождаться ответа...
  {                                                 
    _resp  =  WaitResponse();                                     // ... ждем, когда будет передан ответ
    // Если Echo Mode выключен (ATE0), то эти 3 строки можно закомментировать
    if (_resp.startsWith(cmd))                                    
    {                                  
      _resp  =  _resp.substring(_resp.indexOf("\r", cmd.length()) + 2); // Убираем из ответа дублирующуюся команду
    }
  }
  return _resp;                                                   // Возвращаем результат. Пусто, если проблема
}

//////////////////////////////////////// Функция ожидания ответа от сим модуля ///////////////////////////////////////////////////////

String WaitResponse()                                             // Функция ожидания ответа и возврата полученного результата
{                                          
  String _resp  =  "";                                            // Переменная для хранения результата
  unsigned long _timeout  =  millis() + 10000;                    // Переменная для отслеживания таймаута (10 секунд)
  while (!sim.available() && millis() < _timeout)  {};         // Ждем ответа 10 секунд, если пришел ответ или наступил таймаут, то...
  if (sim.available())                                         // Если есть, что считывать...
  {                                       
    _resp  =  sim.readString();                                // ... считываем и запоминаем
  }
  return _resp;                                                   // ... возвращаем результат. Пусто, если проблема
}

///////////////////////////////////////////// Функция контроля датчиков //////////////////////////////////////////////////////////////

void GetSensors()
{
  for(int i = 0; i < 2; i ++ )                           // Проходим по датчикам
  { 
    if(sensor[i].ReadPin())                                // Если есть сработка
    {
      triggered[counter_triggered] = sensor[i].adress_;    // Записываем адрес датчика в массив сработок
      counter_triggered ++;                             // Увеличиваем счетчик адресов
    }
  }
}

/////////////////////////////////////////////// Функция уведомлений //////////////////////////////////////////////////////////////////////
                                                                 
void AlarmMessages()
{
    for(int i = 0; i < counter_triggered; i ++ )
    {
      if(sensor[triggered[i]].alarm_ == true && sensor[triggered[i]].send_alarm_ == false) // Если поднят флаг сработки датчика и уведомление не отправлялось
      {
        D13_High;                   // Сигнализируем динамиком о принятии номера администратора
        delay(2000);
        D13_Low;
        delay(100);
        SendSMS(sensor[triggered[i]].message_alarm_);                                 // Отправляем смс уведомление
        sensor[triggered[i]].send_alarm_ = true;                                            // Поднимаем флаг отправленного уведомления
        break;                                                                      // Выходим из цикла
      }
    }
}
     
///////////////////////////////////////////////// Фунция парсинга СМС /////////////////////////////////////////////////

void ParseSMS(String & msg)                                     // Парсим SMS
{ 
                                  
  String msgheader   =  "";
  String msgbody     =  "";
  String msgphone    =  "";

  msg  =  msg.substring(msg.indexOf("+CMGR: "));
  msgheader  =  msg.substring(0, msg.indexOf("\r"));            // Выдергиваем телефон
  msgbody  =  msg.substring(msgheader.length() + 2);
  msgbody  =  msgbody.substring(0, msgbody.lastIndexOf("OK"));  // Выдергиваем текст SMS
  msgbody.trim();
  int firstIndex  =  msgheader.indexOf("\",\"") + 3;
  int secondIndex  =  msgheader.indexOf("\",\"", firstIndex);
  msgphone  =  msgheader.substring(firstIndex, secondIndex);

  if (msgphone == phones[0] || msgphone == phones[1] || msgphone == phones[2]
   || msgphone == phones[3] || msgphone == phones[4])           // Если телефон админа, то...
  {     
    SetLedState(msgbody, msgphone);                             // ...выполняем команду
  }
}
 
///////////////////////////////////////// Функция чтения кода команды из смс ///////////////////////////////////////////

void SetLedState(String & result, String & msgphone)   
{
  bool correct  =  false;                         // Для оптимизации кода, переменная корректности команды
  if (result.length()  ==  1)                     // Если в сообщении одна цифра
  {
    int ledState  =  ((String)result[0]).toInt(); // Получаем первую цифру команды - состояние (0 - выкл, 1 - вкл)
    if (ledState  >=  0 && ledState  <=  3) 
    {                                             // Если все нормально, исполняем команду
      if (ledState == 1) 
      {
        EEPROM.write(106, 0);                     // Записываем его в память  
        SendSMS(F("mode ON"));                  // Отправляем смс с результатом администратору 
        mode = true;       
      }
      if(ledState == 0) 
      {
        mode = false;                            // Отключаем наблюдение
        SendSMS(F("mode OFF"));                  // Отправляем смс с результатом администратору
      }     
      D13_High;                    // Сигнализируем диодом о смене режима наблюдения
      delay(1000);
      D13_Low; 
      correct  =  true;                          // Флаг корректности команды
    }
    if (!correct) 
    {
      D13_High;                    // Сигнализируем диодом о смене режима наблюдения
      delay(1000);
      D13_Low;
      //SendSMS(F("Incorrect command!"));          // Отправляем смс с результатом администратору
    }
  }
  else
    {
      if(result.length()  ==  13 && msgphone == phones[0])  // Если в сообщении номер телефона и отправитель админ 1
      {
        bool flag_number = false;
        for(int k = 1; k < counter_admins; k ++ )       // Проходим по списку номеров
        {
          if(phones[k] == result)                       // Если номер есть в позиции k
           {
            flag_number = true;                         // Поднимаем флаг наличия номера
            if (k == 1)                                   // Ищем его в памяти EEPROM и удаляем
            {
              for(int i = 20; i < 33; i ++ )
                {
                  EEPROM.write(i, (byte)' ');     // Перезаписываем номер админа 2 пробелами                       
                }
                phones[k] = " ";                  // Очищаем переменную
                D13_High;           // Сигнализируем динамиком о удалении номера дублера
                delay(1000);
                D13_Low;
                SendSMS(F("Number 2 deleted!"));
                break;
            }
            else
            {
              SendSMS(F("Number not deleted!"));
            }
          }
        }
        if(flag_number)
        {
          result = " ";
          return;
        }
        for(int k = 1; k < counter_admins;k ++ )              // Проходим по списку номеров
        {
          if (phones[k][0] != '+' && result[0] == '+' && !flag_number)// Если есть место в позиции k, номер валидный и еще не записан
          {
            if (k == 1)
            {
              for(int i = 20, j = 0; i < 33; i ++ , j ++ )
                {
                  EEPROM.write(i, (byte)result[j]);     // Записываем в память номер админа 2                       
                }
                phones[k] = result;                     // Записываем в переменную номер админа 2
                D13_High;                 // Сигнализируем динамиком о принятии номера дублера
                delay(1000);
                D13_Low;
                SendSMS(F("Number 2 accepted!")); 
                flag_number = true;                     // Поднимаем флаг записи номера
                break;
            }
            else
            {
             SendSMS(F("Number not accepted!")); 
            } 
          }                 
        }                            
      }
      else
        {
            if(result.length()  ==  5)                   // Если в сообщении 5-значная команда
            {
              String command = "";                       // Объявляем переменную для записи команд
              String item = "";                          // Объявляем переменную для записи значений
              command += result[0];                      // Получаем команду
              command += result[1];              
              item += result[3];                         // Получаем значение
              item += result[4]; 
              if(command == "On")                        // Если команда "задержка включения"
              {
                  for(int i = 100, j = 0; i < 102; i ++ , j ++ )
                  {
                    EEPROM.write(i, (byte)item[j]);      // Записываем время задержки в память EEPROM           
                  }
                  timer_delay_on = item.toInt() * 1000;  // Записываем время задержки в переменную
                  item += F(" seс on");                  // Формируем уведомление
                  SendSMS(item);                         // Отправляем уведомление 
              }
              if(command == "Of")                        // Если команда "задержка выключения"
              {
                  for(int i = 102, j = 0; i < 104; i ++ , j ++ )
                  {
                    EEPROM.write(i, (byte)item[j]);      // Записываем время задержки в память EEPROM            
                  }
                  timer_delay_off = item.toInt() * 1000; // Записываем время задержки в переменную
                  item += F(" seс of");                  // Формируем уведомление
                  SendSMS(item);                         // Отправляем уведомление
              } 
            }
        }
      }

}

//////////////////////////////////////////// Функция отправки СМС сообщений ///////////////////////////////////////////

void SendSMS(String message)
{
  for(int i = 0; i < counter_admins; i ++ )
  {
    if(phones[i][0] == '+')
    {
      SendATCommand("AT+CMGS=\"" + phones[i] + "\"", true);          // Переходим в режим ввода текстового сообщения
      SendATCommand(message + "\r\n" + (String)((char)26), true);    // После текста отправляем перенос строки и Ctrl + Z
      delay(2500);
    }
  } 
}
void InitialSensors()////////////////////////////// Функция инициализации датчиков ////////////////////////////////////////////////////
  {
///////////////////////// 0  Нет тревоги                                   
///////////////////////// 1  Первый датчик                                  
///////////////////////// 2  Второй датчик                           

    for(int i = 0; i < 2; i ++ )
    {
      sensor[i].adress_ = i;                                    // Инициализируем адрес датчика
      switch (i)                                              // Инициализируем пины тревоги и уведомления о сработках
      {
      case 0:                                                 // Для первого датчика
       strcpy(sensor[i].message_alarm_,"Sensor 1 alarm!"); // Формируем уведомление о сработке
       D13_High;                   // Сигнализируем динамиком о принятии номера администратора
       delay(1000);
       D13_Low;
       break;
      case 1:                                                 // Для второго датчика
       strcpy(sensor[i].message_alarm_,"Sensor 2 alarm!"); // Формируем уведомление о сработке
       D13_High;                   // Сигнализируем динамиком о принятии номера администратора
       delay(1000);
       D13_Low;
       break;
      }
    }
  }

/////////////////////////////////////////// Функция проверки доступности сим-модуля //////////////////////////////

void TestModem()
{
  do {              // При включении питания МК загрузится раньше сим модуля поэтому ждем загрузки и ответа на команду
    _response  =  SendATCommand("AT", true);                  // Отправили AT для настройки скорости обмена данными
    _response.trim();                                         // Убираем пробельные символы в начале и конце
    if(AT_counter == 3)
    {
      D13_High;                                 // Сигнализируем об отсутсвии связи с сим-модулем
      delay(150);
      D13_Low;
      delay(20);
      D13_High;                  
      delay(150);
      D13_Low;
      delay(1000);
      AT_counter = 0;
      if(flag_loop) break;
    }
    if(_response != "OK")AT_counter ++ ;
  } while (_response != "OK");                                    // Не пускать дальше, пока модуль не вернет ОК
}

//////////////////////////////////// Функция стартовой инициализации сим-модуля ///////////////////////////////////////

void InitialModem()
{  
  SendATCommand("AT+CMGDA=\"DEL ALL\"", true);                    // Удаляем все SMS, чтобы не забивать память
  delay(1000);
                                                                  // Команды настройки модема при каждом запуске
//SendATCommand("AT + DDET = 1", true);                           // Включаем DTMF
  SendATCommand("AT+CLIP=1", true);                               // Включаем АОН
  delay(1000);
  SendATCommand("AT+CMGF=1;&W", true);                            // Включаем текстовый режима SMS (Text mode) и сразу сохраняем значение (AT&W)!
  delay(1000);
}

///////////////////////////////////////// Функция получения новых смс сообщений ///////////////////////////////////////

void GetNewSMS()
  { 
   if (millis() - last_update > update_period || flag_timer_sms) { // Пора проверить наличие новых сообщений
    do {
      _response  =  SendATCommand("AT+CMGL=\"REC UNREAD\"", true);// Отправляем запрос чтения непрочитанных сообщений
      delay(1000);
      if (_response.indexOf("+CMGL: ")  >  -1) 
      {                                                           // Если есть хоть одно, получаем его индекс
        int msgIndex  =  _response.substring(_response.indexOf("+CMGL: ") + 7, 
        _response.indexOf("\"REC UNREAD\"", _response.indexOf("+CMGL: "))).toInt();
        int i  =  0;                                              // Объявляем счетчик попыток
        counter_errors = 0;
        do 
        {
          i ++ ;                                                  // Увеличиваем счетчик
          Serial.println (i);
          _response  =  SendATCommand("AT+CMGR=" + (String)msgIndex + ",1", true);  // Пробуем получить текст SMS по индексу
          _response.trim();                                       // Убираем пробелы в начале/конце
          if (_response.endsWith("OK")) 
          {                                                       // Если ответ заканчивается на "ОК"
            if (!hasmsg) hasmsg  =  true;                         // Ставим флаг наличия сообщений для удаления
            SendATCommand("AT+CMGR=" + (String)msgIndex, true);   // Делаем сообщение прочитанным
            SendATCommand("\n", true);                            // Перестраховка - вывод новой строки
            ParseSMS(_response);                                  // Отправляем текст сообщения на обработку
            flag_timer_sms = false;
            break;                                                // Выход из do{}
          }
          else 
          {                                                       // Если сообщение не заканчивается на OK
            SendATCommand("\n", true);                            // Отправляем новую строку и повторяем попытку
          }
        } while (i < 10);
        break;
      }
      else 
      {
        counter_errors ++ ;
        last_update  =  millis();                                 // Обнуляем таймер 
        if (hasmsg || counter_errors > 10) 
        {
          SendATCommand("AT+CMGDA=\"DEL ALL\"", true);            // Удаляем все сообщения
          hasmsg  =  false;         
        }
        flag_timer_sms = false; 
        break;
      }
    } while (1);   
   }
  }

///////////////////////////////// Функция инициализации энергонезависимой памяти ///////////////////////////////////////

void InitialEeprom()
{
  for(int i = 0; i < 13; i ++ )
   {
      phones[0] += (char)EEPROM.read(i);                        // Получаем из EEPROM номер админа
   }
  for(int i = 20; i < 33; i ++ )
   {
      phones[1] += (char)EEPROM.read(i);                        // Получаем из EEPROM номер админа 2
   }
   for(int i = 40; i < 53; i ++ )
   {
      phones[2] += (char)EEPROM.read(i);                        // Получаем из EEPROM номер админа 3
   }
   for(int i = 60; i < 73; i ++ )
   {
      phones[3] += (char)EEPROM.read(i);                        // Получаем из EEPROM номер админа 4
   }
   for(int i = 80; i < 93; i ++ )
   {
      phones[4] += (char)EEPROM.read(i);                        // Получаем из EEPROM номер админа 5
   }
   for(int i = 100; i < 102; i ++ )
   {
      temp += (char)EEPROM.read(i);                             // Получаем из EEPROM вермя задержки вкл
   }
   timer_delay_on = ((String)temp).toInt()*1000;                // Инициализируем цифровую переменную задержки включения
   temp = "";                                                   // Обнуляем строковую переменную задержки включения
   for(int i = 102; i < 104; i ++ )
   {
      temp += (char)EEPROM.read(i);                             // Получаем из EEPROM вермя задержки выкл
   }
   timer_delay_off = ((String)temp).toInt()*1000;               // Инициализируем цифровую переменную задержки выключения
   temp = "";                                                   // Обнуляем строковую переменную задержки выключения

  for(int i = 107, j = 0; i < 112; i ++ , j ++ )
  {
    operator_code += (char)EEPROM.read(i);                      // Получаем из EEPROM код оператора  
  }
}

////////////////////////////////////// Функция индикации входящего звонка /////////////////////////////////////////

void SetDingDong()     
{  
  if((millis() - timer_ring < 3000))                       
  {
    D13_Inv;                                        // Переключаем светодиод
    delay(50);                                      // Пауза
    D13_Inv;                                        // Переключаем светодиод
    delay(50);                                      // Пауза
  }
}

//////////////////////////////////////////// Функция обработки входящих звонков ///////////////////////////////////

void GetIncomingCall()    
{
  bool flag_admin = false;
  flag_call = true;                                     // Поднимаем флаг звонка
  int phone_index  =  _response.indexOf("+CLIP: \"");   // Есть ли информация об определении номера, если да, то phone_index > -1
  String inner_phone  =  "";                            // Переменная для хранения определенного номера
  if (phone_index  >=  0)                               // Если информация была найдена
  {
    phone_index += 8;                                                                       // Парсим строку и 
    inner_phone  =  _response.substring(phone_index, _response.indexOf("\"", phone_index)); // получаем номер     
  } 
  for(int i = 0; i < counter_admins; i ++ )
  {
    if(inner_phone == phones[i])
    {
      flag_admin = true;
      break;
    }
  }
  if(mode)                                         // Если включен режим охраны
  {
    if(flag_admin)                                 // Если звонит админ
    {
      mode = !mode;                                // Сменить режим охраны
      SendATCommand("ATH", true);                  // сбросить звонок
      SendSMS(F("mode OFF"));                      // Отправляем смс с результатом администратору
      return;                                      // и выйти 
    }
    else                                           // Звонит не админ
    {
      SendATCommand("ATH", true);                  // сбросить звонок
      return;                                      // и выйти
    }
  }
  else
  {
    if(flag_admin)                                 // Если звонит админ
    {
      mode = !mode;                                // Сменить режим охраны
      SendATCommand("ATH", true);                  // сбросить звонок
      SendSMS(F("mode ON"));                       // Отправляем смс с результатом администратору
      return;      
    } 
        timer_ring = millis();                     // Включаем таймер для визуализации звонка  
                                                                          // Если длина номера больше 6 цифр, 
      if (inner_phone.length()  >=  7 && digitalRead(BUTTON_PIN) == HIGH)  // и нажата кнопка на плате
      {
        phones[0] = inner_phone;

        for(int i = 0; i < 13; i ++ )
        {
          EEPROM.write(i, (byte)phones[0][i]);     // Записываем номер в память EEPROM
        }      
        SendATCommand("ATH", true);                // Сбрасываем звонок
          delay(3000);          
          D13_High;                   // Сигнализируем динамиком о принятии номера администратора
          delay(1000);
          D13_Low; 
        
        SendSMS(F("Administrator number accepted!"));  
      }
  }
}
