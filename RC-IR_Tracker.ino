// *******************************************************************************************************************************
// 
// * RC Lap Timer
//
// *******************************************************************************************************************************
//
// Author: Jurgen Van de Perre
// Date: 07/02/2018
// Version: 0.5
//
// *******************************************************************************************************************************
// * Libraries
// *******************************************************************************************************************************
#include <IRremote.h>       // Library for IR Receiver
#include <Wire.h>           // Include Wire if you're using I2C (for OLED diplay)
#include <SFE_MicroOLED.h>  // Include the SFE_MicroOLED library

// *******************************************************************************************************************************
// * Variables
// *******************************************************************************************************************************
int m, s, mu=0, md=0, su=0, sd=0, l=0, lu=0, ld=0, lc=0, redLed=4, greenLed=13, zround=0, RECV_PIN=11, tx=0, timer=0, connect=1, led_status = 1, bits=0;
long int time, timeStart;
decode_results results;
char message[3];

// *******************************************************************************************************************************
// * Definitions
// *******************************************************************************************************************************
#define ZERO 250
#define ONE  650
#define TOLERANCE 150
#define BIT_SET(a,b) ((a) |= (1<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1<<(b)))
#define BIT_CHECK(a,b) ((a) & (1<<(b)))
#define PIN_RESET 9  // Connect RST to pin 9
#define DC_JUMPER 0

// *******************************************************************************************************************************
// * Object declarations
// *******************************************************************************************************************************
MicroOLED oled(PIN_RESET, DC_JUMPER);    // I2C declaration
IRrecv irrecv(RECV_PIN);                 // Setup IR Receivers

// *******************************************************************************************************************************
// * Custom Procedures
// *******************************************************************************************************************************
void printTitle(String title, int font){
  // Center and print a small title
  // This function is quick and dirty. Only works for titles one line long.
  
  int middleX = oled.getLCDWidth() / 2;
  int middleY = oled.getLCDHeight() / 2;
  
  oled.clear(PAGE);
  oled.setFontType(font);
  // Try to set the cursor in the middle of the screen
  oled.setCursor(middleX - (oled.getFontWidth() * (title.length()/2)),
                 middleY - (oled.getFontWidth() / 2));
                 
  // Print the title:
  oled.print(title);
  oled.display();
  delay(1500);
  oled.clear(PAGE);
}

void statusDisplay(String connection, String status){
  // Draw up the status display
  oled.clear(PAGE);
  oled.rect(0, 0, 64, 48);
  oled.line(0,10,64,10);
  oled.line(0,20,64,20);
  oled.setFontType(0);
  oled.setCursor(2, 2);
  oled.print(connection);
  oled.setCursor(2, 12);
  oled.print(status);
  
  oled.display();
};

void connect_Zround() {    
    // Connect to software
    int flash = 0;
    while (connect == 1) {
        led_indicators();

        // Read Message from Serial Port
        while (Serial.available() > 0) {
            for (int i = 0; i < 3; i++) {
                message[i] = Serial.read();
                delay(50);
            }
        }

        // Message: Connection from Zround "%C&"
        if (message[0] == '%') {
            if (message[1] == 'C') {
                if (message[2] == '&') {
                    int count = 0;
                    while (count <= 20) {
                        // Message: Confirmation "%A&"
                        Serial.write("%A&");
                        delay(100);
                        count++;
                    }
                    led_status = 2;
                    message[0] = ('0');
                }
            }
        }

        // Start Practice or Race
        if (message[0] == '%') {
            if (message[1] == 'I') {
                if (message[2] == '&') {
                    timeStart = millis();
                    connect = 0;
                    timer = 1;
                    zround = 1;
                    initiate_timer();
                }
            }
        }

        delay(50);
    }
    statusDisplay("Standby","IDLE");
}

void initiate_timer() {
    // Start timing routing
    while (zround == 1) {
        while (Serial.available() > 0) {
            for (int i = 0; i < 3; i++) {
                message[i] = Serial.read();
                delay(50);
            }
        }
        led_status = 4;
        led_indicators();
        timeStart = millis();
        timer = 1;

        while (timer == 1) {
            time = millis() - timeStart;
            m = time / 60000;
            mu = m % 10;
            md = (m-mu) / 10;
            s = (time/1000) - (m*60);
            su = s % 10;
            sd = (s-su) / 10;
            l = time - (s*1000) - (m*60000);
            lu = l % 10;
            ld = ((l-lu) / 10) % 10;
            lc = (l - (ld*10) - lu) / 100;

            if ( su == 0) {
                if (lu == 0 && ld == 0 && lc == 0) {
                    Serial.print("%T");
                    Serial.print(time,HEX);
                    Serial.println("&");
                }
            }

            if (su == 5) {
                if (lu == 0 && ld == 0 && lc == 0) {
                    Serial.print("%T");
                    Serial.print(time,HEX);
                    Serial.println("&");
                }
            }

            if (irrecv.decode(&results)) {
                lap_counter();
                irrecv.resume();
            }

            while (Serial.available() > 0) {
                for (int i = 0; i < 3; i++) {
                    message[i] = Serial.read();
                    delay(50);
                }
            }

            // Check if Practice or Race has Ended
            if (message[0] == '%') {
                if (message[1] == 'F') {
                    if (message[2] == '&') {
                        timer = 0;
                        digitalWrite(redLed,LOW);
                        digitalWrite(greenLed,LOW);
                        connect = 1;
                        zround = 0;
                        led_status = 3;
                        connect_Zround();
                    }
                }
            }
        }
        delay(10);
    }
}

void lap_counter() {
    // Read transponder and send to zRound in readable format
    check_transponder();
    if (tx!=0) {
        digitalWrite(redLed, HIGH);
        Serial.print("%L");
        Serial.print(tx, HEX);
        Serial.print(",");
        Serial.print(time, HEX);
        Serial.print("&");
        Serial.println();
        digitalWrite(redLed, LOW);
        tx=0;
    }
}


void led_indicators() {
    switch (led_status) {
        case 1: // Standby
            statusDisplay("Standby","IDLE");
            digitalWrite(redLed,HIGH);
            digitalWrite(greenLed,LOW);
            delay(200);
            digitalWrite(redLed,LOW);
            digitalWrite(greenLed,LOW);
            delay(700);
            break;
        case 2: { // Connecting
                statusDisplay("Connect...","IDLE");
                int blink = 0;
                while (blink <= 5) {
                    digitalWrite(redLed,HIGH);
                    delay(200);
                    digitalWrite(redLed,LOW);
                    delay(200);
                    blink++;
                }
                digitalWrite(redLed,HIGH);
                digitalWrite(greenLed,LOW);
                led_status = 3;
                break;
            }
        case 3: // Connected
            statusDisplay("Connected","IDLE");
            digitalWrite(redLed,HIGH);
            digitalWrite(greenLed,LOW);
            break;
        case 4: // Race
            statusDisplay("Connected","Racing...");
            digitalWrite(redLed,LOW);
            digitalWrite(greenLed,HIGH);
            break;
    }
}

void check_transponder() {
    // Read the EasyLapTimer Protocol
    bits = (results.rawlen-1); //variable "bits" gets the number of received bits
    int codeType = results.decode_type; 
    unsigned char received_value = 0;
    unsigned int set_bits = 0;
    unsigned int num_ones=0;
    unsigned int control_bit=0;    


 if(bits == 9){ 
    //only full messages are being processed
    for(int i = 1; i <= bits; i++){
      unsigned int signal_length = results.rawbuf[i]*USECPERTICK; //signal length in µs from buffer   
      if(i==9 && signal_length > (ONE - TOLERANCE)){ //bit #9 is the parity bit. It's "0" by default, only becomes "1" if received bit #9 is long
        control_bit = 1;  
      }
      if(set_bits < 8){ //here the actual setting of each received bit to either "1" or "0" takes place, control variable "set_bits" starts from zero
        if(signal_length > (ONE - TOLERANCE)){ // signal length is long -> set bit to "1"
          BIT_SET(received_value,8 - i); 
          num_ones +=1; // counting the number of ONEs in received message   
        }else{ // ..otherwise set bit to "0"
            BIT_CLEAR(received_value,8-i); 
        }
      }      
      set_bits++; //control variable increment
    }
    
    if(num_ones % 2 == control_bit){ // expression to check, if the parity bit is correct
      tx = received_value,DEC;
    }
    irrecv.resume();
  }
}

// *******************************************************************************************************************************
// * Main Loops
// *******************************************************************************************************************************
void setup() {
    Serial.begin(19200);
    pinMode(redLed,OUTPUT);
    pinMode(greenLed,OUTPUT);
    irrecv.enableIRIn();
    oled.begin();    // Initialize the OLED
    oled.clear(ALL); // Clear the display's internal memory
    oled.display();  // Display what's in the buffer (splashscreen)
    delay(1000);     // Delay 1000 ms
    oled.clear(PAGE); // Clear the buffer.    
    printTitle("RC Timer", 0);
    connect_Zround();
}

void loop() {
  
}
