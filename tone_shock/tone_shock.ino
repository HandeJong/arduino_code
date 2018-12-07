/*
This scripts does a very simple reward conditioning paradigm. A cue
light is delivered on a random interval for 2s, imediatly followed by
the delivery of a liquid droplet.
*/


// Session-dependend variables
int droplet_size=30; //time (ms) sollenoid wil stay open <- needs to be callibrated
int LED_on=1000; //time (ms) LED will stay on
int SR_delay=2000; //time (ms) between LED onset and sollenoid activation
const int np_trials=150; //number of trials
long interval_min=30000; //minimum time (ms) between trials
long interval_max=60000; //maximum time (ms) between trials
int fraction_o = -10; // Random(0,10) below this will be an omission trial

// hardware layout
int LED_pin=12;
int Solln_pin=10;
int BNC_pin=8;
int Running_pin=6;
int Input_pin=5;
int tone_pin = 3;


// other variables
bool start=false;
bool reward_delivery[np_trials]; // true if reward delivered, false if reward ommission
int tone_f = 11000; // (HZ) frequency of the tone
long random_number; // used to store random output

// function that controls cue-reward delivery
void cue_reward(bool real){
  // bool real controls if liquid will be delivered, or only cue light
  if(real){
    // first cue then liquid
    Serial.print("Trial");
    Serial.print("\n");
    digitalWrite(LED_pin, HIGH);
    digitalWrite(BNC_pin, HIGH);
    tone(tone_pin,tone_f,LED_on);
    delay(LED_on);
    digitalWrite(LED_pin, LOW);
    delay(SR_delay-LED_on);
    digitalWrite(Solln_pin, HIGH);
    digitalWrite(BNC_pin, LOW);
    delay(droplet_size);
    digitalWrite(Solln_pin, LOW);
  } else {
    // cue only
    Serial.print("Cue only");
    Serial.print("\n");
    digitalWrite(LED_pin, HIGH);  // turn LED ON
    digitalWrite(BNC_pin, HIGH);
    tone(tone_pin,tone_f,LED_on);
    delay(LED_on);
    digitalWrite(LED_pin, LOW);
    digitalWrite(BNC_pin, LOW); // TTL pulse only 1sec
  }
}


// called by main once (could also copy to loop()...)
void setup() {
  // set all pins to output except the start button
  pinMode(LED_pin, OUTPUT);
  pinMode(Solln_pin, OUTPUT);
  pinMode(BNC_pin, OUTPUT);
  pinMode(Running_pin, OUTPUT);
  pinMode(Input_pin, INPUT);
  
  // Establising serial connection
  Serial.begin(9600);
}


// continuously called by main (but we'll block that at the end)
void loop() {
  // Pre-start block
  Serial.print("Ready to start");
  Serial.print("\n");
  while(!start){
    // Check if user is pressing button
    bool start_state = digitalRead(Input_pin);
    delay(100);
    if(start_state){
      // Start session
      Serial.print("starting...");
      Serial.print("\n");
      digitalWrite(Running_pin, HIGH);
      start=true;
    }
  }
  
  // Seed the random number generator
  randomSeed(analogRead(A0));
  
  
  // Boolean array that controls reward delivery or ommissions
  for (int i=0; i<=np_trials; i++){
    random_number = random(0,10);
    if (random_number>fraction_o){
      reward_delivery[i]=true;
      Serial.print("true");
    } else {
      reward_delivery[i]=false;
      Serial.print("false");
    }
  }
  
  
  // This is the actual session
  for(int i=0; i<=np_trials; i++ ){
    long interval=random(interval_min, interval_max);
    Serial.print("Interval: ");
    Serial.print(interval);
    Serial.print("\n");
    delay(interval);
    
    cue_reward(reward_delivery[i]);
  }
  
  
  // Wrapping up
  Serial.print("Program done");
  Serial.print("\n");
  Serial.end(); //close serial
  digitalWrite(Running_pin, LOW);
  while(1){} //because we don't actually want to loop
    
}
