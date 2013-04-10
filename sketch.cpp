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


LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

const byte EEPROM_ID = 0x50;      // I2C address for 24LC128 EEPROM

// Assuming a 256KB EEPROM so total space = 256 * 1024, each appointment 
// entry is 100Bytes hence ~ 2600 Entries max.
#define ALL_APPOINTMENTS_BYTE_COUNT 2 // Bytes
// While writing the appointments in EEPROM, the total number of characters ( byte count )
// is stored in the first two bytes of the EEPROM
// ( first byte value ) * 256 + ( second byte value ) = total number of bytes ( for all appointments ) in the EEPROM.

#define NEXT_APPOINTMENT_POS 2 // Bytes
// The third and fourth bytes are used to store the position of the next appointment in EEPROM.
// This is useful in case the microcontroller looses power and is reset.
// The byte address of the next appointment = ( third byte value ) * 256 + ( fourth byte value )

// The first four bytes of the EEPROM must be correctly initialized otherwise the device will not behave correctly.

/* Incase the microcontroller looses power before the next appointment address is written, or it is powered down
 * and in the interim a lot of appointments are missed, then it should be able to resume from the next non expired appointment.
 */

#define MESSAGE_SIZE 100
#define TIME_STAMP 13 // to add an ending \0 terminator where required. hence 12+1
//#define NON_MESSAGE 14 //in an appointment string 201303282305 , : and ~ are not a part of message so 12 characters



//char nextAppointment[MESSAGE_SIZE] = "201303310729:Wake Up";
char readAppointment[MESSAGE_SIZE] = "";
//char appointmentMessage[MESSAGE_SIZE] = "";
char *appointmentMessage;
// First few bytes are reserved for the time of Appointment
// YYYYMMDDHHmm
// Y year
// M month
// D day
// H hour ( 24 hour format )
// m minute



// Increment this everytime an appointment is met. 
// At any given instant the below variable stores 
// the start byte count of the next appointment.
int  nextAppointmentByteAddress = 0;
int  currentAppointmentStartByteAddress = 0;
//the byte count of the '~' of the last appointment
int  appointmentLastByteAddress = 0;

char timeStampAppointment[TIME_STAMP+1]="";
char timeStampCurrentAdj[TIME_STAMP]=""; // current time stamp adjusted by 'readBeforeMins' minutes

int  remindBeforeMins = 0;
int  messageLength;

int  piezoPin = A3;
int  resetLCD = 0;


void setup()  {
	Serial.begin(9600);  
	pinMode(piezoPin,OUTPUT);
	//LCD led turn on.
	pinMode(A2,OUTPUT);
	//digitalWrite(A2,HIGH);//LCD LED backlight turn ON.
	setSyncProvider(RTC.get);   // the function to get the time from the RTC
	lcd.begin(16, 2);
        
        Serial.println("Getting All Appointments' Last Byte Address.");
	getAppointmentLastByteCount();//the byte count of the '~' of the last appointment stored in appointmentLastByteAddress
        Serial.println("All Appointments' last byte address is:");
        Serial.println(appointmentLastByteAddress);
	getNextAppointmentByteCount();//the start (byte) count of an appointment which hasn't expired yet.
        Serial.println("nextAppointmentByteAddress is");
        Serial.println(nextAppointmentByteAddress);
	//READ message from EEPROM
	//ReadNextAppointment(); 

	getNextUnExpiredAppointment();
        Serial.println("Next Unexpired Appointment Address is :");
        Serial.println(nextAppointmentByteAddress);
        Serial.println(readAppointment);
        
	getTimeStampFromAppointment();// this populates the timeStamp string
	//populateDateVariables();
        Serial.println("Time Stamp from appointment");
        Serial.println(timeStampAppointment);
        
}

void loop()
{

	displayDateTime(); //display's date and time on the first row of the 16x2 LCD

	if( ( millis() / 100000 ) % 100 == 0 ){// every one seconds
		setSyncProvider(RTC.get);   // the function to get the time from the RTC
	}

	if( !second() ){// when second == 0 i.e. after 59
		getCurrentAdjustedTimeStamp();
		Serial.println(timeStampAppointment);
		Serial.println(timeStampCurrentAdj);  
	}


	if(!strcmp(timeStampAppointment,timeStampCurrentAdj)){
		getAppointmentMessage();
		Serial.println(appointmentMessage);

		messageLength = strlen(appointmentMessage);
		lcd.setCursor(0,1);
		lcd.print(appointmentMessage);

		tune();

		lcd.scrollDisplayLeft();
		resetLCD = 1;   
		digitalWrite(A2, HIGH); 
	}

	if(strcmp(timeStampAppointment,timeStampCurrentAdj) && resetLCD){ 
		// turns LCD backlight off 
		// increments nextAppointmentByteAddress so that the next appointment is read.
		lcd.home();  
		lcd.clear();
		digitalWrite(A2,LOW);
		resetLCD = 0;
		//nextAppointmentByteAddress += 1;
		ReadNextAppointment();
		getTimeStampFromAppointment();
	}
	if ( Serial.available() ){
		char ch = Serial.read();
		switch (ch){
			case 'T':RTCWrite();
				 setSyncProvider(RTC.get);   // the function to get the time from the RTC
				 break;
			case 'A':APPOINTMENTWrite();
				 break;
			default :break;
		}
	}
}

void APPOINTMENTWrite(){}

#define TIME_MSG_LEN  10   // time sync to PC is HEADER followed by unix time_t as ten ascii digits

void RTCWrite(){

	time_t pctime = 0;
	for(int i=0; i < TIME_MSG_LEN ; i++){   
		char c = Serial.read();          
		if( c >= '0' && c <= '9'){   
			pctime = (10 * pctime) + (c - '0') ; // convert digits to a number    
		}
	}   

	RTC.set(pctime);   // set the RTC and the system time to the received value
	setTime(pctime);          
}
void getNextUnExpiredAppointment(){
	// Keep Reading the Next Appointment till you get an appointment which has not expired.
	// All subsequent appointments must ( by design ) , be also valid ( not expired ).
        Serial.println("Inside getNextUnExpiredAppointment()");

	int oldMin = minute();
        int appointmentYear;
        int appointmentTimeStampWithoutYear;
        
	unsigned long currentTimeStampWithoutYear = getCurrentTimeStampWithoutYear(); 
        Serial.println("Got the Current Time Stamp without Year");
	while(nextAppointmentByteAddress < appointmentLastByteAddress){
                Serial.println("Entered the loop");
	
        	ReadNextAppointment();
		getTimeStampFromAppointment();//timeStampAppointment
        
                appointmentYear = getAppointmentYear();
                appointmentTimeStampWithoutYear = getAppointmentTimeStampWithoutYear();
                
                Serial.print("getAppointmentYear() : ");
                Serial.println(appointmentYear);
                
                Serial.print("getAppointmentTimeStampWithoutYear() : ");
                Serial.println(appointmentTimeStampWithoutYear);
                
                Serial.print("nextAppointmentByteAddress : ");
                Serial.println(nextAppointmentByteAddress);



		if(appointmentYear < year())
			continue;
                else if(appointmentYear > year())
                        break;
		else if( appointmentTimeStampWithoutYear < currentTimeStampWithoutYear)
			continue;
		if ( minute() != oldMin ){
			currentTimeStampWithoutYear = getCurrentTimeStampWithoutYear(); 
			oldMin = minute();
		}
                Serial.println("Still in the loop");
	}
        if(nextAppointmentByteAddress == appointmentLastByteAddress){
          currentAppointmentStartByteAddress = appointmentLastByteAddress;
          //since we have reached the end of the last appointment set the currentAppointmentStartByteAddress to the appointment Last Byte address.
          //This is an indication that there are no new appointments.
          
          Serial.println("No New Appointments.");
          lcd.setCursor(0,1);          
          lcd.print("No New Appointments");
          
        }
	setNextUnexpiredAppointmentPos();
        getNextAppointmentByteCount();
        Serial.println("Going out of : getNextUnExpiredAppointment()");
}

void setNextUnexpiredAppointmentPos(){
	// write back the next UnExpired appointment byte position to the EEPROM
	// Verify if the byte position is < = 256 * NEXT_APPOINTMENT_POS + 256
	
        Serial.println("Inside setNextUnexpiredAppointmentPos()");
                
        int copyOfCurrentAppointmentStartByteAddress = currentAppointmentStartByteAddress;
        Serial.print("currentAppointmentStartByteAddress :");
        Serial.print(currentAppointmentStartByteAddress);
        char ch;
	for ( int nextUnexpiredByteAddress = NEXT_APPOINTMENT_POS + ALL_APPOINTMENTS_BYTE_COUNT - 1 ; nextUnexpiredByteAddress > ALL_APPOINTMENTS_BYTE_COUNT - 1 ; nextUnexpiredByteAddress-- ){
                ch = copyOfCurrentAppointmentStartByteAddress%256;
		I2CEEPROM_Write(nextUnexpiredByteAddress,ch); 
		copyOfCurrentAppointmentStartByteAddress /= 256;
	}
        Serial.println("Going out of: setNextUnexpiredAppointmentPos()");
}

unsigned long getAppointmentTimeStampWithoutYear(){
        
        Serial.println("Inside getAppointmentTimeStampWithoutYear()");
	char timeStampAppointmentCopy[12];
	strcpy(timeStampAppointmentCopy,timeStampAppointment);
	//remove year
	timeStampAppointmentCopy[0] = '0';
	timeStampAppointmentCopy[1] = '0';
	timeStampAppointmentCopy[2] = '0';
	timeStampAppointmentCopy[3] = '0';
        
        Serial.print("timeStampAppointment");
        Serial.println(timeStampAppointment);
        
        
        Serial.print("timeStampAppointmentCopy");
        Serial.println(timeStampAppointmentCopy);
        
        Serial.println("Going out of : getAppointmentTimeStampWithoutYear()");      	
	return (atol(timeStampAppointmentCopy));
}
unsigned long getCurrentTimeStampWithoutYear(){

        Serial.println("Inside : getCurrentTimeStampWithoutYear()");      	
	getCurrentAdjustedTimeStamp();
	//remove year
	timeStampCurrentAdj[0] = '0';
	timeStampCurrentAdj[1] = '0';
	timeStampCurrentAdj[2] = '0';
	timeStampCurrentAdj[3] = '0';
	unsigned long currentTimeStampWithoutYear = atol(timeStampCurrentAdj);
	
        Serial.print("timeStampCurrentAdj : ");
        Serial.println(timeStampCurrentAdj);

        Serial.print("atol(timeStampCurrentAdj) : ");
        Serial.println(currentTimeStampWithoutYear);
        
	//re-set the current adjusted time stamp
	getCurrentAdjustedTimeStamp();

        Serial.println("Going out of : getCurrentTimeStampWithoutYear()");      	
	return(currentTimeStampWithoutYear);
}
int getAppointmentYear(){
	//format is YYYYMMDDHHmm
        Serial.println("Inside getAppointmentYear()");
	char chYear[5];
        Serial.print("timeStampAppointment : ");
        Serial.println(timeStampAppointment);
	
        chYear[0]=timeStampAppointment[0];
	chYear[1]=timeStampAppointment[1];
	chYear[2]=timeStampAppointment[2];
	chYear[3]=timeStampAppointment[3];
        chYear[4]='\0';
        
        Serial.print("chYear : ");
        Serial.println(chYear);
        int returnYear = atoi(chYear);
        
        Serial.print("atoi(chYear) : ");
        Serial.println(returnYear);
        
        Serial.println("Going out of getAppointmentYear()");
	return(returnYear);

}
int getAppointmentMonth(){
	//format is YYYYMMDDHHmm
	char chMonth[2];
	chMonth[0]=timeStampAppointment[4];
	chMonth[1]=timeStampAppointment[5];
	return(atoi(chMonth));
}
int getAppointmentDay(){
	//format is YYYYMMDDHHmm
	char chDay[2];
	chDay[0]=timeStampAppointment[6];
	chDay[1]=timeStampAppointment[7];
	return(atoi(chDay));
}

int getAppointmentHour(){
	//format is YYYYMMDDHHmm
	char chHour[4];
	chHour[0]=timeStampAppointment[8];
	chHour[1]=timeStampAppointment[9];
	return(atoi(chHour));
}
int getAppointmentMin(){
	//format is YYYYMMDDHHmm
	char chMin[2];
	chMin[0]=timeStampAppointment[10];
	chMin[1]=timeStampAppointment[11];
	return(atoi(chMin));
}

void displayDateTime(){
	lcd.setCursor(0, 0);
	//lcd.print("Date: ");
	displayNumLCD(day());
	lcd.print("/");
	displayNumLCD(month());
	lcd.print("/");
	displayNumLCD(year());

	//lcd.setCursor(0, 1);
	//lcd.print("Time: ");
	lcd.print(" ");
	displayNumLCD(hour());
	blinkColon();
	displayNumLCD(minute());
}

void tune(){
	int thisNote = 0;

	// divided by the note type.
	//e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
	int noteDuration = 1000/noteDurations[thisNote];
	tone(speakerPin, melody[thisNote],noteDuration);

	int pauseBetweenNotes = noteDuration + 50;      //delay between pulse
	delay(pauseBetweenNotes+100);

	noTone(speakerPin);                // stop the tone playing
} 

void getCurrentAdjustedTimeStamp(){
	sprintf(timeStampCurrentAdj,"%d%02d%02d%02d%02d",year(),month(),day(),hour(),minute()-remindBeforeMins);
}

void getTimeStampFromAppointment(){// this populates the timeStamp string
        Serial.println("Inside getTimeStampFromAppointment()");
         
        Serial.print("readAppointment : ");
        Serial.println(readAppointment);
        
	strncpy(timeStampAppointment,readAppointment,sizeof(timeStampAppointment));
	//strncpy(timeStampAppointment,nextAppointment,sizeof(timeStampAppointment));
	timeStampAppointment[TIME_STAMP-1]='\0';
        
        Serial.print("timeStampAppointment : ");
        Serial.println(timeStampAppointment);
        
        Serial.println("Going out of : getTimeStampFromAppointment()");
}

void getAppointmentMessage(){
	//strcpy(appointmentMessage,nextAppointment);
	strcpy(appointmentMessage,readAppointment);
	char *messageEnd = strchr(appointmentMessage,'~');
	//char *messageEnd = strchr(nextAppointment,'~');
	messageEnd = '\0';
	//appointmentMessage = nextAppointment + 13; // remove the starting timeStamp
	appointmentMessage = readAppointment + 13; // remove the starting timeStamp
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
                //Serial.println(c);
                //Serial.println(d);
	}
	appointmentLastByteAddress += d;
}
void getNextAppointmentByteCount(){//the start (byte) count of an appointment which hasn't expired yet.
// See comment under #define NEXT_APPOINTMENT_POS
// From the ALL_APPOINTMENTS_BYTE_COUNT'th Byte to the NEXT_APPOINTMENT_POS - 1 byte
        Serial.println("Inside : getNextAppointmentByteCount()");
        char c;
        int d;
	int lastByteAddress = ALL_APPOINTMENTS_BYTE_COUNT+NEXT_APPOINTMENT_POS; //The byte position upto which to read
        Serial.println("lastByteAddress is :");
        Serial.println(lastByteAddress);
	for( int EEPROMByteCount = ALL_APPOINTMENTS_BYTE_COUNT ;  EEPROMByteCount < lastByteAddress  ;  EEPROMByteCount++ ){
		c = I2CEEPROM_Read(EEPROMByteCount);
                d = 0;
                d = c;
                Serial.print("C inside loop: ");
                Serial.println(c);
		if ( EEPROMByteCount != lastByteAddress - 1 ){
			nextAppointmentByteAddress = 256 * d;
                        Serial.print("d inside loop:");          
                        Serial.println(d);          
                }
	}
        Serial.print("d:");
        Serial.println(d);
	nextAppointmentByteAddress += d;
        Serial.println("nextAppointmentByteAddress is:");
        Serial.println(nextAppointmentByteAddress);        
        Serial.println("Going Out Of : getNextAppointmentByteCount()");
}

void ReadNextAppointment(){

        Serial.println(" Inside ReadNextAppointment()");
	currentAppointmentStartByteAddress = nextAppointmentByteAddress ;
        int readAppointmentCounter = 0;
	for ( nextAppointmentByteAddress ; nextAppointmentByteAddress < appointmentLastByteAddress ; nextAppointmentByteAddress++,readAppointmentCounter++)
	//not initialized to zero everytime so that the count is maintained.
	{
		char c = I2CEEPROM_Read(nextAppointmentByteAddress);
		readAppointment[readAppointmentCounter] = c;
		//      Serial.print(c);

		if(c == '~')
		{
			readAppointment[readAppointmentCounter] = '\0';
			//readAppointment[nextAppointmentByteAddress] = '\0';
			//Serial.println();
                        readAppointmentCounter = 0;
			break;     // start over on a new line
		}
	}
        Serial.print("readAppointment : ");
	Serial.println(readAppointment);
	//setNextUnexpiredAppointmentPos();	
        Serial.println(" Going out of : ReadNextAppointment()");
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


