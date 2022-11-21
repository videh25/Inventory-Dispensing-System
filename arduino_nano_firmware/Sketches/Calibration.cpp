// Sketch to Caliberate Initialise LoadCells, Calibrate Compartments and save data to EEPROM

#include <Arduino.h>
#include <HX711_ADC.h>
#include <EEPROM.h>

#define MAX_LOADCELLS 3     // Restricted due to Arduino Nano memory
#define MAX_COMPARTMETNS 12

HX711_ADC LoadCells[MAX_LOADCELLS];
long last_LC_readings[MAX_LOADCELLS];
int num_compartments;
int num_loadcells;
uint8_t compa_loadcells[MAX_COMPARTMETNS];
int compa_calibs[MAX_COMPARTMETNS];

void init_attached_loadcells(int num_LCs){
  bool resume_ = true;
  

  bool *loadcell_rdys = (bool*) malloc(sizeof(bool)*num_LCs);
  memset(loadcell_rdys, false, num_LCs*sizeof(bool));
  for(int i = 0; i < num_LCs; i++){
    LoadCells[i].init(2+2*i, 3+2*i);
  }

  resume_ = false;
  Serial.println("Enter t when connections are done");
  while(!resume_) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 't') {
        while (Serial.available() > 0){ inByte = Serial.read();}
        resume_ = true;
      }
    }
  }

  for(int i = 0; i <num_LCs; i++){
    LoadCells[i].begin(128);
  }

  resume_ = false;
  while (!resume_) { //run startup, stabilization and tare, both modules simultaniously
    for(int i = 0; i<num_LCs; i++){
      if (!loadcell_rdys[i]) loadcell_rdys[i] = LoadCells[i].startMultiple(2000., false);
    }

    resume_ = true;
    for(int i = 0; i<num_LCs; i++){
      resume_ &= loadcell_rdys[i];
    }
  }
  free(loadcell_rdys);

  resume_ = false;
  for(int i = 0; i < num_LCs; i++){
    LoadCells[i].tareNoDelay();
    if (LoadCells[i].update() == 2) resume_ = true;
  }
  
  for(int i = 0; i<num_LCs; i++){
    if (LoadCells[i].getTareTimeoutFlag() || LoadCells[i].getSignalTimeoutFlag()) {
      Serial.print("Timeout, check HX711 number: "); Serial.print(i+1); Serial.println(" wiring and pin designations");
      while (1);
    }
  }

  for(int i  = 0; i < num_LCs; i++){
    LoadCells[i].setTareOffset(0);
    LoadCells[i].setCalFactor(1.0);
  }
  num_loadcells = num_LCs;
}

void calibrate_attached_compartments(){
  int num_compartments_;
  bool resume_ = true;
  Serial.println("Caliberating compartments.");
  Serial.print("Enter the number of compartments: ");

  resume_ = false;
  while(!resume_) {
    if (Serial.available() > 0) {
      num_compartments_ = Serial.parseInt();
      if (num_compartments_ != 0) {
        Serial.println(num_compartments_);
        resume_ = true;
      }
    }
  }

  for(char compa = 'A'; compa < 'A' + num_compartments_; compa++){
    Serial.print("Enter the loadcell of compartment_"); Serial.print(compa); Serial.print(": ");
    resume_ = false;
    int compa_cell_;
    int known_quan_;
    while(!resume_) {
      if (Serial.available() > 0) {
        compa_cell_ = Serial.parseInt();
        if (compa_cell_ != 0) {
          Serial.println(compa_cell_);
          resume_ = true;
        }
      }
    }

    Serial.println("Measuring Compartment Weight");
    LoadCells[compa_cell_-1].refreshDataSet();
    long old_measured_weight = LoadCells[compa_cell_-1].smoothedData();
    Serial.print("Measured a weight of "); Serial.print(old_measured_weight); Serial.print(" when compartment_"); Serial.print(compa); Serial.println(" is empty.");
    Serial.print("Put a known quantity in compartment_"); Serial.print(compa); Serial.print(" and enter the quantity: ");
    resume_ = false;
    while(!resume_) {
      if (Serial.available() > 0) {
        known_quan_ = Serial.parseInt();
        if (known_quan_ != 0) {
          Serial.println(known_quan_);
          resume_ = true;
        }
      }
    }
    
    Serial.println("Measuring Compartment Weight");
    LoadCells[compa_cell_-1].refreshDataSet();
    long current_measured_weight = LoadCells[compa_cell_-1].smoothedData();
    long calib_weight_ = (current_measured_weight - old_measured_weight)/float(known_quan_);
    Serial.print("Compartment_"); Serial.print(compa); Serial.print(" caliberated to value: "); Serial.println(calib_weight_);

    num_compartments = num_compartments_;
    compa_calibs[compa - 'A'] = calib_weight_;
    compa_loadcells[compa - 'A'] = compa_cell_;
    last_LC_readings[compa_cell_ - 1] = current_measured_weight;
  }
}

void save_to_EEPROM(){
  EEPROM.put(0, uint8_t(num_compartments));
  EEPROM.put(1, uint8_t(num_loadcells));
  for(int i = 0; i < num_compartments; i++){
    EEPROM.put(2+4*i, compa_calibs[i]);
    EEPROM.put(82 + i, compa_loadcells[i]);
  }
}

// void read_from_EEPROM(){
//   Serial.print("Number of Compartments: "); Serial.println(EEPROM[0]);
//   Serial.print("Number of Loadcells: "); Serial.println(EEPROM[1]);
//   for(int i = 0; i < EEPROM[0]; i++){
//     int calib_;
//     EEPROM.get(2+4*i, calib_);
//     Serial.print("Compartment "); Serial.print(('A' + i)); Serial.print(" is connected to Loadcell"); Serial.println(EEPROM[82+i]);
//     Serial.print("Caliberation Value for compartment "); Serial.print(char('A' + i)); Serial.print(": "); Serial.println(calib_);
//   }
// }

void setup() {
  Serial.begin(57600);
  Serial.println("Starting...");
  Serial.println("Press c to calibrate, t to test");

  bool resume_ = false;
  char inByte;
  while(!resume_) {
    if (Serial.available() > 0) {
      inByte = Serial.read();
      if ((inByte == 'c') || (inByte == 't')) {
        while (Serial.available() > 0){ inByte = Serial.read();}
        resume_ = true;
      }
    }
  }

  if (inByte == 'c'){
    Serial.println("***");
    Serial.println("Initialising Loadcells:");
    Serial.println("Mount containers on loadcells and connect");
    Serial.println("Empty the containers");
    Serial.print("Enter the number of loadcells connected: ");
    int num_loadcells_;
    resume_ = false;
    while(!resume_) {
      if (Serial.available() > 0) {
        num_loadcells_ = Serial.parseInt();
        if (num_loadcells_ != 0) {
          if(num_loadcells_ > MAX_COMPARTMETNS){
            Serial.println("Too many loadcells");
            while(1);
          }
          Serial.println(num_loadcells_);
          resume_ = true;
        }
      }
    }
    init_attached_loadcells(num_loadcells_);
    calibrate_attached_compartments();
    save_to_EEPROM();
  }else{
    init_attached_loadcells(EEPROM[1]);
    for(int i = 0; i < EEPROM[1]; i++){
      Serial.print("Measuring Initial Weight on LoadCell "); Serial.println(i+1);
      LoadCells[i].refreshDataSet();
      last_LC_readings[i] = LoadCells->smoothedData();
    }
  }

  Serial.println("Starting test");
  Serial.println();
  for(int i = 0; i < EEPROM[1]; i++){
    LoadCells[i].powerDown();
  }
  // read_from_EEPROM();
}

void loop() {
  bool resume_ = false;
  Serial.println("Enter the compartment to measure the quantity change: ");
  char inByte;
  while(!resume_) {
    if (Serial.available() > 0) {
      inByte = Serial.read();
      if ((inByte - 'A' >= 0) && (inByte - 'A' < EEPROM[0])) {
        while (Serial.available() > 0){ inByte = Serial.read();}
        resume_ = true;
      }
    }
  }
  LoadCells[EEPROM[82+inByte-'A'] - 1].powerUp();
  Serial.println("Measuring Compartment Weight");
  LoadCells[EEPROM[82+inByte-'A'] - 1].refreshDataSet();
  long current_reading = LoadCells[EEPROM[82+inByte-'A'] - 1].smoothedData();
  int calibration;
  EEPROM.get(2+4*(inByte-'A'), calibration);
  LoadCells[EEPROM[82+inByte-'A'] - 1].powerDown();
  Serial.print("Current Quantity Change: "); Serial.println(round(float(current_reading - last_LC_readings[EEPROM[82+inByte-'A'] - 1])/float(calibration)));
  last_LC_readings[EEPROM[82+inByte-'A'] - 1] = current_reading;
  if(LoadCells[EEPROM[82+inByte-'A'] - 1].DataOutOfRange()) Serial.println("LOADCELL DATA OUT OF RANGE.");

}