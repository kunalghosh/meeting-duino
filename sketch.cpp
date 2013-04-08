/*
 * I2C EEPROM sketch
 * this version for 24LC128
 */
#include <Wire.h>

const byte EEPROM_ID = 0x50;      // I2C address for 24LC128 EEPROM

#define ALL_APPOINTMENTS_BYTE_COUNT 2

// first visible ASCII character '!' is number 33:
byte a = B00000000;//0
byte b = B00000100;//4
char nextAppointment[100] = " K  201303311139:This is an old message~201304071840:This is my new message~";

char readAppointment[100];
int appointmentLastByteAddress = 0;
//char c = 0 ;
// First few bytes are reserved for the time of Appointment
// YYYYMMDDHHmm
// Y year
// M month
// D day
// H hour ( 24 hour format )
// m minute

//nextAppointment = "201303281845:This is my message~";
void setup()
{
  nextAppointment[0]=a;
  nextAppointment[2]=a;
  nextAppointment[3]=b;
  Serial.begin(9600);
  Wire.begin();
  
  Serial.println("Appointment Last Byte Address");
  getAppointmentLastByteCount();
  Serial.println(appointmentLastByteAddress);
  
  Serial.println("Writing an Appointment");
  Serial.println(nextAppointment);

  for (int i=0; i <= appointmentLastByteAddress; i++)
  {  
    I2CEEPROM_Write(i, int(nextAppointment[i]));
    Serial.print(nextAppointment[i]);
    //if (nextAppointment[i] == '~')   // you could also use if (thisByte == '~')
    //  break;     // start over
  }

  Serial.println("Reading from EEPROM");
  int j =0 ;
  for (int i=4; i < appointmentLastByteAddress ; i++,j++)
  {
    char c = I2CEEPROM_Read(i);
    /*if(!isDigit(c)){
      break;
    }*/
    //Serial.print(c);
    readAppointment[j] = c;
    Serial.print(c);

    if(c == '~')
    {
      readAppointment[j] = '\0';
      Serial.println(readAppointment);
      Serial.println("Next One");
      j = 0;
      //break;     // start over on a new line
    }
  }

  //Serial.println(readAppointment);
  Serial.println();

  /*
  for (int i=0; i < 1000; i++)
   {
   char c = I2CEEPROM_Read(i);
   
   //Serial.print(c);
   readAppointment[i] = char(c);
   readAppointment[i+1] = '\0';
   
   if(c == '~')
   {
   break;
   }
   }
   Serial.print(readAppointment);
   Serial.println("The End");
   */
}

void loop()
{

}

void getAppointmentLastByteCount(){//the byte count of the '~' of the last appointment and store it in appointmentLastByteAddress 
// See comment under #define ALL_APPOINTMENTS_BYTE_COUNT
// From the 1st Byte of EEPROM to the ALL_APPOINTMENTS_BYTE_COUNT'th - 1 byte
        char c;
        int d;
	for( int EEPROMByteCount = 0 ;  EEPROMByteCount < ALL_APPOINTMENTS_BYTE_COUNT ;  EEPROMByteCount++ ){
		c = I2CEEPROM_Read(EEPROMByteCount);
                d = c;
		if ( EEPROMByteCount != ALL_APPOINTMENTS_BYTE_COUNT - 1 )
                       appointmentLastByteAddress = 256 * d;
                Serial.println(c);
                Serial.println(d);
	}
	appointmentLastByteAddress += d;
}

// This function is similar to EEPROM.write()
void I2CEEPROM_Write( unsigned int address, byte data )
{
  Wire.beginTransmission(EEPROM_ID);
  Wire.write((int)highByte(address) );
  Wire.write((int)lowByte(address) );
  Wire.write(data);
  Wire.endTransmission();
  delay(5); // wait for the I2C EEPROM to complete the write cycle
}

// This function is similar to EEPROM.read()
byte I2CEEPROM_Read(unsigned int address )
{
  byte data;
  Wire.beginTransmission(EEPROM_ID);
  Wire.write((int)highByte(address) );
  Wire.write((int)lowByte(address) );
  Wire.endTransmission();
  Wire.requestFrom(EEPROM_ID,(byte)1);
  while(Wire.available() == 0) // wait for data
    ;
  data = Wire.read();
  return data;
}

