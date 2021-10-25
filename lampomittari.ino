// ARDUINO: board= Lolin(Wemos) D1 mini pro

// tämä on lämpömittarisovellus nokian näytöllä
//
// mittaa lampotilan lampovastuksella ja lähettää sen serverille.
// Näytölle tulostetaan mitattu lampo, ulkolampotila ja järven lämpötila.
// Ulkolämpötila haetaan Ilmatieteen laitoksen avoimesta datasta. Järven lämpö omalta serveriltä.

int debug=0; // DEBUG PÄÄLLE JA POIS

// Anturin asetukset
#define THERMISTORNOMINAL 10000      // resistance at 25 degrees C
#define TEMPERATURENOMINAL 25        // temp. for nominal resistance (almost always 25 C)
#define NUMSAMPLES 10                // how many samples to take and average, more takes longer  but is more 'smooth'
#define BCOEFFICIENT 3977            // The beta coefficient of the thermistor (usually 3000-4000) // edellisessä versiossa 3977
#define SERIESRESISTOR 10000         // the value of the 'other' resistor
//#define KORJAUS 0    
int korjaus=30; // korjaus asteen kymmenyksinä

// wifi
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
ESP8266WiFiMulti WiFiMulti;

const char* ssid     = "xxx";
const char* password = "xxx";

String fmiUrl="http://opendata.fmi.fi/wfs?request=getFeature&storedquery_id=fmi::observations::weather::simple&fmisid=874863&parameters=temperature&timestep=30&"; // Temperature in Espoo
String server="http://192.168.1.66";
int intvl = 15; // Measurement interval in minutes

int lampo_lake, lampo_in, lampo_out; // global variables containing latest measurements

// Libraries for Nokia5110
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
 
// pins
const int8_t RST_PIN = D8; // D8,15
const int8_t CE_PIN = D4; // D4,2
const int8_t DC_PIN = D6; // D6,12
const int8_t BL_PIN = D0; // D0,16
const int8_t THERM_PIN = A0;

Adafruit_PCD8544 display = Adafruit_PCD8544(DC_PIN, CE_PIN, RST_PIN);


// fonts
static const unsigned char PROGMEM plus [] = {12,12,63,63,12,12};
static const unsigned char PROGMEM miinus [] = {12,12,12,12,12,12};
static const unsigned char PROGMEM fontti [][2][10] = {
{ {0x00, 0xF0, 0xFE, 0x0E, 0x06, 0x06, 0x06, 0xFE, 0xF8, 0x00}, {0x00, 0x07, 0x1F, 0x30, 0x30, 0x30, 0x38, 0x3F, 0x0F, 0x00}},
{ {0x00, 0x00, 0x18, 0x0C, 0x0C, 0xFE, 0xFE, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x30, 0x30, 0x30, 0x3F, 0x3F, 0x30, 0x00, 0x00}}, 
{ {0x00, 0x0C, 0x06, 0x06, 0x06, 0x86, 0xFE, 0x7C, 0x00, 0x00}, {0x00, 0x30, 0x3C, 0x3E, 0x37, 0x33, 0x31, 0x30, 0x30, 0x00}}, 
{ {0x00, 0x0C, 0x06, 0xC6, 0xC6, 0xEE, 0xFE, 0x3C, 0x18, 0x00}, {0x00, 0x18, 0x30, 0x30, 0x30, 0x31, 0x3B, 0x1F, 0x0E, 0x00}}, 
{ {0x00, 0x80, 0xF0, 0x78, 0x1C, 0x06, 0x06, 0xFE, 0xFE, 0x00}, {0x00, 0x07, 0x07, 0x06, 0x06, 0x06, 0x06, 0x3F, 0x3F, 0x00}}, 
{ {0x00, 0x7E, 0x7E, 0x66, 0x66, 0x66, 0xE6, 0xC6, 0x00, 0x00}, {0x00, 0x10, 0x30, 0x30, 0x30, 0x30, 0x30, 0x1F, 0x0F, 0x00}}, 
{ {0x00, 0xF0, 0xFC, 0x8C, 0xC6, 0xC6, 0xC6, 0xC6, 0x86, 0x00}, {0x00, 0x0F, 0x1F, 0x39, 0x30, 0x30, 0x30, 0x39, 0x1F, 0x00}}, 
{ {0x00, 0x06, 0x06, 0x06, 0x06, 0xC6, 0xF6, 0x7E, 0x1E, 0x00}, {0x00, 0x00, 0x30, 0x3C, 0x3F, 0x0F, 0x03, 0x00, 0x00, 0x00}}, 
{ {0x00, 0x38, 0xFC, 0xCE, 0x86, 0x86, 0xCE, 0xFC, 0x38, 0x00}, {0x00, 0x0F, 0x1F, 0x39, 0x30, 0x30, 0x39, 0x1F, 0x0F, 0x00}}, 
{ {0x00, 0x78, 0xFC, 0x86, 0x86, 0x86, 0x86, 0xFE, 0xFC, 0x00}, {0x00, 0x00, 0x30, 0x31, 0x31, 0x31, 0x31, 0x1F, 0x0F, 0x00}}, 
{ {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}
};
const unsigned char PROGMEM tausta [][84] = {
{0x00, 0x00, 0x20, 0xFC, 0xFC, 0x00, 0x00, 0x00, 0xFC, 0xFC, 0x3C, 0x78, 0xF0, 0xE0, 0xC0, 0x00, 0x00, 0xFC, 0xFC, 0x40, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 

{0x00, 0x00, 0x0C, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x04, 0x00, 0x00, 0x01, 0x03, 0x0F, 0x3C, 0x3F, 0x3F, 0x03, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 

{0x00, 0x00, 0xF8, 0xF8, 0x0C, 0x0C, 0x0C, 0x1C, 0x1C, 0x38, 0xF8, 0x00, 0x00, 0x60, 0xFC, 0xFC, 0x00, 0x00, 0x00, 0x00, 0xFC, 
0xFC, 0x00, 0x00, 0x0C, 0x0C, 0x0C, 0x0C, 0xFC, 0xFC, 0xFC, 0x0C, 0x0C, 0x0C, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 

{0x00, 0x00, 0x03, 0x0F, 0x1C, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x0F, 0x00, 0x00, 0x00, 0x0F, 0x1F, 0x1E, 0x38, 0x38, 0x1E, 0x1F, 
0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 

{0x00, 0xFE, 0xFE, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x80, 0xE0, 0xF8, 0xFC, 0x9E, 0x8E, 0xFE, 0xF8, 0xC0, 0x00, 0x00, 0x00, 0xFE, 
0xFE, 0xF0, 0xB8, 0x3C, 0x1E, 0x0E, 0x06, 0x00, 0x00, 0x00, 0xFE, 0xFE, 0xC6, 0xC6, 0xC6, 0xC6, 0xC4, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 

{0x00, 0x1F, 0x1F, 0x18, 0x18, 0x18, 0x00, 0x00, 0x1F, 0x1F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x1C, 0x00, 0x00, 0x1F, 
0x1F, 0x01, 0x03, 0x03, 0x07, 0x0E, 0x1E, 0x18, 0x00, 0x00, 0x1F, 0x1F, 0x18, 0x38, 0x38, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} 
};

void setup() {
  if (debug==1) Serial.begin(115200);
  if (debug==1) Serial.println("Start"); 

  pinMode(THERM_PIN,INPUT);

  // Display
  pinMode(BL_PIN, OUTPUT);
  digitalWrite(BL_PIN, HIGH);
  display.begin();
  display.setContrast(60);  // Adjust for your display
  display.clearDisplay();
  display.setTextColor(BLACK);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Connecting");
  display.print("SSID: ");
  display.println(ssid);
  display.display();

  // Wifi
  delay(4000);
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);
  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
    display.print(".");
    display.display();
  }
  display.clearDisplay();
  display.println("Connected");
  display.println(WiFi.localIP());
  if (debug==1) Serial.println("connection ok");
  display.display();
  delay(2000);
}

void loop() {
  int err,i;
  String payload;
  WiFiClient client;
  HTTPClient http;
  
  http.begin(client,fmiUrl);
  int httpCode = http.GET();
  if(httpCode > 0) { // httpCode will be negative on error
    if (httpCode == HTTP_CODE_OK) payload = http.getString();
    if (debug==1) Serial.println("FMI GET ok");
    err=0;
  } else {
    err=1;
    if (debug==1) Serial.println("FMI GET error");
    display.clearDisplay();
    display.setTextSize(1);
    display.setRotation(0);
    display.println("[Out] GET... failed");
    display.println(http.errorToString(httpCode).c_str());
    display.display();
    delay(3000);
  }
  delay(100);
  http.end();

  if (err==0) {
    lampo_out = parseFmiTemp(payload);
  } else {
    lampo_out = 999;
  }
  lampo_in = measureLocalTemp();
  lampo_lake = getLakeTemp();
  displayTemps();
  
  if (debug==1) {
    delay(20000);
  } else {
    for  (i=0;i<intvl;i++) delay(60000);
  }
  
}

int parseFmiTemp(String payload) {      
  int pituus,i,k,miinus,lampo_i,hyppaa;
  char lampo[50];
    
  pituus=payload.length();
  hyppaa=5;
  do {
    k=0; i=0;
    while (i<hyppaa) {
      if (payload.charAt(pituus)=='>') i++;
      pituus=pituus-1;
    }  
    pituus=pituus+2;
    while (payload.charAt(pituus+k)!='<') {
      lampo[k]=payload.charAt(pituus+k);
      k++;
    }
    lampo[k]='\0';
    hyppaa=17;
  } while (lampo[0]=='N');  // joskus viimeinen lämpotila on NaN    
  
  miinus=0;
  lampo_i=(lampo[k-1]-'0')+(10*(lampo[k-3]-'0'));
  if (lampo[0]=='-') {
    miinus=1;
    if (k==5) lampo_i=lampo_i+(100*(lampo[1]-'0'));
  } else if (k==4) lampo_i=lampo_i+(100*(lampo[0]-'0'));

  if (miinus) lampo_i=500-lampo_i; else lampo_i=500+lampo_i;

  if (debug==1) Serial.print("ulkona:");
  if (debug==1) Serial.println(lampo_i);
  return lampo_i;
}

int getLakeTemp() {
  int pituus,k,lampo_i,err;
  char lampo[50];
  String payload;
  
  WiFiClient client;
  HTTPClient http;
  err=0;
  String url = server+"/control/kysely.php?toiminto=jarvi";
  http.begin(client, url);
  int httpCode = http.GET();
  
  if(httpCode > 0) {
    if(httpCode == HTTP_CODE_OK) payload = http.getString();
  } else {
    err=1;
    display.clearDisplay();
    display.setTextSize(1);
    display.setRotation(0);
    display.println("[Lake] GET... failed");
    display.println(http.errorToString(httpCode).c_str());
    display.display();
    delay(3000);
  }
  http.end();
  
  if (err==0) {      
    pituus=payload.length();
    k=0;
    while (payload.charAt(14+k)!='<') {
      lampo[k]=payload.charAt(14+k);
      k++;
    }
    
    lampo_i=(lampo[k-1]-'0')+(10*(lampo[k-3]-'0'));
    if (k==5) lampo_i=lampo_i+(100*(lampo[1]-'0'));   
    if (lampo[0]=='-') lampo_i=500-lampo_i; else lampo_i=500+lampo_i;
    lampo_i;
  } else { 
    lampo_i=999;
  }
  if (debug==1) Serial.print("Jarvessa:");
  if (debug==1) Serial.println(lampo_i);
  return lampo_i;
}

int measureLocalTemp() {
  uint8_t i;
  float average;
  int lampo_i;
  int samples[NUMSAMPLES];
  String str = server+"/control/kasky.php?toiminto=KY&teksti=";
  char tempStr[6];
  
  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] = analogRead(THERM_PIN);
   delay(100);
  }
 
  // average all the samples out
  average = 0;
  for (i=0; i< NUMSAMPLES; i++) {
     average += samples[i];
  }
  average /= NUMSAMPLES;
 
  if (debug==1) {
    Serial.print("Average analog reading ");
    Serial.println(average);
  }
 
  // convert the value to resistance
  average = 1023 / average - 1;
  average = SERIESRESISTOR / average;
  float steinhart;
  steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C
  lampo_i=(int) (steinhart*10);

  lampo_i=lampo_i+korjaus;
  tempStr[0]='0'+lampo_i/100;
  tempStr[1]='0'+(lampo_i/10)-((lampo_i/100)*10);
  tempStr[2]='.';
  tempStr[3]='0'+lampo_i-((lampo_i/10)*10);
  tempStr[4]='\0';
  if (debug==0) {
    WiFiClient client;
    HTTPClient http;
    http.begin(client,str+tempStr);
    int httpCode = http.GET();
    http.end();
  }

  lampo_i=500+lampo_i;
  if (debug==1) Serial.print("sisalla:");
  if (debug==1) Serial.println(lampo_i);
  return lampo_i;
}

void displyBackground() {
  display.begin();
  display.fillScreen(0);
  display.setRotation(1);
  
  display.drawBitmap(40, 0, tausta[0], 8, 84, 1);
  display.drawBitmap(32, 0, tausta[1], 8, 84, 1);
  display.drawBitmap(24, 0, tausta[2], 8, 84, 1);
  display.drawBitmap(16, 0, tausta[3], 8, 84, 1);
  display.drawBitmap(8, 0, tausta[4], 8, 84, 1);
  display.drawBitmap(0, 0, tausta[5], 8, 84, 1);
  display.display();
}

void displayTemp(int numero, int rivi) {
  int luku1,luku2,luku3;

  if (numero>499) { 
    numero=numero-500;
    if (numero<499) display.drawBitmap(50-(rivi*16),44,plus,8,6,1); // jos luku on 999 niin ei tulosteta
  } else { 
    numero=-(numero-500);
    display.drawBitmap(50-(rivi*16),44,miinus,8,6,1);
  }
  if (numero<499)  {
    luku1=numero/100;
    luku2=(numero-(luku1*100))/10;
    luku3=numero-(luku1*100)-(luku2*10);
    if (debug==1) Serial.print("tulostetaan:");
    if (debug==1) Serial.print(luku1);
    if (debug==1) Serial.print(luku2);
    if (debug==1) Serial.println(luku3);
    if (luku1==0) luku1=10;
    display.drawBitmap(56-(rivi*16), 51, fontti[luku1][0], 8, 10, 1);
    display.drawBitmap(48-(rivi*16), 51, fontti[luku1][1], 8, 10, 1);
    display.drawBitmap(56-(rivi*16), 61, fontti[luku2][0], 8, 10, 1);
    display.drawBitmap(48-(rivi*16), 61, fontti[luku2][1], 8, 10, 1);
    display.drawBitmap(56-(rivi*16), 74, fontti[luku3][0], 8, 10, 1);
    display.drawBitmap(48-(rivi*16), 74, fontti[luku3][1], 8, 10, 1);
  } else  {
    display.drawBitmap(56-(rivi*16), 51, fontti[10][0], 8, 10, 1);
    display.drawBitmap(48-(rivi*16), 51, fontti[10][1], 8, 10, 1);
    display.drawBitmap(56-(rivi*16), 61, fontti[10][0], 8, 10, 1);
    display.drawBitmap(48-(rivi*16), 61, fontti[10][1], 8, 10, 1);
    display.drawBitmap(56-(rivi*16), 74, fontti[10][0], 8, 10, 1);
    display.drawBitmap(48-(rivi*16), 74, fontti[10][1], 8, 10, 1);
  }
  display.display();
}

void displayTemps() {
  displyBackground();
  displayTemp(lampo_in,1);
  displayTemp(lampo_out,2);
  displayTemp(lampo_lake,3);
}

void displayTempsText() { // Tekstinäyttö
  display.clearDisplay();
  display.print("Sisalla:");
  display.println(print_temp(lampo_in));
  display.print("Ulkona :");
  display.println(print_temp(lampo_out));
  display.print("Jarvi  :");
  display.println(print_temp(lampo_lake));
  display.display();
}

String print_temp(int numero) {
  char str[6] = "----";
  if (numero>999) return str;
  if (numero>499) { 
    numero=numero-500;
    str[0]='+';
  } else { 
    numero=-(numero-500);
    str[0]='-';
  }
  int luku1=numero/100;
  int luku2=(numero-(luku1*100))/10;
  int luku3=numero-(luku1*100)-(luku2*10);
  str[1]='0'+luku1;
  str[2]='0'+luku2;
  str[3]='.';
  str[4]='0'+luku3;
  if (debug==1) Serial.print("temp:");
  if (debug==1) Serial.println(str);
  return str;
}
