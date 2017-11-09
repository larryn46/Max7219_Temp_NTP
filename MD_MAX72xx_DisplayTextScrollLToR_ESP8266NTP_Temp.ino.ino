// -------------------------------------------------------Program to exercise the MD_MAX72XX library
//
// Uses most of the functions in the library
#include <MD_MAX72xx.h>
#include <SPI.h>
// from AnimateText_5_NTP_DST
#include <TimeLib.h> 
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#define XOUT
// Turn on debug statements to the serial output
#define  DEBUG  1

#if  DEBUG
#define	PRINT(s, x)	{ Serial.print(F(s)); Serial.print(x); }
#define	PRINTS(x)	Serial.print(F(x))
#define	PRINTD(x)	Serial.println(x, DEC)

#else
#define	PRINT(s, x)
#define PRINTS(x)
#define PRINTD(x)

#endif

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
/* original
#define	MAX_DEVICES	8
#define	CLK_PIN		13  // or SCK
#define	DATA_PIN	11  // or MOSI
#define	CS_PIN		10  // or SS
*/

#define MAX_DEVICES 4
#define CLK_PIN   D5 // or SCK
#define DATA_PIN  D7 // or MOSI
#define CS_PIN    D2 // or SS originally D8

// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(CS_PIN, MAX_DEVICES);
// Arbitrary pins
// MD_MAX72XX mx = MD_MAX72XX(DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// We always wait a bit between updates of the display
#define  DELAYTIME  100  // in milliseconds

// ----------------------------------------------end exercise the MD_MAX72XX library defs variables

// ---------------------------------------------- ntp time defs variables

#define INTRINSIC_DELAY 6  // INTRINSIC_DELAY used in time_t getNtpTime() because of delay between
                           // NTP time reading and time digits appear on display
#define ONE_HOUR_MS SECS_PER_HOUR*1000 // Milliseconds in 1 hour used in Get_temperature()
#define LEAP_YEAR(Y) ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )
struct strDateTime // declared in NTPtimeESP.h, which is included by including NTPtimeESP.cpp above
{
  byte hour;
  byte minute;
  byte second;
  int year;
  byte month;
  byte day;
  byte dayofWeek;
  boolean valid;
};
strDateTime ConvertUnixTimestamp( unsigned long );
boolean daylightSavingTime(unsigned long );                     
const char ssid[] = "dickoryParima";  //  your network SSID (name)
const char pass[] = "GeorgeWashington";       // your network password
//const int timeZone = 1;     // Central European Time
//const int timeZone = -5;  // Eastern Standard Time (USA)
//const int timeZone = -4;  // Eastern Daylight Time (USA)
const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)
unsigned long last_temp_time = 0;
// NTP Servers:
IPAddress timeServer(192, 168, 1, 216); // 
//  IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov
int x = 0;
unsigned long previousMillis = 0; 
long interval = 60;           // interval at which to display (milliseconds) 60,000 = 60 sec
WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
//String   time_string = "before";
char time_string[18] = "before"; //8 for hh:mm:ss plus 1 for space + 8 for mm/dd/yy + null 
time_t prevDisplay = 0; // when the digital clock was displayed
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

// ---------------------------------------------- end ntp time defs variables

// --------------------------------------------------------- temperature defs variables
time_t last_reading_time = now() - ONE_HOUR_MS; // read temp every hour
int temperature;
/* "tgftp.nws.noaa.gov/data/observations/metar/decoded/KLGB.TXT"
 *  LONG BEACH AIRPORT, CA, United States (KLGB) 33-49N 118-09W 10M
Aug 23, 2017 - 09:53 PM EDT / 2017.08.24 0153 UTC
Wind: from the NW (310 degrees) at 10 MPH (9 KT):0
Visibility: 10 mile(s):0
Sky conditions: clear
Temperature: 71.1 F (21.7 C)
Dew Point: 62.1 F (16.7 C)
Relative Humidity: 73%
Pressure (altimeter): 29.88 in. Hg (1011 hPa)
ob: KLGB 240153Z 31009KT 10SM CLR 22/17 A2988 RMK AO2 SLP119 T02170167
cycle: 2

http://tgftp.nws.noaa.gov/data/observations/metar/stations/KSFO.TXT
2017/08/24 02:56
KSFO 240256Z 28013KT 10SM FEW002 SCT008 16/13 A2991 RMK AO2 SLP129 T01560128 53002
*/
//const char* streamId   = "/data/observations/metar/stations/";
//const char* streamId   = "/data/observations/metar/decoded/KLGB.TXT";
//const char* streamId   = "/data/observations/metar/decoded/CWHY.TXT";
/*
 * These constants are now included in Get_temperature()
const char* host = "tgftp.nws.noaa.gov";
const char* streamId   = "/data/observations/metar/stations/";
const char* privateKey = "";
*/
const char* stationID = "KLGB";
char* sought = "KLGB";

// --------------------------------------------------------- end temperature defs variables
 

// --------------------------------------------------------- int Get_temperature() 

int Get_temperature()
{
  const char* host = "tgftp.nws.noaa.gov";
  const char* streamId   = "/data/observations/metar/stations/";
  const char* privateKey = "";
  Serial.print("Last  temperature reading time ");
  Serial.println(last_reading_time);  //unix time
  Serial.print("current time ");
  Serial.println(now());
  Serial.print(" time since last temperature reading ");
  Serial.println(now()- last_reading_time);
  time_t reading_interval = SECS_PER_HOUR; // 3600 sec
  // don't go to site if less than 1 hour
  if (now()-last_reading_time < reading_interval)
    {
      //temperature = -100;
      return temperature;
    } // if (now()-last_reading_time < reading_interval)
  last_reading_time= now(); 
  Serial.print("connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client; // from ESP8266WiFi.h
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return -100;
  }
   // We now create a URI for the request
  //String url = "/input/"; // changed 20170823 1950
  String url = "";
  url += streamId; // const char* streamId   = "/data/observations/metar/stations/";
  url += stationID; // const char* stationID = "KLGB";
  url += ".TXT";
  
  Serial.println("connection to client.connect(ssid, port) successful ");
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  Serial.print("URL ");
  Serial.println(url);
  delay(500);
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return -100;
    } // end if (millis() - timeout > 5000) 
  } // end  while (client.available() == 0)
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
    //sought = stationID; // KLGB
    int found = line.indexOf(sought);
    if(found >= 0) 
       {
          Serial.print("found at ");
          Serial.println(found);
          int slashFound = line.indexOf('/',found);
          Serial.print("slash found at ");
          Serial.println(slashFound);
    String stringTemp = line.substring(slashFound-3,slashFound);
          Serial.println(stringTemp);
          temperature = stringTemp.toInt();
    Serial.print("From Get_temperature Temperature=");
    Serial.println(temperature);
    return temperature; 
       } // end if(stringMatchLoc(line,  sought) >= 0)
    
  } // end while(client.available())

}  // end int Get_temperature() 

// --------------------------------------------------------- end int Get_temperature() 


// --------------------------------------------------------- void scrollText(char *p)

void scrollText(char *p)
{
  uint8_t	charWidth;
  uint8_t	cBuf[8];	// this should be ok for all built-in fonts

  PRINTS("\nScrolling text");
  mx.clear();

  while (*p != '\0')
  {
    charWidth = mx.getChar(*p++, sizeof(cBuf)/sizeof(cBuf[0]), cBuf);

    for (uint8_t i=0; i<charWidth + 1; i++)	// allow space between characters
    {
      mx.transform(MD_MAX72XX::TSL);
      if (i < charWidth)
        mx.setColumn(0, cBuf[i]);
      delay(DELAYTIME);
    }
  }
}

// ---------------------------------------------------------  end void scrollText(char *p)

// -------------------------------------------------------- void zeroPointSet()

void zeroPointSet()
// Demonstrates the use of setPoint and
// show where the zero point is in the display
{
  PRINTS("\nZero point highlight");
  mx.clear();

  if (MAX_DEVICES > 1)
    // uint8_t setChar(uint16_t col, uint8_t c);
    mx.setChar((2*COL_SIZE)-1, '0');

  for (uint8_t i=0; i<ROW_SIZE; i++)
  {
    mx.setPoint(i, i, true);
    mx.setPoint(0, i, true);
    mx.setPoint(i, 0, true);
    delay(DELAYTIME);
  }

  delay(DELAYTIME*3);
} // end void zeroPointSet()

// --------------------------------------------------------------end void zeroPointSet()

// --------------------------------------------------------------- void lines()

void lines()
// Demonstrate the use of drawLine().
// fan out lines from each corner for up to 4 device blocks
{
  PRINTS("\nLines");
  const uint8_t stepSize = 3;
  const uint8_t maxDev = (MAX_DEVICES > 4 ? 4 : MAX_DEVICES);

  mx.clear();
  for (uint16_t c=0; c<(maxDev*COL_SIZE)-1; c+=stepSize)
  {
    mx.drawLine(0, 0, ROW_SIZE-1, c, true);
    delay(DELAYTIME);
  }

  mx.clear();
  for (uint16_t c=0; c<(maxDev*COL_SIZE)-1; c+=stepSize)
  {
    mx.drawLine(ROW_SIZE-1, 0, 0, c, true);
    delay(DELAYTIME);
  }

  mx.clear();
  for (uint16_t c=0; c<(maxDev*COL_SIZE)-1; c+=stepSize)
  {
    mx.drawLine(ROW_SIZE-1, (MAX_DEVICES*COL_SIZE)-1, 0, (MAX_DEVICES*COL_SIZE)-1-c, true);
    delay(DELAYTIME);
  }

  mx.clear();
  for (uint16_t c=0; c<(maxDev*COL_SIZE)-1; c+=stepSize)
  {
    mx.drawLine(0, (MAX_DEVICES*COL_SIZE)-1, ROW_SIZE-1, (MAX_DEVICES*COL_SIZE)-1-c, true);
    delay(DELAYTIME);
  }
} // end void lines()

// ----------------------------------------------------------- end void lines()

// ----------------------------------------------------------- void rows()

void rows()
// Demonstrates the use of setRow()
{
  PRINTS("\nRows 0->7");
  mx.clear();

  for (uint8_t row=0; row<ROW_SIZE; row++)
  {
    mx.setRow(row, 0xff);
    delay(DELAYTIME);
    mx.setRow(row, 0x00);
  }
} // end void rows()

// --------------------------------------------------------- end void rows()

// ----------------------------------------------------------- void columns()

void columns()
// Demonstrates the use of setColumn()
{
  PRINTS("\nCols 0->max");
  mx.clear();

  for (uint8_t col=0; col<mx.getColumnCount(); col++)
  {
    mx.setColumn(col, 0xff);
    delay(DELAYTIME/MAX_DEVICES);
    mx.setColumn(col, 0x00);
  }
} // end void columns()

// -------------------------------------------------------------- end void columns()

// --------------------------------------------------------------- void cross()

void cross()
// Combination of setRow() and setColumn() with user controlled
// display updates to ensure concurrent changes.
{
  PRINTS("\nMoving cross");
  mx.clear();
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  // diagonally down the display R to L
  for (uint8_t i=0; i<ROW_SIZE; i++)
  {
    for (uint8_t j=0; j<MAX_DEVICES; j++)
    {
      mx.setColumn(j, i, 0xff);
      mx.setRow(j, i, 0xff);
    }
    mx.update();
    delay(3*DELAYTIME);
    for (uint8_t j=0; j<MAX_DEVICES; j++)
    {
      mx.setColumn(j, i, 0x00);
      mx.setRow(j, i, 0x00);
    }
  }

  // moving up the display on the R
  for (int8_t i=ROW_SIZE-1; i>=0; i--)
  {
    for (uint8_t j=0; j<MAX_DEVICES; j++)
    {
      mx.setColumn(j, i, 0xff);
      mx.setRow(j, ROW_SIZE-1, 0xff);
    }
    mx.update();
    delay(3*DELAYTIME);
    for (uint8_t j=0; j<MAX_DEVICES; j++)
    {
      mx.setColumn(j, i, 0x00);
      mx.setRow(j, ROW_SIZE-1, 0x00);
    }
  }

  // diagonally up the display L to R
  for (uint8_t i=0; i<ROW_SIZE; i++)
  {
    for (uint8_t j=0; j<MAX_DEVICES; j++)
    {
      mx.setColumn(j, i, 0xff);
      mx.setRow(j, ROW_SIZE-1-i, 0xff);
    }
    mx.update();
    delay(3*DELAYTIME);
    for (uint8_t j=0; j<MAX_DEVICES; j++)
    {
      mx.setColumn(j, i, 0x00);
      mx.setRow(j, ROW_SIZE-1-i, 0x00);
    }
  }
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
} // end void cross()

// ---------------------------------------------------- end void cross()

// ------------------------------------------------------ void bullseye()

void bullseye()
// Demonstrate the use of buffer based repeated patterns
// across all devices.
{
  PRINTS("\nBullseye");
  mx.clear();
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  for (uint8_t n=0; n<3; n++)
  {
    byte  b = 0xff;
    int   i = 0;

    while (b != 0x00)
    {
      for (uint8_t j=0; j<MAX_DEVICES+1; j++)
      {
        mx.setRow(j, i, b);
        mx.setColumn(j, i, b);
        mx.setRow(j, ROW_SIZE-1-i, b);
        mx.setColumn(j, COL_SIZE-1-i, b);
      }
      mx.update();
      delay(3*DELAYTIME);
      for (uint8_t j=0; j<MAX_DEVICES+1; j++)
      {
        mx.setRow(j, i, 0);
        mx.setColumn(j, i, 0);
        mx.setRow(j, ROW_SIZE-1-i, 0);
        mx.setColumn(j, COL_SIZE-1-i, 0);
      }

      bitClear(b, i);
      bitClear(b, 7-i);
      i++;
    }

    while (b != 0xff)
    {
      for (uint8_t j=0; j<MAX_DEVICES+1; j++)
      {
        mx.setRow(j, i, b);
        mx.setColumn(j, i, b);
        mx.setRow(j, ROW_SIZE-1-i, b);
        mx.setColumn(j, COL_SIZE-1-i, b);
      }
      mx.update();
      delay(3*DELAYTIME);
      for (uint8_t j=0; j<MAX_DEVICES+1; j++)
      {
        mx.setRow(j, i, 0);
        mx.setColumn(j, i, 0);
        mx.setRow(j, ROW_SIZE-1-i, 0);
        mx.setColumn(j, COL_SIZE-1-i, 0);
      }

      i--;
      bitSet(b, i);
      bitSet(b, 7-i);
    }
  }

  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
} // end void bullseye()

// --------------------------------------------------------- end void bullseye()

// ---------------------------------------------------------- void stripe()

void stripe()
// Demonstrates animation of a diagonal stripe moving across the display
// with points plotted outside the display region ignored.
{
  const uint16_t maxCol = MAX_DEVICES*ROW_SIZE;
  const uint8_t	stripeWidth = 10;

  PRINTS("\nEach individually by row then col");
  mx.clear();

  for (uint16_t col=0; col<maxCol + ROW_SIZE + stripeWidth; col++)
  {
    for (uint8_t row=0; row < ROW_SIZE; row++)
    {
      mx.setPoint(row, col-row, true);
      mx.setPoint(row, col-row - stripeWidth, false);
    }
    delay(DELAYTIME);
  }
} // end void stripe()

// ------------------------------------------------------------ end void stripe()

// ------------------------------------------------------------ void spiral()

void spiral()
// setPoint() used to draw a spiral across the whole display
{
  PRINTS("\nSpiral in");
  int  rmin = 0, rmax = ROW_SIZE-1;
  int  cmin = 0, cmax = (COL_SIZE*MAX_DEVICES)-1;

  mx.clear();
  while ((rmax > rmin) && (cmax > cmin))
  {
    // do row
    for (int i=cmin; i<=cmax; i++)
    {
      mx.setPoint(rmin, i, true);
      delay(DELAYTIME/MAX_DEVICES);
    }
    rmin++;

    // do column
    for (uint8_t i=rmin; i<=rmax; i++)
    {
      mx.setPoint(i, cmax, true);
      delay(DELAYTIME/MAX_DEVICES);
    }
    cmax--;

    // do row
    for (int i=cmax; i>=cmin; i--)
    {
      mx.setPoint(rmax, i, true);
      delay(DELAYTIME/MAX_DEVICES);
    }
    rmax--;

    // do column
    for (uint8_t i=rmax; i>=rmin; i--)
    {
      mx.setPoint(i, cmin, true);
      delay(DELAYTIME/MAX_DEVICES);
    }
    cmin++;
  }
} // end void spiral()

// ------------------------------------------------------------------ end void spiral()

// --------------------------------------------------------------------- void bounce()

void bounce()
// Animation of a bouncing ball
{
  const int minC = 0;
  const int maxC = mx.getColumnCount()-1;
  const int minR = 0;
  const int maxR = ROW_SIZE-1;

  int  nCounter = 0;

  int  r = 0, c = 2;
  int8_t dR = 1, dC = 1;	// delta row and column

  PRINTS("\nBouncing ball");
  mx.clear();

  while (nCounter++ < 200)
  {
    mx.setPoint(r, c, false);
    r += dR;
    c += dC;
    mx.setPoint(r, c, true);
    delay(DELAYTIME/2);

    if ((r == minR) || (r == maxR))
      dR = -dR;
    if ((c == minC) || (c == maxC))
      dC = -dC;
  }
} // end void bounce()

// ------------------------------------------------------------------------ end void bounce()

// ------------------------------------------------------------------------- void intensity()

void intensity()
// Demonstrates the control of display intensity (brightness) across
// the full range.
{
  uint8_t row;

  PRINTS("\nVary intensity ");

  mx.clear();

  // Grow and get brighter
  row = 0;
  for (int8_t i=0; i<=MAX_INTENSITY; i++)
  {
    mx.control(MD_MAX72XX::INTENSITY, i);
    if (i%2 == 0)
      mx.setRow(row++, 0xff);
    delay(DELAYTIME*3);
  }

  mx.control(MD_MAX72XX::INTENSITY, 8);
} // end void intensity()

// ------------------------------------------------------------------------ end void intensity()

// ----------------------------------------------------------------------- void blinking()

void blinking()
// Uses the test function of the MAX72xx to blink the display on and off.
{
  int  nDelay = 1000;

  PRINTS("\nBlinking");
  mx.clear();

  while (nDelay > 0)
  {
    mx.control(MD_MAX72XX::TEST, MD_MAX72XX::ON);
    delay(nDelay);
    mx.control(MD_MAX72XX::TEST, MD_MAX72XX::OFF);
    delay(nDelay);

    nDelay -= DELAYTIME;
  }
} // -------------------------------------------------------------------- void blinking()

// ----------------------------------------------------------------------- void scanLimit(void)

void scanLimit(void)
// Uses scan limit function to restrict the number of rows displayed.
{
  PRINTS("\nScan Limit");
  mx.clear();

  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  for (uint8_t row=0; row<ROW_SIZE; row++)
    mx.setRow(row, 0xff);
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);

  for (int8_t s=MAX_SCANLIMIT; s>=0; s--)
  {
    mx.control(MD_MAX72XX::SCANLIMIT, s);
    delay(DELAYTIME*5);
  }
  mx.control(MD_MAX72XX::SCANLIMIT, MAX_SCANLIMIT);
} // end void scanLimit(void)

// --------------------------------------------------------------------- end void scanLimit(void)

// ------------------------------------------------------------------------ void transformation1()

void transformation1()
// Demonstrates the use of transform() to move bitmaps on the display
// In this case a user defined bitmap is created and animated.
{
  uint8_t arrow[COL_SIZE] =
  {
    0b00001000,
    0b00011100,
    0b00111110,
    0b01111111,
    0b00011100,
    0b00011100,
    0b00111110,
    0b00000000
  };

  MD_MAX72XX::transformType_t  t[] =
  {
    MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
    MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
    MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
    MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
    MD_MAX72XX::TFLR,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TRC,
    MD_MAX72XX::TSD, MD_MAX72XX::TSD, MD_MAX72XX::TSD, MD_MAX72XX::TSD,
    MD_MAX72XX::TSD, MD_MAX72XX::TSD, MD_MAX72XX::TSD, MD_MAX72XX::TSD,
    MD_MAX72XX::TFUD,
    MD_MAX72XX::TSU, MD_MAX72XX::TSU, MD_MAX72XX::TSU, MD_MAX72XX::TSU,
    MD_MAX72XX::TSU, MD_MAX72XX::TSU, MD_MAX72XX::TSU, MD_MAX72XX::TSU,
    MD_MAX72XX::TINV,
    MD_MAX72XX::TRC, MD_MAX72XX::TRC, MD_MAX72XX::TRC, MD_MAX72XX::TRC,
    MD_MAX72XX::TINV
  };

  PRINTS("\nTransformation1");
  mx.clear();

  // use the arrow bitmap
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  for (uint8_t j=0; j<mx.getDeviceCount(); j++)
    mx.setBuffer(((j+1)*COL_SIZE)-1, COL_SIZE, arrow);
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  delay(DELAYTIME);

  // run through the transformations
  mx.control(MD_MAX72XX::WRAPAROUND, MD_MAX72XX::ON);
  for (uint8_t i=0; i<(sizeof(t)/sizeof(t[0])); i++)
  {
    mx.transform(t[i]);
    delay(DELAYTIME*4);
  }
  mx.control(MD_MAX72XX::WRAPAROUND, MD_MAX72XX::OFF);
} // end void transformation1()

// -------------------------------------------------------------------------- end void transformation1()

// ------------------------------------------------------------------------- void transformation2()

void transformation2()
// Demonstrates the use of transform() to move bitmaps on the display
// In this case font characters are loaded into the display for animation.
{
  MD_MAX72XX::transformType_t  t[] =
  {
    MD_MAX72XX::TINV,
    MD_MAX72XX::TRC, MD_MAX72XX::TRC, MD_MAX72XX::TRC, MD_MAX72XX::TRC,
    MD_MAX72XX::TINV,
    MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TSD, MD_MAX72XX::TSU, MD_MAX72XX::TSD, MD_MAX72XX::TSU,
    MD_MAX72XX::TFLR, MD_MAX72XX::TFLR, MD_MAX72XX::TFUD, MD_MAX72XX::TFUD
  };

  PRINTS("\nTransformation2");
  mx.clear();
  mx.control(MD_MAX72XX::WRAPAROUND, MD_MAX72XX::OFF);

  // draw something that will show changes
  for (uint8_t j=0; j<mx.getDeviceCount(); j++)
  {
    mx.setChar(((j+1)*COL_SIZE)-1, '0'+j);
  }
  delay(DELAYTIME*5);

  // run thru transformations
  for (uint8_t i=0; i<(sizeof(t)/sizeof(t[0])); i++)
  {
    mx.transform(t[i]);
    delay(DELAYTIME*3);
  }
} // end void transformation2()

// ------------------------------------------------------------------------- end void transformation2()

// --------------------------------------------------------------------------- void wrapText()

void wrapText()
// Display text and animate scrolling using auto wraparound of the buffer
{
  PRINTS("\nwrapText");
  mx.clear();
  mx.wraparound(MD_MAX72XX::ON);

  // draw something that will show changes
  for (uint16_t j=0; j<mx.getDeviceCount(); j++)
  {
    // uint8_t setChar(uint16_t col, uint8_t c);
    mx.setChar(((j+1)*COL_SIZE)-1, (j&1 ? 'M' : 'W'));  // sets char displayed altermating MWMW
  }
  delay(DELAYTIME*5);

  // run thru transformations
  for (uint16_t i=0; i<3*COL_SIZE*MAX_DEVICES; i++)
  {
    mx.transform(MD_MAX72XX::TSL);
    delay(DELAYTIME/2);
  }
  for (uint16_t i=0; i<3*COL_SIZE*MAX_DEVICES; i++)
  {
    mx.transform(MD_MAX72XX::TSR);
    delay(DELAYTIME/2);
  }
  for (uint8_t i=0; i<ROW_SIZE; i++)
  {
    mx.transform(MD_MAX72XX::TSU);
    delay(DELAYTIME*2);
  }
  for (uint8_t i=0; i<ROW_SIZE; i++)
  {
    mx.transform(MD_MAX72XX::TSD);
    delay(DELAYTIME*2);
  }

  mx.wraparound(MD_MAX72XX::OFF);
} // end void wrapText()

// ---------------------------------------------------------------------- end void wrapText()

// ---------------------------------------------------------------------- void showCharset(void)

void showCharset(void)
// Run through display of the the entire font characters set
{
  mx.clear();
  mx.update(MD_MAX72XX::OFF);

  for (uint16_t i=0; i<256; i++)
  {
    mx.clear(0);
    mx.setChar(COL_SIZE-1, i);

    if (MAX_DEVICES >= 3)
    {
      char hex[3];

      sprintf(hex, "%02X", i);

      mx.clear(1);
      mx.setChar((2*COL_SIZE)-1,hex[1]);
      mx.clear(2);
      mx.setChar((3*COL_SIZE)-1,hex[0]);
    }

    mx.update();
    delay(DELAYTIME*2);
  }
  mx.update(MD_MAX72XX::ON);
} // end void showCharset(void)

// ------------------------------------------------------------------ end void showCharset(void)

// ---------------------------------------------- void yield(unsigned long previousMillis, int interval)

void yield( int interval) // interval in ms
{
  unsigned long currentMillis = millis();
  unsigned long previousMillis = millis();
  while(currentMillis - previousMillis <= interval) {
    currentMillis = millis();
  } // end  if(currentMillis - previousMillis > interval)
} // end void yield(unsigned long previousMillis, int interval)

// ----------------------------------------------- end void yield(unsigned long previousMillis, int interval)

// -------------------------------------------------------void Connect_to_wifi()

void Connect_to_wifi()
{
  
  yield(250);
  Serial.println("TimeNTP Example");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(6000); // 6*1000
} // end void Connect_to_wifi()

// -------------------------------------------------------end void Connect_to_wifi()

// ---------------------------------------------------- void digitalClockDisplay()

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  printDigits(day());
  Serial.print(".");
  printDigits(month());
  Serial.print(".");
  printDigits(year()); // 2 digit
  Serial.println(); 
} // end void digitalClockDisplay()

// ---------------------------------------------------- end void digitalClockDisplay()

// ------------------------------------------------ void printDigits(int digits)

void printDigits(int digits){
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
} // end void printDigits(int digits)

// --------------------------------------------- end void printDigits(int digits)

// ------------------------------------------------------------------------------- time_t getNtpTime()

time_t getNtpTime()
{
  for (int i = 5; i > 0; i--)
  {
      while (Udp.parsePacket() > 0) ; // discard any previously received packets
      Serial.print("Transmit NTP Request attempt ");
      Serial.print(6-i);
      Serial.println(" of 5");
      sendNTPpacket(timeServer);
      uint32_t beginWait = millis();
      while (millis() - beginWait < 1000) { 
        int size = Udp.parsePacket();
        //Serial.println();
        //Serial.print( "Line 802 Size: ");
        //Serial.println(size);
        if (size >= NTP_PACKET_SIZE) {
          Serial.println("Receive NTP Response");
          Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
          unsigned long secsSince1900;
          // convert four bytes starting at location 40 to a long integer
          secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
          secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
          secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
          secsSince1900 |= (unsigned long)packetBuffer[43];
          Serial.println(" ");                                // display boot DST 
          Serial.print(" daylight Savings ");                 // display boot DST 
          Serial.println(daylightSavingTime(secsSince1900));  // display boot DST 
          // (daylightSavingTime(secsSince1900)+timeZone)  = 0-8=-8. TZ during dst is 1-8=-7
          return secsSince1900 - 2208988800UL + (daylightSavingTime(secsSince1900)+timeZone) * SECS_PER_HOUR 
               + INTRINSIC_DELAY;
               // INTRINSIC_DELAYis how ling between NTP reading and display on ledMatrix = 6 sec
          
        }
      } //     // end while
      Serial.println("No NTP Response :-(");
  } // for for (int i = 5; i > 0; i--)
  return 0; // return 0 if unable to get the time
} // end time_t getNtpTime()

// ---------------------------------------------------------- end time_t getNtpTime()

// ---------------------------------------------------------- sendNTPpacket

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
} // end void sendNTPpacket(IPAddress &address)

// ----------------------------------------------------------- end sendNTPpacket

// -------------------------------------- int createTag
// insert time and temp into tg[]
#define SIZEOFTAG 50
void createTag(int h, int m, int s, int d, int mo, int y, char* tg, int order, int _temperature){
// order 0 yy/mm/dd hh:mm:ss
// order 1 dd/mm/yy hh:mm:ss
// order 2 mm/dd/yy hh:mm:ss
// order 3 hh:mm:ss yy/mm/dd
// order 4 hh:mm:ss dd/mm/yy
// order 5 hh:mm:ss mm/dd/yy
  y=y%100; // converts y to 2 digit
  if (order < 0 || order > 5) return;
  
  // insert time into tg[]
  switch (order){
    case 0:
      // order 0 yy/mm/dd hh:mm:ss
      tg[0]= y/10 + 48;
      tg[1]= y%10 + 48;
      tg[2]= '/';
      tg[3]= mo/10 + 48;
      tg[4]= mo%10 + 48;
      tg[5]= '/';
      tg[6]= d/10 + 48;
      tg[7]= d%10 + 48;
      tg[8]= 0x20;
      
      tg[ 9]= h/10 + 48;
      tg[10]= h%10 + 48;
      tg[11]= ':';
      tg[12]= m/10 + 48;
      tg[13]= m%10 + 48;
      tg[14]= ':';
      tg[15]= s/10 + 48;
      tg[16]= s%10 + 48;
      tg[17]= 0x20;
      tg[18]= 0x00; 
      break;
    case 1:
      // order 1 dd/mm/yy hh:mm:ss
      tg[0]= d/10 + 48;
      tg[1]= d%10 + 48;
      tg[2]= '/';
      tg[3]= mo/10 + 48;
      tg[4]= mo%10 + 48;
      tg[5]= '/';
      tg[6]= y/10 + 48;
      tg[7]= y%10 + 48;
      tg[8]= 0x20;
      
      tg[ 9]= h/10 + 48;
      tg[10]= h%10 + 48;
      tg[11]= ':';
      tg[12]= m/10 + 48;
      tg[13]= m%10 + 48;
      tg[14]= ':';
      tg[15]= s/10 + 48;
      tg[16]= s%10 + 48;
      tg[17]= 0x20;
      tg[18]= 0x00; 
      break;
    case 2: 
      // order 2 mm/dd/yy hh:mm:ss
      tg[0]= mo/10 + 48;
      tg[1]= mo%10 + 48;
      tg[2]= '/';
      tg[3]= d/10 + 48;
      tg[4]= d%10 + 48;
      tg[5]= '/';
      tg[6]= y/10 + 48;
      tg[7]= y%10 + 48;
      tg[8]= 0x20;
      
      tg[ 9]= h/10 + 48;
      tg[10]= h%10 + 48;
      tg[11]= ':';
      tg[12]= m/10 + 48;
      tg[13]= m%10 + 48;
      tg[14]= ':';
      tg[15]= s/10 + 48;
      tg[16]= s%10 + 48;
      tg[17]= 0x20;
      tg[18]= 0x00; 
      break;
    case 3:
      // order 3 hh:mm:ss yy/mm/dd
      tg[0]= h/10 + 48;
      tg[1]= h%10 + 48;
      tg[2]= ':';
      tg[3]= m/10 + 48;
      tg[4]= m%10 + 48;
      tg[5]= ':';
      tg[6]= s/10 + 48;
      tg[7]= s%10 + 48;
      tg[8]= 0x20;
      
      tg[9]= y/10 + 48;
      tg[10]= y%10 + 48;
      tg[11]= '/';
      tg[12]= mo/10 + 48;
      tg[13]= mo%10 + 48;
      tg[14]= '/';
      tg[15]= d/10 + 48;
      tg[16]= d%10 + 48;
      tg[17]= 0x20;
      tg[18]= 0x00;
      break;
    case 4:
      // order 4 hh:mm:ss dd/mm/yy
      tg[0]= h/10 + 48;
      tg[1]= h%10 + 48;
      tg[2]= ':';
      tg[3]= m/10 + 48;
      tg[4]= m%10 + 48;
      tg[5]= ':';
      tg[6]= s/10 + 48;
      tg[7]= s%10 + 48;
      tg[8]= 0x20;
      
      tg[9]= d/10 + 48;
      tg[10]= d%10 + 48;
      tg[11]= '/';
      tg[12]= mo/10 + 48;
      tg[13]= mo%10 + 48;
      tg[14]= '/';
      tg[15]= y/10 + 48;
      tg[16]= y%10 + 48;
      tg[17]= 0x20;
      tg[18]= 0x00; 
      break;
    case 5:
      // order 5 hh:mm:ss mm/dd/yy
      tg[0]= h/10 + 48;
      tg[1]= h%10 + 48;
      tg[2]= ':';
      tg[3]= m/10 + 48;
      tg[4]= m%10 + 48;
      tg[5]= ':';
      tg[6]= s/10 + 48;
      tg[7]= s%10 + 48;
      tg[8]= 0x20;
      
      tg[ 9]= mo/10 + 48;
      tg[10]= mo%10 + 48;
      tg[11]= '/';
      tg[12]= d/10 + 48;
      tg[13]= d%10 + 48;
      tg[14]= '/';
      tg[15]= y/10 + 48;
      tg[16]= y%10 + 48;
      tg[17]= 0x20;
      tg[18]= 0x00; 
      break;
      
  } // end switch order for time
  //insert temp 
      tg[18]= 't';
      tg[19]= 'e';
      tg[20]= 'm';
      tg[21]= 'p';
      tg[22]= '(';
      tg[23]= 'C';
      tg[24]= ')';
      int next_digit = 24 + 1;  // last character before temp + 1
      String tem = String(_temperature);
      int j = tem.length();
      for (int i = 0; i < j; i++)
        tg[next_digit+i] = tem[i];
      for (int i=0; i<8; i++)
        tg[next_digit+j+i] = 0x20; 
      tg[next_digit+j+8]= 0x00;
  return;   
} // end int createTag

// --------------------------------------- end int createTag


// ------------------------------------------------------------------ void setup()

void setup()
{
  mx.begin();
  Serial.begin(115200);
  Connect_to_wifi();
#if  DEBUG
  
  delay(1000);
#endif
  PRINTS("\n[MD_MAX72XX Test & Demo]");
//  scrollText("MD_MAX72xx Test  ");
} // end void setup()

// ------------------------------------------------------------------------- end void setup()

// --------------------------------------------------------------------- void loop()

void loop()
{
  // get time
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      digitalClockDisplay();
    }
  }
  // get temp
  Serial.println();
  Serial.print("In Loop() Temperature=");
 
  Serial.println(Get_temperature());
  // setup character array to display
  char  textToDisplay[100] = "Time ";
  char tag[50]; // contains time and date
  // void createTag(int h, int m, int s, int d, int mo, int y, char* tg, int order, int _temperature)
  createTag(hour(), minute(), second(), day(), month(), year()%100, tag, 5, temperature);
  strcat(textToDisplay, tag); // appends tag to textToDisplay

  // display char array
#if 0
  scrollText("Graphics  ");
  zeroPointSet();
  lines();
  rows();
  columns();
  cross();
  stripe();
  bullseye();
  bounce();
  spiral();
#endif

#if 0
  scrollText("Control  ");
  intensity();
  scanLimit();
  blinking();
#endif

#if 0
  scrollText("Transform  ");
  transformation1();
  transformation2();
#endif

#if 0
  scrollText("Charset  ");
  wrapText();
  showCharset();
#endif

#if 1
  
  scrollText(textToDisplay);
  
#endif
} // end void loop()

// -------------------------------------------------------------- end void loop()

