#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>

#define RST_PIN         21          // Configurable, see typical pin layout above
#define SS_PIN          5         // Configurable, see typical pin layout above
#define START_BIT       4

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

byte buffersize = 18;
byte readbackblock[18] = {0};  //Array for reading out a block.
bool readBlock(byte blockNumber, byte arrayAddress[]);
bool writeBlock(byte blockNumber, byte arrayAddress[]);
void read_page(byte a_page, int a_from = 0);
void clear_indexes_page(byte a_index = START_BIT);
void show_data_on_card();
void insert_data();
char print_options_and_wait_for_response();

void setup()
{
  Serial.begin(9600);
  while (!Serial);
  SPI.begin();                                    // Init SPI bus
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
  mfrc522.PCD_Init();                             // Init MFRC522
  delay(4);                                       // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();              // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
}

void loop()
{  
  if(!mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }

  if ( mfrc522.PICC_ReadCardSerial())
  {
    char option_chose = print_options_and_wait_for_response();

    if(option_chose == '1') clear_indexes_page();
    else if(option_chose == '2') show_data_on_card();
    else insert_data();
  }
}

//Read specific block
bool readBlock(byte blockNumber, byte arrayAddress[])
{
  MFRC522::StatusCode status = mfrc522.MIFARE_Read(blockNumber, arrayAddress, &buffersize);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Data read failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }
  return true;
}

//Write specific block
bool writeBlock(byte blockNumber, byte arrayAddress[])
{
  MFRC522::StatusCode status = mfrc522.MIFARE_Ultralight_Write(blockNumber, arrayAddress, 4);
  
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Data write failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }
  return true;
}

void read_page(byte a_page, int a_from)
{
  memset(readbackblock, 0, 18);
  readBlock(a_page, readbackblock);
  for(;a_from < 4; ++a_from){
    if(readbackblock[a_from] > 32 && readbackblock[a_from] < 127){
      Serial.write(readbackblock[a_from]);
    }
  }
}

void clear_indexes_page(byte a_index)
{
  Serial.println("clearing...");
  
  for(; a_index<16; ++a_index){
    byte pages_written_indexes[6] = {'~','~','~','~','~','~'};
    writeBlock(a_index, pages_written_indexes);
  }
}

char print_options_and_wait_for_response()
{
    Serial.println("1. Write new data");
    Serial.println("2. Read data");
    Serial.println("3. keep input");
    while(!Serial.available());
    return Serial.readString().charAt(0);
}

void show_data_on_card()
{
  Serial.println("reading...");
  for (byte i = START_BIT;i < 16; ++i){
    read_page(i);
  }
  Serial.println();
}

void insert_data()
{
  Serial.println("Enter String (16 letters max):");
  while(!Serial.available());
  String user_input = Serial.readStringUntil('\n');
  user_input = user_input.substring(0,user_input.length());
  while(user_input.length() % 4 != 0) user_input += '~';
  byte i = 0;
  byte page_index = START_BIT;
  byte max_i = user_input.length();
  unsigned char *to_write = (unsigned char *)malloc(user_input.length());

  user_input.getBytes(to_write,user_input.length(),0);
  do{
    if(writeBlock(page_index, &to_write[i])){
      i+=4;
    }
    ++page_index;
  }while(page_index < 16 && i < max_i);
  free(to_write);
}