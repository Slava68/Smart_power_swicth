#line 1 "Smart_power_switch_1.0.-3.ino"
#include <EEPROM.h>                                                                                        
#include <RCSwitch.h>                                                            

#define Input 0                                                                                                

#define Ch_0 B00111000                                                                                                                  
#define Ch_1 B00001000                                                                                 
#define Ch_2 B00010000                                                                                 
#define Ch_3 B00100000                                                                                 
                                                                                                       

#define prg_mode 0xFF                                                                            

#include "Arduino.h"
byte Code_Decode();
void New_Remote(unsigned int addr);
boolean Verify_Remote (unsigned int address);
void Ch_control (byte Ch);
void Programm_mode();
void setup();
void loop();
#line 14
unsigned long old_time = 0;                                           
byte count_time = 0;                                                                        
byte pad_number = 0;                                     

RCSwitch mySwitch = RCSwitch();                                         


                                                                                
byte Code_Decode() {

  unsigned long  code = mySwitch.getReceivedValue();                                 
  unsigned int address = code >> 8;                                                                             
  byte Button = code & 0xFF;                                                                         

  if (Verify_Remote (address)) {                                                                  
    mySwitch.resetAvailable();                                                
    return Button;                                                                                                          
  }
                                      
  New_Remote(address);                                                                                  
  return 0;                                                                                 
}

                                                                         
void New_Remote(unsigned int addr) {

  if (count_time == 0 ) {
    old_time = millis();                                                                                                                    
  }
  count_time++;                                                                                                                                   
  if ((count_time > 20) && ((millis() - old_time) < 20000) ) {                                                                 
    for (byte x = 0; x < 6; x++) {                                                                                     
      PORTD |= Ch_0;                               
      delay(300);                                                                               
      PORTD &= ~Ch_0;                        
      delay(300);
    }
    delay(1000);                                                                                                  

                                                    

    for (byte x = 0; x <= 3; x++) {                                                                                                 
      Ch_control (x);                                                                                 
      old_time = 1;
      mySwitch.resetAvailable();                                                              
      while (old_time != 0) {
        if (mySwitch.available()) {                                                                                
          EEPROM.write(EEPROM.read(0) * 16 + 3 + x, (mySwitch.getReceivedValue() & 0xFF));                                                                                
          Ch_control (x);
          delay(500);                                                                                              
          Ch_control (x);
          delay(200);
          Ch_control (x);
          delay(200);                                                                          
          Ch_control (x);
          delay(200);
          Ch_control (x);
          old_time = 0;                                                                               
        }
      }
      delay(500);                                                                                                          
    }

    EEPROM.write((EEPROM.read(0) * 16 + 1), (addr & 0xFF));                                                                    
    EEPROM.write((EEPROM.read(0) * 16 + 2), (addr >> 8));                                                                      
    EEPROM.write(0, (EEPROM.read(0) + 1));
    old_time = millis();                                                                                                                 
    count_time = 0;                                                                                                               
  }

  else if ((count_time < 20) && ((millis() - old_time) > 20000)) {                                                                                     
    old_time = millis();                                                                                                                 
    count_time = 0;                                                                                                               
    mySwitch.resetAvailable();                                                                  
    return;                                                                                                    
  }
  mySwitch.resetAvailable();                                                                    
  return;
}

                                                                                         

boolean Verify_Remote (unsigned int address) {                                                                                   

  pad_number =  EEPROM.read(0);                                                                                                          
  for (byte i = 0; i <= pad_number; i++) {                                                             
    unsigned int addr = (EEPROM.read(16 * i + 2) << 8) + EEPROM.read(16 * i + 1);                                                                                        
    if (address == addr) {                                                                                            
      pad_number = i;                                                                                                                        
      return 1;                                                                                                 
    }
  }
  return 0;                                                                                                        
}

                                                                                                 

void Ch_control (byte Ch) {                                                                                                                             

                                          
  if      (Ch == 0 && (PIND & Ch_0) != 0)  PORTD &= ~Ch_0;                                                                  
  else if (Ch == 0 && (PIND & Ch_0) == 0)  PORTD |= Ch_0;                                                                   

  else if (Ch == 1) PORTD ^= Ch_1;                                                                         
  else if (Ch == 2) PORTD ^= Ch_2;                                                                         
  else if (Ch == 3) PORTD ^= Ch_3;                                                                         
                                                                                                              

  return;
}

                                                                           
void Programm_mode() {                                                                                                          

  boolean F_erase = 1;                                                                                                               
  byte erase_time = 50;                                                                                                  
  while (erase_time != 0) {                                                                                     
    mySwitch.resetAvailable();                                                                 
    if (mySwitch.available()) {                                                           
      byte prg = Code_Decode();                                                                                                          


      if (prg == prg_mode) {                                                                                 
        Ch_control(0);                                                                                                                 
        erase_time--;                                                                                      
        delay(100);
      }
                                                                            
      else if (prg == 0x03) {                                                                                         
        F_erase = 0;                                                                                                                                     
        mySwitch.resetAvailable();                                                             
        while (1) {                                                                                                                            
          for (byte x = 3; x < 7; x++) {                                                                                                                                
            PORTD &= ~B01111000;
            PORTD |= (1 << x);                                                                                                    
            delay(300);
          }

          if (mySwitch.available()) {                                                     
            byte prg = Code_Decode();                                                                                                    

            for (byte i = 1 ; i < 5; i++) {                                                                                                                            
              if (prg == EEPROM.read(3 + i + pad_number * 16)) {                                                                                                                    
                if (EEPROM.read(250 + i)) EEPROM.write(250 + i, 0);                                              
                else if (!EEPROM.read(250 + i)) EEPROM.write(250 + i, 0xFF);                                     
                PORTD &= ~Ch_0;                                                             
                for (byte x = 0; x < 10; x++) {                                                                                               
                  Ch_control(i);
                  delay(100);                                                                   
                }
                PORTD &= ~Ch_0;                                                             
                return;
              }
            }
          }
        }                                     

      }                                                                                   

    }                      

  }        

  if (F_erase == 1) {
    delay(2000);
    EEPROM.write(0, 0);                                                                                                                                                                         
    for (byte x = 1; x < 250; x++) EEPROM.write(x, 0xFF);                                                                                                           

    for (byte x = 0; x < 10; x++) {                                                                                                            
      Ch_control(0);
      delay(100);                                                                                
    }
    PORTD &= ~Ch_0;                                                                          
    return;
  }
  return;
}


                                                                                                  
void setup() {

  mySwitch.enableReceive(Input);
  if (EEPROM.read(0) == 255)  {
    EEPROM.write(0, 0);                                                                                                                                                          
    EEPROM.write(250, 0);
    EEPROM.write(251, 0);
    EEPROM.write(252, 0);                                                                                                                    
    EEPROM.write(253, 0);
    EEPROM.write(254, 0);                                                                              
  }
  DDRD |= Ch_0;                                                                                                              
  PORTD |= B11111111 ^ Ch_0;
  PORTB |= B11111111;                                                                                                                                                                                       
  PORTC |= B11111111;

  if (EEPROM.read(0) == 0 ) {                                                                                     
    PORTD |= Ch_0;                                                                                                                                     
    delay(1000);
  }
  PORTD &= ~Ch_0;                                                                           

  PORTD |= Ch_1 & EEPROM.read(251);                                                                                                      
  PORTD |= Ch_2 & EEPROM.read(252);
  PORTD |= Ch_3 & EEPROM.read(253);
                                      



}
                                                                                                  
void loop() {

  if (mySwitch.available()) {                                                                            
    byte Button = Code_Decode();                                                                                
    if (Button == 0xC3) {                                                                           
      byte cur_Ch = PIND & 0x78;                                                                      
      if (cur_Ch != 0) {                                                                     
        count_time = 1;                                                                                                       
        PORTD ^= cur_Ch;                                                                                 
        delay(200);
        PORTD |= cur_Ch;                                                                            
        delay(300);
        PORTD ^= cur_Ch;                                                                                 
        delay(200);
        PORTD |= cur_Ch;                                                                            
      }
    }
    if (Button == prg_mode) Programm_mode();                                                                                                                              

    for (byte i = 0 ; i < 5; i++) {                                                                                                                                             
      if (Button == EEPROM.read(3 + i + pad_number * 16)) {                                                                                                     
        Ch_control (i);                                                                                                   
        old_time = millis();                                                                                          
      }
    }
    delay(300);                                                                                                   
    mySwitch.resetAvailable();                                                         
  }
  if (((PIND & Ch_0) != 0) && (millis() - old_time > 1800000) && (count_time == 0)) {
                                                                                                                               
    PORTD &= ~Ch_0;                                                                                                                                         
  }
}


