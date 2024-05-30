#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <vector>
#define RST_PIN         21          // Configurable, see typical pin layout above
#define SS_PIN          5         // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

byte data[4][6] = {
  {'_','_','_','_','\n','\n'},
  {'_','h','e','l','\n','\n'},
  {'l','o','_','w','\n','\n'},
  {'o','r','l','d','\n','\n'}
};

byte pages_written[15] = {0};
/*
byte data2[5] = {'_','h','e','l','\n'};
byte data3[5] = {'l','o','_','w','\n'};
byte data4[5] = {'o','r','l','d','\n'};
*/
byte buffersize = 18;
byte readbackblock[18] = {0};  //Array for reading out a block.
bool readBlock(byte blockNumber, byte arrayAddress[]);
bool writeBlock(byte blockNumber, byte arrayAddress[]);
void read_page(byte a_page, int a_from = 0);

bool manage_pages_written(String const &a_data, bool a_is_new = false)
{
  byte index = 3;
  memset(pages_written, 0, 15);
  writeBlock(2, &data[0][0]);
  int remains_to_write = a_data.length();
  int ptr_pos = 0;
  while(remains_to_write){
    if(writeBlock(index, (byte*)&(a_data.c_str()[ptr_pos]))){
      ptr_pos += 4;
      remains_to_write -= 4;
      pages_written[index] = 1;
    }
    ++index;
    if(index > 15 && remains_to_write){
      return false;
    }
  }
  return true;
}

void read_pages_written()
{
  for(byte i = 0; i<15; ++i){
    if (pages_written[i] == 1){
      read_page(i);
    }
  }
}

void setup()
{
  Serial.begin(9600);
  while (!Serial);
  SPI.begin();                              // Init SPI bus
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
  mfrc522.PCD_Init();                       // Init MFRC522
  delay(4);                                 // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();        // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
}

std::vector<byte> indexes;
void loop()
{  
  if(!mfrc522.PICC_IsNewCardPresent()){
    return;
  }

  if ( mfrc522.PICC_ReadCardSerial()){

    /*
    Serial.println("Enter data:");
    while(!Serial.available());
    String inputData = Serial.readStringUntil('\n');
    bool result;
    if(inputData.charAt(0) == '!'){
      result = manage_pages_written(inputData.substring(1), true);
    }
    else{
      result = manage_pages_written(inputData);
    }

    if(result){
      read_pages_written();
    }
    */
    Serial.println("1. Write new data");
    Serial.println("2. Read data");
    Serial.println("3. keep input");
    while(!Serial.available());
    String user_input = Serial.readString();
    if(user_input[0] == '1'){
      Serial.println("clearing...");
      indexes.clear();
    }
    else if(user_input[0] == '2'){
      Serial.println("reading...");
      for (byte i = 0;i < indexes.size(); ++i){
        Serial.print("reading page:");
        Serial.println(indexes[i]);
        read_page(indexes[i]);
      }
    }
    Serial.println("Enter page number (2-15):");
    while(!Serial.available());
    byte page = Serial.readString().toInt();
    Serial.println("Enter data:");
    while(!Serial.available());
    byte dataIndex = Serial.readString().toInt();
    if(writeBlock(page, &data[dataIndex][0])){
      indexes.push_back(page);
      Serial.println("success");
    } else {
      Serial.println("fail");
    }
  }
  // Dump debug info about the card; PICC_HaltA() is automatically called
  //mfrc522.PICC_DumpMifareUltralightToSerial();
}

//Read specific block
bool readBlock(byte blockNumber, byte arrayAddress[])
{
  MFRC522::StatusCode status = mfrc522.MIFARE_Read(blockNumber, arrayAddress, &buffersize);//&buffersize is a pointer to the buffersize variable; MIFARE_Read requires a pointer instead of just a number
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
  //writing data to the block
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
    Serial.write(readbackblock[a_from]);
  }
}