// The sketch that runs on Arduino Nanos

#include <Arduino.h>
#include <HX711_ADC.h>
#include <EEPROM.h>
#include <Wire.h>
#define MAX_LOADCELLS 3     // Restricted due to Arduino Nano memory
#define MAX_COMPARTMENTS 12

uint8_t i2c_add = 4;

HX711_ADC LoadCells[MAX_LOADCELLS];
long last_readings[MAX_LOADCELLS];
int change_this_time[MAX_COMPARTMENTS];
char count_buff[3*MAX_COMPARTMENTS];

char latest_Byte = 'Z';      // Using 'Z' as a placeholder --> No msg received
bool there_is_error = false;
bool dispensing_on = false;
bool openned_this_time[MAX_COMPARTMENTS];

void init_attached_loadcells(){
  bool resume_ = true;
  

  bool *loadcell_rdys = (bool*) malloc(sizeof(bool)*EEPROM[1]);
  memset(loadcell_rdys, false, EEPROM[1]*sizeof(bool));
  for(int i = 0; i < EEPROM[1]; i++){
    LoadCells[i].init(2+2*i, 3+2*i);
  }

  for(int i = 0; i <EEPROM[1]; i++){
    LoadCells[i].begin(128);
  }

  resume_ = false;
  while (!resume_) { //run startup, stabilization and tare, both modules simultaniously
    for(int i = 0; i<EEPROM[1]; i++){
      if (!loadcell_rdys[i]) loadcell_rdys[i] = LoadCells[i].startMultiple(2000., false);
    }

    resume_ = true;
    for(int i = 0; i<EEPROM[1]; i++){
      resume_ &= loadcell_rdys[i];
    }
  }
  free(loadcell_rdys);

  resume_ = false;
  for(int i = 0; i < EEPROM[1]; i++){
    LoadCells[i].tareNoDelay();
    if (LoadCells[i].update() == 2) resume_ = true;
  }
  
  for(int i = 0; i<EEPROM[1]; i++){
    if (LoadCells[i].getTareTimeoutFlag() || LoadCells[i].getSignalTimeoutFlag()) {
      there_is_error = true;
    }
  }

  for(int i  = 0; i < EEPROM[1]; i++){
    LoadCells[i].setTareOffset(0);
    LoadCells[i].setCalFactor(1.0);
  }
}

void init_else(){
  // Init the compartment door pins 
  // Init the count buffer
  memset(count_buff, 0, sizeof(char)*MAX_COMPARTMENTS*3);
  for(char compa = 'A'; compa - 'A' < EEPROM[0]; compa++){
    pinMode(2 + 2*EEPROM[1] + compa - 'A', INPUT_PULLUP);
    count_buff[3*(compa-'A')] = compa;
  }
}

void brace_up(){
  Serial.println("Bracing Up!");
  Serial.flush();
  for(int i = 0; i < EEPROM[1]; i++){
    Serial.print("i: "); Serial.println(i); Serial.flush();
    // LoadCells[i].powerUp();
    LoadCells[i].refreshDataSet();
    last_readings[i] = LoadCells[i].smoothedData();
    Serial.println(last_readings[i]);
    Serial.flush();
  }
  Serial.println("Braced Up!");
  Serial.flush();
}

void check_errors(){
  bool no_errors = true;
  no_errors = (EEPROM[1] != 0) && no_errors;
  for(int i = 0; i < EEPROM[1]; i++){
    LoadCells[i].tareNoDelay();
  }
  
  for(int i = 0; i<EEPROM[1]; i++){
    if (LoadCells[i].getTareTimeoutFlag() || LoadCells[i].getSignalTimeoutFlag()) {
      no_errors = false;
      Serial.println("There is Error.");
      break;
    }
  }

  there_is_error =  !no_errors;
}

void dispensing(){
  Serial.println("Dispensing Mode On!");
  Serial.flush();
  while(latest_Byte != 'C'){
    for (int i = 0; i < EEPROM[0]; i++){
      openned_this_time[i] = openned_this_time[i] || (LOW == digitalRead(2 + 2*EEPROM[1] + i));
      delay(500);
      Wire.flush();
      // Serial.print("In the loop"); Serial.println(i);
    }
  }
  Serial.println("Dispensing Mode Off!");
  Serial.flush();
}

void relax(){
  Serial.println("Relaxing with a sigh...");
  dispensing_on = false;
}

void fromMaster(int howMany)
{
  // Serial.print("HowMany: "); Serial.println(howMany);
  Serial.println("fromMaster");
  latest_Byte = Wire.read();
  delay(10);
}

void toMaster() {
  if(latest_Byte == '0'){
    if(there_is_error){
      Wire.write('1');
    }else{
      Wire.write('0');
    }
  }else if(latest_Byte == 'D'){
    char num_buff[2];
    int num = EEPROM[0];
    sprintf(num_buff, "%02d", num);
    Wire.write(num_buff);
  }else if(latest_Byte == 'A'){
    if(there_is_error){
      Wire.write('1');
    }else{
      brace_up();
      Wire.write('0');
      Wire.flush();
      dispensing_on = true;
    }
  }else if(latest_Byte == 'C'){
    relax();
    if (there_is_error){
      Wire.write('1');
    }else{
      Wire.write('0');
    }
  }else if(latest_Byte == 'B'){
    bool checked_this_LC[MAX_LOADCELLS];
    for (int i = 0; i < MAX_LOADCELLS; i++){
      checked_this_LC[i] = false;
    }
    bool duplicate_LC = false;
    for(char compa = 'A'; compa - 'A' < EEPROM[0]; compa++){
      if(openned_this_time[compa - 'A']){
        openned_this_time[compa - 'A'] = false;
        if (checked_this_LC[EEPROM[82+compa-'A'] - 1]){
          duplicate_LC = true;
          break;
        }else{
          LoadCells[EEPROM[82+compa-'A'] - 1].refreshDataSet();
          long current_reading = LoadCells[EEPROM[82+compa-'A'] - 1].smoothedData();
          int calibration;
          EEPROM.get(2+4*(compa-'A'), calibration);
          // LoadCells[EEPROM[82+compa-'A'] - 1].powerDown();
          checked_this_LC[EEPROM[82+compa-'A'] - 1] = true;
          int change = (round(float(current_reading - last_readings[EEPROM[82+compa-'A'] - 1])/float(calibration)));
          Serial.print("Change of "); Serial.print(change); Serial.print(" detected on Compartment_"); Serial.println(compa);
          last_readings[EEPROM[82+compa-'A'] - 1] = current_reading;
          count_buff[3*(compa-'A') + 1] = char('0' + int(change + 50)/10);
          count_buff[3*(compa-'A') + 2] = char('0' + int(change + 50)%10);
        }
      }else{
        count_buff[3*(compa-'A') + 1] = '5';
        count_buff[3*(compa-'A') + 2] = '0';
        Serial.print("No door open detected on Compartment_"); Serial.println(compa);
      }
    }
    if(duplicate_LC){
      Serial.println("Two compartments on same load cell accessed!");
      for(char compa = 'A'; compa - 'A' < EEPROM[0]; compa++){
        count_buff[3*(compa-'A') + 1] = '0';
        count_buff[3*(compa-'A') + 2] = '0';
      }
    }
    for(int j = 0; j < 3*EEPROM[0]; j++){
      Wire.write(count_buff[j]);
    }
  }
  latest_Byte == 'Z';
  return;
}

void setup() {
  memset(openned_this_time, false, sizeof(bool)*MAX_COMPARTMENTS);

  Serial.begin(57600);
  Wire.begin(i2c_add);
  Wire.onReceive(fromMaster);
  Wire.onRequest(toMaster);

  init_attached_loadcells();
  init_else();
  Serial.println("here0");
}

void loop() {
  if(dispensing_on){
    dispensing();
  }
}