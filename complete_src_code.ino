//#include <MFRC522.h> 
//#include <SPI.h>
//#include <EEPROM.h>

#define SS_PIN      10
#define RST_PIN     9
#define BUZZER_PIN  11
#define BUTTON_PIN  12
#define LED_PIN     13
#define GATE_PIN    14

// Signal codes
#define SAVE_SUCCESS_SIGNAL     0x00
#define SAVE_ERROR_SIGNAL       0x01
#define READ_ERROR_SIGNAL       0x02
#define ABOUT_TO_BLOCK_SIGNAL   0x03
#define SYSTEM_RESETED_SIGNAL   0x04
#define BUTTON_15S_SIGNAL       0x05

// Error codes
#define MEMORY_WRITE_ERROR      0x00


// Time related defines, in ms
#define GATE_PIN_DURATION   750
#define TIME_TO_SAVE_ACTION 10E3

// EEPROM related defines
// {used/free, uid_1, uid_2, uid_3, uid_4}
#define MASTER_TAG_ADD  0
#define SYSTEM_BLOCK_ADD 1
#define MAX_TAG_NUMBER  10
#define START_ADRESS 10
#define END_ADRESS      ((MAX_TAG_NUMBER * 5) + STARTADRESS)
#define ADRESS_IN_USE   0x01
#define ADRESS_FREE     0x00


//----------------------------------------------------------
void openGate();            //V Opens the gate
void outputSignal(char);    //V Outputs a buzzer and/or led signal 

bool verifyTag();               //V Check if is a supported tag
bool isMasterTag(byte*, bool);  //V Check if tag is the master key
bool isTagValid(byte*);         //V Check if tag is a valid key
void validateTag(byte*);        //V Move unkown tag to valid keys list
void deleteTag(byte*);          //V Remove tag from valid tags list 

void systemBlock(bool*);         // Block the system
void systemUnblock();       // Unblock the system
void systemReset();         // Reset the system, deleting all tags
void generateError();       // Leaves the system in a error state

void pinsSetup();                   //V Configure pins as input/output
bool checkActionSaved();            //V Check if master key is detected to save command
void checkEepromSaved(int, byte);   //V Check if value was stored in EEPROM

//---------------------------------------------------

void setup() {

    MFRC522 rfid(SS_PIN, RST_PIN);

    // {uid_1, uid_2, uid_3, uid_4, count}
    byte master_key[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0};

    char invalid_tag_count = 0;
    bool system_blocked = false;

    pinsSetup();

    while(1) {

        if (system_blocked)
    		return;

        if (!verifyTag())
            return;
        
        byte tag_uid[4] = {rfid.uid.uidByte[0], rfid.uid.uidByte[1], rfid.uid.uidByte[2], rfid.uid.uidByte[3]};
        
        // If master key is detected
        if (isMasterTag(tag_uid)) {
            
            // If master key has been detected 3 times
            if (master_key[4] == 3) {
                outputSignal(ABOUT_TO_BLOCK_SIGNAL);
                return;
            }
            // If master key is detected 4 times, the system is blocked
            else if (master_key[4] > 3) {
                systemBlock(system_blocked);
                return;
            }
        }

        if (digitalRead(BUTTON_PIN) && (master_key[4] > 0)) {
            
            delay(50);
            
            // Get duration of button press. System will be on halt during button press
            unsigned long timer_begin = millis(); 
            while (digitalRead(BUTTON_PIN) && !(timer_begin + millis() < 15E3)){}
            unsigned long button_press_duration = millis() - timer_begin;

            // There will be a press to reset 
            if (button_press_duration < 5E3)
                master_key[4] = 0;

            else if ((button_press_duration >= 15E3) && (master_key[4] == 1)) 
                systemUnblock(); 
        }

        // If valid key is detected
        if (isTagValid(tag_uid)) {

            // If master key has been detected before, key will be deleted from database
            if (master_key[4] == 1)
                deleteTag(tag_uid)
            // Otherwise, the gate will open
            openGate();
            return;
        }

        // Since if it's not valid or invalid, its unknown
        if (master_key[4] == 1) {
            validateTag(tag_uid);
            return;
        }
        
        // Since it's not valid or master tag, it's unknown
        invalidateTag(tag_uid);
        
        invalid_tag_count++;
        switch (invalid_tag_count)
        {
        case 2:
            outputSignal(ABOUT_TO_BLOCK_SIGNAL);
            break;

        case 3:
            // One of these should reset the invalid count (redundaant, since no card will work) 
            systemReset();
            systemBlock(system_blocked);
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
void verifyTag()
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
bool isMasterTag(byte *tag_uid, bool action_started = false) {
    
    if (master_key[0] == tag_uid[0] &&
        master_key[1] == tag_uid[1] &&
        master_key[2] == tag_uid[2] &&
        master_key[3] == tag_uid[3]) {
        
        if (ACTION_STARTED) {
            master_key[4] = 0;
            EEPROM.write(MASTER_TAG_ADD, master_key[4]);
        }
        else {
            master_key[4]++;
            EEPROM.write(MASTER_TAG_ADD, master_key[4]);
        }
        
        checkEepromSaved(MASTER_TAG_ADD, master_key[4]);

        return true;
    }
    else
        return false;

}

/*************************************************************
        isTagValid(byte *) - returns void
*************************************************************/
bool isTagValid(byte *tag_uid) {

    for (int address = START_ADDRESS; address < END_ADDRESS; address+= 5) {
        // Check if address there will be a card stored in
        if (EEPROM.read(address) == ADDRESS_IN_USE) {
            // Checking in a cascate way trying to avoid reading EEPROM to much
            if (tag_uid[0] == EEPROM.read(adress + 1)){
                if (tag_uid[1] == EEPROM.read(adress + 2)){
                    if (tag_uid[2] == EEPROM.read(adress + 3)){
                        if (tag_uid[3] == EEPROM.read(adress + 4)){
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
void validateTag(byte *tag_uid) {

    if (checkActionSaved()) {
        for (int address = START_ADDRESS; address < END_ADDRESS; address += 5) {
            
            if (EEPROM.read(address) == ADRESS_FREE) {

                EEPROM.write(address, ADDRESS_IN_USE);

                EEPROM.update((address + 1), tag_uid[0]);
                EEPROM.update((address + 2), tag_uid[1]);
                EEPROM.update((address + 3), tag_uid[2]);
                EEPROM.update((address + 4), tag_uid[3]);

                checkEepromSaved(adress, ADDRESS_IN_USE);
                checkEepromSaved((address + 1), tag_uid[0]);
                checkEepromSaved((address + 2), tag_uid[1]);
                checkEepromSaved((address + 3), tag_uid[2]);
                checkEepromSaved((address + 4), tag_uid[3]);
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
void deleteTag(byte *tag_uid) {

    if (checkActionSaved()) {
        for (int address = START_ADDRESS; address < END_ADDRESS; address += 5) {
            if (EEPROM.read(address) == ADRESS_IN_USE) {
                // Checking in a cascate way trying to avoid reading EEPROM to much
                if (tag_uid[0] == EEPROM.read(address + 1)){
                    if (tag_uid[1] == EEPROM.read(address + 2)){
                        if (tag_uid[2] == EEPROM.read(address + 3)){
                            if (tag_uid[3] == EEPROM.read(address + 4)){
                                EEPROM.write(address, ADRESS_FREE);
                                checkEepromSaved(address, ADRESS_FREE)
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
        checkEepromSaved(int, byte) - returns void
*************************************************************/
void checkEepromSaved(int address, byte number) {
    
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
        
        if (isMasterTag(rfid.uid.uidByte, true)) {
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
    system_blocked = true;
    EEPROM.write(SYSTEM_BLOCK_ADD, ADRESS_IN_USE);
    outputSignal(SYSTEM_BLOCKED);
}


void systemReset() {
					
    for (int address = START_ADDRESS; address < END_ADDRESS; address += 5) {
        EEPROM.update(address, 0x00);
        checkActionSaved(adress, 0x00);
    }
    outputSignal(SYSTEM_RESETED);
}
