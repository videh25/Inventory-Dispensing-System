#ifndef ANCounter__H___
#define ANCounter__H___

#include <Arduino.h>
#include <EEPROM.h>
#include <HX711_ADC.h>
// #include <LowPower.h>
// #include <Wire.h>

static HX711_ADC *LoadCells = (HX711_ADC*)malloc(sizeof(HX711_ADC)*EEPROM[1]);
bool DEBUG = true;

inline bool is_valid_compartment(const unsigned char &compartment){
    
    return ('A' <= compartment) && (compartment <= 'A' + EEPROM[0] - 1);
}

inline bool is_valid_loadcell(const uint8_t &load_cell){
    return (1 <= load_cell) && (load_cell <= EEPROM[1]);
}

inline bool get_caliberation_value(const unsigned char &compartment, float &caliberation_value_container){
    if (!is_valid_compartment(compartment)){
        return false;
    }

    EEPROM.get((compartment - 'A')*4 + 2, caliberation_value_container);
    return true;
}

inline bool get_tare_offset(const uint8_t &loadcell, long tare_container){
    if (!is_valid_loadcell(loadcell)){
        return false;
    }

    EEPROM.get((loadcell - 1)*4 + 50, tare_container);
    return true;
}

inline bool get_loadcell(const unsigned char &compartment, uint8_t &loadcell_container){
    if (!is_valid_compartment(compartment)){
        return false;
    }

    EEPROM.get((compartment - 'A') + 82, loadcell_container);
    return true;
}

inline bool eeprom_is_ok(){
    bool check = true;

    check = check && (1 <= EEPROM[0]) && (EEPROM[0] <= 12); // Compartment number follows bound
    check = check && (1 <= EEPROM[1]) && (EEPROM[1] <= 8);  // LoadCell number follows bound
    check = check && (EEPROM[0] + EEPROM[1]*2 <= 16);       // Pin bound followed

    // All non-specified values are zero: Calibration Values
    for(uint8_t i = EEPROM[0]*4 + 2; i < 50; i++){
        check = check  && (!EEPROM[i]);
    }

    // All non-specified values are zero: Tare Values
    for(uint8_t i = EEPROM[1]*4 + 50; i < 82; i++){
        check = check  && (!EEPROM[i]);
    }

    // All non-specified values are zero: Load Cell Values and the appendix
    for(uint8_t i = EEPROM[0] + 82; i < 512; i++){
        check = check  && (!EEPROM[i]);
    }

    return check;
}

inline bool init_loadcells(){
    // Begin Load Cells
    for(int i = 0; i < EEPROM[1]; i++){
        LoadCells[i] = HX711_ADC(2*i+5 + 2*(i>5), 2*i+6 + 2*(i>5));
        LoadCells[i].begin();
    }

    bool *loadcell_rdys = (bool*)malloc(sizeof(bool)*EEPROM[1]);
    memset(loadcell_rdys, false, EEPROM[1]*sizeof(bool));

    bool all_done = false;

    // Start the LoadCells
    while(!all_done){
        for (int i = 0; i < EEPROM[1]; i++){
            if(!loadcell_rdys[i]) loadcell_rdys[i] = LoadCells[i].startMultiple(2000, false);
        }

        all_done = true;
        for (int i = 0; i < EEPROM[1]; i++){
            all_done = all_done && loadcell_rdys[i];
        }
    }
    free(loadcell_rdys);

    // Set Tare Offsets
    for(int i = 0; i < EEPROM[1]; i++){
        long tare_offset_;
        get_tare_offset(i+1, tare_offset_);
        LoadCells[i].setTareOffset(tare_offset_);
    }

    // Set Calibration Values
    // 1.0 for each load cell, quantity calc will be performed later
    for(int i = 0; i < EEPROM[1]; i++){
        LoadCells[i].setCalFactor(1.0);
    }
    
    if(DEBUG) Serial.println("LoadCells Initialised Successfully!");
    return true;
}

inline bool init_limit_switches(){
    for(int i = 0; i < EEPROM[0]; i++){
        pinMode(2*EEPROM[1]+5 + 2*(2*EEPROM[1]+i>11) + i, INPUT_PULLUP);
    }

    if(DEBUG) Serial.println("Limit Switches Initialised.");
    return true;
}

inline bool sleep_(){
    // Include Arduino low-power sleep when tested with i2c
    for(int i = 0; i < EEPROM[1]; i++){
        LoadCells[i].powerDown();
    }

    return true;
}

inline bool wake_up_(){
    // Include Arduino low-power sleep when tested with i2c
    for(int i = 0; i < EEPROM[1]; i++){
        LoadCells[i].powerUp();
    }

    return true;
}

// Blocks till the quantity is received
inline bool get_quantity_once(const unsigned char &compartment, float &qunaity_container_){
    static bool newDataReady = false;
    uint8_t loadcell_;
    if(!get_loadcell(compartment, loadcell_)){
        if(DEBUG) Serial.println("Compartment Value Not Proper");
        return false;
    }

    while(!(LoadCells[loadcell_-1].update())){
        //block
    }

    float weight = LoadCells[loadcell_-1].getData();
    float unit_weight;
    if(!get_caliberation_value(compartment, unit_weight)){
        if(DEBUG) Serial.println("Invalid compartment char");
        return false;
    } 
    qunaity_container_ = weight/unit_weight;
    if (DEBUG) {
        Serial.print("Calculated the quantity in compartment "); Serial.print(compartment); Serial.print(" to be "); Serial.println(qunaity_container_);
    }

    return true;

}

// Blocks till the qunatity readings are stabilised
inline bool get_stable_quantity(const unsigned char &compartment, int &qunaity_sotrage_){
    int window = 5;
    float permissible_err = 0.05;
    float prev_readings[window];

    int i = 0;
    float error = 1;

    while(error>permissible_err){
        i++;
        get_quantity_once(compartment, prev_readings[i%window]);

        float maxum = 0, minum = 0, meanum = 0;
        for (int i = 0; i < window; i++){
            if (i != 0){
                maxum = max(prev_readings[i], prev_readings[i-1]);
                minum = min(prev_readings[i], prev_readings[i-1]);
            }
            meanum += (float)prev_readings[i]/(float)window;
        }

        error = (maxum - minum)/meanum;
        qunaity_sotrage_ = round(meanum);
    }

    return true;
}

#endif