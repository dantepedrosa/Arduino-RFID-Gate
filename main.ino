#include <MFRC522.h> 
#include <SPI.h>
#include <EEPROM.h>

//------DEFINES----------------------------------------------

// Pin related defines
#define SS_PIN      10
#define RST_PIN     9
#define BUZZER_PIN  11
#define BUTTON_PIN  12
#define LED_PIN     13
#define GATE_PIN    14

//------FUNCTION-PROTOTYPES-----------------------------------

void pinsSetup();                   //V Configure pins as input/output -- MAIN



// Initialize RFID object
MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
    
    char master_key_count = 0;      // Counts how many times Master Key was detected without reseting
    char invalid_tag_count = 0;     // Counts how many invalid keys were detected

    bool system_blocked = false;    // This variable blocks the system if set to true
    byte tag_uid[4];        // Array to copy and store RFID bytes to manipulate

    pinsSetup();        // Sets up all pins

    while(1) {

        if (system_blocked)
    		return;

        if (!verifyTag())
            return;

        digitalWrite(BUZZER_PIN, true);
        digitalWrite(LED_PIN, true);
        delay(1000);
        digitalWrite(BUZZER_PIN, false);
        digitalWrite(LED_PIN, false);

    }
}


void loop() {}      // Loop is done in setup


//------------------------------------------------------------
//------FUNCTIONS---------------------------------------------
//------------------------------------------------------------


/*************************************************************
        pinsSetup() - returns bool
    Configures pins with specified I/O and initial state
*************************************************************/
void pinsSetup() {

    pinMode(SS_PIN, OUTPUT);
    pinMode(RST_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(GATE_PIN, OUTPUT);
    digitalWrite(GATE_PIN, false);

    // Init procedure to check if led and buzzer are OK

    delay(200);
    digitalWrite(BUZZER_PIN, true);
    digitalWrite(LED_PIN, true);
    delay(1000);
    digitalWrite(BUZZER_PIN, false);
    digitalWrite(LED_PIN, false);

}