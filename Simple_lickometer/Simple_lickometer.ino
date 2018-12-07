/*

*/

#include <CapacitiveSensor.h>

// Hardware layout
long threshold = 50;
int c_pin_1 = 8;
int c_pin_2 = 12;
int output_pin = 7;
int LED_pin = 13;
int sync_pin = 10;

// Other variables
int counter = 0;                    // Counts # >baseline in a row
float drifting_baseline = 0;         // Used for baseline
long total1;                        // Were we store the capacitance measurement

// Sensor itself
CapacitiveSensor   cs_sensor = CapacitiveSensor(c_pin_1, c_pin_2);        // 10M resistor between pins 4 & 2, pin 2 is sensor pin, add a wire and or foil if desired// To add more sensors...//CapacitiveSensor   cs_4_6 = CapacitiveSensor(4,6);        // 10M resistor between pins 4 & 6, pin 6 is sensor pin, add a wire and or foil//CapacitiveSensor   cs_4_8 = CapacitiveSensor(4,8);        // 10M resistor between pins 4 & 8, pin 8 is sensor pin, add a wire and or                    

void setup() {
  cs_sensor.set_CS_AutocaL_Millis(0xFFFFFFFF);
  pinMode(output_pin, OUTPUT);
  pinMode(LED_pin, OUTPUT);
  pinMode(sync_pin, OUTPUT);

  digitalWrite(sync_pin,LOW);
  
  Serial.begin(9600);
}

void loop() {
    total1 =  cs_sensor.capacitiveSensor(30);

    Serial.print(total1);
    Serial.print(" ");
    Serial.println(drifting_baseline);

    
    if (total1>threshold + drifting_baseline){
      digitalWrite(output_pin, HIGH);
      digitalWrite(LED_pin, HIGH);
      digitalWrite(sync_pin, HIGH);
      counter++;
    }else{
      digitalWrite(output_pin, LOW);
      digitalWrite(LED_pin, LOW);
      digitalWrite(sync_pin, LOW);
      drifting_baseline = drifting_baseline*0.999 + 0.001*total1;
      counter = 0;
    }

    // Controls the sampling interval
    delay(5);
    
    // if a lick is longer then 100ms, it's >99.9% of the distribution. That's weird. Probably noise -> reset capacitance
    if(counter>20){
      //cs_sensor = CapacitiveSensor(c_pin_1, c_pin_2);
      //cs_sensor.set_CS_AutocaL_Millis(0xFFFFFFFF);
      //Serial.println("Performed reset");
      counter = 0;
    }
    
}
