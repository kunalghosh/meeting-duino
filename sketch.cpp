/*
 * TimeRTCSet.pde
 * example code illustrating Time library with Real Time Clock.
 *
 * RTC clock is set in response to serial port time message 
 * A Processing example sketch to set the time is inclided in the download
 */

#include <Time.h>  
#include <Wire.h>  
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t
#include <LiquidCrystal.h>

#define MESSAGE_SIZE 100
#define TIME_STAMP 12
#define NON_MESSAGE 14 //in an appointment string 201303282305 , : and ~ are not a part of message so 12 characters
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

const byte EEPROM_ID = 0x50;      // I2C address for 24LC128 EEPROM

char nextAppointment[MESSAGE_SIZE] = "201303282305:This is my message~";
char readAppointment[MESSAGE_SIZE] = "";
char appointmentMessage[MESSAGE_SIZE-NON_MESSAGE] = "";

char timeStampAppointment[TIME_STAMP]="";
char timeStampCurrentAdj[TIME_STAMP]=""; // current time stamp adjusted by 'readBeforeMins' minutes

int  remindBeforeMins = 5;

int piezoPin = A3;


// First few bytes are reserved for the time of Appointment
// YYYYMMDDHHmm
// Y year
// M month
// D day
// H hour ( 24 hour format )
// m minute


void setup()  {


  //Serial.begin(9600);  
  pinMode(piezoPin,OUTPUT);
  //LCD led turn on.
  pinMode(A2,OUTPUT);
  digitalWrite(A2,HIGH);
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  lcd.begin(16, 2);

  //READ message from EEPROM
  ReadNextAppointment(); 
  getTimeStampFromAppointment();// this populates the timeStamp string
  //populateDateVariables();

  if ( appYear == year()){
    for(int i =0 ; i < 400; i ++){
      analogWrite(A3,i); 
      //delay(100);
      noTone(100);
    }
  }
}

void loop()
{
  lcd.print(appYear);
  delay(3000);
  lcd.setCursor(0, 0);
  lcd.print("Date: ");
  displayNumLCD(day());
  lcd.print("/");
  displayNumLCD(month());
  lcd.print("/");
  displayNumLCD(year());

  lcd.setCursor(0, 1);
  lcd.print("Time: ");
  displayNumLCD(hour());
  blinkColon();
  displayNumLCD(minute());
  //lcd.print(":");
  //displayNumLCD(second());

  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  // lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  //delay(10000);

  if( ( millis() / 100000 ) % 100 == 0 )
    setSyncProvider(RTC.get);   // the function to get the time from the RTC

  getCurrentAdjustedTimeStamp();

  if(!strcmp(timeStampAppointment,timeStampCurrentAdj)){
	for(int i = 0 ; i < 1000 ; i++)
		beep(50);
  }
}

void beep(unsigned char delayms){
	analogWrite(piezoPin, 20);      // Almost any value can be used except 0 and 255
	// experiment to get the best tone
	delay(delayms);          // wait for a delayms ms
	analogWrite(piezoPin, 0);       // 0 turns it off
	delay(delayms);          // wait for a delayms ms   
} 

void getCurrentAdjustedTimeStamp(){
	sprintf(timeStampCurrentAdj,"%d%d%d%d%d",year(),month(),day(),hour(),minute()-remindBeforeMins);
}

void blinkColon(){
  if(second() % 2 == 0)
    lcd.print(":");
  else
    lcd.print(" ");

}

void displayNumLCD(int value){
  /* This logic is used in cases where there is a cyclic counting of
   * numbers happening. i.e. when counting seconds (0 to 59), minutes ( 0 to 59) etc.
   * when after 59 , 0 is displayed, lcd shows it as 09. The 9 of 59 is not cleared.
   * So, we can either do lcd.clear() repeatedly. Or make single digit numbers double digit
   * by adding a 0 in front of them.
   *
   * NOTE : This logic will not work if the cyclic count is of 3 digits, 0 to 999
   * In that case the logic would have to be different. It will be. single digit numbers prepend two zeros.
   * two digit numbers prepend one zero and for 3 digit numbers prepend no zeros."
   */
  if(value < 10){
    lcd.print("0");
  }
  lcd.print(value);
}

void getTimeStampFromAppointment(){// this populates the timeStamp string
  strncpy(timeStamp,readAppointment,sizeof(timeStamp));
}

void ReadNextAppointment(){
  for ( EEPROMreadPosition=0; EEPROMreadPosition < 100 ; EEPROMreadPosition++)
  {
    char c = I2CEEPROM_Read(EEPROMreadPosition);
    readAppointment[EEPROMreadPosition] = c;
    //      Serial.print(c);

    if(c == '~')
    {
      readAppointment[++EEPROMreadPosition] = '\0';
      //Serial.println();
      break;     // start over on a new line
    }
  }

  //Serial.println(readAppointment);

}

/*
void populateDateVariables(){
 
 //sample appointment => "201303282305:This is my message~";
 char temporary[4] = "";
 
 temporary[0] = readAppointment[0]
 temporary[1] = readAppointment[1]
 temporary[2] = readAppointment[2]
 temporary[3] = readAppointment[3]
 
 appointmentYear = atoi(temporary);
 
 temporary = "";
 temporary[0] = readAppointment[4]
 temporary[1] = readAppointment[5]
 temporary[2]='\0'
 
 
 appointmentSecond;
 appointmentMinute;
 appointmentHour;
 appointmentDay;
 appointmentMonth;
 
 
 appointmentMessage;
 
 
 }
 */

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





