#define BLYNK_TEMPLATE_ID "TMPL2tAh-lTC3"
#define BLYNK_TEMPLATE_NAME "IoT based Smart Boat Tracking System"
#define BLYNK_AUTH_TOKEN "P4L4DcRiPWZ8gHMd75ENYIq5-Copk1s_"
#define BLYNK_PRINT Serial


#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// Blynk credentials
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Setsom Technology";
char pass[] = "614444259";

BlynkTimer timer;
LiquidCrystal_I2C lcd(0x27,16,2);

static const int RXPin=D5;
static const int TXPin=D6;
SoftwareSerial gpsSerial(RXPin,TXPin);
TinyGPSPlus gps;

#define BUTTON_PIN D3
#define RED_LED_PIN D4
#define GREEN_LED_PIN D7
#define BUZZER_PIN D8

#define VPIN_LAT V0
#define VPIN_LON V1
#define VPIN_DATE V2
#define VPIN_STATUS V3
#define VPIN_CLEAR V4
#define VPIN_SPEED V5
#define VPIN_MAPLINK V6
#define VPIN_HEADING V7

bool emergencySent=false;
bool lastButtonState=HIGH;

void setNormalState(){
  digitalWrite(RED_LED_PIN,LOW);
  digitalWrite(GREEN_LED_PIN,HIGH);
  digitalWrite(BUZZER_PIN,LOW);
}

void setEmergencyState(){
  digitalWrite(GREEN_LED_PIN,LOW);
  digitalWrite(RED_LED_PIN,HIGH);
}

void getGPSData(){
  while(gpsSerial.available()) gps.encode(gpsSerial.read());
}

void sendTrackingData(){
  if(!gps.location.isValid()){
    Blynk.virtualWrite(VPIN_STATUS,"NO GPS");
    return;
  }

  String lat=String(gps.location.lat(),6);
  String lon=String(gps.location.lng(),6);

  Blynk.virtualWrite(VPIN_LAT,lat);
  Blynk.virtualWrite(VPIN_LON,lon);

  if(gps.date.isValid()){
    String d=String(gps.date.month())+"/"+String(gps.date.day())+"/"+String(gps.date.year());
    Blynk.virtualWrite(VPIN_DATE,d);
  }

  Blynk.virtualWrite(VPIN_SPEED,gps.speed.kmph());
  Blynk.virtualWrite(VPIN_HEADING,gps.course.deg());

  String map="https://maps.google.com/?q="+lat+","+lon;
  Blynk.virtualWrite(VPIN_MAPLINK,map);

  if(!emergencySent)
    Blynk.virtualWrite(VPIN_STATUS,"TRACKING");

  Serial.println("------");
  Serial.println("LAT: "+lat);
  Serial.println("LON: "+lon);
  Serial.print("Speed: "); Serial.println(gps.speed.kmph());
  Serial.print("Heading: "); Serial.println(gps.course.deg());
  Serial.println(map);
}

void beepBuzzer(){
  static unsigned long last=0;
  static bool on=false;
  if(millis()-last >= (on?200:300)){
    on=!on;
    digitalWrite(BUZZER_PIN,on);
    last=millis();
  }
}

void triggerEmergencyAlert(){
  emergencySent=true;
  setEmergencyState();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("BOAT ALERT!");

  sendTrackingData();
  Blynk.virtualWrite(VPIN_STATUS,"BOAT ALERT");

  if(gps.location.isValid()){
    lcd.setCursor(0,1);
    lcd.print(String(gps.location.lat(),6).substring(0,16));
  }else{
    lcd.setCursor(0,1);
    lcd.print("Tracking Lost");
  }
}

void checkButtonPress(){
  bool state=digitalRead(BUTTON_PIN);
  if(lastButtonState==HIGH && state==LOW && !emergencySent){
    triggerEmergencyAlert();
  }
  lastButtonState=state;
}

BLYNK_WRITE(VPIN_CLEAR){
  if(param.asInt()){
    emergencySent=false;
    setNormalState();

    Blynk.virtualWrite(VPIN_LAT,"--");
    Blynk.virtualWrite(VPIN_LON,"--");
    Blynk.virtualWrite(VPIN_DATE,"--");
    Blynk.virtualWrite(VPIN_STATUS,"CLEARED");
    Blynk.virtualWrite(VPIN_SPEED,"--");
    Blynk.virtualWrite(VPIN_HEADING,"--");
    Blynk.virtualWrite(VPIN_MAPLINK,"--");

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Boat Data");
    lcd.setCursor(0,1);
    lcd.print("Cleared");
  }
}

void setup(){
  Serial.begin(115200);
  gpsSerial.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Smart Boat");
  lcd.setCursor(0,1);
  lcd.print("Tracking Ready");

  pinMode(BUTTON_PIN,INPUT_PULLUP);
  pinMode(RED_LED_PIN,OUTPUT);
  pinMode(GREEN_LED_PIN,OUTPUT);
  pinMode(BUZZER_PIN,OUTPUT);

  setNormalState();

  Blynk.begin(auth,ssid,pass);

  timer.setInterval(5000L,sendTrackingData);
}

void loop(){
  Blynk.run();
  timer.run();
  getGPSData();
  checkButtonPress();
  if(emergencySent) beepBuzzer();
}
