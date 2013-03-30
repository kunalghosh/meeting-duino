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

/*************************************************
 * Public Constants
 *************************************************/
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_C4_1 260
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978

int speakerPin = A3;

// notes in the melody:
int melody[] = {
  NOTE_CS4,NOTE_C4, NOTE_D4, NOTE_C4,NOTE_F4,NOTE_E4,
  NOTE_C4_1,NOTE_C4,NOTE_D4,NOTE_C4,NOTE_G4,NOTE_F4,
  NOTE_C4_1,NOTE_C4,NOTE_C5,NOTE_A4,NOTE_F4,NOTE_E4,NOTE_D4,
  NOTE_AS4,NOTE_AS4,NOTE_A4,NOTE_F4,NOTE_G4,NOTE_F4};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  6, 6, 3, 3,3,3,
  6, 6, 3, 3,3,3,
  6, 6, 3, 3,3,3,3, 
  6, 6, 3, 3,3,3 };

#define MESSAGE_SIZE 100
#define TIME_STAMP 13 // to add an ending \0 terminator where required. hence 12+1
//#define NON_MESSAGE 14 //in an appointment string 201303282305 , : and ~ are not a part of message so 12 characters
// initialize the library with the numbers of the interface pins


LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

const byte EEPROM_ID = 0x50;      // I2C address for 24LC128 EEPROM

char nextAppointment[MESSAGE_SIZE] = "201303300330:Wake Up";
char readAppointment[MESSAGE_SIZE] = "";
//char appointmentMessage[MESSAGE_SIZE] = "";
char *appointmentMessage;

char timeStampAppointment[TIME_STAMP+1]="";
char timeStampCurrentAdj[TIME_STAMP]=""; // current time stamp adjusted by 'readBeforeMins' minutes

int  remindBeforeMins = 0;
int messageLength;

int piezoPin = A3;
int resetLCD = 0;

int EEPROMreadPosition = 0;
// First few bytes are reserved for the time of Appointment
// YYYYMMDDHHmm
// Y year
// M month
// D day
// H hour ( 24 hour format )
// m minute


void setup()  {


  Serial.begin(9600);  
  pinMode(piezoPin,OUTPUT);
  //LCD led turn on.
  pinMode(A2,OUTPUT);
  //digitalWrite(A2,HIGH);//LCD LED backlight turn ON.
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  lcd.begin(16, 2);

  //READ message from EEPROM
  ReadNextAppointment(); 
  getTimeStampFromAppointment();// this populates the timeStamp string
  //populateDateVariables();

}

void loop()
{
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

  Serial.println(timeStampAppointment);
  Serial.println(timeStampCurrentAdj);

  if(!strcmp(timeStampAppointment,timeStampCurrentAdj)){
    getAppointmentMessage();
    messageLength = strlen(appointmentMessage);
    lcd.setCursor(0,1);
    lcd.print(appointmentMessage);
    tune();

    //lcd.setCursor(0,1);
    //lcd.print(appointmentMessage);
    //for(int i=0 ; i < strlen(appointmentMessage);i++){
    //  lcd.scrollDisplayLeft();
    //  delay(700);
    //}  
    resetLCD = 1;   
    digitalWrite(A2, HIGH); 
    
  }
  if(strcmp(timeStampAppointment,timeStampCurrentAdj) && resetLCD){  
    lcd.home();  
    lcd.clear();
    digitalWrite(A2,LOW);
    resetLCD = 0;
  }
}

void tune(){
  int thisNote = 0;
  //for (int thisNote = 0,scroll=0; thisNote < 26; thisNote++) {

  // to calculate the note duration, take one second 
  // divided by the note type.
  //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
  int noteDuration = 1000/noteDurations[thisNote];
  tone(speakerPin, melody[thisNote],noteDuration);

  int pauseBetweenNotes = noteDuration + 50;      //delay between pulse
  delay(pauseBetweenNotes+100);

  noTone(speakerPin);                // stop the tone playing

  //------------

  //for(int i=0 ; i < strlen(appointmentMessage);i++){
  //if(scroll++ <= messageLength){
  //lcd.scrollDisplayLeft();
  //}
  //else{
  lcd.scrollDisplayLeft();
  //lcd.home();  
  //lcd.clear();
  //scroll = 0;
  //}
  //delay(700);
  //}
  //}
} 

void getCurrentAdjustedTimeStamp(){
  sprintf(timeStampCurrentAdj,"%d%02d%02d%02d%02d",year(),month(),day(),hour(),minute()-remindBeforeMins);
}

void getTimeStampFromAppointment(){// this populates the timeStamp string
  //strncpy(timeStampAppointment,readAppointment,sizeof(timeStampAppointment));
  strncpy(timeStampAppointment,nextAppointment,sizeof(timeStampAppointment));
  timeStampAppointment[TIME_STAMP-1]='\0';
}

void getAppointmentMessage(){
  //strcpy(appointmentMessage,nextAppointment);
  //char *messageEnd = strchr(appointmentMessage,'~');
  char *messageEnd = strchr(nextAppointment,'~');
  messageEnd = '\0';
  appointmentMessage = nextAppointment + 13; // remove the starting timeStamp

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




