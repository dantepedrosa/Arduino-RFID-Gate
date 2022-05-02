#include <MFRC522.h> 
#include <SPI.h>
#include <EEPROM.h>

// Pin related defines
#define SS_PIN      10
#define RST_PIN     9
#define BUZZER_PIN  11
#define BUTTON_PIN  12
#define LED_PIN     13
#define GATE_PIN    14

// Master key related defines
#define MASTER_KEY_UID_1    0xFF
#define MASTER_KEY_UID_2    0xFF
#define MASTER_KEY_UID_3    0xFF
#define MASTER_KEY_UID_4    0xFF

// Signal codes
#define SAVE_SUCCESS_SIGNAL     0x00
#define SAVE_ERROR_SIGNAL       0x01
#define READ_ERROR_SIGNAL       0x02
#define ABOUT_TO_BLOCK_SIGNAL   0x03
#define SYSTEM_RESETED_SIGNAL   0x04
#define SYSTEM_BLOCKED_SIGNAL   0x06
#define BUTTON_15S_SIGNAL       0x05

// Error codes
#define MEMORY_WRITE_ERROR      0x00

// Time related defines, in ms
#define GATE_PIN_DURATION   750
#define TIME_TO_SAVE_ACTION 10E3
#define RESET_DURATION      15E3
#define PRESS_DURATION      5E3

// EEPROM related defines
// {used/free, uid_1, uid_2, uid_3, uid_4}
#define MASTER_TAG_ADD      0
#define SYSTEM_BLOCK_ADD    1
#define MAX_TAG_NUMBER      10
#define START_ADDRESS       10
#define END_ADDRESS         ((MAX_TAG_NUMBER * 5) + START_ADDRESS)
#define ADDRESS_IN_USE      0x01
#define ADDRESS_FREE        0x00


//----------------------------------------------------------
void openGate();            //V Opens the gate
void outputSignal(char);    //V Outputs a buzzer and/or led signal 

bool verifyTag();               //V Check if is a supported tag
bool isMasterTag(byte*, char*, bool=false);  //V Check if tag is the master key
bool isTagValid(byte*);         //V Check if tag is a valid key
void validateTag(byte*);        //V Move unkown tag to valid keys list
void deleteTag(byte*);          //V Remove tag from valid tags list 

void systemBlock(bool*);         // Block the system
void systemUnblock(bool*);       // Unblock the system
void systemReset();         // Reset the system, deleting all tags
void generateError(char);       // Leaves the system in a error state

void pinsSetup();                   //V Configure pins as input/output
bool checkActionSaved();            //V Check if master key is detected to save command
void writeEEPROM(int, char, bool=false);   //V Check if value was stored in EEPROM
unsigned long checkPressDuration();

MFRC522 rfid(SS_PIN, RST_PIN);


//---------------------------------------------------

void setup() {

    char master_key_count = 0;
    char invalid_tag_count = 0;

    bool system_blocked = false;
    byte tag_uid[4];

    pinsSetup();

    while(1) {

        if (system_blocked)
    		return;

        if (!verifyTag())
            return;
        
        tag_uid[0] = rfid.uid.uidByte[0];
        tag_uid[1] = rfid.uid.uidByte[1];
        tag_uid[2] = rfid.uid.uidByte[2];
        tag_uid[3] = rfid.uid.uidByte[3];
        
        // If master key is detected
        if (isMasterTag(tag_uid, &master_key_count)) {
            
            // If master key has been detected 3 times
            if (master_key_count == 3) {
                outputSignal(ABOUT_TO_BLOCK_SIGNAL);
                return;
            }
            // If master key is detected 4 times, the system is blocked
            else if (master_key_count > 3) {
                systemBlock(&system_blocked);
                return;
            }
        }

        if (digitalRead(BUTTON_PIN) && (master_key_count > 0)) {
            
            unsigned long button_press_duration = checkPressDuration();

            // There will be a press to reset 
            if (button_press_duration < PRESS_DURATION)
                master_key_count = 0;

            else if ((button_press_duration >= RESET_DURATION) && (master_key_count == 1)) 
                systemUnblock(&system_blocked); 
        }

        // If valid key is detected
        if (isTagValid(tag_uid)) {

            // If master key has been detected before, key will be deleted from database
            if (master_key_count == 1)
                if (checkActionSaved())
                    deleteTag(tag_uid);
            // Otherwise, the gate will open
            openGate();
            return;
        }

        // Since if it's not valid or invalid, its unknown
        if (master_key_count == 1) {
            validateTag(tag_uid);
            return;
        }
        
        // Since it's not valid or master tag, it's unknown        
        invalid_tag_count++;
        switch (invalid_tag_count)
        {
        case 2:
            outputSignal(ABOUT_TO_BLOCK_SIGNAL);
            break;

        case 3:
            // One of these should reset the invalid count (redundaant, since no card will work) 
            systemReset();
            systemBlock(&system_blocked);
            outputSignal(SYSTEM_RESETED_SIGNAL);
            break;
        }
    }
}

void loop() {}




// ---------------------------------------------------------------------------------------


/*************************************************************
        verifyTag() - returns void
*************************************************************/
bool verifyTag()
{

    // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
    if (!rfid.PICC_IsNewCardPresent())
        return false;

    // Verify if the NUID has been readed
    if (!rfid.PICC_ReadCardSerial())
        return false;

    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);

    // Check is the PICC of Classic MIFARE type
    if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
        piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
        piccType != MFRC522::PICC_TYPE_MIFARE_4K)
    {
        outputSignal(READ_ERROR_SIGNAL);
        return false;
    }

    return true;

}

/*************************************************************
        openGate() - returns void
    Outputs a signal the gate pin.
    To be decided how the acrual circuit will be.
*************************************************************/
void openGate() {

    digitalWrite(GATE_PIN, HIGH);
    delay(GATE_PIN_DURATION);
    digitalWrite(GATE_PIN, LOW);

}

/*************************************************************
        outputSignal(char signal_code) - returns void
    Outputs a combination of a visual (LED) signal and a sound
(buzzer) signal.
    The definition of the signals code don't really matter,
as long as they are different.
*************************************************************/
void outputSignal(char signal_code) {

    switch (signal_code)
    {
    case SAVE_SUCCESS_SIGNAL:
        digitalWrite(BUZZER_PIN, HIGH);
        delay(250);
        digitalWrite(BUZZER_PIN, LOW);
        delay(250);
        digitalWrite(BUZZER_PIN, HIGH);
        delay(250);
        digitalWrite(BUZZER_PIN, LOW);
        break;
    
    case SAVE_ERROR_SIGNAL:
        digitalWrite(BUZZER_PIN, HIGH);
        delay(400);
        digitalWrite(BUZZER_PIN, LOW);
        delay(400);
        digitalWrite(BUZZER_PIN, HIGH);
        delay(400);
        digitalWrite(BUZZER_PIN, LOW);
        delay(400);
        digitalWrite(BUZZER_PIN, HIGH);
        delay(400);
        digitalWrite(BUZZER_PIN, LOW);        
        break;

    case READ_ERROR_SIGNAL:
        digitalWrite(BUZZER_PIN, HIGH);
        delay(1000);
        digitalWrite(BUZZER_PIN, LOW);
        break;

    case ABOUT_TO_BLOCK_SIGNAL:
        digitalWrite(BUZZER_PIN, HIGH);
        delay(1000);
        digitalWrite(BUZZER_PIN, LOW);
        break;

    case SYSTEM_RESETED_SIGNAL:
        digitalWrite(BUZZER_PIN, HIGH);
        delay(1000);
        digitalWrite(BUZZER_PIN, LOW);
        break;

    case BUTTON_15S_SIGNAL:
        digitalWrite(BUZZER_PIN, HIGH);
        delay(1000);
        digitalWrite(BUZZER_PIN, LOW);
        break;

    default:
        break;
    }

}

/*************************************************************
        openGate() - returns void
*************************************************************/
bool isMasterTag(byte tag_uid[4], char* master_key_count, bool action_started = false) {
    
    if (MASTER_KEY_UID_1 == tag_uid[0] &&
        MASTER_KEY_UID_2 == tag_uid[1] &&
        MASTER_KEY_UID_3 == tag_uid[2] &&
        MASTER_KEY_UID_4 == tag_uid[3]) {
        
        if (action_started) {
            *master_key_count = 0;
            writeEEPROM(MASTER_TAG_ADD, *master_key_count);
        }
        else {
            *master_key_count++;
            writeEEPROM(MASTER_TAG_ADD, *master_key_count);
        }
        return true;
    }
    else
        return false;

}

/*************************************************************
        isTagValid(byte *) - returns void
*************************************************************/
bool isTagValid(byte tag_uid[4]) {

    for (int address = START_ADDRESS; address < END_ADDRESS; address+= 5) {
        // Check if address there will be a card stored in
        if (EEPROM.read(address) == ADDRESS_IN_USE) {
            // Checking in a cascate way trying to avoid reading EEPROM to much
            if (tag_uid[0] == EEPROM.read(address + 1)){
                if (tag_uid[1] == EEPROM.read(address + 2)){
                    if (tag_uid[2] == EEPROM.read(address + 3)){
                        if (tag_uid[3] == EEPROM.read(address + 4)){
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

/*************************************************************
        validateTag(byte) - returns void
*************************************************************/
void validateTag(byte tag_uid[4]) {

    if (checkActionSaved()) {
        for (int address = START_ADDRESS; address < END_ADDRESS; address += 5) {
            
            if (EEPROM.read(address) == ADDRESS_FREE) {

                writeEEPROM(address, ADDRESS_IN_USE);

                writeEEPROM((address + 1), tag_uid[0], true);
                writeEEPROM((address + 2), tag_uid[1], true);
                writeEEPROM((address + 3), tag_uid[2], true);
                writeEEPROM((address + 4), tag_uid[3], true);

            }
        }
        outputSignal(SAVE_SUCCESS_SIGNAL);
    }
    else 
        outputSignal(SAVE_ERROR_SIGNAL);
}

/*************************************************************
        deleteTag(byte) - returns void
*************************************************************/
void deleteTag(byte tag_uid[4]) {

    if (checkActionSaved()) {
        for (int address = START_ADDRESS; address < END_ADDRESS; address += 5) {
            if (EEPROM.read(address) == ADDRESS_IN_USE) {
                // Checking in a cascate way trying to avoid reading EEPROM to much
                if (tag_uid[0] == EEPROM.read(address + 1)){
                    if (tag_uid[1] == EEPROM.read(address + 2)){
                        if (tag_uid[2] == EEPROM.read(address + 3)){
                            if (tag_uid[3] == EEPROM.read(address + 4)){
                                writeEEPROM(address, ADDRESS_FREE);
                            }
                        }
                    }
                }
            }
        }
        outputSignal(SAVE_SUCCESS_SIGNAL);
    }
    else 
        outputSignal(SAVE_ERROR_SIGNAL);
}

/*************************************************************
        writeEEPROM(int, char) - returns void
*************************************************************/
void writeEEPROM(int address, char number, bool update = false) {
    
    if (update)
        EEPROM.update(address, number);
    else
        EEPROM.write(address, number);

    char count = 0;

    while (EEPROM.read(address) != number) {
        
        EEPROM.write(address, number);
        count++;
        
        if (count == 5) {
            generateError(MEMORY_WRITE_ERROR);
            break;
        }
    }
}

/*************************************************************
        checkActionSaved() - returns bool
*************************************************************/
bool checkActionSaved() {

    unsigned long time_init = millis();
    
    // Espera pela master_tag para confirmar a ação por 10 segundos
    while ((millis() - time_init) < TIME_TO_SAVE_ACTION) {

        if (!verifyTag())
            continue;
        
        if (isMasterTag(rfid.uid.uidByte, 0, true)) {
            return true;
        }
    }
    return false;
}

/*************************************************************
        pinsSetup() - returns bool
*************************************************************/
void pinsSetup() {

    pinMode(SS_PIN, OUTPUT);
    pinMode(RST_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BUTTON_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(GATE_PIN, OUTPUT);

}


void systemBlock(bool *system_blocked){
    
    *system_blocked = true;
    writeEEPROM(SYSTEM_BLOCK_ADD, ADDRESS_IN_USE);
    outputSignal(SYSTEM_BLOCKED_SIGNAL);

}

void systemUnblock(bool* system_blocked) {

    *system_blocked = false;
    writeEEPROM(SYSTEM_BLOCK_ADD, ADDRESS_IN_USE);

}


void systemReset() {
					
    for (int address = START_ADDRESS; address < END_ADDRESS; address += 5)
        writeEEPROM(address, 0x00, true);
    
    outputSignal(SYSTEM_RESETED_SIGNAL);
}


void generateError(char error) {
    while(error) {}
}


unsigned long checkPressDuration() {

    delay(50);
    
    // Get duration of button press. System will be on halt during button press
    unsigned long timer_begin = millis(); 
    while (digitalRead(BUTTON_PIN) && !(timer_begin + millis() < 16E3)){}
    
    return(millis() - timer_begin);
    
}