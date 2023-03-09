#include <Arduino.h>
#include <Wire.h>
#include "OneWire.h"
#include "DallasTemperature.h"
#include <LCD_1602_RUS.h>

// use D2-D9, interface SPI (D11, D12) is free
#define Ten1 2
#define Ten2 3
#define Ten3 4
#define Ten4 5
#define Ten5 6
#define Ten6 7
#define Ten7 8
#define Ten8 9

// use pin D10
#define TemperatPin 10

// use D13, pumpe control
#define Pump 13

// use analog pins als digital A1-A3
#define buttonLeft 15
#define buttonOk 16
#define buttonRight 17

//check pump, HIGH if pump open (pin A7)
#define pumpCheck 21 

//check power, protection if power less as 4,5 V
int powCheck = A0;
// analog pin for getting drucks value
int DruckPin = A6;


//The heating temperature is set by the user or automatically. 
//The purpose of the program is to heat the boiler to the set temperature
//and maintain it unchanged.
double heating_temperature=40;
double druck=1;
double current_temperature;
double preview_temperature;
double deltaHeat; //variable to control the temperature change in the process of turning off the heaters
double deltaWait; //variable to control the temperature change in the process of turning on the heaters

//0 bit - 0 heat mode, 1 wait mode
//1 bit - flag 0 - all of, flag 1 - all on
//2 bit - 0 user mode in control of ten, 1 automatic
//3 bit - 0 - temperature reaching mode, 1 - keep warm mode
//4 bit use for crooked nail in yield for pooling buttonOk:
    //- 0 - buttonOk not pressed, 1 - pressed.
//5 bit - 0 druck protection, 1 no druck protection
//6 bit - 0 temperature protection, 1 no temperature protection
//7 bit - 0 pump protection, 1 no pump protection
uint8_t modeHeat=0;

uint8_t count=0;

//number of used tens (from 0 to 8)
//0-3 bit define program
//4-7 bit define user  - has first prioritat
uint8_t usedTlegal=0b10001000;
uint8_t usedT;

//synchronisation tens control in time, 
//not allowed turn of tens less than an interval 15 sec.
unsigned long previewMillis=millis();
unsigned long tm=millis();

//for debug in control memory
// Переменные, создаваемые процессом сборки,
// когда компилируется скетч (need for memory control)
//extern int __bss_end;
//extern void *__brkval;

//for information from Serial
uint8_t serNum = 0; 

//self simbols
byte left[8] = { 0b00011, 0b00111, 0b01111, 0b11111, 0b11111, 0b01111, 0b00111, 0b00011 };  //  arrow left
byte right[8] = { 0b11000, 0b11100, 0b11110, 0b11111, 0b11111, 0b11110, 0b11100, 0b11000 };  //  arrow right
byte gradus[8] = { 0b01100, 0b10010, 0b10010, 0b01100, 0b00000, 0b00000, 0b00000, 0b00000 };  //  gradus
byte topLeft[8] = { 0b00001, 0b00001, 0b00011, 0b00011, 0b00111, 0b00111, 0b01111, 0b11111 };  //  arrow top left
byte topRight[8] = { 0b10000, 0b10000, 0b11000, 0b11000, 0b11100, 0b11100, 0b11110, 0b11111 };  //  arrow top right

// Obj temperature
OneWire oneWire(TemperatPin);  //port sensors setting
DallasTemperature ds(&oneWire);

// Obj display
LCD_1602_RUS lcd(0x27,20,4);

//definition functions
void lcdError_test();
void sel_h_tem();
void con_but(uint8_t button);
uint8_t aktive_tens(uint8_t *tensArr);
void tens_control(uint8_t parametrs);
//int memoryFree(); //for debug
void(* resetFunc) (void) =0; // reset

void setup() {
  // Init pins
  pinMode(Pump, OUTPUT);

  pinMode(Ten1, OUTPUT);
  pinMode(Ten2, OUTPUT);
  pinMode(Ten3, OUTPUT);
  pinMode(Ten4, OUTPUT);
  pinMode(Ten5, OUTPUT);
  pinMode(Ten6, OUTPUT);
  pinMode(Ten7, OUTPUT);
  pinMode(Ten8, OUTPUT);
  
  delay(250);
  pinMode(buttonOk, INPUT_PULLUP);
  pinMode(buttonRight, INPUT_PULLUP);
  pinMode(buttonLeft, INPUT_PULLUP);
  pinMode(pumpCheck, INPUT_PULLUP);

  //initialisation ports monitor, sensor ds18b20, display 
  Serial.begin(115200);
  delay(1000);
  //Serial.println("Start! Initialisation sensor DS18B20 and LCD 2004 display."); 
  Serial.println("serNum"+String(serNum));
  ds.begin();
  lcd.init();//sda - A4 pin, scl - A5 pin

  // Pump turn on
  //Serial.println("Pin 13 Arduino D13 Pump logical level High.");
  digitalWrite(Pump, HIGH); 
  //Serial.println("Pump turn on - the built-in LED on the board should light up /-/");
  //serNum =1;
  //Serial.println("serNum"+String(serNum));
  delay(500);
  
  // light for LCD display, Cursors position, Test lcd print
  lcd.backlight();
  //lcdError_test();
  lcd.setCursor(7, 1);
  //Serial.println("LCD 2004 display start print...");
  serNum =2;
  Serial.println("serNum"+String(serNum));
  lcd.print("ИНИЦИАЛИЗАЦИЯ");
  lcdError_test();
  delay(1000);
  for (int i = 0; i < 7; i++) {
    lcd.scrollDisplayLeft();
    delay(300);
  }
  delay(1000);
  
  //start settings print //Fuction print "Начальная настройка!"
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("НАЧАЛЬНАЯ НАСТРОЙКА!");
  //lcdError_test();
  delay(2000);

  //Serial.println("LCD in funktion lcdPrint_set_temperat() print --Set Temperature of heatting press OK--");
  //serNum =3;
  //Serial.println("serNum"+String(serNum));
  lcd.setCursor(5, 0);
  lcd.clear(); 
  lcd.print("УСТАНОВИТЕ");
  lcd.setCursor(5, 1);
  lcd.print("ТЕМПЕРАТУРУ");
  lcd.setCursor(0, 2);
  lcd.print("НАГРЕВАНИЯ, НАЖАВ OK");
  lcd.setCursor(7, 3);
  lcd.print("<");
  lcd.print(" OK ");
  lcd.print(">");
  //lcdError_test();
  delay(500);
  // wait 5 min. wenn user ok press und select after heating temperature
  // if user not press ok while 5 min., than heating temperature 85C default
  //you must check to buttons working before
  //Serial.println("Wait 5 min wenn OK press");
  //serNum =4;
  //Serial.println("serNum"+String(serNum));
  for (uint16_t i = 0; i<30000; i++) {
    if(digitalRead(buttonOk) == LOW){
      //Serial.println("Ok pressed, select heating temperature.");
      //serNum =5;
      //Serial.println("serNum"+String(serNum));
      sel_h_tem();
      //lcdError_test();
      break;
    }
    if(i==29999){
      //Serial.println("LCD print notification default heating temperature 40C");
      serNum =6;
      Serial.println("serNum"+String(serNum));
      lcd.setCursor(4, 0);
      lcd.clear();
      lcd.print("ПО УМОЛЧАНИЮ");
      lcd.setCursor(2, 1);
      lcd.print("УСТАНАВЛИВАЕТСЯ");
      lcd.setCursor(5, 2);
      lcd.print("ТЕМПЕРАТУРА");
      lcd.setCursor(3, 3);
      lcd.print("НАГРЕВАНИЯ 40");
      lcd.print("C");
      //lcdError_test();
      delay(4000);
    }
    delay(10);
  }
  //Serial.println("Define. Heating temperature: ");
  //serNum =7;
  //Serial.println("serNum"+String(serNum));
  //Serial.println(heating_temperature);
} 


void loop() {
  //Standart view
  //String str;
  //unsigned char* buf;
  //const char *str2;
  uint8_t scroll_pointer;
  uint8_t tensArr;
  // measurement temperature, druck on standat view
  //Serial.println("LCD print standart view");
  serNum =8;
  Serial.println("serNum"+String(serNum));
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ДАВЛЕНИЕ");
  lcd.setCursor(0, 1);
  lcd.print("ТЕМПЕРАТУРА");
  lcd.setCursor(0, 2);
  lcd.print("<");
  lcd.setCursor(9, 2);
  lcd.print("OK");
  lcd.setCursor(19, 2);
  lcd.print(">");
  lcd.setCursor(1, 3);
  lcd.print("ИЗМЕНИТЬ НАСТРОИКИ");
  //add print error and notification
  lcdError_test();
  // confirmation that button OK release
  //Serial.println("LCD print temperature, druck value on standat view");
  //serNum =9;
  //Serial.println("serNum"+String(serNum));
  con_but(buttonOk);
  modeHeat=modeHeat & 0b11101111;//update flag buttonOk

  //mainly cycle
  while (!((modeHeat & 0b10000)>>4)){//while buttonOk not pressed
    //protection power check
    if(analogRead(powCheck)<920){
      //Serial.println("Unstable power! is (Volt) ");
      serNum =54;
      Serial.println("serNum"+String(serNum));
      Serial.println(String((analogRead(powCheck) * 5.0 ) / 1024.0));
      //get array of aktive tens
      tensArr=aktive_tens(&tensArr);
      // all of
      for(uint8_t i=0; i<8; i++){
        if(((1<<i) & tensArr)>>i==1){
          tens_control(i | 0b01010000);
        }
      }
      con_but(buttonOk);
      while(digitalRead(buttonOk)==HIGH){
        lcd.clear();
        delay(200);
        lcd.setCursor(9, 0);
        lcd.print("!!");
        lcd.setCursor(0, 1);
        lcd.print("НЕСТАБИЛЬНОЕ ПИТАНИЕ");
        //lcd.setCursor(8, 2);
        //lcd.print(String(analogRead(powCheck)));
        //lcd.print(" V");
        lcd.setCursor(7, 3);
        lcd.print("<  OK  >");
        delay(4750);
      }
      resetFunc();
    }
    //protection pump check
    //turn off all tens in emergency mode if pump of.
    if((digitalRead(pumpCheck)!=HIGH) && (!((modeHeat & 0b10000000)>>7))){
      //Serial.println("There is no signal from the pump!");
      serNum =55;
      Serial.println("serNum"+String(serNum));
      //get array of aktive tens
      tensArr=aktive_tens(&tensArr);
      // all of
      for(uint8_t i=0; i<8; i++){
        if(((1<<i) & tensArr)>>i==1){
          tens_control(i | 0b01010000);
        }
      }
      con_but(buttonOk);
      count=0;
      while(digitalRead(buttonOk)==HIGH){
        lcd.clear();
        delay(200);
        lcd.setCursor(2, 0);
        lcd.print("OTCYTCTBYET СИГНАЛ");
        lcd.setCursor(5, 1);
        lcd.print("OT HACOCA!");
        lcd.setCursor(1, 2);
        lcd.print("ОТКЛЮЧИТЬ ЗАЩИТУ?");
        lcd.setCursor(7, 3);
        lcd.print("<  OK  >");
        delay(4750);
        count=count+1;
        if(count==180){
          resetFunc();
        }
      }
      modeHeat=modeHeat | 0b10010000; //turn of pump protection and on flag buttonOk
      count=100;
    }
    //print druck onse in 30 sec.
    if(count==100){
      druck=druck/10;
      //Serial.println("Average value druck is ");
      serNum =51;
      Serial.println("serNum"+String(serNum));
      Serial.println(String(druck));
      //str = String(druck);
      //buf = new unsigned char[10];
      //str.getBytes(buf, 10, 0);
      //str2 = (const char*)buf;
      lcd.setCursor(10, 0);
      lcd.print(String(druck));
      lcd.print(" bar");
      //delete []buf;
      //protection druck
      //turn off all tens in emergency mode if druck is critikal and on druck protection.
      if((druck<0.5 || druck>5) && (!((modeHeat & 0b100000)>>5))){
        //Serial.println("Critical pressure!");
        serNum =56;
        Serial.println("serNum"+String(serNum));
        //get array of aktive tens
        tensArr=aktive_tens(&tensArr);
        // all of
        for(uint8_t i=0; i<8; i++){
          if(((1<<i) & tensArr)>>i==1){
            tens_control(i | 0b01010000);
          }
        }
        con_but(buttonOk);
        count=0;
        while(digitalRead(buttonOk)==HIGH){
          lcd.clear();
          delay(200);
          lcd.setCursor(0, 0);
          lcd.print("КРИТИУЕСКОЕ ДАВЛЕНИЕ");
          lcd.setCursor(4, 1);
          lcd.print("! ");
          lcd.print(String(druck));
          lcd.print(" bar !");
          lcd.setCursor(1, 2);
          lcd.print("ОТКЛЮУИТЬ ЗАЩИТУ?");
          lcd.setCursor(7, 3);
          lcd.print("<  OK  >");
          delay(4750);
          count=count+1;
          if(count==180){
            resetFunc();
          }
        }
        modeHeat=modeHeat | 0b110000; //turn of druck protection and on flag buttonOk
      }
      druck=0;
      count=0;
    }
    //request temperature once in 3 secunde
    if((count==0) || (count==10) || (count==20) || (count==30) || (count==40) || (count==50) || (count==60) || (count==70) || (count==80) || (count==90)){
      //Serial.println("Count is ");
      serNum =61;
      Serial.println("serNum"+String(serNum));
      Serial.println(count);
      //Serial.println("Memore free is ");
      //serNum =30;
      //Serial.println("serNum"+String(serNum));
      //Serial.println(memoryFree()); // печать количества свободной оперативной памяти
      druck=druck+((double)analogRead(DruckPin)*10.3421/818.4-1.293);
      //Serial.println("Druck is ");
      serNum =58;
      Serial.println("serNum"+String(serNum));
      Serial.println(String(analogRead(DruckPin)*10.3421/818.4-1.293));
      ds.requestTemperatures();
      current_temperature=ds.getTempCByIndex(0);
      //Serial.println("Current_temperature is ");
      serNum =59;
      Serial.println("serNum"+String(serNum));
      Serial.println(String(current_temperature));
      //str = String(current_temperature);
      //buf = new unsigned char[10];
      //str.getBytes(buf, 10, 0);
      //str2 = (const char*)buf;
      lcd.setCursor(13, 1);
      lcd.print(String(current_temperature));
      lcd.print(" C");
      //delete []buf;
      //Serial.println(".");
      //serNum =10;
      //Serial.println(serNum);
      
      //get array of aktive tens
      tensArr=aktive_tens(&tensArr);
      //Serial.println("Array of aktive tens is ");
      serNum =60;
      Serial.println("serNum"+String(serNum));
      Serial.println(String(tensArr, BIN));

      //protection temperature check
      //turn off all tens in emergency mode if temperature is critikal and on temperature protection.
      if((current_temperature < -2 || current_temperature >90) && (!((modeHeat & 0b1000000)>>6))){
        //Serial.println("Critical temperature!");
        serNum =57;
        Serial.println("serNum"+String(serNum));
        // all of
        for(uint8_t i=0; i<8; i++){
          if(((1<<i) & tensArr)>>i==1){
            tens_control(i | 0b01010000);
          }
        }
        con_but(buttonOk);
        count=0;
        while(digitalRead(buttonOk)==HIGH){
          lcd.clear();
          delay(200);
          lcd.setCursor(3, 0);
          lcd.print("! КРИТИЧЕСКАЯ ТЕМПЕРАТУРА ");
          lcd.setCursor(0, 1);
          lcd.print("ТЕМПЕРАТУРА ");
          lcd.print(String(current_temperature));
          lcd.print("C !");
          lcd.setCursor(1, 2);
          lcd.print("ОТКЛЮЧИТЬ ЗАЩИТУ?");
          lcd.setCursor(7, 3);
          lcd.print("<  OK  >");
          delay(4750);
          count=count+1;
          if(count==180){// in 15 min.
            resetFunc();
          }
        }
        modeHeat=modeHeat | 0b1010000; //turn of temperature protection and on flag buttonOk
      }

      //algorithm
      // 1. Программа работает в двух режимах:
      //      - режим нагревания (mode heating) - когда текущая температура ниже установленной температуры нагревания, 
      //        то есть следует нагреть систему.  
      //      - режим ожидания (mode waiting) - когда текущая температура выше установленной температуры нагревания, 
      //        то есть следует дать системе остыть.
      //    Так как температура не сразу понижается/повышается с момента выключения/включения тэнов,
      //    то вводятся два параметра, учитывающие разницу:
      //      - deltaHeat - максимальное изменение температуры в сторону увеличения в процессе выключения тэнов
      //        начальное значение 4,5
      //      - deltaWait - максимальное изменение температуры в сторону уменьшения в процессе включения тэнов
      //        начальное значение 0,5
      //    Кроме вышеназванных режимов, для удобного подсчета параметров д и используются следующие два режима:
      //      -  режим достижения установленной температуры (temperature reaching mode)- выставляется когда 
      //         пользователь переустановил поддерживаемую температуру нагревания, а также при запуске  программы.
      //      -  режим поддержки установленной температуры (keep warm mode) - когда  
      //         текущая температура вблизи установленной и требуется лишь её поддерживать.
      // 2. В режиме нагревания (mode heating):
      //    2.1  Проверяем текущую температуру:
      //         Если она превышает установленную (+0,5) на deltaHeat градусов, то переключает на режим ожидания, 
      //           определяем два параметра preview_temperature и deltaHeat. Preview_temperature по сути равняется 
      //           текущей температуре. Если текущий режим достижения (temperature reaching mode), то deltaHeat 
      //           оставляется без изменений, в противном случае выставляется начальное значение deltaHeat,
      //           равное превышению текущей температуры над установленной (+0,5 - deltaHeat).
      //    2.2  Если текущая температура не превысила установленную, то измеряется deltaHeat, равное разнице между 
      //           максимально низкой текущей температурой и preview_temperature (preview_temperature определяется 
      //           как текущая температура при переключении режимов). Также если текущий режим поддержки температуры 
      //           (keep warm mode), то устанавливается разрешение на включение только одного тэна. Если его  
      //           недостаточно и температура продолжает падать, то к разрешению добавляется еще один тэн, и так до 8.
      //    2.3  Добавляется по одному включенному тэну при соблюдении ряда условий:
      //           - для включения ищется первый выключенный тэн с начала списка, причем разрешается включать 
      //             столько тэнов, сколько позволено пользователем (приоритет).
      //           - Включается один тэн, если число включенных тенов не превышает число разрешенных программой 
      //             в пункте 2.2 и если текущий режим нагревания и с момента последнего включения/выключения прошло 
      //             не менее 10 секунд. 
      //             Алгоритм может включить меньше тенов, чем установлено пользователем, но не больше.
      //  3. В режиме ожидания (mode waiting):
      //    3.1  Проверяем текущую температуру:
      //         Если она стала ниже установленной (-4,5) на deltaWait градусов, то переключает на режим нагревания, 
      //           также устанавливается разрешение на включение только одного тэна и также
      //           определяем два параметра preview_temperature и deltaWait. Preview_temperature по сути равняется 
      //           текущей температуре. Если текущий режим достижения (temperature reaching mode), то deltaWait 
      //           оставляется без изменений, в противном случае выставляется начальное значение deltaWait,
      //           равное превышению установленной температуры над текущей (-4,5 + deltaWait).
      //    3.2  Если текущая температура все ещё выше установленной, то измеряется deltaHeat, равное разнице между 
      //           максимально высокой текущей температурой и preview_temperature (preview_temperature определяется 
      //           как текущая температура при переключении режимов). 
      //    3.3  Добавляется по одному выключенному тэну и если текущий режим ожидания и с момента последнего  
      //         включения/выключения прошло не менее 10 секунд. Для выключения ищется первый включенный тэн 
      //         с конца списка
      // 
      //    Добавления (+0,5) в пункте 2.1 и (-4,5) в пункте 2.1 означают что при поддержке температура будет 
      //    варьироваться в интервале от heating_temperature-4,5 до heating_temperature+0,5 градусов, где
      //    heating_temperature - установленная температура нагревания.  
      if(!(modeHeat & 1)){ //mode heating
        //Serial.println("Mode heating in moment");
        //check temperature
        if(heating_temperature + 0.5 - deltaHeat < current_temperature){
          //Serial.println("The current temperature has exceeded.");
          serNum =46;
          Serial.println("serNum"+String(serNum));
          modeHeat=modeHeat | 1; //set mode wait
          if(!((modeHeat & 0b1000)>>3)){//if temperature reaching mode
            preview_temperature=current_temperature;
            modeHeat=modeHeat | 0b1000; //set keep warm mode
          }
          else{
            //Serial.print("DeltaWait in prozess is ");
            serNum =47;
            Serial.println("serNum"+String(serNum));
            Serial.println(deltaWait);
            preview_temperature=heating_temperature + 0.5 - deltaHeat;
            deltaHeat=current_temperature - preview_temperature;
          }
        }
        else{
          if(preview_temperature-current_temperature>deltaWait){
            deltaWait=preview_temperature-current_temperature;//find max change temperature
            if((modeHeat & 0b1000)>>3){ //if keep warm mode
              // allow mehr ten in programs define
              if(deltaWait>5){
                usedTlegal=(usedTlegal & 0b11110000) | 8;
              }
              else if(deltaWait>4.5){
                usedTlegal=(usedTlegal & 0b11110000) | 7;
              }
              else if(deltaWait>4){
                usedTlegal=(usedTlegal & 0b11110000) | 6;
              }
              else if(deltaWait>3.5){
                usedTlegal=(usedTlegal & 0b11110000) | 5;
              }
              else if(deltaWait>3){
                usedTlegal=(usedTlegal & 0b11110000) | 4;
              }
              else if(deltaWait>2.5){
                usedTlegal=(usedTlegal & 0b11110000) | 3;
              }
              else if(deltaWait>2){
                usedTlegal=(usedTlegal & 0b11110000) | 2;
              }
            }
          }
        }
        //get number active tens
        usedT=0;
        for(uint8_t i=0; i<8; i++){
          if(((1<<i) & tensArr)>>i){
            usedT=usedT+1;
          }
        }
        //ten of/on
        for(uint8_t i=0; i<8; i++){
          if(((1<<i) & tensArr)>>i==0){
            //turn on as many heaters as the user allows
            if(usedT < ((usedTlegal & 0b11110000)>>4)){
              //turn on as many heaters as the program allows
              if(usedT < (usedTlegal & 0b1111)){
                //if heating mode
                if(!(modeHeat & 1)){
                  //Serial.println("Turn on one ten.");
                  serNum =45;
                  Serial.println("serNum"+String(serNum));
                  //turn on one ten, interval 10 sec
                  tens_control(i | 0b00001000);
                }
              }
            }
            break;
          }
        }
      } 
      else{//mode waiting
        //Serial.println("Mode waiting in moment");
        //check temperature
        if(heating_temperature - 4.5 + deltaWait>current_temperature){
          //Serial.println("The current temperature has dropped.");
          serNum =49;
          Serial.println("serNum"+String(serNum));
          modeHeat=modeHeat & 0b11111110; //set mode heat
          usedTlegal=(usedTlegal & 0b11110000) | 1; // allow only one ten in programs define
          if(!((modeHeat & 0b1000)>>3)){//if temperature reaching mode
            preview_temperature=current_temperature;
            modeHeat=modeHeat | 0b1000; //set keep warm mode
          }
          else{
            //Serial.print("DeltaHeat in prozess is ");
            serNum =50;
            Serial.println("serNum"+String(serNum));
            Serial.println(deltaHeat);
            preview_temperature=heating_temperature - 4.5 + deltaWait;
            deltaWait=preview_temperature-current_temperature;
          }
        }
        else{
          if(current_temperature-preview_temperature>deltaHeat){
            deltaHeat=current_temperature-preview_temperature;//find max change temperature
          }
        }
        // ten of/on
        if(modeHeat & 1){ //if mode waiting
          for(uint8_t i=8; i>0; i--){
            i--;
            if(tensArr==0){
              //modeHeat=modeHeat & 0b11111101;//set flag all of
              break;
            }
            if(((1<<i) & tensArr)>>i){
              //Serial.println("Turn of one ten.");
              serNum =48;
              Serial.println("serNum"+String(serNum));
              //turn of one ten, interval 10 sec
              tens_control(i);
              break;
            }
            i++;
          }
        }
      }
    }
    else{
      delay(200); //?
    }
    count=count+1;
  }

  //change heating temperature, turn 
  //Serial.println("LCD print user settings.");
  serNum =11;
  Serial.println("serNum"+String(serNum));
  lcd.clear();
  //Serial.println("LCD print simbol / see how it");
  //serNum =12;
  //Serial.println(serNum);
  lcd.setCursor(0, 0);
  lcd.print("ON/OF    ИЗМЕНИТЬ");
  lcd.setCursor(0, 1);
  lcd.print("  ТЭН    ТЕМПЕРАТУРУ");
  lcd.setCursor(3, 2);
  lcd.print("<>");
  lcd.setCursor(7, 3);
  lcd.print("<  OK  >");
  scroll_pointer = 0;
  // confirmation that button OK release
  //Serial.print("Scrol pointers buttons action time is 200 ms.");
  //serNum =13;
  //Serial.println("serNum"+String(serNum));
  con_but(buttonOk);
  while(digitalRead(buttonOk)==HIGH){
    if(digitalRead(buttonLeft)==LOW){
      lcd.setCursor(14, 2);
      lcd.print("  ");//clear alt scroll pointer
      lcd.setCursor(3, 2);
      lcd.print("<>");
      scroll_pointer = 0;
      delay(200);
    }
    if(digitalRead(buttonRight)==LOW){
      lcd.setCursor(3, 2);
      lcd.print("  ");//clear alt scroll pointer
      lcd.setCursor(14, 2);
      lcd.print("<>");
      scroll_pointer = 1;
      delay(200);
    }
  }
  //Serial.print("Settings selected. Scroll pointers position is ");
  serNum =14;
  Serial.println("serNum"+String(serNum));
  Serial.println(String(scroll_pointer));
   
  //see what is selected: control tens or change temperature
  if(!scroll_pointer){
    //lcdError_test();
    //select ten and turn on/of
    //Serial.println("LCD print select tens settings");
    serNum =15;
    Serial.println("serNum"+String(serNum));
    //return nummer of selected ten (from 0 to 7)
    //return 8 if user selected automatic settings
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ТЕКУЩАЯ МОЩНОСТЬ ");
    tensArr=aktive_tens(&tensArr);
    scroll_pointer=0;
    for(uint8_t i =0; i<8; i++){
      if(((1<<i) & tensArr)>>i){
        scroll_pointer=scroll_pointer+4;
      }
    }
    //str = String(scroll_pointer);
    //buf = new unsigned char[10];
    //str.getBytes(buf, 4, 0);
    //str2 = (const char*)buf;
    //lcd.print(str2);
    lcd.print(String(scroll_pointer));
    //delete []buf;
    lcd.setCursor(2, 1);
    lcd.print("УСТАНОВИТЬ ");
    lcd.print("kWatt:");
    if(scroll_pointer==0){
      lcd.setCursor(10, 2);
      lcd.print("0> kWt");
    }
    else if (scroll_pointer==32)
    {
      lcd.setCursor(9, 2);
      lcd.print("<32 kWt");
    }
    else{
      if (scroll_pointer<10){
        lcd.setCursor(9, 2);
      }
      else{
        lcd.setCursor(8, 2);
      }
      lcd.print("<");
      lcd.print(String(scroll_pointer));
      lcd.print("> kWt");
    }
    lcd.setCursor(7, 3);
    lcd.print("< OK >");
    //Serial.println("LCD print simbol plus on active tens");
    //serNum =16;
    //Serial.println("serNum"+String(serNum);
    //serNum =18;
    //Serial.println("serNum"+String(serNum));
        
    //control button
    //Serial.println("Scrol pointers buttons action time is 100 ms.");
    //serNum =18;
    //Serial.println("serNum"+String(serNum));
    con_but(buttonOk);
    //user select tens (kWatt) from 0 to 32 use button right and left
    while (digitalRead(buttonOk)==HIGH){//while buttonOk not pressed
      if(digitalRead(buttonLeft)==LOW){
        if(scroll_pointer>0){
          scroll_pointer=scroll_pointer-4;
          //Serial.print("Scroll_pointer value:  ");
          //serNum =19;
          //Serial.println("serNum"+String(serNum));
          //str = String(scroll_pointer);
          //Serial.println(String(scroll_pointer));
          //buf = new unsigned char[10];
          //str.getBytes(buf, 4, 0);
          //str2 = (const char*)buf;
          //delete []buf;
          lcd.setCursor(8, 2);
          if(scroll_pointer==0){
            lcd.print("  0");
          }
          else{
            if (scroll_pointer<10){
              lcd.print(" <");
              lcd.print(String(scroll_pointer));
            }
            else{
              lcd.print("<");
              lcd.print(String(scroll_pointer));
              lcd.print(">");
            }
          }
        }
        delay(200);
      }
      if(digitalRead(buttonRight)==LOW){
        if(scroll_pointer<32){
          scroll_pointer=scroll_pointer+4;
          //Serial.print("Scroll_pointer value: ");
          //serNum =20;
          //Serial.println("serNum"+String(serNum));
          //str = String(scroll_pointer);
          //Serial.println(String(scroll_pointer));
          //buf = new unsigned char[10];
          //str.getBytes(buf, 4, 0);
          //str2 = (const char*)buf;
          //delete []buf;
          lcd.setCursor(8, 2);
          if(scroll_pointer==32){
            lcd.print(" <32");
          }
          else{
            if (scroll_pointer<10){
              lcd.print(" ");
            }
            lcd.print("<");
            lcd.print(String(scroll_pointer));
          }
        }
        delay(200);
      }
      if(digitalRead(buttonOk)==LOW){
        break;
      }
    }
    //Serial.println("Polling of three buttons completed");
    //Serial.println("Perhaps arduino not support divide operation.");
    //Serial.print("Selected ten is  ");
    serNum =21;
    Serial.println("serNum"+String(serNum));
    Serial.println(scroll_pointer/4);

    //user turn on/of tens
    //return parametrs from user, which next funktion handle 
    //user control on/of ten
    //if(scroll_pointer<8){
      lcd.clear();
      //Serial.println("LCD print confirmation on/of selected ten");
      serNum =22;
      Serial.println("serNum"+String(serNum));
      lcd.setCursor(0, 0);
      lcd.print("УСТАНОВИТЬ МОЩНОСТЬ ");
      lcd.setCursor(0, 1);
      lcd.print(String(scroll_pointer));
      lcd.print(" kWatt");
      lcd.print("?");
      lcd.setCursor(1, 3);
      lcd.print("OTMEHA");
      lcd.print("< OK >");
      
      con_but(buttonOk);
      while(1){
        if(digitalRead(buttonOk)==LOW){
          //Serial.println("LCD print intervals notification on/of selected ten");
          serNum =23;
          Serial.println("serNum"+String(serNum));
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("ИНТЕРВАЛ ВКЛЮЧЕНИЯ/ ВЫКЛЮЧЕНИЯ 10 CEK.");
          modeHeat = modeHeat & 0b11111011; //set 0 flag in automatic bit
          //Serial.println("Control of tens is not automatic in moment.");
          //serNum =24;
          //Serial.println("serNum"+String(serNum));
          //scroll_pointer=scroll_pointer/4;
          for(uint8_t i =0; i<8; i++){
            if(scroll_pointer>i){
              if(!(((1<<i) & tensArr)>>i)){
                tens_control(0b00111000 | i);
              }
            }
            else{
              if(((1<<i) & tensArr)>>i){
                tens_control(0b00110000 | i);
              }
            }
          }
          usedTlegal=(usedTlegal & 0b1111) | (scroll_pointer<<4);
          break;
        }
        if(digitalRead(buttonLeft)==LOW){
          //Serial.println("User selected chancel.");
          serNum =27;
          Serial.println("serNum"+String(serNum));
          break;
        }
      }
    //}
    //else{
      //modeHeat = modeHeat | 0b100; //set 1 flag in automatic bit
      //Serial.println("User selected automatic control.");
      //serNum =28;
      //Serial.println("serNum"+String(serNum));
    //}
  }
  else{
    //Serial.println("Select heating temperature.");
    serNum =29;
    Serial.println("serNum"+String(serNum));
    sel_h_tem();
    //lcdError_test();
  }   
}

//dont know how work this function, need testing
void lcdError_test(){
  //Serial.print("LCD write error: ");
  serNum =31;
  Serial.println("serNum"+String(serNum));
  Serial.println(lcd.getWriteError());
  if(lcd.getWriteError()!=0){
    lcd.clearWriteError();
    //Serial.println("LCD write error was clear");
    serNum =32;
    Serial.println("serNum"+String(serNum));
  }
}

void yield() {
  if(digitalRead(buttonOk)==LOW){
    modeHeat=modeHeat | 0b10000;//on flag buttonOk;
  }
}

//Fuction print select heating temperature
void sel_h_tem(){
  uint8_t scroll_pointer = (uint8_t)heating_temperature;
  lcd.clear();
  lcd.setCursor(7, 3);
  lcd.print("< OK >");
  lcd.setCursor(8, 2);
  if(scroll_pointer==20){
    lcd.print(" ");
    lcd.print(String(scroll_pointer));
    lcd.print(">");
  }
  else if(scroll_pointer==80){
    lcd.print("<");
    lcd.print(String(scroll_pointer));
    lcd.print(" ");
  }
  else{
    lcd.print("<");
    lcd.print(String(scroll_pointer));
    lcd.print(">");
  }
  lcd.setCursor(5, 1);
  lcd.print("УСТАНОВИТЬ:");
  lcd.setCursor(1, 0);
  lcd.print("ПОДДЕРЖИВАЕТСЯ ");
  lcd.print(String(scroll_pointer));
  lcd.print("C");
  //lcdError_test();
  con_but(buttonOk);
  while (digitalRead(buttonOk)==HIGH){//while buttonOk not pressed
    if(digitalRead(buttonLeft)==LOW){
      if(scroll_pointer>20){
        scroll_pointer=scroll_pointer-5;
        lcd.setCursor(8, 2);
        if(scroll_pointer==20){
          lcd.print(" ");
          lcd.print(String(scroll_pointer));
          lcd.print(">");
        }
        else{
          lcd.print("<");
          lcd.print(String(scroll_pointer));
          lcd.print(">");
        }
        delay(400);
      }
    }
    if(digitalRead(buttonRight)==LOW){
      if(scroll_pointer<80){
        scroll_pointer=scroll_pointer+5;
        lcd.setCursor(8, 2);
        if(scroll_pointer==80){
          lcd.print("<");
          lcd.print(String(scroll_pointer));
          lcd.print(" ");
        }
        else{
          lcd.print("<");
          lcd.print(String(scroll_pointer));
          lcd.print(">");
        }
        delay(400);
      }
    }
    if(digitalRead(buttonOk)==LOW){
      break;
    }
  }
  //LCD print confirmation selected heating temperature
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("УСТАНОВИТЬ");
  lcd.setCursor(0, 1);
  lcd.print("ТЕМПЕРАТУРУ ");
  lcd.print(String(scroll_pointer));
  lcd.print("?");
  lcd.setCursor(1, 3);
  lcd.print("OTMEHA");
  lcd.print("< OK >");
  con_but(buttonLeft);
  con_but(buttonOk);
  while(1){
    if(digitalRead(buttonOk)==LOW){
      heating_temperature=scroll_pointer;
      usedTlegal=(usedTlegal & 0b11110000) | 0b1000; // allow 8 ten in programs define
      modeHeat=modeHeat & 0b11110111; //set temperature reaching mode
      break;
    }
    if(digitalRead(buttonLeft)==LOW){
      break;
    }
  }
  //Serial.println("Heating temperature is setted ");
  serNum =38;
  Serial.println("serNum"+String(serNum));
  Serial.println(heating_temperature);
}

// confirmation that button release
void con_but(uint8_t button){
  //Serial.println("Confirmation that button release.");
  //serNum =39;
  //Serial.println("serNum"+String(serNum));
  while(1){
    //pressing no more than 1 time in 900 ms
    if((millis()-tm>900) && (digitalRead(button) == HIGH)){
      tm = millis();
      break;
    }
  }
}

//return binary byte of active tens, 1-ten on, 0-ten of
//for example: 0b00000001 - only first ten on, 0b00000010 - first ten of, second ten on
uint8_t aktive_tens(uint8_t *tensArr){
  for(uint8_t i=0; i<8; i++){
    //the pins in output. How do?
    if(digitalRead(i+2)==HIGH){ // from Ten1 (2) to Ten8 (9)
      *tensArr=(1<<i) | *tensArr;
    }
    else{
      *tensArr=(~(1<<i)) & (*tensArr); 
    }
  }
  //Serial.println("Attention! Bitwise operations with referens. Need testing.");
  //Serial.print("Actuel binary byte (from left to right) of active tens is  ");
  //serNum =40;
  //Serial.println("serNum"+String(serNum));
  //Serial.println(String(*tensArr, BIN));
  return *tensArr;
}

//tens control - on/off 
//receiv binary parametrs
//0-2 bit - tens nummer (from 0 bis 7)
//3 bit - on/of ten
//4 bit - 1-wait to interval, not wait but further do programm
//5-6 bit - 00-interval=10 sec. 01-interval=20 sec. 10-interval 9 sec, crash mode
//7 bit not use, reserve
void tens_control(uint8_t parametrs){//you can code the only one char
  uint16_t interval;
  if((parametrs & 0b100000)>>5){
    interval=20000;
  }
  else if((parametrs & 0b1000000)>>6){
    interval=9000;
  }  
  else{
    interval=10000;
  }
  //Serial.println("previewMillis is ");
  //serNum =44;
  //Serial.println("serNum"+String(serNum));
  //Serial.println(previewMillis);
  //Serial.println("pin is ");
  //serNum =52;
  //Serial.println("serNum"+String(serNum));
  //Serial.println(((parametrs & 0b111)+2), BIN);
  //Serial.println("value is ");
  //serNum =53;
  //Serial.println("serNum"+String(serNum));
  //Serial.println(((parametrs & 0b1000)>>3), BIN);
  if((millis()-previewMillis) > interval){
    //Serial.println("Do tens!");
    //serNum =41;
    //Serial.println("serNum"+String(serNum));
    digitalWrite(((parametrs & 0b111)+2), ((parametrs & 0b1000)>>3));
    previewMillis=millis();
  }
  else{
    if((parametrs & 0b10000)>>4){
      //Serial.println("Wait and after do tens.");
      //serNum =42;
      //Serial.println("serNum"+String(serNum));
      delay(interval-(millis()-previewMillis));
      digitalWrite(((parametrs & 0b111)+2), ((parametrs & 0b1000)>>3));
      previewMillis=millis();
    }
    //else{
      //Serial.println("Yet not time do tens.");
    //  serNum =43;
    //  Serial.println("serNum"+String(serNum));
    //}
  }
}

// Функция, возвращающая количество свободного ОЗУ (RAM) (it need for debugging)
//int memoryFree()
//{
//   int freeValue;
//   if((int)__brkval == 0)
//      freeValue = ((int)&freeValue) - ((int)&__bss_end);
//   else
//      freeValue = ((int)&freeValue) - ((int)__brkval);
//   return freeValue;
//}