//***************************** Библиотеки для работы с периферией *****************************************
#include <Adafruit_Fingerprint.h>           //библиотека для модуля отпечатков пальцев
#include <SoftwareSerial.h>                 //библиотека для UART
#include <LiquidCrystal.h>                  //библиотека для LCD
#include <LiquidCrystalRus.h>               //библиотека для поддержки русского языка на LCD
//**********************************************************************************************************

//****************************** Объекты *******************************************************************
//объявляется объект типа LiquidCrystalRus с определёнными параметрами (пины LCD -> Arduino)
LiquidCrystalRus myLCD(4, 5, 10, 11, 12, 13); 

//объявляется объект типа SoftwareSerial с параметрами (Rx, Tx). Можно указывать любые выводы, 
//поддерживающие прерывание PCINTx
SoftwareSerial myUART(2, 3);        

// объявляется объект типа Adafruit_Fingerprint (ptrSerial). ptrSerial - ссылка на объект UART 
//к которому подключен модуль отпечатка пальцев
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&myUART);
//**********************************************************************************************************

//***************************** Глобальные константы *******************************************************
const uint8_t  pinGreenLed    = A4;// пин, к которому подключен  зелёный светодиод
const uint8_t  pinRedLed  = A3;// пин, к которому подключен  красный светодиод

const uint8_t  pinBeep   = 6;// пин, к которому подключен зуммер
const uint8_t  pinKey    = A5;// пин, к которому подключен ключ

const uint8_t  pinButtonA    = 8;// пин, к которому подключена кнопка A
const uint8_t  pinButtonB    = 9;// пин, к которому подключена кнопка B
//**********************************************************************************************************

//***************************** Глобальные переменные ******************************************************
uint16_t  timeButtonA    = 0;// время удержания кнопки A (в сотых долях секунды)
uint16_t  TIM_Button_B    = 0;// время удержания кнопки B (в сотых долях секунды)

uint32_t  TIM_mode_ACCESS = 0;// время установки флага FLG_mode_ACCESS указывающего о необходимости открытия замка
int       ACK_finger_FUN  = 0;// результат выполнения функции библиотеки Adafruit_Fingerprint

uint8_t   VAR_mode_MENU   = 0;// текущий режим вывода меню

uint8_t   VAR_count_ID    = 0;// количество найденных ID в базе
uint8_t   VAR_first_ID    = 0;// номер первого выведенного ID из всех найденных
uint8_t   VAR_this_ID     = 0;// номер ID, шаблон которого требуется сохранить/удалить
uint8_t   VAR_array_ID[162];// массив найденных ID в базе

bool      FLG_result_FUN  = 0;// флаг указывающий на результат сохранения шаблона
bool      FLG_mode_ACCESS = 0;// флаг указывающий о необходимости открытия замка
bool      FLG_state_WORK  = 1;// флаг указывающий о состоянии работы замка
bool      FLG_display_UPD = 1;// флаг указывающий о необходимости обновления информации на дисплее

String    str = ""; //строка для работы с bluetooth модуль
//**********************************************************************************************************

void setup(){
  pinMode(pinButtonA,    INPUT);// устанавливаем режим работы вывода pinButtonA,   как "вход"
  pinMode(pinButtonB,    INPUT);// устанавливаем режим работы вывода pinButtonB,   как "вход"
  pinMode(pinGreenLed,   OUTPUT);// устанавливаем режим работы вывода PIN_led_OPEN,   как "выход"
  pinMode(pinRedLed, OUTPUT);// устанавливаем режим работы вывода PIN_led_CLOSED, как "выход"
  pinMode(pinKey,   OUTPUT);// устанавливаем режим работы вывода pinKey,   как "выход"

  Serial.begin(9600);  // инициируем модуль Bluetooth, с подключением через программный UART на скорости 9600 (скорость модуля по умолчанию)
  Serial.setTimeout(50);
  Serial.println("Bluetooth connected.");

  myLCD.begin(16, 2); // инициируем модуль lcd, 2е строки и 16 столбцов
  //Выводится приветствие на LCD
  myLCD.print("    ПРИВЕТ!!!  ");
  myLCD.setCursor(0, 1);
  myLCD.print("     ВИВТ!      ");
  
  delay(1000);                                                              // обязательная задержка перед инициализацией модуля отпечатков пальцев
  finger.begin(57600);                                                     // инициируем модуль отпечатков пальцев, с подключением через программный UART на скорости 57600 (скорость модуля по умолчанию)
  
  myLCD.clear();                                                             // стираем информацию с дисплея
  myLCD.setCursor(0, 0);        myLCD.print(F("Поиск сенсора..."));              // выводим текст "Scan sensor..."
  myLCD.setCursor(0, 1);                                                     // устанавливаем курсор в позицию: 0 столбец, 1 строка
  if( finger.verifyPassword() ){
    myLCD.print(F("Сенсор Обнаружен")); // если модуль отпечатков обнаружен, то выводим сообщение "сенсор обнаружен"
  }               
  else{// если модуль отпечатков не обнаружен
    myLCD.print(F("Сенсора нет(((")); //выводим сообщение "сенсор не обнаружен"
    while(1); //входим в бесконечный цикл: while(1);
  } 
  delay(1000);                                                             // необязательная задержка, чтоб можно было прочитать сообщение об обнаружении модуля
}

void loop(){
// Передаём управление кнопкам
   Func_buttons_control();                                                 // вызываем функцию Func_buttons_control();

// Обновляем информацию на дисплее
   if(FLG_display_UPD){Func_display_show();}                               // если установлен флаг FLG_display_UPD (нужно обновить информацию на дисплее), то вызываем функцию Func_display_show();

// Общаемся с модулем отпечатков пальцев
   Func_sensor_communication();                                            // вызываем функцию Func_sensor_communication();

  if(Serial.available()>0){
    str = Serial.readString();
    if(str == "1"){
      FLG_mode_ACCESS = 1; 
      TIM_mode_ACCESS = millis();
    }
    if(str == "0"){
      FLG_mode_ACCESS = 0; 
    }
  }

// Управляем замком, светодиодами и зуммером
   if(FLG_state_WORK){                                                     // если установлен флаг FLG_state_WORK (замок работает, «State: ENABLE»), то ...
     digitalWrite(pinKey,   FLG_mode_ACCESS);                       // если используется электромагнитный замок, где 0-открыто, а 1-закрыто, то второй параметр указывается с восклицательным знаком: digitalWrite(pinKey, !FLG_mode_ACCESS); 
     digitalWrite(pinGreenLed,   !FLG_mode_ACCESS);                       // включаем или выключаем светодиод подключённый к выводу pinGreenLed
     digitalWrite(pinRedLed, FLG_mode_ACCESS);                       // включаем или выключаем светодиод подключённый к выводу pinRedLed
     if(FLG_mode_ACCESS){tone(pinBeep, 2000, 100);}                  // если установлен флаг FLG_mode_ACCESS (замок открыт), то отправляем меандр на вывод pinBeep (к которому подключён Trema зуммер) длительностью 100 мс с частотой 2000 Гц
   }

// Сбрасываем флаг FLG_mode_ACCESS, указывающий о необходимости открытия замка, через 5 секунд после его установки
   if(FLG_mode_ACCESS){
     if( TIM_mode_ACCESS      >millis()){FLG_mode_ACCESS=0; FLG_display_UPD = 1; if(VAR_mode_MENU==1){VAR_mode_MENU=0;}}
     if((TIM_mode_ACCESS+3000)<millis()){FLG_mode_ACCESS=0; FLG_display_UPD = 1; if(VAR_mode_MENU==1){VAR_mode_MENU=0;}}
   }
}

// Функция управления кнопками:
void Func_buttons_control(){
  timeButtonA=0;                                                                                                                                    // время удержания кнопки A (в сотых долях секунды)
  TIM_Button_B=0;                                                                                                                                    // время удержания кнопки B (в сотых долях секунды)
  uint8_t f=0;                                                                                                                                       // 1-зафиксировано нажатие кнопки, 2-зафиксировано нажатие кнопки и очищен дисплей, 3-обе кнопки удерживались дольше 2 сек
  if(digitalRead(pinButtonA)){noTone(pinBeep);}                                                                                              // если нажата кнопка A то отключаем меандр на выводе pinBeep
  if(digitalRead(pinButtonB)){noTone(pinBeep);}                                                                                              // если нажата кнопка B то отключаем меандр на выводе pinBeep
  while(digitalRead(pinButtonA)||digitalRead(pinButtonB)){                                                                                       // если нажата кнопка A и/или кнопка B, то создаём цикл, пока кнопка(и) не будет(ут) отпущена(ы)
     if(digitalRead(pinButtonA)&&timeButtonA<65535){timeButtonA++; if(f==0){f=1;}}                                                               // если удерживается кнопка A, то увеличиваем время её удержания
     if(digitalRead(pinButtonB)&&TIM_Button_B<65535){TIM_Button_B++; if(f==0){f=1;}}                                                               // если удерживается кнопка B, то увеличиваем время её удержания
     if(f==1){myLCD.clear(); f=2;}                                                                                                                     // если зафиксировано нажатие кнопки, то стираем информацию с дисплея
     if(f<3 && timeButtonA>200 && TIM_Button_B>200){VAR_mode_MENU=0; Func_display_show(); f=3;}                                                     // если обе кнопки удерживаются дольше 2 сек, то выходим из меню (VAR_mode_MENU=0) и обновляем информацию на дисплее (Func_display_show();)
     delay(10);                                                                                                                                      // пропускаем 0,01с (подавляем дребезг кнопок)
  }
  if(f==2){                                                                                  FLG_display_UPD = 1;                                    // если зафиксировано нажатие на кнопку, то ...
    switch(VAR_mode_MENU){
      case  0: /* вне меню */                         if(timeButtonA >0 && TIM_Button_B >0){VAR_mode_MENU   = 10;                                 } // войти в меню
                                                      if(timeButtonA >0 && TIM_Button_B==0){FLG_mode_ACCESS = 1; TIM_mode_ACCESS=millis();        } // открыть замок
                                                      if(timeButtonA==0 && TIM_Button_B >0){FLG_mode_ACCESS = 1; TIM_mode_ACCESS=millis();        } // открыть замок
      break;
      case 10: /* 1 уровень меню - вкл/выкл замок  */ if(timeButtonA >0 && TIM_Button_B==0){VAR_mode_MENU   = 99;                                 } // перейти к  предыдущему пункту меню
                                                      if(timeButtonA==0 && TIM_Button_B >0){VAR_mode_MENU   = 20;                                 } // перейти к  следующему  пункту меню
                                                      if(timeButtonA >0 && TIM_Button_B >0){VAR_mode_MENU   = 11;                                 } // выбрать    текущий     пункт  меню
      break;
      case 11: /* 2 уровень меню - вкл/выкл замок  */ if(timeButtonA >0 && TIM_Button_B==0){FLG_state_WORK  = FLG_state_WORK?0:1;                 } // вкл/выкл
                                                      if(timeButtonA==0 && TIM_Button_B >0){FLG_state_WORK  = FLG_state_WORK?0:1;                 } // вкл/выкл
                                                      if(timeButtonA >0 && TIM_Button_B >0){VAR_mode_MENU   = 10;                                 } // выйти   из текущего    пункта меню
      break;
      case 20: /* 1 уровень меню - показать ID     */ if(timeButtonA >0 && TIM_Button_B==0){VAR_mode_MENU   = 10;                                 } // перейти к  предыдущему пункту меню
                                                      if(timeButtonA==0 && TIM_Button_B >0){VAR_mode_MENU   = 30;                                 } // перейти к  следующему  пункту меню
                                                      if(timeButtonA >0 && TIM_Button_B >0){VAR_mode_MENU   = 21;                                 } // выбрать    текущий     пункт  меню
      break;
//    case 21: /* 2 уровень меню - показать ID     */ это поиск ID с надписью Please wait ...
//                                                    без реакции на нажатие кнопок
//
      case 22: /* 3 уровень меню - показать ID     */ if(timeButtonA >0 && TIM_Button_B==0){VAR_first_ID   -= VAR_first_ID>0?3:0;                 } // уменьшить первый выводимый на дисплей ID
                                                      if(timeButtonA==0 && TIM_Button_B >0){VAR_first_ID   += ((VAR_first_ID+3)<VAR_count_ID)?3:0;} // увеличить первый выводимый на дисплей ID
                                                      if(timeButtonA >0 && TIM_Button_B >0){VAR_mode_MENU   = 20;                                 } // выйти   из текущего    пункта меню
      break;
      case 30: /* 1 уровень меню - сохранить ID    */ if(timeButtonA >0 && TIM_Button_B==0){VAR_mode_MENU   = 20;                                 } // перейти к  предыдущему пункту меню
                                                      if(timeButtonA==0 && TIM_Button_B >0){VAR_mode_MENU   = 40;                                 } // перейти к  следующему  пункту меню
                                                      if(timeButtonA >0 && TIM_Button_B >0){VAR_mode_MENU   = 31;                                 } // выбрать    текущий     пункт  меню
      break;
//    case 31: /* 2 уровень меню - сохранить ID    */ это поиск ID с надписью Please wait ...
//                                                    без реакции на нажатие кнопок
//
      case 32: /* 3 уровень меню - сохранить ID    */ if(timeButtonA >0 && TIM_Button_B==0){VAR_this_ID    -= VAR_this_ID>0?1:0;                  } // уменьшить ID по которому будет сохранён отпечаток
                                                      if(timeButtonA==0 && TIM_Button_B >0){VAR_this_ID    += VAR_this_ID<162?1:0;                } // увеличить ID по которому будет сохранён отпечаток
                                                      if(timeButtonA >0 && TIM_Button_B >0){VAR_mode_MENU   = 33;                                 } // выбрать    текущий     пункт  меню
      break;
//    case 33: /* 4 уровень меню - сохранить ID    */ это ожидание прикладываемого пальца (в первый раз) с надписью Put finger ...
//                                                    без реакции на нажатие кнопок
//
//    case 34: /* 5 уровень меню - сохранить ID    */ это ожидание конвертации изображения в шаблон (в первый раз) с надписью converting ...
//                                                    без реакции на нажатие кнопок
//
//    case 35: /* 6 уровень меню - сохранить ID    */ это ожидание отсутствия пальца перед сканером с надписью remove finger ...
//                                                    без реакции на нажатие кнопок
//
//    case 36: /* 7 уровень меню - сохранить ID    */ это ожидание прикладываемого пальца (во второй раз) с надписью Put finger ...
//                                                    без реакции на нажатие кнопок
//
//    case 37: /* 8 уровень меню - сохранить ID    */ это ожидание конвертации изображения в шаблон (во второй раз) с надписью converting ...
//                                                    без реакции на нажатие кнопок
//
//    case 38: /* 9 уровень меню - сохранить ID    */ это ожидание создания и сохранения шаблона с надписью Saved ...
//                                                    без реакции на нажатие кнопок
//
      case 39: /*10 уровень меню - сохранить ID    */ if(timeButtonA >0 && TIM_Button_B==0){VAR_mode_MENU   = 30;                                 } // в начало   текущего    пункта меню
                                                      if(timeButtonA==0 && TIM_Button_B >0){VAR_mode_MENU   = 30;                                 } // в начало   текущего    пункта меню
                                                      if(timeButtonA >0 && TIM_Button_B >0){VAR_mode_MENU   = 30;                                 } // в начало   текущего    пункта меню
      break;
      case 40: /* 1 уровень меню - удалить ID      */ if(timeButtonA >0 && TIM_Button_B==0){VAR_mode_MENU   = 30;                                 } // перейти к  предыдущему пункту меню
                                                      if(timeButtonA==0 && TIM_Button_B >0){VAR_mode_MENU   = 99;                                 } // перейти к  следующему  пункту меню
                                                      if(timeButtonA >0 && TIM_Button_B >0){VAR_mode_MENU   = 41; VAR_this_ID=0;                  } // выбрать    текущий     пункт  меню
      break;
      case 41: /* 2 уровень меню - удалить ID      */ if(timeButtonA >0 && TIM_Button_B==0){VAR_this_ID     = VAR_this_ID==255?0:255;             } // 255-all / 0-one
                                                      if(timeButtonA==0 && TIM_Button_B >0){VAR_this_ID     = VAR_this_ID==255?0:255;             } // 255-all / 0-one
                                                      if(timeButtonA >0 && TIM_Button_B >0){VAR_mode_MENU   = VAR_this_ID==255?42:43;             } // выбрать    текущий     пункт  меню
      break;
//    case 42: /* 3 уровень меню - удалить ID      */ это удаление всех ID с надписью Please wait ...
//                                                    без реакции на нажатие кнопок   
//
//    case 43: /* 4 уровень меню - удалить ID      */ это поиск ID с надписью Please wait ...
//                                                    без реакции на нажатие кнопок
      case 44: /* 5 уровень меню - удалить ID      */ if(timeButtonA >0 && TIM_Button_B==0){VAR_this_ID    -= VAR_this_ID>0?1:0;                  } // уменьшить ID по которому будет удалён отпечаток
                                                      if(timeButtonA==0 && TIM_Button_B >0){VAR_this_ID    += VAR_this_ID<162?1:0;                } // увеличить ID по которому будет удалён отпечаток
                                                      if(timeButtonA >0 && TIM_Button_B >0){VAR_mode_MENU   = 45;                                 } // выбрать    текущий     пункт  меню
      break;
//    case 45: /* 6 уровень меню - удалить ID      */ это удаление выбранного ID с надписью Please wait ...
//                                                    без реакции на нажатие кнопок   
//
      case 99: /* 1 уровень меню - выйти из меню   */ if(timeButtonA >0 && TIM_Button_B==0){VAR_mode_MENU   = 40;                                 } // перейти к  предыдущему пункту меню
                                                      if(timeButtonA==0 && TIM_Button_B >0){VAR_mode_MENU   = 10;                                 } // перейти к  следующему  пункту меню
                                                      if(timeButtonA >0 && TIM_Button_B >0){VAR_mode_MENU   = 0;                                  } // выбрать    текущий     пункт  меню
      break;
    }
  }
}

// функция вывода информации на дисплей
void Func_display_show(){ 
  FLG_display_UPD=0;        
  myLCD.clear();
  switch(VAR_mode_MENU){
    case  0: /* вне меню */                   
                                                    myLCD.setCursor(0, 0); myLCD.print(F("ДОСТУП:")); myLCD.print(FLG_mode_ACCESS ? F("ОТКРЫТО") : F("ЗАКРЫТО" ) );
                                                    
    break;                                         
    case  1: /* замок открыт отпечатком */          myLCD.setCursor(0, 0); myLCD.print(F("НАЙДЕННЫЕ ID:")); if(VAR_this_ID<100){myLCD.print(F(""));} if(VAR_this_ID<10){myLCD.print(F(""));} myLCD.print(VAR_this_ID);
                                                    myLCD.setCursor(0, 1); myLCD.print(F("ДОСТУП: ОТКРЫТО"));
    break;
    case 10: /* 1 уровень меню - вкл/выкл замок  */ myLCD.setCursor(0, 0); myLCD.print(F("  <   МЕНЮ   > "));
    break;
    
    case 20: /* 1 уровень меню - показать ID     */ myLCD.setCursor(0, 0); myLCD.print(F("МЕНЮ>ПОКАЗАТЬ ID"));
    break;
    case 21: /* 2 уровень меню - показать ID     */ myLCD.setCursor(0, 0); myLCD.print(F("Поиск Сенсора..."));
                                                    myLCD.setCursor(0, 1); myLCD.print(F("Подождите..."));
    break;
    case 22: /* 3 уровень меню - показать ID     */ myLCD.setCursor(0, 0); myLCD.print(F("НАЙДЕННЫ ")); myLCD.print(VAR_count_ID); myLCD.print(F(" ID:"));
                                                    myLCD.setCursor(0, 1); myLCD.print(VAR_first_ID==0?F(""):F("<")); for(int i=0; i<3; i++){if(VAR_count_ID>VAR_first_ID+i){myLCD.print(i==0?F(" "):F(",")); if(VAR_array_ID[(VAR_first_ID+i)]<100){myLCD.print(F(""));} if(VAR_array_ID[(VAR_first_ID+i)]<10){myLCD.print(F(""));} myLCD.print(VAR_array_ID[(VAR_first_ID+i)]);}} if(VAR_count_ID>(VAR_first_ID+3)){myLCD.setCursor(15,1); myLCD.print(F(">"));}
    break;
    case 30: /* 1 уровень меню - сохранить ID    */ myLCD.setCursor(0, 0); myLCD.print(F("МЕНЮ>ДОБАВИТЬ ID"));
    break;
    case 31: /* 2 уровень меню - сохранить ID    */ myLCD.setCursor(0, 0); myLCD.print(F("Поиск Сенсора..."));
                                                    myLCD.setCursor(0, 1); myLCD.print(F("Подождите..."));
    break;
    case 32: /* 3 уровень меню - сохранить ID    */ myLCD.setCursor(0, 0); myLCD.print(F("МЕНЮ>NewID>SetID"));
                                                    myLCD.setCursor(6, 1); if(VAR_this_ID<100){myLCD.print(F(""));} if(VAR_this_ID<10){myLCD.print(F(""));} myLCD.print(VAR_this_ID);
                                                    myLCD.setCursor(4, 1); if(VAR_this_ID>0  ){myLCD.print(F("<"));}
                                                    myLCD.setCursor(10,1); if(VAR_this_ID<162){myLCD.print(F(">"));}
    break;
    case 33: /* 4 уровень меню - сохранить ID    */ myLCD.setCursor(0, 0); myLCD.print(F("МЕНЮ >ДОБАВИТЬ ID ")); if(VAR_this_ID<100){myLCD.print(F(""));} if(VAR_this_ID<10){myLCD.print(F(""));} myLCD.print(VAR_this_ID);
                                                    myLCD.setCursor(0, 1); myLCD.print(F("Приложите палец..."));
                                                    delay(1000);
    break;
    case 34: /* 5 уровень меню - сохранить ID    */ myLCD.setCursor(0, 0); myLCD.print(F("МЕНЮ >ДОБАВИТЬ ID ")); if(VAR_this_ID<100){myLCD.print(F(""));} if(VAR_this_ID<10){myLCD.print(F(""));} myLCD.print(VAR_this_ID);
                                                    myLCD.setCursor(0, 1); myLCD.print(F("Преобразование..."));
    break;
    case 35: /* 6 уровень меню - сохранить ID    */ myLCD.setCursor(0, 0); myLCD.print(F("МЕНЮ >ДОБАВИТЬ ID ")); if(VAR_this_ID<100){myLCD.print(F(""));} if(VAR_this_ID<10){myLCD.print(F("0"));} myLCD.print(VAR_this_ID);
                                                    myLCD.setCursor(0, 1); myLCD.print(F("Уберите палец..."));
                                                    delay(1000);
    break;
    case 36: /* 7 уровень меню - сохранить ID    */ myLCD.setCursor(0, 0); myLCD.print(F("МЕНЮ >ДОБАВИТЬ ID ")); if(VAR_this_ID<100){myLCD.print(F(""));} if(VAR_this_ID<10){myLCD.print(F(""));} myLCD.print(VAR_this_ID);
                                                    myLCD.setCursor(0, 1); myLCD.print(F("Приложите палец..."));
                                                    delay(1000);
    break;
    case 37: /* 8 уровень меню - сохранить ID    */ myLCD.setCursor(0, 0); myLCD.print(F("МЕНЮ >ДОБАВИТЬ ID ")); if(VAR_this_ID<100){myLCD.print(F(""));} if(VAR_this_ID<10){myLCD.print(F(""));} myLCD.print(VAR_this_ID);
                                                    myLCD.setCursor(0, 1); myLCD.print(F("Преобразование..."));
    break;
    case 38: /* 9 уровень меню - сохранить ID    */ myLCD.setCursor(0, 0); myLCD.print(F("МЕНЮ >ДОБАВИТЬ ID ")); if(VAR_this_ID<100){myLCD.print(F(""));} if(VAR_this_ID<10){myLCD.print(F(""));} myLCD.print(VAR_this_ID);
                                                    myLCD.setCursor(0, 1); myLCD.print(F("СОХРАНЕНИЕ ..."));
    break;
    case 39: /*10 уровень меню - сохранить ID    */ myLCD.setCursor(0, 0); myLCD.print(F("МЕНЮ >ДОБАВИТЬ ID ")); if(VAR_this_ID<100){myLCD.print(F(""));} if(VAR_this_ID<10){myLCD.print(F(""));} myLCD.print(VAR_this_ID);
                                                    myLCD.setCursor(0, 1); myLCD.print(FLG_result_FUN?F("Добавлено!"):F("Ошибка :("));
    break;
    case 40: /* 1 уровень меню - удалить ID      */ myLCD.setCursor(0, 0); myLCD.print(F("МЕНЮ >УДАЛИТЬ ID"));
    break;
    case 41: /* 2 уровень меню - удалить ID      */ myLCD.setCursor(0, 0); myLCD.print(F("МЕНЮ>УДАЛИТЬ"));
                                                    myLCD.setCursor(13,0); myLCD.print(F("ВСЕ"));
                                                    myLCD.setCursor(13,1); myLCD.print(F("1"));
                                                    myLCD.setCursor(11, VAR_this_ID==255?0:1); myLCD.print(F(">"));
    break;
    case 42: /* 3 уровень меню - удалить ID      */ myLCD.setCursor(0, 0); myLCD.print(F("УДАЛЕНИЕ ID ..."));
                                                    myLCD.setCursor(0, 1); myLCD.print(F("Подождите..."));
    break;
    case 43: /* 4 уровень меню - удалить ID      */ myLCD.setCursor(0, 0); myLCD.print(F("Поиск Сенсора ..."));
                                                    myLCD.setCursor(0, 1); myLCD.print(F("Подождите..."));
    break;
    case 44: /* 5 уровень меню - удалить ID      */ myLCD.setCursor(0, 0); myLCD.print(F("МЕНЮ>УДАЛИТЬ 1ID"));
                                                    myLCD.setCursor(6, 1); if(VAR_this_ID<100){myLCD.print(F(""));} if(VAR_this_ID<10){myLCD.print(F(""));} myLCD.print(VAR_this_ID);
                                                    myLCD.setCursor(4, 1); if(VAR_this_ID>0  ){myLCD.print(F("<"));}
                                                    myLCD.setCursor(10,1); if(VAR_this_ID<162){myLCD.print(F(">"));}
    break;
    case 45: /* 6 уровень меню - удалить ID      */ myLCD.setCursor(0, 0); myLCD.print(F("МЕНЮ>УДАЛИТЬ ID ")); if(VAR_this_ID<100){myLCD.print(F(""));} if(VAR_this_ID<10){myLCD.print(F(""));} myLCD.print(VAR_this_ID);
                                                    myLCD.setCursor(0, 1); myLCD.print(F("Подождите..."));
    break;
    case 49: /*10 уровень меню - удалить ID      */ myLCD.setCursor(0, 0); myLCD.print(F("МЕНЮ> УДАЛИТЬ ")); if(VAR_this_ID==255){myLCD.print(F("Все ID"));}else{myLCD.print(F("ID "));} if(VAR_this_ID<100){myLCD.print(F(""));} if(VAR_this_ID<10){myLCD.print(F(""));} myLCD.print(VAR_this_ID);
                                                    myLCD.setCursor(0, 1); myLCD.print(FLG_result_FUN?F("Палец удален!"):F("Ошибка :("));
    break;
    case 99: /* 1 уровень меню - выйти из меню   */ myLCD.setCursor(0, 0); myLCD.print(F("МЕНЮ > ВЫХОД"));
    break;
  }
}

// функция общения с модулем отпечатков пальцев
void Func_sensor_communication(){
  switch(VAR_mode_MENU){
//            Если требуется сверять отпечатки пальцев
    case  0:  if(!FLG_mode_ACCESS){
              if(millis()%500              == 0){                                    // проверяем каждые 500 мс
              if(finger.getImage()         == FINGERPRINT_OK){                       // Захватываем изображение, если результат выполнения равен константе FINGERPRINT_OK (корректная загрузка изображения), то проходим дальше
              if(finger.image2Tz()         == FINGERPRINT_OK){                       // Конвертируем полученное изображение, если результат выполнения равен константе FINGERPRINT_OK (изображение сконвертировано), то проходим дальше
              if(finger.fingerFastSearch() == FINGERPRINT_OK){                       // Находим соответствие в базе данных отпечатков пальцев, если результат выполнения равен константе FINGERPRINT_OK (найдено соответствие), то проходим дальше
                VAR_this_ID     = finger.fingerID;
                VAR_mode_MENU   = 1;
                FLG_display_UPD = 1;
                FLG_mode_ACCESS = 1;
                TIM_mode_ACCESS = millis();
              }else{finger.fingerFastSearch();}}}}}                                  // Эта строка не являеется обработчиком отсутствия соответствий в базе данных отпечатков пальцев! Просто без неё в некоторых версиях Arduino IDE компилятор не сможет корректно подставить результат функции fingerFastSearch() для сравнения с константой FINGERPRINT_OK в условии оператора if().
    break;
//            Если требуется найти все ID сохранённые в базе отпечатков
    case  21: VAR_first_ID=0; VAR_count_ID=0;                                        // сбрасываем количество найденных Id и номер первого среди отображенных
              for(uint8_t id = 0; id<162; id++){                                     // проход по 162 идентификаторам
                if(finger.loadModel(id) == FINGERPRINT_OK){                          // если по указанному идентификатору найден шаблон отпечатка, то ...
                  VAR_array_ID[VAR_count_ID]=id; VAR_count_ID++;                     // сохраняем номер идентификатора и увеличиваем количество найденных ID
                }
              } VAR_mode_MENU=22; FLG_display_UPD=1;                                 // переходим в 3 уровень меню ( показать ID ) и устанавливаем флаг указывающий о необходимости обновить информацию на дисплее
    break;
//            Если требуется найти первый свободный ID в базе отпечатков для его изменения на требуемый для сохранения
    case  31: VAR_this_ID=161;                                                       // устанавливаем номер 161 для первого свободного ID в базе отпечатков
              for(uint8_t id = 0; id<162; id++){                                     // проход по 162 идентификаторам
                if(VAR_this_ID==161){if(finger.loadModel(id)!=FINGERPRINT_OK){VAR_this_ID=id;}} // если по указанному идентификатору не найден шаблон отпечатка, то сохраняем его
              } VAR_mode_MENU=32; FLG_display_UPD=1;                                 // переходим в 3 уровень меню ( сохранить ID ) и устанавливаем флаг указывающий о необходимости обновить информацию на дисплее
    break;
//            Если требуется определить наличие пальца на сканере и сосканировать его
    case  33: ACK_finger_FUN=finger.getImage();                                      // захватываем изображение, с сохранением результата в переменную ACK_finger_FUN
              if(ACK_finger_FUN==FINGERPRINT_OK){                                    // изображение отпечатка пальца корректно загрузилось
                VAR_mode_MENU=34; FLG_display_UPD=1;                                 // переходим в 5 уровень меню ( сохранить ID ) и устанавливаем флаг указывающий о необходимости обновить информацию на дисплее
              }else if(ACK_finger_FUN==FINGERPRINT_NOFINGER){                        // сканер не обнаружил отпечаток пальца, остаёмся в текущем уровне
              }else{FLG_result_FUN=0; VAR_mode_MENU=39; FLG_display_UPD=1;}          // переходим в 10 уровень меню ( результат сохранения ID ) и устанавливаем флаг указывающий о необходимости обновить информацию на дисплее
    break;
//            Если требуется сконвертировать первое полученное отсканированное изображение
    case  34: FLG_result_FUN=0; FLG_display_UPD=1;                                   // устанавливаем флаг указывающий о необходимости обновить информацию на дисплее
              VAR_mode_MENU=finger.image2Tz(1)==FINGERPRINT_OK?35:39;                // конвертируем первое изображение, если результат выполнения данной операции == FINGERPRINT_OK, значит изображение сконвертированно
    break;
//            Если требуется зафиксировать отсутствие пальца на сканере
    case  35: delay(500); VAR_mode_MENU=36; FLG_display_UPD=1;                       // ждём 0.5 сек и пытаемся перейти в 7 уровень меню ( сохранить ID )
              while(finger.getImage() != FINGERPRINT_NOFINGER){;}                    // не выходим из цикла, пока палец не перестанет сканироваться
    break;
//            Если требуется определить наличие пальца на сканере и сосканировать его
    case  36: ACK_finger_FUN=finger.getImage();                                      // захватываем изображение, с сохранением результата в переменную ACK_finger_FUN
              if(ACK_finger_FUN==FINGERPRINT_OK){                                    // изображение отпечатка пальца корректно загрузилось
                VAR_mode_MENU=37; FLG_display_UPD=1;                                 // переходим в 8 уровень меню ( сохранить ID ) и устанавливаем флаг указывающий о необходимости обновить информацию на дисплее
              }else if(ACK_finger_FUN==FINGERPRINT_NOFINGER){                        // сканер не обнаружил отпечаток пальца, остаёмся в текущем уровне
              }else{FLG_result_FUN=0; VAR_mode_MENU=39; FLG_display_UPD=1;}          // переходим в 10 уровень меню ( результат сохранения ID ) и устанавливаем флаг указывающий о необходимости обновить информацию на дисплее
    break;
//            Если требуется сконвертировать второе полученное отсканированное изображение
    case  37: FLG_result_FUN=0; FLG_display_UPD=1;                                   // устанавливаем флаг указывающий о необходимости обновить информацию на дисплее
              VAR_mode_MENU=finger.image2Tz(2)==FINGERPRINT_OK?38:39;                // Конвертируем второе изображение, если результат выполнения данной операции == FINGERPRINT_OK, значит изображение сконвертированно
    break;
//            Если требуется создать и сохранить шаблон из первых двух отсканированных изображений
    case  38: FLG_result_FUN=0; VAR_mode_MENU=39; FLG_display_UPD=1;                 // пытаемся перейти в 10 уровень меню ( результат сохранения ID ) и устанавливаем флаг указывающий о необходимости обновить информацию на дисплее
              if(finger.createModel()==FINGERPRINT_OK){                              // создаем модель (шаблон) отпечатка пальца, по двум изображениям
              if(finger.storeModel(VAR_this_ID)==FINGERPRINT_OK){                    // сохраняем модель (шаблон) отпечатка пальца, по двум изображениям
                FLG_result_FUN=1;                                                    // если создание и сохранение модель (шаблона) выполнено успешно, то устанавливаем флаг указывающий об успешном сохранении
              }else{finger.storeModel(VAR_this_ID);}                                 // Эта строка не является обработкой ошибки сохранения модели (шаблона) отпечатка пальца! Просто без неё в некоторых версиях Arduino IDE компилятор не сможет корректно подставить результат функции storeModel()  для сравнения с константой FINGERPRINT_OK в условии оператора if().
              }else{finger.createModel();}                                           // Эта строка не является обработкой ошибки создания   модели (шаблона) отпечатка пальца! Просто без неё в некоторых версиях Arduino IDE компилятор не сможет корректно подставить результат функции createModel() для сравнения с константой FINGERPRINT_OK в условии оператора if().
    break;
    case  39: delay(2000); FLG_result_FUN=0; VAR_mode_MENU=30; FLG_display_UPD=1;    // выходим из меню через 2 сек
    break;
//            Если требуется удалить все ID из базы отпечатков
    case  42: FLG_result_FUN=1; VAR_mode_MENU=49; FLG_display_UPD=1;                 // устанавливаем результат и пытаемся перейти в 10 уровень меню ( результат удаления ID ) установив флаг указывающий о необходимости обновить информацию на дисплее
              for(uint8_t id = 0; id<162; id++){                                     // проход по 162 идентификаторам
                if(finger.loadModel(id) == FINGERPRINT_OK){                          // если по указанному идентификатору найден шаблон отпечатка, то ...
                  if(finger.deleteModel(id)!=FINGERPRINT_OK){FLG_result_FUN=1;}      // если шаблон не удаляется, сбрасываем результат
                }
              }
    break;
//            Если требуется найти последний занятый ID в базе отпечатков для его изменения на требуемый для сохранения
    case  43: VAR_this_ID=0;                                                         // сбрасываем номер последнего занятого ID в базе отпечатков
              for(uint8_t id = 0; id<162; id++){                                     // проход по 162 идентификаторам
                if(finger.loadModel(id)==FINGERPRINT_OK){VAR_this_ID=id;}            // если по указанному идентификатору найден шаблон отпечатка, то сохраняем его
              } VAR_mode_MENU=44; FLG_display_UPD=1;                                 // переходим в 5 уровень меню ( удалить ID ) и устанавливаем флаг указывающий о необходимости обновить информацию на дисплее
    break;
//            Если требуется удалить выбранный ID из базы отпечатков
    case  45: FLG_result_FUN=0; VAR_mode_MENU=49; FLG_display_UPD=1;                 // пытаемся перейти в 10 уровень меню ( результат удаления ID ) и устанавливаем флаг указывающий о необходимости обновить информацию на дисплее
              if(finger.deleteModel(VAR_this_ID)==FINGERPRINT_OK){FLG_result_FUN=1;} // Удаляем шаблон отпечатка пальца VAR_this_ID из базы данных, при успешном выполнении, устанавливаем флаг FLG_result_FUN
    break;
    case  49: delay(2000); FLG_result_FUN=0; VAR_mode_MENU=40; FLG_display_UPD=1;    // выходим из меню через 2 сек
    break;
    default: break;
  }
}
