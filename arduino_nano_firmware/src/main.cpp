#include <Arduino.h>
#include <HX711_ADC.h>

HX711_ADC* LoadCells = (HX711_ADC*) malloc(sizeof(HX711_ADC));
int num_compartments;
int num_loadcells;
int compa_loadcells[12];
int compa_calibs[12];

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
  HX711_ADC* dummy_LoadCells = LoadCells;
  LoadCells = (HX711_ADC*)malloc(sizeof(HX711_ADC)*num_loadcells_);
  free(dummy_LoadCells);
  for(int i = 0; i < num_loadcells_; i++){
    LoadCells[i] = HX711_ADC(4+2*i, 5+2*i);
  }

  resume_ = false;
  Serial.println("Enter t to start tare");
  while(!resume_) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 't') {
        while (Serial.available() > 0){ inByte = Serial.read();}
        resume_ = true;
      }
    }
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
    LoadCells[i].update();
  }
  
  for(int i = 0; i<num_loadcells_; i++){
    if (LoadCells[i].getTareTimeoutFlag() || LoadCells[i].getSignalTimeoutFlag()) {
      Serial.print("Timeout, check MCU>HX711 number: "); Serial.print(i+1); Serial.println(" wiring and pin designations");
      while (1);
    }
  }

  // for(int i = 0; i < num_loadcells_; i++){
  //   LoadCells[i].start(2000, true);
  //   if (LoadCells[i].getTareTimeoutFlag() || LoadCells[i].getSignalTimeoutFlag()) {
  //     Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
  //     while (1);
  //   }
  // }

  for(int i  = 0; i < num_loadcells_; i++){
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

    Serial.println("here0");
    LoadCells[compa_cell_-1].refreshDataSet();
    Serial.println("here1");
    float old_measured_weight = LoadCells[compa_cell_-1].getData();

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
    
    Serial.println("Caliberating Item Weight... ");
    delay(1500);
    Serial.println("Caliberating Item Weight... ");
    delay(1500);
    Serial.println("Caliberating Item Weight... ");
    delay(1500);
    LoadCells[compa_cell_-1].refreshDataSet();
    float current_measured_weight = LoadCells[compa_cell_-1].getData();
    float calib_weight_ = (current_measured_weight - old_measured_weight)/known_quan_;
    Serial.print("Compartment_"); Serial.print(compa); Serial.print(" caliberated to value: "); Serial.println(calib_weight_);

    compa_calibs[compa - 'A'] = calib_weight_;
    compa_loadcells[compa - 'A'] = compa_cell_;
  }
}

void setup() {
  Serial.begin(57600); delay(10);
  Serial.println();
  Serial.println("Starting...");

  init_attached_loadcells();
  calibrate_attached_compartments();
  // LoadCell.begin();
  // //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
  // unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  // boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  // LoadCell.start(stabilizingtime, _tare);
  // if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
  //   Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
  //   while (1);
  // }
  // else {
  //   LoadCell.setCalFactor(1.0); // user set calibration value (float), initial value 1.0 may be used for this sketch
  //   Serial.println("Startup is complete");
  // }
  // while (!LoadCell.update());
  // calibrate(); //start calibration procedure
  Serial.println("Thats all folks!");
}

void loop() {
  // put your main code here, to run repeatedly:
}