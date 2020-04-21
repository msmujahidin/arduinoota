#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <NewPing.h>

#ifndef STASSID
#define STASSID "KODETANI"
#define STAPSK  "12345678"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

#define TRIGGER1 16
#define TRIGGER2 15
#define ECHO1 14
#define ECHO2 12
#define TOMBOL 0
#define RED 4
#define GREEN 5
#define RELAY 13
#define LAMA_TUNGGU 1     // lama nunggu (Detik) setelah terdeteksi orang masuk
#define LAMA_NYEMPROT 5   //dalam detik
#define JARAK_MINIMUM 20  //dalam centimeter
NewPing ping1(TRIGGER1, ECHO1, 400);
NewPing ping2(TRIGGER2, ECHO2, 400);
Ticker wait_tick;
Ticker spray_tick;

ulong echo1_cm = 0;
ulong echo2_cm = 0;
volatile byte buzzer_count = 3;
volatile bool TOMBOL_DITEKAN=false;
volatile bool AKTIF = false;
ulong waktu_tunggu = millis();
ulong semprot_ts = millis();
ulong serial_ts = millis();
ulong tombol_bounce = millis();
bool SEDANG_MENUNGGU = false;
bool SEDANG_NYEMPROT = false;

//#define TESTING

void ICACHE_RAM_ATTR TOMBOL_ISR();

void TOMBOL_ISR(){
  if(digitalRead(TOMBOL)==0){
    tombol_bounce = millis();
  }else{
    if(millis()-tombol_bounce >= 5){
      TOMBOL_DITEKAN = true;
    }
  }
}

void Log(const char* pesan){
  Serial.println(pesan);
}
void stop_pump(){
  AKTIF = false;
  Log("POMPA OFF");
  digitalWrite(RELAY,0);    //matikan pompa
  digitalWrite(RED, 0);     //matikan lampu merah
  digitalWrite(GREEN, 1);   //nyalakan lampu hijau
}
void start_pump(){
  if(!AKTIF){
    AKTIF = true;    
    Log("POMPA ON");  
    digitalWrite(RELAY,1);    //nyalakan pompa
    digitalWrite(RED, 0);     //nyalakan lampu merah
    digitalWrite(GREEN, 1);   //matikan lampu hijau
    spray_tick.once(LAMA_NYEMPROT, stop_pump);
  }else{
    stop_pump();
  }
}

void start_wait(){
    start_pump();  
}

void user_setup(){
  #ifndef TESTING
  pinMode(RELAY, OUTPUT);
  pinMode(TRIGGER1, OUTPUT);
  pinMode(TRIGGER2, OUTPUT);
  pinMode(TOMBOL, INPUT_PULLUP);
  pinMode(ECHO1, INPUT_PULLUP);
  pinMode(ECHO2, INPUT_PULLUP);
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(TOMBOL), TOMBOL_ISR, CHANGE);
  digitalWrite(GREEN,0);    //nyalakan lampu hijau
  digitalWrite(RED,1);      //matikan lampu merah
  #else
  pinMode(RELAY, OUTPUT);
  pinMode(TRIGGER1, OUTPUT);
  pinMode(TRIGGER2, OUTPUT);
  pinMode(TOMBOL, INPUT_PULLUP);
  pinMode(ECHO1, INPUT_PULLUP);
  pinMode(ECHO2, INPUT_PULLUP);
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(TOMBOL), TOMBOL_ISR, CHANGE);
  #endif 
}

void user_loop(){
  #ifndef TESTING
  echo1_cm = ping1.ping_cm();
  echo2_cm = ping2.ping_cm();
  echo1_cm = echo1_cm == 0? 400: echo1_cm;
  echo2_cm = echo2_cm == 0? 400: echo2_cm;
  Serial.printf("ping1: %d, ping2: %d\r\n", echo1_cm, echo2_cm);
  delay(50);
  if( (echo1_cm > 0 && echo1_cm <= JARAK_MINIMUM) || (echo2_cm > 0 && echo2_cm <= JARAK_MINIMUM) ){ 
    digitalWrite(RED, 0);
    digitalWrite(GREEN, 1);
    wait_tick.once(LAMA_TUNGGU, start_wait);  
    
  }

  if(TOMBOL_DITEKAN){    
    //override semprotan selama 7 detik    
    Log("Tombol di tekan");
    TOMBOL_DITEKAN = false;
    if(AKTIF){
      AKTIF = false;                 
      waktu_tunggu = millis();
    }else{
      AKTIF = true;
      semprot_ts = millis();  
      waktu_tunggu = millis();    
    }
  }

  #else
  echo1_cm = ping1.ping_cm();
  echo2_cm = ping2.ping_cm();
  Serial.printf("echo1: %dcm \techo2: %dcm\r\n", echo1_cm, echo2_cm);
  delay(50);
  if(echo1_cm > 0 && echo1_cm <= JARAK_MINIMUM){
    digitalWrite(RED,1);
  }else{
    digitalWrite(RED,0);
  }
  if(echo2_cm > 0 && echo2_cm <= JARAK_MINIMUM){
    digitalWrite(GREEN,1);
  }else{
    digitalWrite(GREEN,0);
  }
  if(TOMBOL_DITEKAN){
    TOMBOL_DITEKAN=false;
    digitalWrite(RELAY, !digitalRead(RELAY));
    digitalWrite(RED,1);
    delay(250);
    digitalWrite(GREEN,1);
    digitalWrite(RED,0);
    delay(250);
    digitalWrite(GREEN,0);
  }
  #endif
}


void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.softAP(ssid, password);
  user_setup();
}

void loop() {
  user_loop();  
}
