#ifndef ANCounter__H___
#define ANCounter__H___

#include <Arduino.h>
#include <EEPROM.h>
#include <HX711_ADC.h>
// #include <Wire.h>

static HX711_ADC *LoadCells = (HX711_ADC*)malloc(sizeof(HX711_ADC)*EEPROM[1]);

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

inline bool get_tare_offset(uint8_t const &loadcell, long tare_container){
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
    long tare_offset;
    for(int i = 0; i < EEPROM[1]; i++){
        LoadCells[i] = HX711_ADC(2*i+5 + 2*(i>5), 2*i+6 + 2*(i>5));
        LoadCells[i].begin();
        LoadCells[i].start(2000, false);        // 2000 ms for stabilization and do not do tare
        if (get_tare_offset(i+1, tare_offset)){
            LoadCells[i].setTareOffset(tare_offset);
        }else{
            return false;
        }
    }
    return true;
}


#endif