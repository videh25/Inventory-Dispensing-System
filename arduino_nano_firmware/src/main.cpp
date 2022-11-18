#include <Arduino.h>
#include <HX711_ADC.h>

HX711_ADC LoadCells[8];
int num_compartments;
int num_loadcells;
int compa_loadcells[12];
int compa_calibs[12];
long last_reading;

void init_attached_loadcells(){
  bool resume_ = true;
  Serial.println("***");
  Serial.println("Initialising Loadcells:");
  Serial.println("Mount containers on loadcells and connect them to respective pins");
  Serial.println("Remove any materials if placed in the container.");
  Serial.print("Enter the number of loadcells connected: ");
  int num_loadcells_;
  resume_ = false;
  while(!resume_) {
    if (Serial.available() > 0) {
      num_loadcells_ = Serial.parseInt();
      if (num_loadcells_ != 0) {
        Serial.println(num_loadcells_);
        resume_ = true;
      }
    }
  }

  bool *loadcell_rdys = (bool*) malloc(sizeof(bool)*num_loadcells_);
  memset(loadcell_rdys, false, num_loadcells_*sizeof(bool));
  for(int i = 0; i < num_loadcells_; i++){
    LoadCells[i].init(2+2*i, 3+2*i);
  }

  resume_ = false;
  Serial.println("Enter t when all connections are done");
  while(!resume_) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 't') {
        while (Serial.available() > 0){ inByte = Serial.read();}
        resume_ = true;
      }
    }
  }

  for(int i = 0; i <num_loadcells_; i++){
    LoadCells[i].begin(128);
  }

  resume_ = false;
  while (!resume_) { //run startup, stabilization and tare, both modules simultaniously
    for(int i = 0; i<num_loadcells_; i++){
      if (!loadcell_rdys[i]) loadcell_rdys[i] = LoadCells[i].startMultiple(2000., false);
    }

    resume_ = true;
    for(int i = 0; i<num_loadcells_; i++){
      resume_ &= loadcell_rdys[i];
    }
  }

  resume_ = false;
  for(int i = 0; i < num_loadcells_; i++){
    LoadCells[i].tareNoDelay();
    if (LoadCells[i].update() == 2) resume_ = true;
  }
  
  for(int i = 0; i<num_loadcells_; i++){
    if (LoadCells[i].getTareTimeoutFlag() || LoadCells[i].getSignalTimeoutFlag()) {
      Serial.print("Timeout, check MCU>HX711 number: "); Serial.print(i+1); Serial.println(" wiring and pin designations");
      while (1);
    }
  }

  for(int i  = 0; i < num_loadcells_; i++){
    LoadCells[i].setTareOffset(0);
    LoadCells[i].setCalFactor(1.0);
  }
  num_loadcells = num_loadcells_;
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
    
    LoadCells[compa_cell_-1].refreshDataSet();
    long current_measured_weight = LoadCells[compa_cell_-1].smoothedData();
    long calib_weight_ = (current_measured_weight - old_measured_weight)/float(known_quan_);
    Serial.print("Compartment_"); Serial.print(compa); Serial.print(" caliberated to value: "); Serial.println(calib_weight_);

    compa_calibs[compa - 'A'] = calib_weight_;
    compa_loadcells[compa - 'A'] = compa_cell_;
    last_reading = current_measured_weight;
  }
}

void setup() {
  Serial.begin(57600); delay(10);
  Serial.println();
  Serial.println("Starting...");

  init_attached_loadcells();
  calibrate_attached_compartments();
  Serial.println("Thats all folks!");
}

void loop() {
  // put your main code here, to run repeatedly:
  for (int i = 0; i < num_loadcells; i++){

    LoadCells[i].refreshDataSet();
    long current_reading = LoadCells[i].smoothedData();
    Serial.print("Current Weight: "); Serial.println(round(float(current_reading - last_reading)/float(compa_calibs[i])));
    last_reading = current_reading;
    if(LoadCells[i].DataOutOfRange()) Serial.println("DATA OUT OF RANGE.");

    bool resume_ = false;
    Serial.println("Enter c after you change the quantity");
    while(!resume_) {
      if (Serial.available() > 0) {
        char inByte = Serial.read();
        if (inByte == 'c') {
          while (Serial.available() > 0){ inByte = Serial.read();}
          resume_ = true;
        }
      }
    }

  }
}