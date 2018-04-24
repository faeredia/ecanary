#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <math.h>
#include "src/myPCD8544/PCD8544.h"
#include <Adafruit_BMP085.h>
#include <Time.h>
#include <TimeAlarms.h>
#include "src/myRTC/myRTClib.h"
#include "src/MemoryFree-master/MemoryFree.h"

//constants for the DHT - humidity and temperature sensor
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//constants for the BMP - barmoetric pressuree, temperature, altometer
Adafruit_BMP085 bmp;
//BMP_SCL to A5
//BMP_SDA to A4
//Vin to 3v3

//constants for the DS1302 - Realtime Clock
//apparently its not a good one though....
DS1302 rtc(5, 6, 4);

//Constants for the PhotoResistor
#define PHOTOPIN A0

//Constants for the Nokia Screen
static PCD8544 lcd(13,12,11,9,8);
//Constants for the '%' icon. Uncomment one of the below styles.
static const byte PERCENT_CHAR = 1;
//static byte percent_glyph[] = { 0x98, 0x58, 0x20, 0xd0, 0xc8 };
//static byte percent_glyph[] = { 0x46, 0x26, 0x10, 0xc8, 0xc4 };
static byte percent_glyph[] = { 0x23, 0x13, 0x08, 0x64, 0x62 };
#define SCREEN_LED 10
//globals for screen lines. TODO, these can be 15 wide.
//TODO:
  //Do we need to stoor this globally?
  //Can we describe each line in a smaller way? Hashing, or maybe update flags (bool)
char lcdPage [6][16];
//The currently displayed page
uint8_t page = 0;

//global variables
//TODO:
  //Do these have to be global? can we make them local and just write them out to sd?
float hum;
float temp_db;
float temp_wb;
uint8_t light;
char str_time[16];
float baro_p;
float baro_t;
float baro_alt;


#define dht_interval 3
#define bmp_interval 2
#define rtc_interval 1
#define light_interval 1

void strcpy_15(char *buf, char *str){
//buf should be 15 (or greater) characters wide, thats 14 useful and one null byte.
//this should be a page line.
//if strlen(str) > 14, output to buf is trimmed
//if strlen(str) < 14, pad with whitespace (or the lcd wont clear these characters)
  uint8_t a = min(strlen(str), 14);
  for(uint8_t i=0; i<a; i++){
    buf[i] = str[i];
  }
  for(uint8_t i=a; i<14;i++){
    buf[i] = " ";
  }
  buf[14] = '\0';
}

void get_dht(){
  hum = dht.readHumidity();
  temp_db = dht.readTemperature();
  temp_wb = calc_temp_wb(temp_db, hum);
  Serial.println("updated dht");
}

void get_bmp(){
  baro_t = bmp.readTemperature();
  baro_p = bmp.readPressure() / 1000.0;
  baro_alt = bmp.readAltitude();
}

void get_light(){
  light = analogRead(PHOTOPIN);
}

float calc_temp_wb(float db, float h){
  //Function for calculating Wet Bulb temperature from Dry Bulb and Relative Humidity
  //move the constants into #define to reduce local mem
  const float A = 0.151977;
  const float B = 8.313659;
  const float C = 1.676331;
  const float D = 0.00391838;
  const float E = 0.023101;
  const float F = 4.686035;

  return db*atanf(A*pow(h + B, 1.0/2)) + atanf(db + h) - atanf(h - C) + D*pow(h, 3.0/2)*atanf(E*h) - F;
}

void ito2digitstr(char *to, int i){
  //takes an empty string and an int of 0 or 2 digits. fills the string with a 2 digit string
  if(i >=0 && i < 10){
    sprintf(to, "0%d", i);
  } else {
    sprintf(to, "%d", i);
  }
}

void get_str_datetime(){
// returns the datetime string for L0
//TODO:
  //Look at moving this into the myRTC library
  //convert in there straight into the correct format. save me and cpu
  time_t t = rtc.getDS1302Time();
  char str_dt[16];
  char str_y[3];
  char str_m[3];
  char str_d[3];
  char str_hr[3];
  char str_min[3];

  ito2digitstr(str_y, year(t)%100);
  ito2digitstr(str_m, month(t));
  ito2digitstr(str_d, day(t));
  ito2digitstr(str_hr, hour(t));
  ito2digitstr(str_min, minute(t));

  str_dt[0] = '\0';
  strcat(str_dt, str_d);
  strcat(str_dt, "/");
  strcat(str_dt, str_m);
  strcat(str_dt, "/");
  strcat(str_dt, str_y);
  strcat(str_dt, " ");
  strcat(str_dt, str_hr);
  strcat(str_dt, ":");
  strcat(str_dt, str_min);
  
  strcpy(str_time, str_dt);
  Serial.println(str_time);
}

void updatePage(){
  //TODO
    //Maybe one function per page for brevity
    //will also allow special code paths to be implented
    //move the screen printing into its own function. 
    //point (dont copy) to the char*
//string representation of the floats
  char str_temp_db[16];
  char str_temp_wb[6];
  char str_hum[6];
  char str_baro_p[6];

  dtostrf(temp_db, 4, 1, str_temp_db);
  dtostrf(temp_wb, 4, 1, str_temp_wb);
  dtostrf(hum, 4, 1, str_hum);
  dtostrf(baro_p, 4, 1, str_baro_p);
  
  //compiler is throwing a nasty error when trying to sprintf (with formatting) 
  //directly into the book. (something about running out of registers....)
  //work around is sprintf into a buffer, then strcpy into the book.
  char buf[6][16];
  if(page == 0){
    sprintf(buf[0], "%s", str_time);
    sprintf(buf[1], "T Db: %s C", str_temp_db);
    sprintf(buf[2], "T Wb: %s C", str_temp_wb);
    sprintf(buf[3], "RHum: %s \001", str_hum); //\001 is the shortcut defined earlier for %
    sprintf(buf[4], "Baro:%skPa", str_baro_p);
    sprintf(buf[5], " <  PSYCH   > ");
  } else if(page == 1){
    sprintf(buf[0], "%s", str_time);
    sprintf(buf[1], "Vel : %s m/s", "00.0");
    sprintf(buf[2], "Wide:  %s m", "0.0");
    sprintf(buf[3], "High:  %s m", "0.0");
    sprintf(buf[4], "Flow: %sm3/s", "00.0");
    sprintf(buf[5], " <   FLOW   > ");
  } else if(page == 2){
    sprintf(buf[0], "%s", str_time);
    sprintf(buf[1], "O2  : %s \001", "00.0");
    sprintf(buf[2], "CO  : %s ppm", "00.0");
    sprintf(buf[3], "CO2 : %s \001", "00.0");
    sprintf(buf[4], "NO2 : %s ppm", "00.0");
    sprintf(buf[5], " <   GAS    > ");
  } else if(page == 3){
    sprintf(buf[0], "%s", str_time);
    sprintf(buf[1], "Light: %d", light);
    sprintf(buf[2], "%s", "");
    sprintf(buf[3], "%s", "");
    sprintf(buf[4], "%s", "");
    sprintf(buf[5], " <  ENVIRO  > ");
  } else if(page == 4){
    sprintf(buf[0], "%s", str_time);
    sprintf(buf[1], "Pitch: %s", "5.6");
    sprintf(buf[2], "Roll : %s", "-3.1");
    sprintf(buf[3], "Yaw  : %s", "1.2");
    sprintf(buf[4], "");
    sprintf(buf[5], " <   GYRO   > ");
  }
  
  for(int L=0;L<6;L++){
    if(strcmp(lcdPage[L], buf[L]) != 0){
      //strings are unequal
      strcpy(lcdPage[L], buf[L]);
      lcd.setCursor(0,L);
      //annoyingly, pcd8544 ignores the whitespaces we created... clear the line.
      lcd.clearLine();
      lcd.print(lcdPage[L]);
    }
  }

}

void autoFlipPages(){
  page++;
  if(page >= 5){
    page = 0;
  }
}

void setup() {
  //Start the serial console
  Serial.begin(9600);

  pinMode(SCREEN_LED, OUTPUT);
  analogWrite(SCREEN_LED, 20);
  lcd.begin(84, 48);
  //setup the custom icons
  lcd.createChar(PERCENT_CHAR, percent_glyph);
  lcd.print("Hello there!");
  
  //Set the time
  get_str_datetime();
  Alarm.timerRepeat(rtc_interval, get_str_datetime);
  
  //start the temp/humidity sensor
  dht.begin();
  //get an intial value set:
  get_dht();
  //Set a timer for the DHT, take a reading every 'interval' seconds
  Alarm.timerRepeat(dht_interval, get_dht);

  //start the BMP barometric pressure sensor
  bmp.begin();
  get_bmp();
  Alarm.timerRepeat(bmp_interval, get_bmp);

  Alarm.timerRepeat(light_interval, get_light);

  //demo mode, flip the pages every now and then
  Alarm.timerRepeat(10, autoFlipPages);
}

void loop() {
  //Wait a bit for DHT22 to sort itself out
  //Read from all the sensors
  //light = get_light();
  //get_str_datetime();


  updatePage();


  Alarm.delay(500);
}

