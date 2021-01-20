#include "RL_LEDArray.h"
#include "RL_HumidityTemp.h"
#include "RL_LCD.h"
#include "Buzzer.h"
#include  "RL_RFID.h"
#include <SPI.h>
#include "RL_RGBLED.h"
#include <Wire.h>
#include "RL_RTC.h"

#define BUZ_PIN 11
RTC rtc;

RGBLED rgbLed;

RFID rfid;

Buzzer buzzer;

LCD lcd(0x3F, 16, 2);

HTSensor ht;

LED led;

volatile int state = 0;
int command = 2;
const byte lightPin = A0; // set the light measurement pin
unsigned long previousMillis_light = 0; // to store old time
unsigned long previousMillis_temperature = 0; // to store old time
unsigned long previousMillis_RFID = 0; // to store old time

const unsigned long interval = 500;
unsigned int LowTemp = 0;
unsigned int HighTemp = 0;
unsigned int LowTemp_New = 10;
unsigned int HighTemp_New = 30;

int rfid_flip = 1;// rfid_flip  "1" to check the door, "2" to add cards, "3" to delete cards

const int nb = 4;
const int stuff = 10;
int securitycard[stuff][nb]={0} ;


int interface_change = 1;
int i = 0;
char charTemp[10] = {};
static bool show_interface = true;
static int count = 0;

int light_flip = 4; // 2 LEDarray AllON, 3 LEDarray ALLOff, 4 AUTO

void setup() {
  Serial.begin(57600);  // setup serial comms(light)

  //LED array initialize
  pinMode(lightPin, INPUT); //set the pin mode for input (light)
  led.AllOff();

  //DHT initialize
  ht.begin();

  //lcd initialize
  lcd.init();
  lcd.backlight();

  //buzzer initialize
  buzzer.begin(BUZ_PIN);

  //motor initialize
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  digitalWrite(7, 0);
  digitalWrite(8, 0);

  // RFID initialize
  rfid.PCD_Init();


  rgbLed.begin();

  attachInterrupt(0, interrupt, RISING);

  if(!rtc.begin())
  {
    Serial.println("Couldn't fing RTC");
    while(1);
    }

   if(!rtc.isrunning())
   {
    Serial.println("RTC is not running!");
     rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));   
    }
}

void loop() {
  unsigned long long  currentMillis = millis();

  //Light control
  //  in case the "currentMillis" overflow
  if ((currentMillis - previousMillis_light) > interval || currentMillis == 0 )
  {
    previousMillis_light = currentMillis;
    if(light_flip == 2)     light_detect(2);
    else if(light_flip == 3) light_detect(3);
    else if(light_flip == 4) light_detect(4);
  }

  //temperature control
  if ((currentMillis - previousMillis_temperature) > 2 * interval || currentMillis == 0 )
  {
    previousMillis_temperature = currentMillis;
    temperature_humidity_detect();
     
  }

  // check RFID
  if ((currentMillis - previousMillis_RFID) > 1600 || currentMillis == 0 )
  {
    previousMillis_RFID = currentMillis;
    if ( rfid_flip == 1)  security_card(previousMillis_RFID);
    else if(rfid_flip == 2) add_NewCard();
    else if(rfid_flip == 3) delete_card();
  }
  user_interface();
  if (state == 1)
  {
    if (command == 1)
    {
      Serial.print("PIR detected! ");
      time_display();
     
      state = 0;
    }
  }
}

void time_display()
{
 
   DateTime now = rtc.now();
   Serial.print("Time: ");  

   Serial.print(now.hour(), DEC);  
   Serial.print(':'); 
   Serial.print(now.minute(), DEC);   
   Serial.print(':');  
   Serial.print(now.second(), DEC);  
   Serial.print("  ");
  

   Serial.print(now.day(), DEC); 
   Serial.print('/');  
   Serial.print(now.month(), DEC); 
   Serial.print('/'); 
   Serial.print(now.year(), DEC);   
   Serial.println();

 }



void interrupt()
{
  state = 1;
}

void serialEvent()
{
  if (interface_change == 1) // designed which interface to pop up
  {
    if (Serial.available() > 0)
    {
      char value  ;
      value = (char)Serial.read();
      //Serial.print("What value is: ");
      //Serial.println (value);
      
      if (value == '0') // for safe mode
      {
        command = 1;
        Serial.println(" ");
        Serial.println("Safe Mode opend!  (Door locked, PIR Sensor opend)");
      }
      else if (value == '1')
      {
        command = 2;
        Serial.println(" ");
        Serial.println("Safe Mode closed! (Door unlocked, PIR Sensor closed)");
      }

       else if (tolower(value) == 'b') // for temperature range change
      {
        interface_change = 2;
     
        count = 0;
        
      }
       else if(tolower(value) == 'a') //  back to menu
       {
        show_interface = true;
       }
       else if(tolower(value) == 'c') // add new cards
       {
        interface_change =  4;
        count = 0;
        }
        else if(tolower(value) == 'd') //  delete cards
        {
          interface_change = 5;
          count = 0;
          
          }
        else if(tolower(value) == 'e') //  display cards
        {

          interface_change = 6;
          count = 0;
          
          }
        else if(tolower(value) == 'f') // delete cards manully
        {
         // Serial.println("HAHA");
          interface_change = 7;
         // count = 0;
          }
       else if(tolower(value) == '2') // delete cards manully
        {
            light_flip = 2;
            Serial.println("ALL lights on !");
          }
          else if(tolower(value) == '3') // delete cards manully
        {
           light_flip = 3;
            Serial.println("ALL lights off !");
           
          }
            else if(tolower(value) == '4') // delete cards manully
        {
            light_flip = 4;
             Serial.println("ALL lights AUTO !");
          }
    }
  }


  if (interface_change == 2) // set temperature range (Low Temperature) 
  {
 

    count = count + 1;
    if (count == 1)  
    {
      Serial.println(" ");
      Serial.println("   ## Set Temperature Range ##");
      Serial.print("Current LowTemp:");
      Serial.print(LowTemp_New);
      Serial.print(" *C    HighTemp:");
      Serial.print(HighTemp_New);
      Serial.println(" *C");
      
      Serial.print("Please enter new LowTemp  (end with \"*\"): ");
    }

    char endMaker = '*';
    char backMaker = 'a';
    char rc ;
    if (Serial.available() > 0)
    {
      rc = Serial.read();
      if(rc != endMaker && rc!= backMaker )
      {
        charTemp[i] = rc;
         i = i + 1;
      }
      
       else if(tolower(rc) == backMaker) // if user want to give up and back to menu
       {
        
        for(int c = 0; c<10; ++c) // clear charTemp[];
           {
            charTemp[c] = {' '};
            }
        LowTemp =0;   //reset variables
        HighTemp = 0;
        interface_change = 1;
        Serial.println("Back to main menu!");
        Serial.println("");
        show_interface = true; // call menu back
        count = 0;
         i=0;
         }
        else if(rc == endMaker) // if user finish enter
        {
            charTemp[i] == '*';
            LowTemp = atoi(charTemp); // convert "char" style to "int" style
            Serial.print(LowTemp);
       
              // Serial.println("    Back to MENU (A)"); 
           for(int c = 0; c<10; ++c) // clear chatTemp[]
           {
            charTemp[c] = {' '};
            }
           // charTemp[10]={ };
            interface_change = 3; // move to next interface 
            count = 0;
             i=0;    
      }     
    }
 }

  if (interface_change == 3) // set temp range (High Temperature) interface
  {
    
   
    count = count + 1; 
    if (count == 1) // make sure that the words do not display repeatly
    {
      Serial.println(" ");
   
      
      Serial.print("Please enter new HighTemp (end with \"*\"): ");
      
    }

    char endMaker = '*';
    char backMaker = 'a';
    char rc ;
    if (Serial.available() > 0)
    {
      rc = Serial.read();
      if(rc != endMaker && rc != backMaker)
      {
        charTemp[i] = rc;   // read form Serial moniter
         i = i + 1;
      }
      
       else if(tolower(rc) == backMaker) 
       {
        for(int c = 0; c<10; ++c) // if user want to give up and back to menu
           {
            charTemp[c] = {' '};
            }
        LowTemp =0;
        HighTemp = 0;
        interface_change = 1;
        Serial.println(" Back to main menu!");
        Serial.println("");
        show_interface = true; // 
        count = 0;
         i=0;
        }
       
       else if (rc == '*') {      // if user finish enter number
            charTemp[i] == '*';
            HighTemp = atoi(charTemp);
            Serial.print(charTemp); 
          
            //Serial.println("    Back to MENU (A)"); 
           for(int c = 0; c<10; ++c) // clear chatTemp
           {
            charTemp[c] = {' '};
           }
           
           // charTemp[10]={0};
            count = 0;
             i=0;
            if(HighTemp <= LowTemp) // if high temperature is lower than low temperature, warning users
            {
              Serial.println("\nSorry, HighTemp is Lower than LowTemp. New values are not accepted!  ");
              Serial.println("");
              }
              else
              {
                HighTemp_New = HighTemp;   
                LowTemp_New = LowTemp;
                  Serial.print("\nTemperature Range Setting Successfully!  ");
                  Serial.print(LowTemp_New);
                  Serial.print(" *C - ");
                  Serial.print(HighTemp_New);
             
                  Serial.println(" *C");
                for(int c = 0; c<10; ++c) // clear charTemp[];
           {
            charTemp[c] = {' '};
           }
             i = 0;
                
                }
            
            show_interface = true; // back to main menu   
            interface_change = 1;  
      }   
    }
 }

  if(interface_change == 4) // "Add new cards" interface 
  {
     count = count+1;
     if (count == 1) // make sure that the instructions do not display repeatlly 
     {
      Serial.println("      ## Add New Security Cards ## ");
      Serial.println("Please scan cards to ACTIVE or back to menu (A)  ");
        rfid_flip = 2; // rfid_flip  "1" to check the door, "2" to add cards, "3" to delete cards
      }    
        
        char rc ;
        if (Serial.available() > 0) // get ready to read form Serial moniter
     {
      rc = Serial.read();  
      
      if (tolower(rc) == 'a') // 'a' means  user want to back to main menu 
      {
        rfid_flip = 1; // rfid_flip  "1"  rfid sensor is used to check the door , "2" to add cards, "3" to delete cards
        interface_change = 1; // back to main menu
        count = 0;
        show_interface = true;  // back to main menu
       }
      }
  }

if(interface_change == 5) // "Delete cards" interface 
  {
     count = count+1;
     if (count == 1) // make sure that the instructions do not display repeatlly 
     {
      Serial.println("       ## Delete Security Cards ##");
      Serial.println("  Please scan cards to DELETE or back to menu (A)  :");
        rfid_flip = 3; // rfid_flip  "1" rfid sensor is used to to check the door, "2" to add cards, "3" to delete cards
      }    
        
        char rc ;
        if (Serial.available() > 0)
     {
      rc = Serial.read();
      if (tolower(rc) == 'a')
      {
        rfid_flip = 1; // rfid_flip  "1" rfid sensor is used to check the door, "2" to add cards, "3" to delete cards
        interface_change = 1; // back to main menu
        count = 0;
        show_interface = true;  // back to main menu
       }
     }
  } 
      
   if(interface_change == 6) // "display cards" interface 
      {
        
       count = count+1;
      if (count == 1) 
       {
       Serial.println("\n#        Security Cards List       #");
       Serial.println(   "-----------------------------------");
       display_card();
      }    
        
        char rc ;
        if (Serial.available() > 0)//back to main MENU
       {
         rc = Serial.read();
        if (tolower(rc) == 'a')
        {
       
        interface_change = 1; // back to main menu
        count = 0;
        show_interface = true;  // back to main menu
       }
        

   }
  }

     if(interface_change == 7) // delete cards manully 
     {
     
       count = count + 1;
    if (count == 1)  
    {
      Serial.println(" ");
      Serial.println("     ## Delete Security Cards Manully ##");
      Serial.print("Please enter the card number to DELETE (Add a space between each number and end with \"*\"): ");
    }
    static int aa = 0; // control the position of "delete_card[4]"
    static int delete_card [4] = {0};   // save int number which come from charTemp2
    static char charTemp2[10] = {}; //save the number read from Serial monitor
      static int a = 0; //control the position where to store number inside the charTemp2[10]
    //  Serial.print("times: ");  
    char endMaker = '*'; // means user finished enter
    char backMaker = 'a'; // means user want to back to menu
    char rc ;
    
    if (Serial.available() > 0)
    {
      rc = (char)Serial.read();
      if(rc != endMaker && rc!= backMaker && rc != ' ' )
      {
            // Serial.print("RC: ");  
          //  Serial.println(rc); 
        charTemp2[a] = rc;
        //Serial.println("charTemp2: ");  
        //Serial.print(charTemp2); 
        //   Serial.print("a: ");  
      //   Serial.println(a); 
        
         a = a + 1;
      }
      
       else if(tolower(rc) == backMaker) //back to MENU
       {
        
        for(int c = 0; c<10; ++c) //clear chatTemp2[]
           {
            charTemp2[c] = {' '};
            }
    
        interface_change = 1;
        Serial.println(" Back to MENU!");
        Serial.println("");
        show_interface = true; // back to menu interface
        count = 0;
         a=0;
         aa = 0;
         }
        
        else if (rc == ' ') // means we have got one complete number
        {
         
          // Serial.println("Come here delete_card");  
           
           delete_card[aa] = atoi (charTemp2);
          // Serial.println(delete_card[aa]);  
           a = 0;
           for(int c = 0; c<10; ++c) //clear chatTemp2[]
           {
            charTemp2[c] = {' '};  
            }
            a = 0;
          aa = aa + 1;
          if(aa > nb) aa = nb;
          
          }
        else if(rc == endMaker) // enter finished
        {
           delete_card[aa] = atoi (charTemp2); // convert 'char' type to 'int' type 
           delete_card_manully(delete_card); // call function delete_card_manully()
            a = 0;
            aa =0;
           for(int c = 0; c<10; ++c) //clear chatTemp2[]
           {
            charTemp2[c] = {' '};
            }
          
            
         
         delete_card[nb] = {0};
            // count = 0;
                 
      }     
    }
   }

}
  
void delete_card_manully(int delete_card[])
{
    int aa = 0; // to store the card  which shoule be deleted
   static bool sameCard = true; // used to check if the card existed 
   
    for(int a = 0; a < stuff; ++a)
    {
           if(sameCard == true && a != 0)
           {
           // Serial.print("when break a is:");
           // Serial.println(a)   ; 
            aa = a; // find the same card number in Arduino, store the position in "aa" 
            break;  // end the loop
           }else
             {
               sameCard = true;// reset the "sameCard" is ture, to let the check continue
             
              }
      for (int b = 0; b < nb;++b)
       { 
          //Serial.print("Card:");
          //Serial.print(securitycard[a][b]);
         // Serial.print("  sotre:");
         // Serial.println(store_card[b]);
        if(securitycard[a][b] != delete_card[b] ) 
        {
          sameCard = false; // if we did not find the card number here, set "sameCard" false
          break;   
          }
        }
      }
    
     if(sameCard  == false) //  we do not find the card in Arduino
     {
     
         Serial.println("\nCan't find the Card Number in Arduino! Please check again! ");
         count = 1;
      }else  //  we find the card in Arduino 
      {
              
             
             for(int c = 0; c < nb; ++c)
             {
              securitycard[aa-1][c] = 0; // delete the card number from Arduino
              }
              Serial.print("\n\nCard Number:"); // present deleted card information to  users 
              for(int a = 0; a < nb; ++a)
              {
               Serial.print (delete_card[a]);
               Serial.print (" ");
            
                }
               
              Serial.println("  DELETED successfully!  Continue or Back to menu (A) ");
        }
  }

void display_card()  // display security card list
{
      static bool space = true;  // check if this position inside securitycard[][] is blank,and nothing to display
      static int numb = 0;
       for( int b = 0; b < stuff; ++b)  // check if shere is any space to store new card
       {
           // Serial.print("when break Wa is:");
            //Serial.println(b);
            
            if(space == true  )  
            {}  // here is blank, nothing to display
              else  // here is a card, diaplay the number of the card
              {
                Serial.print("#   Card ");
                Serial.print(numb);
                Serial.print(" Number: ");
                for(int a = 0 ; a < nb; ++a)
                {
                 Serial.print(securitycard[b-1][a]);
                 Serial.print(" ");
                }
                space = true;
                Serial.println("   #");     
              }
        for(int a = 0; a < nb; ++a) 
        {
          
          if(securitycard[b][a] != 0) // check if securitycard[b][0] to securitycard[b][3] are all 0. 
          {
            //Serial.println("space is false");
            space = false; // if not all zero, space = false
            numb = numb+ 1;
            break;
            }      
          }
       }
}


void delete_card()
{
    int aa = 0; // to store the positon of the card  which gonna be deleted
   static bool sameCard = true; //   if the card existed, ture : yes, false : no
   
    if (!rfid.isNewCardPresent()) return; 
     //select on of the cards
    if (!rfid.readCardSerial()) return;
   
   int store_card[nb]={0}; // reading RFID and save the card number 
   for (byte i = 0; i < rfid.uid.size; i++)
    {
       int store = rfid.uid.uidByte[i]; 
        store_card[i] = store;
    }
  
  for(int a = 0; a < stuff; ++a)
    {
           if(sameCard == true && a != 0)
           {
          
            aa = a;  // find the same card number in Arduino, store the position in "aa" 
            break;  // end the loop
           } else
              {
                sameCard = true; // reset the value of "sameCard" is ture, to let the check continue
             
              }
      for (int b = 0; b < nb;++b)
       { 
          //Serial.print("Card:");
          //Serial.print(securitycard[a][b]);
         // Serial.print("  sotre:");
         // Serial.println(store_card[b]);
        if(securitycard[a][b] != store_card[b] ) 
        {
          sameCard = false; // if we did not find the card number in this line, set "sameCard" false
          break;   
          }
        }
      }
    
     if(sameCard  == false) // If we did not find the card in Arduino
     {
         rgbled_buzzer_door(2);
         Serial.println("\nDid't find this card. ");
      }else  // If we find the card in Arduino 
      {
              
              rgbled_buzzer_addCard();
             for(int c = 0; c < nb; ++c)
             {
              securitycard[aa-1][c] = 0; // delete the card number from Arduino
              }
              Serial.print("\nCard Number:"); // present the deleted card information to user
              for(int a = 0; a < nb; ++a)
              {
               Serial.print (store_card[a]);
               Serial.print (" ");
                
                }
              Serial.println("  DELETED successfully!   Continue or Back to menu (A) ");   
        } 
  }

void add_NewCard()  // active new cards 
{
  
   bool sameCard = true; //  check the statue if the card has been sotred already  
   if (!rfid.isNewCardPresent()) return;
  
   //select on of the cards
   if (!rfid.readCardSerial()) return;
 
   int store_card[nb]={0};
   for (byte i = 0; i < rfid.uid.size; i++) // store the number readed from card 
    {
       int store = rfid.uid.uidByte[i]; 
        store_card[i] = store;
    }
    for(int a = 0; a < stuff; ++a)
    {
           if(sameCard == true && a != 0)
           {
          //  Serial.print("when break a is:");
           // Serial.println(a);
        
            break; // find the same card number stored in arduino
           }else
           {
            sameCard = true;
            }
      for (int b = 0; b < nb;++b)
       {
        
        if(securitycard[a][b] != store_card[b] )  // check if the number stored in arduino as same as the number of card
        {
          sameCard = false; 
          break;   
          }
        }
      }
     if (sameCard == true)
     {
      rgbled_buzzer_door(2);
   
      Serial.println("\nSorry, it's not a NEW card! Please check again!  Back to MENU (A) ");
      }
      else 
      {
        rgbled_buzzer_addCard();
       static bool space = true;
       for( int b = 0; b < stuff; ++b)  // check if there is any space to store new card, 
                                        // we need to store new card data in the first blank line finded in "Securitycard[][]", 
                                        // which is the best way to save storge resource.
       {
           if(space == true && b != 0)
            {
             
                for(int c = 0; c<nb; ++c)
                 {
                  securitycard[b-1][c] = store_card[c] ; 
                 }
                break;
              }  
              else 
              {
                space = true;     
              }
        for(int a = 0; a < nb; ++a) 
        {
          if(securitycard[b][a] != 0)
          {
          
            space = false;
            break;
            }
          
          }
       }
          Serial.print("\nNew Card Number: "); // display the card information 
          for(int a = 0; a < nb; ++a)
              {
               Serial.print (store_card[a]);
               Serial.print (" ");
               }
         Serial.println(" ACTIVE successfully!  Continue or Back to menu (A) ");
       }
        
 }    

  
void rgbled_buzzer_addCard() 
{
     while (true)
  {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis_RFID <= 200)
    {
      rgbLed.OnRgb(0, 255, 0);
      buzzer.On();
    }
    else
    {
      rgbLed.OnRgb(0, 0, 0);
      buzzer.Off();
      break;
    }
    
  }
}

void user_interface()  // the main menu 
{
  
  if (show_interface == true) // make sure that the menu does not display repeatlly on moniter
  {
    show_interface = false;

    Serial.println("      *********************************************************");
   // Serial.println("");
    Serial.println("      *                  ** CONTROL MENU **                   *");
    Serial.println("      *                                                       *");
    Serial.println("      *     - Back to MENU:                          ( A )    *");
    Serial.println("      *     - Set temperature range :                ( B )    *");
    Serial.println("      *     - Add New Security Cards:                ( C )    *");   
    Serial.println("      *     - Delete  Security Cards:                ( D )    *");  
    Serial.println("      *     - Display All Security Cards:            ( E )    *");   
    Serial.println("      *     - Delete Security Cards Manully:         ( F )    *"); 
    Serial.println("      *     - Safe Mode: On( 0 ) /  Off( 1 )                  *");
    Serial.println("      *     - Lights: ALLOn( 2 ) / ALLOff( 3 ) / AUTO( 4 )    *");
  //  Serial.println("");

    
    Serial.println("      *********************************************************");

  }

}




void security_card(unsigned long previousMills_RFID) // user can only  open the door by sccaning the card
{
  if(command == 1) // safe mode so the door locked
  {
    
   while (true)  // set the color of rgbled and sound of buzzer 
  {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis_RFID <= 100)
    {
      rgbLed.OnRgb(0, 255, 0);
      buzzer.On();
    }
    else
    {
      rgbLed.OnRgb(0, 0, 0);
      buzzer.Off();
      break;
    }
    
  }
    }
 
  else if (command == 2) // check the card to open the door
  {
  rgbLed.OnRgb(0, 0, 0);
  int check[4];  // store the card number from rfid sensor
  //look for new cards
  if (!rfid.isNewCardPresent()) {
    return;
  }

  //select on of the cards
  if (!rfid.readCardSerial()) {
    return;
  }

  bool flip = true; // store the statue
  for (int i = 0; i < rfid.uid.size; i++)
  {
    int store = rfid.uid.uidByte[i];

     check[i] = store; // read from card
  }
     
     for(int a = 0; a < stuff; ++a)  // check whether the card has been activited 
    {
       
           
           static bool check_door_card = true; // to control the times of loop
           if(check_door_card == true && a != 0)
           {
              flip = true;
            break;
           }
           else 
           {           
                check_door_card =true; 
                flip = false; // flip between open the door or do not
            }
      for ( int b = 0; b < nb; ++b)
       {  
      
        if( securitycard[a][b] != check[b] )  // if we can not find the card number in Arduino, we can't open the door 
        {
          check_door_card = false; 
          break;   
          }
        }
      }
   

  if (flip == false) // the card is not correct! rgbled and buzzer waringing
  {
    rgbled_buzzer_door(2);
  
  }
  else
  {
    motor_door();
 
  }  
}
}

void rgbled_buzzer_door (int rb) // rgb and buzzer (open door)
{
  while (true)
  {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis_RFID <= 500)
    {
      rgbLed.OnRgb(255, 0, 0);
      buzzer.On();
    }
    else
    {
      rgbLed.OnRgb(0, 0, 0);
      buzzer.Off();
      break;
    }
  }
}


void motor_door(void) // open and close the door
{
  while (true)
  {
    unsigned long currentMillis = millis();
    //Serial.print("Millis: " );
    //Serial.println(currentMillis);
    //Serial.print("Previous: " );
    // Serial.println(currentMillis);
    if (currentMillis - previousMillis_RFID <= 2500)
    {

      rgbLed.OnRgb(0, 255, 0);
      digitalWrite(7, 1);
      digitalWrite(8, 0);
    }
    else if (currentMillis - previousMillis_RFID <= 3000)
    {
      digitalWrite(8, 0);
      digitalWrite(7, 0);

    }
    else if (currentMillis - previousMillis_RFID > 3000 && currentMillis - previousMillis_RFID <= 5500 )
    {
      digitalWrite(8, 1);
      digitalWrite(7, 0);
    }
    else
    {
      digitalWrite(8, 0);
      digitalWrite(7, 0);
      rgbLed.OnRgb(0, 0, 0);
      break;

    }
  }
}

void light_detect (int flip)  // control the lightValue
{
  if     (flip == 2)      led_display( 0); // ALL led on. pass 0 to led_display
  else if(flip == 3)      led_display (1000); //All led off. pass 1000 to led_diaplay
  else if(flip == 4)
  { 
  int lightValue = 1024 - analogRead(lightPin); //Read the light value
  
  led_display(lightValue); // pass the real value of lightValue to led_diaplay
}
}

void led_display(int LightValue)
{
  int LightValue_Old = 0;
  static int count = 0;
 
  // check if the LightValue has changed greatly
  if ((LightValue - LightValue_Old > 50) || (LightValue - LightValue_Old > (-50)))   count = count + 1;

  // if the results of last 3 checks confirmed that the LightValue changed greatly, we need to change the number of LED tured on.
  if (count == 2)
  {
    count = 0;
    LightValue_Old = LightValue;
    
    if (LightValue > 750 && LightValue <= 850)
    {
      led.On(1);
      led.Off(2);
      led.Off(3);
      led.Off(4);
      led.Off(5);
      led.Off(6);
      led.Off(7);
      led.Off(8);
    }
    else if (LightValue > 650 && LightValue <= 750)
    {

      led.On(1);
      led.On(2);
      led.Off(3);
      led.Off(4);
      led.Off(5);
      led.Off(6);
      led.Off(7);
      led.Off(8);

    }
    else if (LightValue > 550 && LightValue <= 650)
    {
      led.On(1);
      led.On(2);
      led.On(3);
      led.Off(4);
      led.Off(5);
      led.Off(6);
      led.Off(7);
      led.Off(8);
    }
    else if (LightValue > 450 && LightValue <= 550)
    {
      led.On(1);
      led.On(2);
      led.On(3);
      led.On(4);
      led.Off(5);
      led.Off(6);
      led.Off(7);
      led.Off(8);

    }
    else if (LightValue > 400 && LightValue <= 450)
    {
      led.On(1);
      led.On(2);
      led.On(3);
      led.On(4);
      led.On(5);
      led.Off(6);
      led.Off(7);
      led.Off(8);
    }
    else if (LightValue > 350 && LightValue <= 400)
    {
      led.On(1);
      led.On(2);
      led.On(3);
      led.On(4);
      led.On(5);
      led.On(6);
      led.Off(7);
      led.Off(8);
    }
    else if (LightValue > 300 && LightValue <= 350)
    {
      led.On(1);
      led.On(2);
      led.On(3);
      led.On(4);
      led.On(5);
      led.On(6);
      led.On(7);
      led.Off(8);

    }
    else if (LightValue <= 300) led.AllOn(); // dim, all lights on
  
    else  led.AllOff(); // britht, all lights off

  }
}

void temperature_humidity_detect (void) 
{
  float h = ht.readHumidity(); // Reading humidity
  float t = ht.readTemperature(); // Reading temperature as Celsius
  //Check if any reads failed and exit early
  if (isnan(h) || isnan(t) )
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  lcd_display(h, t);


  if (t > 40)  fire_alarm();
  else if (t < LowTemp_New)  // if temperature lower than the temperature range setted before, motor will start to work
  {
    temp_change( 1 );
  }
  else if (t > HighTemp_New) // if temperature higher than the temperature range setted before, motor will start to work
  
  {
    temp_change(2);
  }
  else if (t  >= LowTemp && t <= HighTemp) // if temperature is appropriate, stop the motor
  {
    temp_change(0);
 
  }

}

void lcd_display(float h, float t) // LCD display temperature and humidity
{
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");

  lcd.print(t);
  lcd.print(" *C");

  lcd.setCursor(0, 1);
  lcd.print("HUM : ");
  lcd.print(h);
  lcd.print(" *F");

  // Serial.print("H:");
  // Serial.println(t);
}

void temp_change(int change)
{

  static int counter = 0;
  static int change_old = 100;
  if (change_old == 100) // if it is the first time this function be called
  {
    change_old = change;
  }
  else
  {

    if (change_old != change) 
    {
      counter = 0;
      change_old = change;
      
    }
    else if (change_old == change)
    {
      if (change == 1) // turn clockwise
      {
    
        counter = counter + 1;
        if (counter > 1)
        {
          digitalWrite(7, 1);
          digitalWrite(8, 0);

        }
      }
      else if (change == 2) // turn anticlockwise
      {
        counter = counter + 1;
        if (counter > 1)
        {
          digitalWrite(7, 0);
          digitalWrite(8, 1);
        }
      }
      else if (change == 0 ) // stop motor
      {
        counter = counter + 1;
        if (counter > 1)
        {
          digitalWrite(7, 0);
          digitalWrite(8, 0);
          if (counter == 1000) counter = 3;
        }
      }
    }
  }
}


void fire_alarm(void) //  if temperature is ery high, buzzer on 
{
  static int count = 0;
  count = count + 1;
  if (count > 2)
  {
    buzzer.On();
    if ( count <= 4) buzzer.Off();
    if (count > 5 ) count = 2;
  }
}

