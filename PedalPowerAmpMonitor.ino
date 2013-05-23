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
 * 1.7: adjusted amps and volts calcs for RTB smart power box; changed from AMPS0-5 to AMPS1-6.
 */

const char * VERSION = "1.7";

const float AVG_CYCLES = 20.0;
const unsigned int BLINK_INTERVAL = 1000;
const unsigned int DISPLAY_INTERVAL = 1000;
const unsigned int MEASURE_INTERVAL = 10;
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
unsigned long lastMeasure = 0;
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
unsigned int adcComp[ICOUNT] = {3,4,2,3,16,10}; //add this to raw adc values to get ~ 512 adc @ 0 amps.

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

  if(time - lastMeasure > MEASURE_INTERVAL){
    lastMeasure = time;
    doVolts();
    doAmps();
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
}

void doAmps(){
  for(i = 0; i < ICOUNT; i++){
    ampAdc[i] = analogRead(pinAmp[i]);
    ampAdc[i] = ampAdc[i] + adcComp[i];
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
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(amps[i]);
    Serial.print(", amps");
    Serial.print(i + 1);
    Serial.print(" raw: ");
    Serial.print(ampAdc[i]);
    Serial.print(", watts");
    Serial.print(i + 1);
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
    Serial.print(i + 1);
    Serial.print(" ");
    Serial.println(amps[i]);
    Serial.print("WAT");
    Serial.print(i + 1);
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

 ////adc = v * 10/110/3.3 * 1024 == v * 28.209366391184573;
 //adc = v * 2.5/(36.4 + 2.5)/3.3 * 1024 == v * 19.9423541325824
 return v * 19.9423541325824;
}

float adc2volts(float adc){
  //// v = adc * 110/10 * 3.3 / 1024 == adc * 0.03544921875;
  // v = adc * 38.9/2.5 * 3.3 / 1024 == adc * 0.05014453125
  return adc * 0.05014453125;
}

//converts adc value to adc pin volts
float adc2pV(float adc){
 // adc * 3.3 / 1024 == adc *  0.00322265625
 return adc * 0.00322265625;
}

// amp sensor conversion factors
// 0A == 512 adc == 1.65pV // current sensor offset
// pV/A = .04 pV/A (@5V) * 3.3V/5V = .0264 pV/A (@3.3V) // sensor sensitivity (pV = adc input pin volts) 
// adc/pV = 1024 adc / 3.3 pV = 310.3030303030303 adc/pV  // adc per pinVolt
// adc/A = 310.3030303030303 adc/pV * 0.0264 pV/A = 8.192 adc/A
// A/adc = 1 A / 8.192 adc = 0.1220703125 A/adc

float adc2amps(float adc){
  // A/adc = 0.1220703125 A/adc
  return (adc - 512) * 0.1220703125;
}

float amps2adc(float amps){
  // adc/A = 8.192 adc/A
  return amps * 8.192 + 512;
}

