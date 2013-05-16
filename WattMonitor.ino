/* PedalPowerSystems.com Watt Monitor 
 * Written by:
 * Thomas Spellman <thomas@thosmos.com>
 * Copyright: 2013, Thomas Spellman (http://pedalpowersystems.com)
 * License: This code is distributed under the GPL license: http://www.gnu.org/licenses/gpl.html
 * 1.2: modified for ICOUNT number of inputs
 * 1.3: fixed pinAmp[] values
 * 1.4: minor code cleanup
 * 1.5: file name change
 * 1.6: display fix
 */

const char * VERSION = "1.6";

const float AVG_CYCLES = 20.0;
const unsigned int BLINK_INTERVAL = 1000;
const unsigned int DISPLAY_INTERVAL = 1000;
const unsigned int VOLT_INTERVAL = 10;
const unsigned int VOLT_TEST_INTERVAL = 1000;

const float VOLT_CUTOFF = 15.0;
const float VOLT_RECOVER = 14.5;
const byte ICOUNT = 6;

//IN
const byte pinVolt = 0;
const byte pinAmp[ICOUNT] = {1,2,3,4,5,6}; 

//OUT
const byte pinRelay = 2;
const byte pinLed = 13;

unsigned long time = 0;
unsigned long lastTime = 0;
unsigned long lastVolt = 0;
unsigned long lastVoltTest = 0;
unsigned long lastDisplay = 0;
unsigned long lastBlink = 0;

byte i = 0;
char in;
boolean enableAutoDisplay = false;
boolean isBlinking = false;
boolean isRelayOn = false;

unsigned int voltAdc = 0;
float voltAdcAvg = 0;

unsigned int ampAdc[ICOUNT] = {0};
float ampAdcAvg[ICOUNT] = {0};

float volts = 0;
float amps[ICOUNT] = {0};
float watts[ICOUNT] = {0};


void setup(){
  
  Serial.begin(57600);
  Serial.print("PedalPowerSystems.com Watt Monitor  v. ");
  Serial.print(VERSION);
  Serial.println("!");

  pinMode(pinRelay, OUTPUT); 
  pinMode(pinLed, OUTPUT);  

  pinMode(pinVolt, INPUT); // voltage ADC
  for(i = 0; i < ICOUNT; i++){
    pinMode(pinAmp[i], INPUT); // amps 1 ADC
  }
}

void loop(){
  time = millis();

  if(time - lastVolt > VOLT_INTERVAL){
    lastVolt = time;
    doVolts();
  }

  if(time - lastDisplay > DISPLAY_INTERVAL && enableAutoDisplay){
    lastDisplay = time;
    doDisplay();
  }

  if(time - lastTime > BLINK_INTERVAL){
    lastTime = time;
    doBlink();
  } 
  
  
  while (Serial.available() > 0) {
    // get incoming byte:
    in = Serial.read();
    switch(in){
      case 'a': // rAhhhh
        enableAutoDisplay = !enableAutoDisplay;
        break;
      case 'p':
        doDisplay();
        break;
      case 'd':
        doData();
        break;
      default:
        break;
    }
  }
}

void doVolts(){

  voltAdc = analogRead(pinVolt);

  if(voltAdcAvg == 0)
    voltAdcAvg = voltAdc;

  voltAdcAvg = average(voltAdcAvg, voltAdc);
  
  doAmps();
}

void doAmps(){
  for(i = 0; i < ICOUNT; i++){
    ampAdc[i] = analogRead(pinAmp[i]);
    if(ampAdcAvg[i] == 0)  
      ampAdcAvg[i] = ampAdc[i];
    ampAdcAvg[i] = average(ampAdcAvg[i], ampAdc[i]);
  }
}

void doVoltTest(){
  float volts = adc2volts(voltAdcAvg);
  if(volts > VOLT_CUTOFF)
  {
    digitalWrite(pinRelay, HIGH);
    isRelayOn = true;
  }
  else if(isRelayOn && volts < VOLT_RECOVER)
  {
    digitalWrite(pinRelay, LOW);
    isRelayOn = false;
  }
}

float average(float avg, unsigned int val){
  if(avg == 0)
    avg = (float)val;
  return (((float)val) + (avg * (AVG_CYCLES - 1))) / AVG_CYCLES;
}


void calcValues(){
  volts = adc2volts(voltAdcAvg);
  for(i = 0; i < ICOUNT; i++){
    amps[i] = adc2amps(ampAdcAvg[i]);
    watts[i] = amps[i] * volts;
  }
}

void doDisplay(){
  calcValues();
  Serial.print("volts: ");
  Serial.print(volts);
  Serial.print(", volts raw: ");
  Serial.println(voltAdc);
  
  for(i = 0; i < ICOUNT; i++){
    Serial.print("amps");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(amps[i]);
    Serial.print(", amps");
    Serial.print(i);
    Serial.print(" raw: ");
    Serial.print(ampAdc[i]);
    Serial.print(", watts");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(watts[i]);
  }
  
  Serial.println();
}

void doData() {
  calcValues();
  Serial.println("START_DATA");
  Serial.print("Volt ");
  Serial.println(volts, 2);

  for(i = 0; i < ICOUNT; i++){
    Serial.print("AMP");
    Serial.print(i);
    Serial.print(" ");
    Serial.println(amps[i]);
    Serial.print("WAT");
    Serial.print(i);
    Serial.print(" ");
    Serial.println(watts[i]);
  }
  Serial.println("STOP_DATA");

}

void doBlink(){
  isBlinking = !isBlinking;
  digitalWrite(pinLed, isBlinking);
  
  //FOR TESTING: 
  //digitalWrite(pinRelay, isBlinking);
}

static int volts2adc(float v){
 /* voltage calculations
 *
 * Vout = Vin * R2/(R1+R2), where R1 = 100k, R2 = 10K 
 * 30V * 10k/110k = 2.72V      // at ADC input, for a 55V max input range
 *
 * Val = Vout / 5V max * 1024 adc val max (2^10 = 1024 max vaue for a 10bit ADC)
 * 2.727/5 * 1024 = 558.4896 
 */
//int led3volts0 = 559;

/* 24v
 * 24v * 10k/110k = 2.181818181818182
 * 2.1818/5 * 1024 = 446.836363636363636  
 */
//int led2volts4 = 447;


 //adc = v * 10/110/3.3 * 1024 == v * 28.209366391184573;
 return v * 28.209366391184573;
 
 //adc = v * 10/110/5 * 1024 == v * 18.618181818181818;
//return v * 18.618181818181818;


}

float adc2volts(float adc){
  // v = adc * 110/10 * 3.3 / 1024 == adc * 0.03544921875;
  return adc * 0.03544921875; // 36.3 / 1024 = 0.03544921875; 
  
  // v = adc * 110/10 * 5 / 1024 == adc * 0.0537109375;
  //return adc * 0.0537109375; // 55 / 1024 = 0.0537109375; 
}


// amp sensor conversion factors
// 0.055v/A                       // sensor sensitivity (v = adc input volts, not main power system volts) 
// 3.3v/1024adc                     // adc2v conversion ratio
// 0A == 1.65v                     // current sensor offset
//VOUT = (0.055 V/A * i + 1.65 V) * Vcc / 3.3 V

//((adc * 3.3 / 1024) - 1.65) / 0.055;
// adc2v = xadc * 3.3v/1024adc                         = xadc * 0.00322265625 = vin

float adc2amps(float adc){
// adc2A = ((adc * 3.3 / 1024) - 1.65) / .055            = adc * 0.05859375 - 30
return adc * 0.05859375 - 30;
}

float amps2adc(float amps){
// A2adc = ((A * .055) + 1.65) * 1024 / 3.3              = A * 17.06666666666667 + 512
return amps * 17.06666666666667 + 512;
}


