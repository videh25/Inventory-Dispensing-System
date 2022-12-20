// The sketch that runs on Arduino Nanos

#include <Arduino.h>
#include <HX711_ADC.h>
#include <EEPROM.h>
#include <Wire.h>
#define MAX_LOADCELLS 3     // Restricted due to Arduino Nano memory
#define MAX_COMPARTMENTS 12


uint8_t i2c_add = 8;

HX711_ADC LoadCells[MAX_LOADCELLS];
long last_readings[MAX_LOADCELLS];
int change_this_time[MAX_COMPARTMENTS];
char count_buff[3*MAX_COMPARTMENTS];
int load_cell_dout_pins[MAX_COMPARTMENTS] = {2, 4};
int extra_LC = 1;

char latest_Byte = 'Z';      // Using 'Z' as a placeholder --> No msg received
bool there_is_error = false;
bool dispensing_on = false;
bool bracing_call = false;
bool braced_up = false;
bool calculating_call = false;
bool calculated_change = false;
bool openned_this_time[MAX_COMPARTMENTS];

void init_attached_loadcells(){
  bool resume_ = true;
  

  bool *loadcell_rdys = (bool*) malloc(sizeof(bool)*EEPROM[1]);
  memset(loadcell_rdys, false, EEPROM[1]*sizeof(bool));
  for(int i = 0; i < EEPROM[1]; i++){
    LoadCells[i].init(load_cell_dout_pins[i], load_cell_dout_pins[i]+1);
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
    pinMode(4 + 2*(EEPROM[1]) + compa - 'A', INPUT_PULLUP);
    count_buff[3*(compa-'A')] = compa;
    pinMode(17, OUTPUT);
    digitalWrite(17, HIGH);
  }
}

void brace_up(){
  Serial.println("Bracing Up!");
  Serial.flush();
  for(int i = 0; i < EEPROM[1]; i++){
    Serial.print("Loadcell: "); Serial.println(i); Serial.flush();
    LoadCells[i].powerUp();
    LoadCells[i].refreshDataSet();
    last_readings[i] = LoadCells[i].smoothedData();
    Serial.println(last_readings[i]);
    Serial.flush();
  }
  Serial.println("Braced Up!");
  Serial.flush();
  digitalWrite(17, LOW);
  dispensing_on = true;
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
  while(latest_Byte == 'A'){
    for (int i = 0; i < EEPROM[0]; i++){
      openned_this_time[i] = openned_this_time[i] || (HIGH == digitalRead(4 + 2*(EEPROM[1]+extra_LC) + i));
      delay(500);
      Wire.flush();
      Serial.print("In the loop"); Serial.println(4 + 2*(EEPROM[1]+extra_LC) + i);
    }
  }
  Serial.println("Dispensing Mode Off!");
  Serial.flush();
}

void relax(){
  Serial.println("Relaxing with a sigh...");
  digitalWrite(17, HIGH);
  dispensing_on = false;
}

void fromMaster(int howMany)
{
  // Serial.print("HowMany: "); Serial.println(howMany);
  Serial.println("fromMaster");
  Wire.read();
  byte check_value = Wire.read();
  if (check_value != 255){
    latest_Byte = char(int(check_value));
    Serial.println(latest_Byte);
  } 
  delay(10);
}

void toMaster() {
  Serial.println(latest_Byte);
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
    if(!bracing_call){
      bracing_call = true;
      Wire.write('1');
      Serial.println('1');
      Wire.flush();
    }else if(!braced_up){
      Serial.println('1');
      Wire.write('1');
      Wire.flush();
    }else{
      bracing_call = false;
      braced_up = false;
      Serial.println('0');
      Wire.write('0');
      Wire.flush();
    }
  }else if(latest_Byte == 'C'){
    relax();
    if (there_is_error){
      Wire.write('1');
    }else{
      Wire.write('0');
    }
  }else if(latest_Byte == 'B'){
    for(int j = 0; j < 3*EEPROM[0]; j++){
      Serial.write(count_buff[j]);
      Wire.write(count_buff[j]);
    }
  }else if(latest_Byte == 'E'){
    if(!calculating_call){
      calculating_call = true;
      Wire.write('1');
      Wire.flush();
    }else if(!calculated_change){
      Wire.write('1');
      Wire.flush();
    }else{
      calculating_call = false;
      calculated_change = false;
      Wire.write('0');
      Wire.flush();
    }
  }
  latest_Byte == 'Z';
  return;
}

void calculate_change(){
  for(char compa = 'A'; compa - 'A' < EEPROM[0]; compa++){
    if(openned_this_time[compa - 'A']){
      LoadCells[EEPROM[82+compa-'A'] - 1].refreshDataSet();
      long current_reading = LoadCells[EEPROM[82+compa-'A'] - 1].smoothedData();
      int calibration;
      EEPROM.get(2+4*(compa-'A'), calibration);
      LoadCells[EEPROM[82+compa-'A'] - 1].powerDown();
      int change = (round(float(current_reading - last_readings[EEPROM[82+compa-'A'] - 1])/float(calibration)));
      Serial.print("Change of "); Serial.print(change); Serial.print(" detected on Compartment_"); Serial.println(compa);
      last_readings[EEPROM[82+compa-'A'] - 1] = current_reading;
      count_buff[3*(compa-'A') + 1] = char('0' + int(change + 50)/10);
      count_buff[3*(compa-'A') + 2] = char('0' + int(change + 50)%10);
    }else{
      count_buff[3*(compa-'A') + 1] = '5';
      count_buff[3*(compa-'A') + 2] = '0';
      Serial.print("compa num: "); Serial.println(compa-'A');
      Serial.print("No door open detected on Compartment_"); Serial.println(compa);
    }
  }
}

void setup() {
  memset(openned_this_time, false, sizeof(bool)*MAX_COMPARTMENTS);

  Serial.begin(57600);
  Wire.begin(i2c_add);
  Wire.onReceive(fromMaster);
  Wire.onRequest(toMaster);
  Serial.print("Loadcells: "); Serial.println(EEPROM[1]);
  Serial.print("Comaprtments: "); Serial.println(EEPROM[0]);

  init_attached_loadcells();
  init_else();
  Serial.println("here0");
}

void loop() {
  if(dispensing_on){
    dispensing();
  }
  if (bracing_call){
    brace_up();
    braced_up = true; 
  }
  if (calculating_call){
    calculate_change();
    calculated_change = true;
  }
}