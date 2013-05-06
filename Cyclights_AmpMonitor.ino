const float VOLT_AVG_CYCLES = 20.0;
const unsigned int BLINK_INTERVAL = 1000;
const unsigned int DISPLAY_INTERVAL = 1000;
const unsigned int VOLT_INTERVAL = 10;
const unsigned int VOLT_TEST_INTERVAL = 1000;

const float VOLT_CUTOFF = 15.0;
const float VOLT_RECOVER = 14.5;

//IN
const byte pinVolt = 0;
const byte pinAmp1 = 1;
const byte pinAmp2 = 2;

//OUT
const byte pinRelay = 2;
const byte pinLed = 13;

unsigned long time = 0;
unsigned long lastTime = 0;
unsigned long lastVolt = 0;
unsigned long lastVoltTest = 0;
unsigned long lastDisplay = 0;
unsigned long lastBlink = 0;

boolean isRelayOn = false;

unsigned int voltAdc = 0;
float voltAdcAvg = 0;

unsigned int amp1Adc = 0;
float amp1AdcAvg = 0;
unsigned int amp2Adc = 0;
float amp2AdcAvg = 0;


boolean isBlinking = false;

void setup(){
  
  Serial.begin(57600);
  Serial.println("PedalPowerSystems.com Capacitor Monitor  v. 1.1!");

  pinMode(pinRelay, OUTPUT); 
  pinMode(pinLed, OUTPUT);  

  pinMode(pinVolt, INPUT); // voltage ADC
  pinMode(pinAmp1, INPUT); // amps 1 ADC
  pinMode(pinAmp2, INPUT); // amps 2 ADC
}

   char in;
boolean enableAutoDisplay = false;

void loop(){
  time = millis();

  if(time - lastVolt > VOLT_INTERVAL){
    lastVolt = time;
    doVolts();
  }

//  if(time - lastVoltTest > VOLT_TEST_INTERVAL){
//    lastVoltTest = time;
//    doVoltTest();
//  }

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
//      case 's':
//        enableSafety = !enableSafety;
//        Serial.print("SAFETY: ");
//        Serial.println(enableSafety);
//        doSafety();
//        break;
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
  amp1Adc = analogRead(pinAmp1);
  amp2Adc = analogRead(pinAmp2);

  if(amp1AdcAvg == 0)
    amp1AdcAvg = amp1Adc;
  if(amp2AdcAvg == 0)
    amp2AdcAvg = amp2Adc;

  amp1AdcAvg = average(amp1AdcAvg, amp1Adc);
  amp2AdcAvg = average(amp2AdcAvg, amp2Adc);
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
  return (((float)val) + (avg * (VOLT_AVG_CYCLES - 1))) / VOLT_AVG_CYCLES;
}

float volts = 0;
float amps1 = 0;
float amps2 = 0;
float watts1 = 0;
float watts2 = 0;

void calcValues(){
  volts = adc2volts(voltAdcAvg);
  amps1 = adc2amps(amp1AdcAvg);
  amps2 = adc2amps(amp2AdcAvg);
  watts1 = amps1 * volts;
  watts2 = amps2 * volts;
}

void doDisplay(){
  calcValues();
  Serial.print("volts: ");
  Serial.print(volts);
  Serial.print(" volts raw: ");
  Serial.print(voltAdc);
  Serial.print(" amps1: ");
  Serial.print(amps1);
  Serial.print(" amp1 raw: ");
  Serial.print(amp1Adc);
  Serial.print(" amps2: ");
  Serial.print(amps2);
  Serial.print(" amps2 raw: ");
  Serial.print(amp2Adc);
  Serial.print(" watts 1: ");
  Serial.print(watts1);
  Serial.print(" watts 2: ");
  Serial.print(watts2);
  
//  if(isRelayOn)
//  Serial.print("   RELAY is ON!");
  Serial.println();
}

void doData() {
  calcValues();
  Serial.println("START_DATA");
  Serial.print("Volt ");
  Serial.println(volts, 2);
    
    Serial.print("AMP1 ");
    Serial.println(amps1, 2);
    Serial.print("WAT1 ");
    Serial.println(watts1, 2);

    Serial.print("AMP2 ");
    Serial.println(amps2, 2);
    Serial.print("WAT2 ");
    Serial.println(watts2, 2);
  
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


