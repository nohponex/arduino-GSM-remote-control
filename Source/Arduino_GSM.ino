// include the GSM library
#include <GSM.h>

// PIN Number for the SIM
//#define PIN_NUMBER "1234" //Uncomment if SIM Card has PIN number

#define PASSWORD_CHAR '@'
#define IGNORE_CHAR '-'

#define MESSAGE_BUFFER 160

#define NUMBER_OF_DEVICES 2
#define DEVICES_PINS { 10, 11 }

//Encodings
#define ENCODING_DEFAULT 0
#define ENCODING_HEX 1



GSM gsmAccess;
GSM_SMS sms;


char hex[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

// Array to hold the number a SMS is retreived from
char senderNumber[20];  
char message[MESSAGE_BUFFER];

byte devices = NUMBER_OF_DEVICES; //Number of devices
byte device_pins[ ] = DEVICES_PINS; // I/O Pin for each device

//Setup function
void setup() 
{
  for( int i =0 ; i < devices ; i++){
    pinMode( device_pins[ i ], OUTPUT);
  }
  // initialize serial communications and wait for port to open:
  Serial.begin(9600);
  /*while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }*/
  
  Serial.println("Messages Receiver Starting");
  // connection state
  boolean notConnected = true;

  // Start GSM connection
  while(notConnected)
  {
#ifndef PIN_NUMBER
    if(gsmAccess.begin()==GSM_READY)
#else
      if(gsmAccess.begin( PIN_NUMBER )==GSM_READY)    
#endif
        notConnected = false;
      else
      {
        Serial.println("Not connected");
        delay(1000);
      }
  }
  Serial.println("GSM initialized");
  Serial.println("Waiting for messages");
}
//Read incoming message
void readMessage( ){
  char c;
  byte i=0;
  while( c=sms.read() ){
    Serial.print(c);
    if( i < MESSAGE_BUFFER ){
      message[ i ] = c;
    }
    i++;
  }
  message[ i ] = '\0';
  Serial.println();
}
//Parse SMS message
boolean parseMessage( ){
  byte index = 0;
  char c;

  byte length = message_length();
  
  if( !length ){
    Serial.println( "Invalid request" );
  }
  byte encoding = ENCODING_DEFAULT;

  //Check password
  if( message[ 0 ] == PASSWORD_CHAR ){
    Serial.println( "Password is correct" );
  }else{ 
    // Try to decode as HEX
    Serial.println( "Try to decode as HEX" );
    
    if( length < 4 ){
      Serial.println( "Invalid request length < 4" );
    }
    
    char password_hex[4];
    int_to_hexarray( PASSWORD_CHAR, password_hex );
    
    boolean password_match = true;
    Serial.println( "Password");
    for( byte i = 0; i < 4; i++ ){
      Serial.print( password_hex[i] ); 
      if( password_hex[i] != message[i] ){
        password_match = false;
      }
    }
    Serial.println( "" );
    if( !password_match ){
      Serial.println( "Password is incorrect" );
      return false;
    }
    Serial.println( "Password is correct" );
    Serial.println( "Encoding is HEX" );
    encoding = ENCODING_HEX;
    
    if( !decodeHEX_message( length ) ){
      Serial.println( "Error decoding message" );
      return false;
    }
    Serial.println( "ENCODED MESSAGE" );
    int length=0;
    while( message[ length ] != '\0' ){
      Serial.print( message[ length ] );
      length++;
    }
    Serial.println( "" );
    Serial.println( "END OF ENCODED MESSAGE" );
    length = message_length();

  }
  byte device = 0;
  char operation = 't';

  //Read device byte
  c = message[ 1 ];
  if( c >= '0' && c - '0' <= (devices-1) ){
    device = (byte)( c - '0' );
  }
  else{
    Serial.println( "Invalid device" );
    return false;
  }

  //Read operation
  if( length > 2 ){
    c = message[ 2 ];
    if( c != IGNORE_CHAR && c != '\0' ){
      if( c == '0' || c == '1' || c == 't' ){
        operation = c;
      }
      else{
        Serial.println( "Invalid operation" );
        return false;
      }
    }
  }
  
  //Process request
  Serial.println( "Valid request" );

  Serial.print( "Device : ");
  Serial.println( device );

  Serial.print( "Operation : ");
  Serial.println( operation );

  Serial.print( "Device pin : ");
  Serial.println( device_pins[ device ] );

  //do operation I/0
  int OutputValue = LOW;
  if( operation == '0' ){
    OutputValue = LOW;
  }
  else if( operation == '1' ){
    OutputValue = HIGH;
  }
  else if( operation == 't' ){
    if( digitalRead( device_pins[ device ] ) ){
      OutputValue = LOW;
    }else{
      OutputValue = HIGH;
    }
  }
  Serial.print( "Set output : " );
  Serial.print( OutputValue );
  digitalWrite( device_pins[ device ], OutputValue );

  //log

  return true;
}
//Loop function    
void loop() 
{
  // If there are any SMSs available() 
  if (sms.available())
  {
    Serial.println("Message received from:");

    // Get remote number
    sms.remoteNumber(senderNumber, 20);
    Serial.println(senderNumber);
    Serial.println( "message :" );
   
    //Read message
    readMessage();
    Serial.println( "PARSE MESSAGE" );
    parseMessage();

    Serial.println("\nEND OF MESSAGE");

    // Delete message from modem memory
    sms.flush();
    Serial.println("MESSAGE DELETED");
  }
  
  delay(1000);
}

//Returns the HEX encoded value from unsigned int
void int_to_hexarray( unsigned int value, char hex_value[]  ){
  int length = 3;
  /* int to hex */
  do{
    unsigned int power = 1;
    
    for( int i = 0; i < length ; i++){
      power *= 16U;
      
    }
    hex_value[ 3- length ] = (char)hex[ ( unsigned int )( value / power ) ] ;
    value = value % power;
    length--;
  }
  while( length >= 0 );
}

//Get the length of the message
int message_length(){
  int length=0;
  while( message[ length ] != '\0' ){
    length++;
  }
  return length;
}

//Decode the message from the HEX encoding
boolean decodeHEX_message( int length ){
  char new_message[ (int)(length/4)  ]; 
  int value = 0;
  byte index = 0;
  byte new_index = 0;
  
  for( ;; ){
    //if end of message
    if( message[ index ] == '\0' ){
      new_message[ new_index ] = '\0';
      break;
    }
    unsigned int power = 1;
    for( int i = 0; i < 3-(index%4) ; i++){
      power*= 16;
    }
    
    int character_index = -1;
    for( int i = 0 ; i < 16 ; i++){
      if( hex[i] == message[ index ] ){
        character_index = i;
        break;
      }
    }
    //if character not found
    if( character_index == -1 ){
      Serial.println( "ERROR" );
      return false;
    }
    value += character_index*power;
    //End of 4 bytes block
    if( !((index+1)%4)  ){
      new_message[ new_index ] = (char)value;
      new_index++;
      value = 0;
    }
    index++;
  }
  //Make sure stings ends with null character
  if( new_message[ new_index ] != '\0' ){
    new_message[ new_index + 1 ] = '\0';
  }
  
  index = 0;
  //Copy
  do{
    message[ index ] = new_message[ index ];
    index++;
  }while( new_message[ index-1 ] != '\0' );
  
  return true;
}
