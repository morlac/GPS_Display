#include <avr/pgmspace.h>
// Workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34734
#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

#include <Wire.h>
#include <Adafruit_GFX.h>

#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display2 = Adafruit_SSD1306(4);

#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 3);
Adafruit_GPS GPS(&mySerial);

// for voltage-measurement
static const int voltage_pin = A7;
static const float voltage_r1 = 103800.0;
static const float voltage_r2 = 9930.0;
static const float resistorFactor = 1023.0 * (voltage_r2/ (voltage_r1 + voltage_r2));
float vbat;

char gps_date[13];
char gps_time[9];
char gps_deg[10];
//char gps_lat[14];
//char gps_lon[14];
char printbuffer[23];

boolean usingInterrupt = false;
//void useInterrupt(boolean); // Func prototype keeps Arduino 0023 happy

// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}

char *ftoa(char *a, double f, int precision) {
  long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};
  
  char *ret = a;
  long heiltal = (long)f;
  itoa(heiltal, a, 10);
  while (*a != '\0') a++;
  *a++ = '.';
  long desimal = abs((long)((f - heiltal) * p[precision]));
  itoa(desimal, a, 10);
  return ret;
}

void setup() {
//  Serial.begin(9600);
  
  // for voltage measurement via a voltage-divider
  analogReference(INTERNAL);
  pinMode(voltage_pin, INPUT);

  display2.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display2.setTextWrap(false);
//  display2.setTextSize(1);
  display2.setTextColor(WHITE);

  GPS.begin(9600);

  GPS.sendCommand(PMTK_SET_BAUD_4800);
//  GPS.sendCommand(PMTK_SET_BAUD_9600);
//  GPS.sendCommand(PMTK_SET_BAUD_38400);
//  GPS.sendCommand(PMTK_SET_BAUD_57600);
  mySerial.end();
  
  GPS.begin(4800);
  
//  GPS.sendCommand(PMTK_SET_BAUD_9600);
//  GPS.sendCommand(PMTK_SET_BAUD_38400);
//  GPS.sendCommand(PMTK_SET_BAUD_57600);

//  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
//  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_ALLDATA);
  
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
//  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_5HZ);

//  GPS.sendCommand(PGCMD_ANTENNA);
   GPS.sendCommand(PGCMD_NOANTENNA);

  useInterrupt(true);
}

uint32_t timer = millis();

void loop() {
  display2.clearDisplay();
  display2.setTextSize(1);

  if (! usingInterrupt) {
    // read data from the GPS in the 'main loop'
    char c = GPS.read();
    // if you want to debug, this is a good time to do it!
  }

  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences! 
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    //Serial.println(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false
  
    if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
      return;  // we can fail to parse a sentence in which case we should just wait for another
  }

  if (timer > millis())  timer = millis();
  
  if (millis() - timer > 1000) {
    timer = millis();

    sprintf(gps_date, "20%02d-%02d-%02d", GPS.year, GPS.month, GPS.day);

    display2.setCursor(0, 0);
    display2.setTextSize(1);
    display2.print(gps_date);  

    sprintf(gps_time, "%02d:%02d:%02d", GPS.hour, GPS.minute, GPS.seconds);

    display2.setCursor(80, 0);
    display2.print(gps_time);

    vbat = (analogRead(voltage_pin) / resistorFactor) * 1.1;
    display2.setCursor(98, 24);
//    sprintf(printbuffer, "% 4sV", ftoa(gps_deg, vbat, 2));
//    display2.print(printbuffer);
    display2.print(vbat); display2.print(F("V"));

    if (GPS.fix) {
      display2.setCursor(66, 0);
      display2.print(GPS.satellites, DEC);

      display2.setCursor(0, 8);
      sprintf(printbuffer, "Lt:% 9s%c", ftoa(gps_deg, GPS.latitude, 4), GPS.lat);
      display2.print(printbuffer);
      
      display2.setCursor(0, 16);
      sprintf(printbuffer, "Ln:% 9s%c", ftoa(gps_deg, GPS.longitude, 4), GPS.lon);
      display2.print(printbuffer);

      display2.setCursor(86, 16);
//      sprintf(printbuffer, "DOP:%s", ftoa(gps_deg, GPS.HDOP, 1));
//      display2.print(printbuffer);
      display2.print(F("DOP:")); display2.print(GPS.HDOP, 1);
      
      display2.setCursor(0, 24);
      display2.print(F("Alt:")); display2.print(GPS.altitude); display2.print(F("m"));
    } else {
      display2.setTextSize(2);
      display2.setCursor(0, 16);
      display2.print(F("No fix"));
    }

    display2.display();
  }
  
}
