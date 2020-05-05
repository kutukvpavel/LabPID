
/*  Note. The code below has been edited many times since first introducing comments to it, so the latter may be not super-informative and beautiful now, I didn't bother much to correct them
 *   before device development is completed.
 *  It still requieres some refactoring and formatting. I wish arduino IDE aupported #region directives, because sometimes code gets completely unreadable
 *  because of huge pieces being placed between various linked routines.
 *  
 *  NB! Some of the libraries used (e.g. PID) have been modified! The code is valid only if modified libs are used.
 *  
 *  Current major TODO:
 *  Consider various error-handling techniques (e.g. for broken thermocouple etc).
 *  _____________________________________
 *  Developed by Kutukov Pavel, 2016.
 *  This software is provided as-is and the author is not responsible for any damage caused by this software. I.e. use it at your own risk.
 *  Everyone is free to make changes to this software and adjust it according to one's needs as long as this copyright note is kept intact.
 *  Commercial use of this software is forbidden unless you have a written permission of the original developer.
 */
 
#include <max6675.h>  //Thermocouple
#include <SPI.h>
#include <OneWire.h>  //DS18B20
#include <DallasTemperature.h>
#include <LiquidCrystal.h>  //LCD
#include <MsTimer2.h> //Timers
#include <TimerOne.h>
#include <ClickEncoder.h> //Encoder
#include <PID_v1.h> //PID
#include <EEPROM.h> //Misc
#include <Average.h>

//Defenitions of constants
//#define DEBUG                           //Debug info enable
//#define NO_CJC  -- Deprecated! [No Cold Junction Compensation (i.e. no ds18b20)]

#define FIRMWARE_VERSION "1a"
#define INFO "Lab temperature control system controller."

#define PIN_GPIO 11                     //General purpose IO pin
#define PIN_INPUT A7                    //ADC input
#define TIMING 1000                     //Timing for PID in milliseconds
#define PWM_PIN 9                       //Pin for PWM PID output
#define ENCODER_PIN1 2                  //Encoder pins
#define ENCODER_PIN2 4
#define BUTTON_PIN 3
#define RW_PIN A2                       //LCD pins
#define RS_PIN A1
#define E_PIN A0
#define D4_PIN 8
#define D5_PIN 7
#define D6_PIN 6
#define D7_PIN 5
#define LBL_LEN 6 //"123.45"            //Display configuration
#define LBLM_LEN 2 //"N0"
#define DECIMAL_PLACES 3//"123"         //Index of the last characted in decimal part or length of decimal part of setpoint and input (interchangable due to comma presence)
#define LCD_X 16
#define LCD_Y 2
#define INIT_TK 0.3                     //Factory-default temperature coeff. for external amplifier
#define ONE_WIRE_BUS A3                 //Onewire bus pin
#define SS 10
#define EEPROM_START 1                  //EEPROM virtual start address
#define ERROR_TIMEOUT 3                 //Number of seconds to wait (re-checking the conditions) before showing error status
#define PWM_MAX 1023

#define SETPOINT_ADDR sAddr
#define K_ADDR SETPOINT_ADDR + sizeof(Setpoint)
#define COEFF_ADDR K_ADDR + sizeof(K)
#define ITHRESHOLD_ADDR COEFF_ADDR + sizeof(TK)
#define CHANNEL_ADDR ITHRESHOLD_ADDR + sizeof(Threshold)
#define AVERAGING_ADDR CHANNEL_ADDR + sizeof(imode)
#define DSINDEX_ADDR AVERAGING_ADDR + sizeof(averaging)
#define DSDEFAULT_ADDR DSINDEX_ADDR + sizeof(address)
#define MODE_ADDR DSDEFAULT_ADDR + sizeof(edsTemp)
#define CAL_ADDR MODE_ADDR + sizeof(aggr)
#define BIAS_ADDR CAL_ADDR + sizeof(calibration)
#define DOM_ADDR BIAS_ADDR + sizeof(bias)
#define RAMP_ADDR DOM_ADDR + sizeof(dom)
#define GPIO_ADDR = RAMP_ADDR + sizeof(ramp)

//Global variables
//The quantity of global vars probably is more than it could be (some could have been transfered to local scope), 
//  but it's easier to esteemate required space using arduino IDE if most of the vars are global. 
//Also, it seems the programm has plenty of RAM left even with vars being organised in the way they are now, so nobody cares.
//And one more point: I know that short and int (along with double and float) are the same for AVR (but it's a pity they are), therefore, there's no technical difference between
//  variables declared below as short and as int, but, just for a good measure (and thanks to a habit) I've decided to introduce both types depending on how big the stored number can be. 
//Also [2], some names should be changed to more representative ones in view of variable usage (for example: imode and aggr have changed their usage and now they should be called "ichannel" and "rmode" respectively).
//  They are not renamed yet due to poor IDE's refactoring capabilities.
char mn=0;                                                            // Menu state flag: 0 - setpoint, 1 - mode, 2 - calibration (input field), 3 - power (for manual mode)
char imode=0x00;                                                      // Current input used: 0 - ADC, 1 - SPI, 2 - DS18B20
char imodeBcp=0xff;                                                   // Backup used to check for changes, thus determinig if display update is needed
char aggr=0x00;                                                       // Regulation mode flag: 0 - normal, 1 - aggressive, 2 - distillation, 3 - manual
char aggrBcp=0xff;                                                    // Backup again...
char b[35];                                                           // Buffer for final log string
char st2[5][7];                                                       // Buffers for log's strings
char str_temp[7];                                                     // Buffer for LCD strings
unsigned char dsaddr[2][8];                                           // Address temporary storage, introduced to try to speed up onewire-related actions. 0 - ambient, 1 - channel
unsigned char s[64];                                                  // Virtual input buffer for serial interface
unsigned char gpioMode=0;
double setBcp=-1;                                                     // Backup needed for display update necessity check...
double inputBcp=-1;                                                   //  Same, more to come...
double Setpoint, Input, Output;                                       // PID main values
double Threshold=0.35;                                                // Integral term bound (in percents)
double K[][2]={{18, 20},{0.006,0.008},{15,14}};                       // Current regulation constants
double TK=INIT_TK;                                                    // Coefficient used for working with external amplifier
double calibration[3]={0,0,0};                                        // Input calibration values
double bias=0;                                                          // Bias for distillation
double dstemp=0;                                                       // Current DS18B20 (ambient) temperature
double edsTemp=25.0f;                                                  // Default ambient temperature
double dom=5;                                                          // Tolerated setpoint overshoot in D-mode
double dstempc=0;                                                      // Current DS18B20 (channel) temperature
double ramp=2;
const double mult[]={100,10,1,0.1,0.01,0.001,0.0001};                 // Multipliers for building up numbers
short address[2]={0,1};                                                 // DS18B20 indexes. 0 - ambient, 1 - channel
short pwr=0;                                                            // Current power value
short pwrBcp=-1;                                                        // Similar backup for power value
short x=0;                                                              // Screen X coordinate0
short y=0;                                                              // Screen Y coordinate (seems it never actually changes)
short cur=0;                                                            // Current cursor type
short i=0;                                                            // Index used for serial data transfering to virtual buffer (s)
short dl=0;                                                           // Delay before error status (for the device not to lock when lowering setpoint manually)
const short sAddr=EEPROM_START;                                         // Starting EEPROM address  TODO: consider changing it to dynamic var
int v=0;                                                              // Encoder raw value
int ct=0;                                                             // Virtual timer counter (750ms)
int ctn=0;                                                            // Virtual timer counter (500ms)
int ctnd=0;                                                           // Virtual timer counter (display)
int sz=0;                                                             // EEPROM address buffer
int16_t last, value;                                                  // Encoder values
bool averaging[]={1,1,0};                                             // Averaging state array: {mode 0, mode 1, mode 2}
volatile bool blankBcp=0;                                                      // Backup used to determine if some fields are already blanked out and no update is needed (menu in mode #2)
bool logging[]={0,0};                                                 // Logging enable flags
bool disp = 1;                                                        // Flag for display update enable
bool condition[4]={0,0,0,0};                                          // Flags for error states behavior control {Safety error, DS chain: <= 1 sensor, DS chain: 0 sensors, DS chain changed}
bool readready=1;                                                     // Flag of readiness for reading (for MAX6675 proper timing: >400ms)
ClickEncoder *encoder;                                                // Encoder object
ClickEncoder::Button button;                                          // Encoder's button  
LiquidCrystal lcd(RS_PIN, E_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);     // LCD Object 
PID myPID(&Input, &Output, &Setpoint, K[0][0], K[1][0], K[2][0], DIRECT);    // PID Object
MAX6675 thermocouple;                                  // MAX6675 thermocouple amplifier object
Average<float> ave(3);                                                // Averaging container
OneWire oneWire(ONE_WIRE_BUS);                                        // Onewire bus object
DallasTemperature sensors(&oneWire);                                  // DS18B20 Object

void display(void);   // Declared here just not to move this long function upper

template<typename T> void mem(T *variable, bool wrt = false)    //Function template for working with EEPROM memory
{
  if(wrt) { EEPROM.put(sz, *variable); }
  else { EEPROM.get(sz, *variable); };
  sz+=sizeof(*variable);
}

char ConvertAggr(char src)  //Routine for converting user-friendly into system-recognized values of current mode (var. "aggr") and vice-versa
{
  switch(src)
  {
    case 'N':
      return 0x00;
    case 'A':
      return 0x01;
    case 'D':
      return 0x02;
    case 'M':
      return 0x03;
    case 0x00:
      return 'N';
    case 0x01:
      return 'A';
    case 0x02:
      return 'D';
    case 0x03:
      return 'M';
    default:
      return 0xff;
  }
}

void setup() {
  asm("cli");   // No interrupts while we aren't ready yet
  // put your setup code here, to run once:
  pinMode(RW_PIN, OUTPUT);                                // LCD & PWM stuff
  digitalWrite(RW_PIN, LOW);
  lcd.begin(LCD_X, LCD_Y);
  lcd.clear();
  lcd.home();
  lcd.print("Booting...");
  Serial.begin(9600);                                     // Initialize serial communications with the PC
  SPI.begin();                                            // Init hardware spi for max6675-compatible devices
  //int j;
  char t=0;
  sz = sAddr;                                             // Put 
  pinMode(BUTTON_PIN, INPUT_PULLUP);                          //Provide ability to reset to factory defaults by holding encoder's button while booting up
  t=EEPROM.read(0);                                           //Check if this is the first run and we'll have to fill the EEPROM up too
  if((t != 0x01)||(!digitalRead(BUTTON_PIN)))
  {
    lcd.home();
    lcd.print("Resetting...");
    
    for (int i = 0 ; i < EEPROM.length() ; i++)               //Clear EEPROM first
    {
      EEPROM.write(i, 0);
    }
    
    Setpoint = 18.0f;
    
    mem(&Setpoint, 1);
    mem(&K, 1);
    mem(&TK, 1);
    mem(&Threshold, 1);
    mem(&imode, 1);
    mem(&averaging, 1);
    mem(&address, 1);
    mem(&edsTemp, 1);
    mem(&aggr, 1);
    mem(&calibration, 1);
    mem(&bias, 1);
    mem(&dom, 1);
    mem(&ramp, 1);
    mem(&gpioMode, 1);
    
    EEPROM.write(0, 0x01);                                //Set the 'filled' flag on
    sz = sAddr;
    
#ifdef DEBUG
    Serial.println("EEPROM refilled.");
#endif
  }
  lcd.home();
  lcd.print("Booting...");
  //pinMode(BUTTON_PIN, INPUT);                             // Reset button pin to default state for further work
  analogReference(EXTERNAL);                              // Initialize ADC stuff
  pinMode(PIN_INPUT, INPUT);
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW);

  mem(&Setpoint, 0);                                      //Get all of the saved parameters loaded into memory
  mem(&K, 0);
  mem(&TK, 0);
  mem(&Threshold, 0);
  mem(&imode, 0);
  mem(&averaging, 0);
  mem(&address, 0);
  mem(&edsTemp, 0);
  mem(&aggr, 0);
  mem(&calibration, 0);
  mem(&bias, 0);
  mem(&dom, 0);
  mem(&ramp, 0);
  mem(&gpioMode, 0);

  pinMode(GPIO_PIN, 
  
#ifdef DEBUG
  cfOut();
#endif
  myPID.SetSampleTime(TIMING);                          //Time-related stuff init
  myPID.SetOutputLimits(0,PWM_MAX);
  unsigned long l = (unsigned long)TIMING;
  l=l*1000;
  Timer1.initialize(l);
  //Timer1.attachInterrupt(isr1);
  Timer1.pwm(PWM_PIN, 0);
  MsTimer2::set(1, isr2);
  MsTimer2::start();
  encoder = new ClickEncoder(ENCODER_PIN1, ENCODER_PIN2, BUTTON_PIN, 4, 0);   //Encoder init
  //turn the PID on
  myPID.SetMode(AUTOMATIC);
  thermocouple.begin(SS);
#ifndef NO_CJC
  setup_ds();                                          //Setup onewire sensors
#endif                                          
  last=-1;                                             //Set last encoder's value out of normal range to force update
  lcd.clear();
  lcd.print("S:");                                       //Print basic symbols on the screen
  lcd.setCursor(0,1);
  lcd.print("R:");
  lcd.setCursor(10,1);
  lcd.print("P:");
  lcd.setCursor(10,0);
  lcd.print("M:");
  asm("sei");                                         // Remember to turn the interrupts back on
}

void setup_ds()
{
  sensors.begin();                            //Init onewire bus and sensors
  condition[1]=0;                             //Defaulting, reqired for further calls
  int dsc = sensors.getDeviceCount();         //Get the number of sensors
  if(dsc)   //Warn in case of one or more sensors aren't present
  {
    if(dsc > 1)
    {
      for(uint8_t ia = 0; ia < 2; ia++)
      {
        if(dsc <= address[ia])
        {
          condition[3] = 1; //Raise appropriate flag if we've to rearrange the EEPROM'ed chain structure
          address[ia] = dsc;  //Set the out-of-bounds-sensor index to last+1 and decrement it until we reach a free spot
          bool moveFurther = true;
          while(moveFurther)
          {
            address[ia] -= 1;
            moveFurther = false;
            for(uint8_t ja = 0; ja < 2; ja++)
            {
              if(ja == ia) continue;
              moveFurther = moveFurther || (address[ja] == address[ia]);
            }
          }
        }
        sensors.getAddress(dsaddr[ia], address[ia]);      //If everything's OK then load addresses (to make further communication processes faster) and send request to start the conversion
      }
    }
    else 
    {
      condition[1] = 1;
      sensors.getAddress(dsaddr[1], address[1]); //Treat single sensor as a channel (not as ambient)
    }
    sensors.requestTemperatures();
  }
  else
  {
    condition[1] = 1;
    condition[2] = 1;
  }
}

ISR(TIMER1_OVF_vect)                               // 1 sec timer routine
{
  myPID.Compute();                            // For PID computations, safety check and output updating
  safety_check();
  Timer1.setPwmDuty(PWM_PIN, Output);
  logging[1]=true;                            //Also enables logging flag for the log to be periodically created (with 1 sec period, of course)
}

void isr2(void)                               // 1 ms timer routine
{
  encoder->service();                         // For encoder poll
  if(ctnd>199)
  {
    display();                          // Display update logic
    disp=1;
    ctnd=0;
  }
  if(ctn>499)                          // Timer for max6675 thermocouple interface polling (500ms)
  {
    readready=1;
    ctn=0;
  }
  ct++;                                       //Onewire poll timer
  ctn++;                                      //max6675 timer
  ctnd++;                                     //Display timer
}

void cfOut(void)                              //Outputs all the current technical data into terminal connection
{                                             //Shitty written, I could have used formatted print and cycles, but who cares about a routine used only several times during a run.
  Serial.println(">D:");
  Serial.print(Input);Serial.print(" ");
  Serial.print(dstemp);Serial.print(" ");
  Serial.print(ConvertAggr(aggr));Serial.println((int)imode);
  Serial.print(K[0][0], 4);Serial.print(" ");
  Serial.print(K[1][0], 4);Serial.print(" ");
  Serial.println(K[2][0], 4);
  Serial.print(K[0][1], 4);Serial.print(" ");
  Serial.print(K[1][1], 4);Serial.print(" ");
  Serial.println(K[2][1], 4);
  Serial.print(Threshold, 4);Serial.print(" ");
  Serial.print(bias, 4);Serial.print(" ");
  Serial.println(TK, 4);
  Serial.print(calibration[0], 4);Serial.print(" ");
  Serial.print(calibration[1], 4);Serial.print(" ");
  Serial.println(calibration[2], 4);
  Serial.print(averaging[0]);Serial.print(" ");
  Serial.print(averaging[1]);Serial.print(" ");
  Serial.println(averaging[2]);
  Serial.print(Setpoint);Serial.print(" ");
  Serial.print(edsTemp);Serial.print(" ");
  Serial.println(dom);
  Serial.print(ramp);Serial.print(" ");
  Serial.print(
}

void setcursor()                              // Cursor modes switching
{
    switch (cur)                              // 0 - disabled, 1 - underlining, 2 - underlining + blinking
    {
      case 0:
        lcd.noBlink();      
        lcd.noCursor();
        break;
      case 1:
        lcd.noBlink();                        // Don't forget to turn previous mode off, because it isn't reset automatically
        lcd.cursor();
        break;
      case 2:
        lcd.blink();
    }
}

void print_double(double dbl, int x, int y)   //Incapsulated for convinience, prints a double on the screen at specified coordinates
{
  dtostrf(dbl, 6, 2, str_temp);
  sprintf(b, "%s", str_temp);
  lcd.setCursor(x,y);
  lcd.print(b);
}

void pwrprint()
{
  sprintf(b, "%3i%%", pwr);
  pwrBcp=pwr;
  lcd.setCursor(12, 1);
  lcd.print(b);
}
void checkpwr()
{
  if(pwr < 0)
  {
    pwr = 0;
  }
  else
  {
    if(pwr > 100) { pwr = 100; };
  }
}

void display(void)                              // Main routine for display logic
{
  v = encoder->getValue();                  // Get encoder and button value/state
  value += v;
  button = encoder->getButton();
  if (button != ClickEncoder::Open) {          // Button handling
#ifdef DEBUG
  Serial.print("Button: ");
  Serial.println(button);
#endif
    switch (button) {
      case ClickEncoder::Clicked:             // Clicked
        switch (cur)
        {
          case 1:                             // If underlining cursor is on then switch to the blinking one
            cur=2;
            break;
          case 2:
            cur=1;                            // If blinking is on, then do vice-versa
            break;
          default: break;
        }
        break;
      case ClickEncoder::DoubleClicked:       // Double-clicked
        if(cur>0)                             // Cycle: Off -> setpoint field -> mode field -> input field -> Off
        {
          if(((mn > 1)&&(aggr != 0x03))||(mn > 2))
          {
            cur=0;
            mn=0;
          }
          else
          {
            mn++;
          }
        }
        else
        {
          cur=1;
        }
        break;
      default:
        break;
    }
  }                           
  if(value != last)                         // Check if anything changed since last cycle
  {
#ifdef DEBUG
  Serial.println("Encoder!");
#endif
    int bound;                              // Choose bounds: 6 chars for setpoint field, 2 characters for mode field, 6 characters for input field, 3 characters for power field
    switch(mn)
    {
      case 0x00:
        bound = LBL_LEN;
        break;
      case 0x01:
        bound = LBLM_LEN;
        break;
      case 0x02:
        bound = LBL_LEN;
        break;
      case 0x03:
        bound = 3;
      default : break;
    }
    switch(cur)                             // Determine what to do with the values
    {
      case 1:                               // 1 - Move cursor
        x = abs(value % bound); //TODO: make cursor movement more stable, there's definitely something wrong here
        if(x==3)                            // Skip the comma in setpoint field
        {
          (last<value) ? x++ : x--;
        }
        break;
      case 2:                            // 2 - Change value
        int k = x;
        if(x>DECIMAL_PLACES)             // Fix skipped comma index incrementation (in order to fit into multipliers array's bounds)
        {
          k--;
        }
        switch(mn)                      // Determine what value to change
        {
          case 0x00:                    // 0 - setpoint
            Setpoint+=mult[k]*v;        // Change setpoint using array of multipliers chosen according to the cursor index
            break;
          case 0x01:                    // 1 - mode
            switch(x)
            {
              case 0:                     // Scroll between N, A, D and M
                aggr = (char)abs(((int)aggr + v) % 4);
                break;
              case 1:                     // Scroll between 0 and 2
                imode = (char)abs(((int)imode + v) % 3);
                break;
              default:
                break;
            }
            break;
          case 0x02:                      // 2 - calibration
            calibration[(int)imode]+=mult[k]*v; //Similar to setpoint changement
            break;
          case 0x03:                      // 3 - power
            pwr += mult[k]*v;
            checkpwr();
            break;
          default : break;
        }
        break;
    }
    last=value;                       // Update backed up value
  }
}

void curpos()                             // Setting the cursor position, including shift chosen according to the field being edited
{
  y = ((mn==0x02)||(mn==0x03)) ? 1 : 0;
  short margin;
  if(mn==0x01) { margin = 13; }
  else
  {
    if(mn==0x03) { margin = 12; }
    else { margin = 2; };
  };
  if((mn==0x01)&&(x>2)) { x=0; };         // A workaround for mode field
  lcd.setCursor(x + margin, y);
}

double convert()                      // Putting all converted characters together into a double, again using our multipliers array
{
  double t=0;
  short j;
  for(j=2;j<9;j++)    //TODO: Add some logic to check for garbage presence
  {
    t+=(s[j]-'0')*mult[j-2];          //A neat way to convert char to corresponding int, possible due to numbers being consequent in ASCII (starting at '0'==48)
  }
  if(s[1]=='-') {t=-t;};
  return t;
}

void cmd(short cnt)                       // Handling PC commands, general packet structure: {[0..9],{A,T,P,I,D,C,S}}{+,-}(000.0000){CR+LF,{}}
{
  switch (s[0])
  {
    case '0': 
      Setpoint=convert();               // 0 - basic setpoint changing
#ifdef DEBUG
      Serial.println(Setpoint);
#endif
      break;
    case '1': 
      K[0][0]=convert();                // 1 - First coeff. (P)
      EEPROM.put(sAddr+sizeof(Setpoint), K);
#ifdef DEBUG
      EEPROM.get(sAddr+sizeof(Setpoint), K);
      Serial.println(K[0][0], 5);
#endif
      break;
    case '2':
      K[1][0]=convert();                // 2 - Second coeff. (I)
      EEPROM.put(sAddr+sizeof(Setpoint), K);
#ifdef DEBUG
      Serial.println(K[1][0], 5);
#endif
      break;
    case '3': 
      K[2][0]=convert();                // 3 - Third coeff. (D)
      EEPROM.put(sAddr+sizeof(Setpoint), K);
#ifdef DEBUG
      Serial.println(K[2][0], 5);
#endif
      break;
    case '9':                           // 9 - Log
      logging[0] = (convert() > 0);
#ifdef DEBUG
      Serial.println(logging[0]);
#endif
      break;
    case '8':                           // 8 - Info
      cfOut();
      break;
    case '4':
      TK=convert();                     // 4 - external amplifier coeffecient
      EEPROM.put(sAddr+sizeof(Setpoint)+sizeof(K), TK);
#ifdef DEBUG
      Serial.println(TK, 4);
#endif   
      break;   
    case '5':
      Threshold=convert();                // 5 - integral cut-off threshold
      EEPROM.put(sAddr+sizeof(Setpoint)+sizeof(K)+sizeof(TK), Threshold);
#ifdef DEBUG
      Serial.println(Threshold, 4);
#endif   
      break;
    case '7':                                 // 7 - Input channel
      imode=(char)((int)convert());
#ifdef DEBUG
      Serial.println((int)imode);
#endif
      break;
    case '6':                                 // 6 - Averaging enable/disable for current mode
      averaging[(int)imode]=(convert()>0);
      EEPROM.put(sAddr+sizeof(Setpoint)+sizeof(K)+sizeof(TK)+sizeof(Threshold)+sizeof(imode), averaging);
#ifdef DEBUG
      Serial.print(averaging[0]);Serial.print(" ");
      Serial.print(averaging[1]);Serial.print(" ");
      Serial.println(averaging[2]);
#endif 
      break;   
    case 'T':                                  // T - Default ambient temperature
      edsTemp = (float)convert();  
      EEPROM.put(sAddr+sizeof(Setpoint)+sizeof(K)+sizeof(TK)+sizeof(Threshold)+sizeof(imode)+sizeof(averaging)+sizeof(address), edsTemp);
#ifdef DEBUG
      Serial.println(edsTemp);
#endif      
      break;
    case 'A':                                 // A - regulation mode (N,A,D,M -> 0..3)
      aggr = (char)((int)convert());
#ifdef DEBUG
      Serial.println(ConvertAggr(aggr));
#endif  
      break;
      case 'P': 
      K[0][1]=convert();                      // P - First aggressive coeff. (Pa)
      EEPROM.put(sAddr+sizeof(Setpoint), K);
#ifdef DEBUG
      Serial.println(K[0][1], 5);
#endif
      break;
    case 'I':
      K[1][1]=convert();                      // I - Second arggessive coeff. (Ia)
      EEPROM.put(sAddr+sizeof(Setpoint), K);
#ifdef DEBUG
      Serial.println(K[1][1], 5);
#endif
      break;
    case 'D': 
      K[2][1]=convert();                      // D - Third aggressive coeff. (Da)
      EEPROM.put(sAddr+sizeof(Setpoint), K);
#ifdef DEBUG
      Serial.println(K[2][1], 5);
#endif
      break;
    case 'S':                                 // S - sensor address (by current index)
    {
      address[0] = (int)(s[2] - '0');
      address[1] = (int)(s[3] - '0');
      EEPROM.put(sAddr+sizeof(Setpoint)+sizeof(K)+sizeof(TK)+sizeof(Threshold)+sizeof(imode)+sizeof(averaging), address);
#ifdef DEBUG
      Serial.print(address[0]);Serial.print(" ");Serial.println(address[1]);
#endif     
      break;
    }
    case 'C':                                 // C - current calibration coef
      calibration[(int)imode]=convert();
#ifdef DEBUG
      Serial.print(calibration[0], 5);Serial.print(" ");
      Serial.print(calibration[1], 5);Serial.print(" ");
      Serial.println(calibration[2], 5);
#endif
      break;
    case 'V':
      Serial.println(">I:");
      Serial.print("Firmware version: ");Serial.println(FIRMWARE_VERSION);
      Serial.print("Additional info: ");Serial.println(INFO);
      break;
    case 'B':                                         // B - distillation bias
      bias = convert();
      EEPROM.put(sAddr+sizeof(Setpoint)+sizeof(K)+sizeof(TK)+sizeof(Threshold)+sizeof(imode)+sizeof(averaging)+sizeof(address)+sizeof(edsTemp)+sizeof(aggr)+sizeof(calibration), bias);
#ifdef DEBUG
      Serial.println(bias, 4);
#endif
      break;
    case 'L':                                           // S - current sensors' addresses
      Serial.println(">S:");
      for(short k = 0; k < 2; k++)
      {
        for(short j = 0; j < 8; j++)
        {
          Serial.print(dsaddr[k][j]);
        }
        Serial.print(k>0 ? "\r\n" : " ");
      }
      break;
    case 'W':                                     // W - set power (only for manual mode)
      if(aggr!=0x03) { break; }                    
      pwr = (int)convert();
      checkpwr();
      break;
    case 'O':                                         // O - tolerated distillation overshoot
      dom = abs(convert());
      EEPROM.put(sAddr+sizeof(Setpoint)+sizeof(K)+sizeof(TK)+sizeof(Threshold)+sizeof(imode)+sizeof(averaging)+sizeof(address)+sizeof(edsTemp)+sizeof(aggr)+sizeof(calibration)+sizeof(bias), dom);
#ifdef DEBUG
      Serial.println(dom, 2);
#endif
      break;
    case 'R':
      ramp = convert();
      EEPROM.put(RAMP_ADDR, ramp);
#ifdef DEBUG
      Serial.println(ramp, 2);
#endif
      break;
    default : break;
  }
}

void serial()
{
  if (Serial.available()>0)                 //Update serial data
  {
      s[i]=(unsigned char)Serial.read();
      i++;
  }
  else
  {
      if (i>0)
      {  
        cmd(++i);                         //Run cmd depending on recieved data (actually 'i' is not used)
        i=0;
      };
  };
}

void safety_check()                 // Check if something goes wrong (real temp rises more than 20C above the setpoint, ambient temperature rises above 60C, ds18b20 failed, no ds18b20 when using mode #2, broken thermocouple)
{
  condition[0] = (((Input-Setpoint > 20)&&(Output > 0))||(dstemp > 60)||(dstemp < -25)||(dstempc < -25)||(condition[2]&&(imode==0x02))||(!(Input==Input)));
  if(condition[0]) 
  {
    if(dl>ERROR_TIMEOUT)
    {
      if(myPID.GetMode() == AUTOMATIC)
      {
        myPID.SetMode(MANUAL);
        Serial.println(">E!");
      }
      Output = 0;
    }
    else
    {
      dl++;
    }
  }
  else
  {
    if((myPID.GetMode() == MANUAL)&&(aggr != 0x03))             // Return to normal operation if the problem's solved (and if automatic mode is enabled)
    {
      myPID.SetMode(AUTOMATIC);
      dl=0;
    }
  }
}

void read_input()                           //Update input according to the mode being used
{
  double in=0;
  switch(imode)
  {
    case 0x00:                              // 0 - external amplifier (adc input)
      in = analogRead(PIN_INPUT)*TK + dstemp; 
      break;
    case 0x01:                              // 1 - max6675 (spi)
      in = thermocouple.readCelsius();
      break;
    case 0x02:                              // 2 - ds18b20  (1-wire)
      in = dstempc;   // Updated in "readds"
      break;
    default:
      break;
  }
  in += calibration[(int)imode];          // Apply calibration value
  if(averaging[(int)imode])
  {
    ave.push((float)in);                           // Apply averaging if enabled for current mode
    Input = ave.mean();
  }
  else
  {
    Input = in;
  }
}

void readds()         // One-wire devices poll routine
{
    if(cur==0)       // And for polling sensors (worst-case period of convertion: 750ms, may actually reach 600-650ms),
    {                                           // don't poll when cursor is on (for the interface not to freeze)
      if(condition[2] || condition[1])                          //If there are no sensors then set default ambient temperature
      {
        dstemp = edsTemp;
      }
      else
      {
        dstemp = sensors.getTempC(dsaddr[0]);
      }
      if(!condition[2])
      {
        if(imode==0x02)                           // Poll second sensor only in mode 2
        {
          dstempc = sensors.getTempC(dsaddr[1]);
        }
        sensors.requestTemperatures();
      }
      ct = 0;                                   //Don't forget to null out the virtual timer
    }
}

void loop() {
  // put your main code here, to run repeatedly:
  serial();                             // First, check for serial data
  switch(aggr)                              //Apply updated coefficients according to current mode of regulation (actual update process takes place in 'cmd' and 'convert')
  {
    case 0x01:
      myPID.SetTunings(K[0][1],K[1][1],K[2][1]);
      myPID.SetDistillBias(0);
      break;
    case 0x00:
      myPID.SetTunings(K[0][0],K[1][0],K[2][0]);
      myPID.SetDistillBias(0);
      break;
    case 0x02:
      myPID.SetTunings(K[0][1],K[1][1],K[2][1]);
      myPID.SetDistillBias((Input - Setpoint > dom) ? 0 : bias);  //Check for overshoot (for D-mode only)
      break;
    default : break;
  }
  myPID.SetMaxITerm(Threshold);
  myPID.SetRampLimit(ramp);
  if((cur>0)&&(imode==0x02))            //Disable PID regulating if input is not updated
  {
    myPID.SetMode(MANUAL);
  }
  else
  {
    if((myPID.GetMode()==MANUAL)&&(!condition[0])&&(aggr != 0x03)) { myPID.SetMode(AUTOMATIC); };   //Enable PID regulating if there no more problems
  }
  if(ct>700)                                    //Onewire communications
  {
    readds();
  }
  if(readready)                                 //Read inputs' values
  {
    read_input();
    readready=0;
  }
  if(aggr == 0x03)                                      //If manual mode is enabled then turn off the PID and count output value based on power value
  {
    if(myPID.GetMode() != MANUAL)
    {
      myPID.SetMode(MANUAL);
    }
    Output = (long)((PWM_MAX * (long)pwr) / 100);
  }
  else
  {
    pwr = (int)((Output * 100) / PWM_MAX);             //Else - count power value based on output
  }
  
  if(disp&&(ctnd<180))                        //Display logic (printing part, cursor part has been moved to ISR). This condition was intended to synchronize interrupt and main loop parts
  {                                                     //  for the cursor not to jump around occasionally. It works somehow. P.S. Formatting required.
    if(Setpoint != setBcp)                  // Searching for changed data and partially updating screen
  {
    setBcp=Setpoint;
    print_double(setBcp, 2, 0);
  }
  if((cur>0)&&(imode==0x02))              // Blank out input and power fields if menu is enabled in mode #2 (due to non-updating values)
  {
    if(!blankBcp)
    {
      if(mn!=0x02)                        // Don't blank out input field when user's editing calibration values
      {
        lcd.setCursor(2,1);
        lcd.print("      ");
      }
      if(aggr != 0x03)                    // If user is able to edit power (in manual mode) then don't blank it out (taking into account following actions this condition exists just to save some execution time)
      {
        lcd.setCursor(12,1);
        lcd.print("    ");
      }
      blankBcp=1;
    }
    if(aggr == 0x03) { pwrprint(); };     // always update power value in manual mode (otherwise it will appear to be stuck while in fact it will be changing)
  }
  else
  {
    if((Input != inputBcp)||blankBcp)   // Remeber to fill blanked fields
    {
      inputBcp=Input;
      print_double(inputBcp, 2, 1);
    }
    if((pwr != pwrBcp)||blankBcp)
    {
      pwrprint();
    }
    blankBcp=0;
  }
  if(condition[0]&&(dl>ERROR_TIMEOUT))                    // Major error sign - label "ERR!" in mode field
  {
    lcd.setCursor(12, 0);
    lcd.print("ERR!");
  }
  else
  {
    if((imode != imodeBcp)||(aggr != aggrBcp))             // Mode field structure (normal): "M: {N,A,D,M}[0..2]{!}"
    {
      sprintf(b, " %c%1i ", ConvertAggr(aggr), (int)imode);
      lcd.setCursor(12, 0);
      lcd.print(b);
      imodeBcp=imode;
      aggrBcp=aggr;
      if(!myPID.GetMode()==MANUAL) { myPID.SetMode(AUTOMATIC); }; //Reset PID to fix a strange bug causing power to stay at ~20% after channel selection no matter how input changes
    }
    if(condition[1] || condition[2] || condition[3])                  // Warning sign - exclamation mark in the right upper corner of the screen
    {
      lcd.setCursor(15,0);
      lcd.print("!");
    }
  } 
  curpos();                           // Move cursor back to it's position immediately to prevent jumping to the last filled field
  disp = 0;                           // Reset the flag
  }

  sz=sAddr;                           // Set memory pointer to start
  mem(&Setpoint, 1);                  // Load some parameters (which can be changed by means of display-encoder interface) into EEPROM
  sz+=sizeof(K)+sizeof(TK)+sizeof(Threshold); // Skip some variables manually
  mem(&imode, 1);
  sz+=sizeof(averaging)+sizeof(address)+sizeof(edsTemp);
  mem(&aggr, 1);
  mem(&calibration, 1);
         
  if(logging[0] && logging[1])        // Check whether logging is enabled it's time to send log info
  {
    dtostrf(setBcp, 6, 2, st2[0]);    //Build up the strings. TODO: consider improving execution speed.
    dtostrf(inputBcp, 6 ,2, st2[1]);
    dtostrf(dstemp, 6, 2, st2[2]);
    sprintf(st2[3], "%i", pwrBcp);
    sprintf(st2[4], "%c%1i", ConvertAggr(aggr), (int)imodeBcp); 
    sprintf(b, ">L: %s %s %s %s %s", st2[0], st2[1], st2[2], st2[3], st2[4]);   //Join them
    Serial.println(b);              //And print
    logging[1] = false;             //Reset the timing flag
    //delay(1);
  }
  else
  {
    delay(20);                        // Some delay if no long-time operations have been taking place
  }

  setcursor();                          // Incapsulated for convinience, changes cursor according to 'cur' volatile variable
  curpos();
}
