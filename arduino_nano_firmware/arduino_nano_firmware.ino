/*
Board: "Arduino Nano"
Prozessor: ATmega328P (Old Bootloader)
 */

#include <Servo.h> 

//#define DEBUG
//#define DEBUGFULL
//#define DEBUGPPM
//#define DEBUGVOL
#define FAILSAFEPPM //activate failsafe for PPM signal loss
#define FAILSAFEVOL //activate failsafe for under-voltage
#define BLINKON // enable blink code
//#define V6INPUT //12k_68k voltage divider for input voltage of 5.5V to 7.2V (initial solution)
#define V8INPUT //10k_68k voltage divider for input voltage of 5.5V to 8.4V (better solution)
//#define PISTOLSTICK // mapping of input signal 0%=-100% +100%=+100%

Servo escout; 

const int servoch1inPin = 2;
const int servoch1outPin = 7;
const int sensorPin = A0; 
const int led_output=13;
const int voltagelimitdelay=5; //failsafe is activated if voltage measurement is 5 times below limit (~5*10ms=50ms delay)
const int ppmlossdelay=5; //failsafe is activated if ppm signal is not detected for more than 5 loops (~5*10ms=50ms delay)
const int maindelay=10; //delay of main loop in ms
const int rxreadydelay=1000; // wait for RX to get ready in ms
const int blinkinterval=50; // LED on time during blink as multiply of maindelay
const int blinkpause=200; // pause between blink series as multiply of maindelay
const int servo1min=1000; //limit of outputsignal
const int servo1max=2000; //limit of outputsignal
const int servoneutral=1500; //neutral postion of pistole transmitter
const int servo1validmin=800; //limit for plausibility, failsafe if beyond
const int servo1validmax=2200; //limit for plausibility, failsafe if beyond
const int voltagelimit=4500; //4500=4.5V threshold for faisafe

const int failsafe_pos=1050; //output position in case of failsafe

int servoout=failsafe_pos;
int voltagelimitdelaycounter=voltagelimitdelay; 
int ppmlossdelaycounter=ppmlossdelay;
int failsafewasactive=0;
int blinkcounter=blinkinterval;
int blinkstatus=0;
boolean redledstatus=false; 
boolean failsafeindicator=false;

const unsigned long delaylimit=100000; //100.000=0.1s failsafe in case of PPM signal loss
unsigned long delaymax=0;
unsigned long delaycurrent=0;
volatile unsigned long prev_time=0;
unsigned long prev_time_temp=0;
unsigned long micros_temp=0;
volatile unsigned long servoch1inraw=0;
int servoch1in=0;
long supplyvol=0;  

void setup() {
  analogReference(INTERNAL); //1.1V reference
  
  #ifdef DEBUG 
  //Initialize serial and wait for port to open:
  Serial.begin(9600); 
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  #endif
  
  pinMode(servoch1inPin, INPUT_PULLUP); 
  pinMode(sensorPin, INPUT);
  pinMode(led_output, OUTPUT);
  // when pin D2 (=0) goes high, call the rising function
  attachInterrupt(0, rising, RISING);
  
  escout.attach(servoch1outPin);  // attaches the servo on pin XX to the servo object 
  escout.writeMicroseconds(failsafe_pos);              // outout set to failsafe position 
  digitalWrite(led_output,HIGH);
  delay(rxreadydelay);   //wait for RX to get ready
  digitalWrite(led_output,LOW);
}


void loop() {
  
  //skip invalid input reads
  if ((servoch1inraw <servo1validmin)||(servoch1inraw >servo1validmax))
  {
    servoch1in=failsafe_pos;
    }
    else
    {
      servoch1in=(int)servoch1inraw;
    }
   
 
 #ifdef PISTOLSTICK 
   servoch1in=map(servoch1in,servoneutral,servo1max,servo1min,servo1max);
 #endif
 
  //avoid PPM signal limits 
  if (servoch1in>=servo1max)
    {
    servoch1in=servo1max;
    }
    
  if (servoch1in<=servo1min)
    {
    servoch1in=servo1min;
    }
  
  servoout=servoch1in; //regular handover of ESC control signal
  
  supplyvol = analogRead(sensorPin); //read supply voltage
  
  #ifdef V6INPUT
    supplyvol = map(supplyvol, 0, 1023, 0, 7205); //correct for 12k/68k voltage divider
  //supplyvol*1100/1023*(655)/100; 
  #endif
  
  #ifdef V8INPUT 
    supplyvol = map(supplyvol, 0, 1023, 0, 8159); //correct for 10k/68k voltage divider
  #endif
  
  //voltage check
  if (supplyvol<=voltagelimit)
  {
    voltagelimitdelaycounter=voltagelimitdelaycounter-1; //voltage too low, decrease counter
  }
  else
 {
    voltagelimitdelaycounter=voltagelimitdelay; //single measurement of sufficient voltage resets counter
  }
  
    if (voltagelimitdelaycounter<=0) //if counter approaches zero then activate failsafe
  {
    voltagelimitdelaycounter=0;
    
    #ifdef FAILSAFEVOL
    servoout=failsafe_pos;
    failsafewasactive=2; //second kind of failsafe for blink code
    #endif
    
    #ifdef DEBUGVOL
    Serial.print("time: ");
    Serial.println(micros());
    Serial.print("Vol: ");
    Serial.println(supplyvol);
    #endif
   }
  
  
  prev_time_temp=prev_time; //to avoid overwrite by interrupt
  micros_temp=micros();
  delaycurrent=(unsigned long)(micros_temp-prev_time_temp);
  
  if (delaycurrent>delaymax)
  {
    delaymax=delaycurrent; //store max delay for debug purpose
  }
   
  //PPM check
  if (delaycurrent>=delaylimit)
  {
    ppmlossdelaycounter=ppmlossdelaycounter-1; //delay too low, decrease counter
  }
  else
 {
    ppmlossdelaycounter=ppmlossdelay; //single good PPM signal resets counter
  } 
        
  //PPM signal loss check
   if (ppmlossdelaycounter<=0)
  {
    ppmlossdelaycounter=0;
    
    #ifdef FAILSAFEPPM
    servoout=failsafe_pos;
    failsafewasactive=1; //first kind of failsafe for blink code
    #endif
    
    #ifdef DEBUGPPM
    Serial.print("time: ");
    Serial.println(micros());
    Serial.print("micros_temp: ");
    Serial.println(micros_temp);
    Serial.print("prev_time_temp: ");
    Serial.println(prev_time_temp);
    Serial.print("delaycurrent: ");
    Serial.println(delaycurrent);
    #endif
  }
  
  #ifdef DEBUGFULL
  //debug output servo position
  Serial.print("In raw: ");
  Serial.println(servoch1inraw);
  Serial.print("In: ");
  Serial.println(servoch1in);
  Serial.print("Out: ");
  Serial.println(servoout);
  Serial.print("Vol: ");
  Serial.println(supplyvol);
  Serial.print("delaymax: ");
  Serial.println(delaymax);
  #endif

  #ifdef BLINKON
  //realize blink code
  if (blinkstatus>0)
  {
     blinkcounter=blinkcounter-1;
     if (blinkcounter<=0)
      {
      blinkcounter=blinkinterval;
      redledstatus=!redledstatus;
      digitalWrite(led_output,redledstatus);
      blinkstatus=blinkstatus-1;
      }
  }
  else
  {
    blinkstatus=failsafewasactive*2;
    blinkcounter=blinkpause; //define pause between blink codes
  }
  
  //indicate failsafe position, overwrites blink code
   if (servoout<=failsafe_pos)
  {
    digitalWrite(led_output,HIGH);
    failsafeindicator=true;
  }
  else
  {
    if (failsafeindicator==true)
    {
        digitalWrite(led_output,LOW);
        failsafeindicator=false;
    }    
    
  }
  #endif
  
  //set ESC output
  escout.writeMicroseconds(servoout);          
   
  delay(maindelay);
}


void rising() {
  attachInterrupt(0, falling, FALLING);
  prev_time = micros();
  }
 
void falling() {
  attachInterrupt(0, rising, RISING);
  servoch1inraw = (unsigned long)(micros()-prev_time);
  }
