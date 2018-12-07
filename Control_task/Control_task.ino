/*
Control_task is the main logic for a head-fixed task with the following trial structure:

  1. A cue light (1s) predicts reward availability
  2. A one sec delay after cue offset
  3. A timeblock of length 'reward_delay' in which the mouse can lick to obtain reward
      - Note that the reward is only delivered after a lick
      - Reward delivery is associated with an audible click from the solenoid
      
  Furthermore during a fraction of the trials (defined by 'fr_shock') a tone is
  presented at the same time as the light cue and remains on for the time 'shock_delay'.
  During this interval a lick is followed by both reward delivery as well as an
  electric shock. (Strength decided by the pulse generator).
  
  Example of a trial with reward_delay = 7000 and shock_delay = 4000:
  
  t = 0": cue on, tone on
  t = 1": cue off
  t = 2": reward available (licks both rewarded and punished)
  t = 4": tone off, licks no longer punished
  t = 7": reward no longer available
  
  So reward_delay is counted from t = 2", while shock_delay is counted from tone onset.
  Note that most trials are without tone, and licks are not punished in these trials.
  
  The session_structure array is printed to the serial port at the beginning of the session.
  In this array the trials type is indicated as follows:
  
  1: standard trial
  2: omission trial (no reward)
  3: tone-shock trial (also reward)
  4: tone-shock and no reward trial (probe)
  5: rewards only (no cue) (probe)
  6: Cue-reward (2sec) but no reinforced action
  
  To make it easy, TTL pulses send during this program convey information in the same way.
  For instance, a 2sec TTL pulse indicates an omission trial. Imporantly, trials start at
  TTL pulse offset.

  a 0.1s TTL pulse signals the onset of reward delivery.
  
  This Arduino Sketch was written by Han de Jong, j.w.dejong@berkeley.edu.
*/


// hardware layout
int button_one = 5;         // Button on the main Arduino shield
int button_two = 52;        // Blue button on remote
int lick_in = 2;            // Lickometer input
int cue_light = 12;         // Signals reward availability
int soln_pin = 10;          // Controls the reward delivery
int BNC_pin = 8;            // Used for sending TTL pulses
int tone_pin = 3;           // Controls tone
int shock_pin = 7;         // Controls the shock generator
int running_pin = 6;        // Indicates the progrm is running (Should be outside of opperant box)
bool start = false;         // Checks if the program is running
bool button_press = false;  // Checks if button_two pressed


// Varialbles to be set by the user
bool auto_start = true;     // Does not wait for user to press start button
const int nr_trials = 150;  // The number of rewards
int fr_shock = 15;          // % of trials with tone-shock
int fr_omission = 0;       // % of trial with reward omitted
bool shock_probes = false;  // Causes 10 tone-shock-only (no reward or rewad cue) to be in the session (type 4)
bool reward_probes = false;  // Causes 10 rewards-only no cue (type 5);
bool pavlov_session = false; // Licks are now without scheduled concequences
int reward_delay = 7000;    // Maximum time that the reward is available.
int free_reward = 2000;     // Even without lick, mouse will get reward after this time.
int reward_time_out = 0; // Delay after which the reward is delivered to the mouse
int fr_free_reward = 30;     // % of trials that will receive a free reward after the free_reward time.
int shock_delay = 3000;     // During this interval, licks are punished
int shock_length = 500;     // Length of the shock pulse (can also use as TTL for shock generator)
long min_interval = 25000;  // Minimal interval length
long max_interval = 50000;  // Maximal interval length
int droplet_size = 30;      // Size of the reward (needs to be callibrated)
int tone_f = 11000;         // (HZ) frequency of the tone


// These variables contain collected data
int trial_outcome;          // Outcome of the last trial
int outcome_counter[5];     // Counts the 4 trial outcomes


// Other variables
int session_structure[nr_trials]; // Contains the session structure
long intervals[nr_trials];        // Containts the interval lengths
int trial_counter = 0;            // Counts the trials
bool free_reward_bool;            // Used to check if mouse will get a free reward
unsigned long start_time;         // Time when user pressed start button
unsigned long trial_start_time;   // Time that the current trial started
unsigned long intr_start_time;     // Time that the current interval started
unsigned long c_time;             // The current time (use in long switch and if statements)


int trial(bool reward, bool shock){
  // trial controls the reward-shock trials. Bool reward determines
  // if a reward will be delivered while bool shock determines
  // if the mouse is exposed to a tone and a shock.
  //
  // Note: both the reward and the shock are only delivered if the
  // mouse licks
  //
  // Returns as follows:
  // 0: Mouse did not licked
  // 1: Mouse licked
  // 2: Mouse got shocked
  // 3: Mouse got shocked and reward
  // 4: Mouse got a free reward (used during training)

  // Output value
  int return_value = 0; // means no lick

  // Start time
  trial_start_time = millis();

  // If it's a no-shock trial
  if(reward && !shock){
    digitalWrite(cue_light,HIGH);

    // Pick a random variable between 1-100 to see if the mouse will get a free reward
    if(random(1,100) <= fr_free_reward){
      free_reward_bool = true;
    }else{
      free_reward_bool = false;
    }

    // Loop will run until lick or reward_delay times out
    while(millis()<trial_start_time+reward_delay){

      // This turns on the cue light ater 1sec
      if(millis()>trial_start_time+1000){
        digitalWrite(cue_light,LOW);
      }

      // Checks if the mouse is licking and if so -> gives reward and end cycle
      if(digitalRead(lick_in)){
        delay(reward_time_out);
        give_reward();
        return_value = 1;
        break;
      }

      // During training the mouse gets a reward anyway a certain interval after cue light.
      if(free_reward_bool && millis()>trial_start_time + free_reward){
        give_reward();
        return_value = 4;
        break;
      } 
    }
  }

  // if it's an omission trial
  if(!reward && !shock){
    // tone-nothing trial
    digitalWrite(cue_light,HIGH);

     // Loop will run until lick or reward_delay times out
    while(millis()<trial_start_time+reward_delay){

      // This turns on the cue light ater 1sec
      if(millis()>trial_start_time+1000){
        digitalWrite(cue_light,LOW);
      }

      // Checks if the mouse is licking and if so -> gives reward and end cycle
      if(digitalRead(lick_in)){
        return_value = 1;
        break;
      }
    }    
  }

  // If it's a shock trial
  if(reward && shock){
    // Tone-shock trial
    digitalWrite(cue_light,HIGH);
    tone(tone_pin, tone_f);

    while(millis()<trial_start_time+reward_delay){

      if(digitalRead(lick_in)){
        // on lick -> check the time

        c_time = millis() - trial_start_time;

        if (c_time<shock_delay){
          // shock and reward
          give_reward();
          give_shock();
          return_value = 3;
          noTone(tone_pin);
          Serial.print("    both   ");
          Serial.print(c_time);
          Serial.print("    ");
          break;
  
        }else{
          // reward only
          Serial.print("    only reward   ");
          Serial.print(c_time);
          Serial.print("    ");
          give_reward();
          return_value = 1;
          break;
        }
      }

      // turn cue light off after 1000ms
      if(millis()>trial_start_time+1000){
        digitalWrite(cue_light, LOW);
      }

      // turn tone off after shock_delay
      if(millis()>trial_start_time+shock_delay){
        noTone(tone_pin);
      }
    }    
  }

   // if it's a shock only trial (not reward omission, there is also NO cue)  
   if(!reward && shock){
      // Tone-shock trial
      tone(tone_pin, tone_f);
  
      while(millis()<trial_start_time+reward_delay){
  
        if(digitalRead(lick_in)){
          // on lick -> check the time
  
          c_time = millis() - trial_start_time;
  
          if (c_time<shock_delay){
            // shock and reward
            give_shock();
            return_value = 2;
            noTone(tone_pin);
            Serial.print("    shock   ");
            Serial.print(c_time);
            Serial.print("    ");
            break;
    
          }
        }
  
        // turn tone off after shock_delay
        if(millis()>trial_start_time+shock_delay){
          noTone(tone_pin);
        }
      }    
    }
    
  // Backup in case anything is wrong
  noTone(tone_pin);
  digitalWrite(cue_light,LOW);
  
  return return_value;
}


void give_shock(){
  // controls punishment shock
  digitalWrite(shock_pin, HIGH);
  delay(shock_length);
  digitalWrite(shock_pin, LOW); 
}


void give_reward(){
  // controls reward delivery and TTL out
  digitalWrite(BNC_pin,HIGH);
  digitalWrite(soln_pin, HIGH);
  delay(droplet_size);
  digitalWrite(soln_pin, LOW);
  delay(100-droplet_size);
  digitalWrite(BNC_pin,LOW);
}


void build_session(){
  // build_session uses the variables above as well as random generated
  // values to build the entire session.
  Serial.print("\n");
  Serial.print("Building Session...");
  Serial.print("\n");

  // Where we will store outcome of random variables
  bool shock_bool;
  bool omission_bool;
  
  // Seeding random number generator
  randomSeed(analogRead(A0));
  
  // Building session
  for(int i=0; i<nr_trials; i++){
    intervals[i] = random(min_interval, max_interval);

    // Is this a pavlov session? Because that's easy
    if(pavlov_session){
      session_structure[i] = 6;
      continue;
    }

    // Shock on this trial?
    shock_bool = 0;
    if(random(1,100) <= fr_shock){
      shock_bool = 1;
    }

    // Omission on this trial?
    omission_bool = 0;
    if(random(1,100) <= fr_omission){
      omission_bool = 1;
    }

    if(shock_bool && omission_bool){
      session_structure[i] = 4; //Note: these trial also don't give cue light!
    }else if(shock_bool && !omission_bool) {
      session_structure[i] = 3;
    }else if(!shock_bool && omission_bool) {
      session_structure[i] = 2;
    }else if(!shock_bool && !omission_bool) {
      session_structure[i] = 1;
    }
  }

  // Put 10 tone-shock-only probes in there if requested
  if(shock_probes){
    session_structure[6] = 4;
    session_structure[11] = 4;
    session_structure[22] = 4;
    session_structure[38] = 4;
    session_structure[42] = 4;
    session_structure[50] = 4;
    session_structure[66] = 4;
    session_structure[71] = 4;
    session_structure[84] = 4;
    session_structure[92] = 4;
  }

// Put 10 reward probes in there if requested
  if(reward_probes){
    session_structure[8] = 5;
    session_structure[13] = 5;
    session_structure[25] = 5;
    session_structure[31] = 5;
    session_structure[47] = 5;
    session_structure[53] = 5;
    session_structure[69] = 5;
    session_structure[74] = 5;
    session_structure[89] = 5;
    session_structure[91] = 5;
  }

  // Printing the whole session
  for(int i=0; i<nr_trials; i++){
        Serial.print(session_structure[i]);
  }
  
  // Finishing up
  Serial.print("\n");
  delay(1000);
}

void end_session(){
  // This function will end the session when called and pause the program

  // Print that the session has ended
  Serial.print("\n");
  Serial.print("done");
  Serial.print("\n");

  // Turn of the running pin
  digitalWrite(running_pin, LOW);

  // Print some information about the session
  Serial.print("\n");
  Serial.print("Failures: ");
  Serial.print(outcome_counter[0]);
  Serial.print("\n");
  Serial.print("Succesfull: ");
  Serial.print(outcome_counter[1]);
  Serial.print("\n");
  Serial.print("Shocks: ");
  Serial.print(outcome_counter[3]);
  Serial.print("\n");
  Serial.print("Free reward: ");
  Serial.print(outcome_counter[4]);
  Serial.print("\n");

  // Pauses the program untill reset
  while(true){};
  
}

void setup() {
  
    //Set pinmode layout
    pinMode(button_one, INPUT);
    pinMode(button_two, INPUT);
    pinMode(cue_light, OUTPUT);
    pinMode(lick_in, INPUT);
    pinMode(soln_pin, OUTPUT);
    pinMode(BNC_pin, OUTPUT);
    pinMode(tone_pin, OUTPUT);
    pinMode(running_pin, OUTPUT);
    pinMode(shock_pin, OUTPUT);
    
    // Start serial connection
    Serial.begin(9600);
    Serial.print("\n");
    Serial.print("*************************************************************");
    Serial.print("\n");
    Serial.print("Ready to start.");
    Serial.print("\n");

    // Zero the outcome coutner
    outcome_counter[0]=0;
    outcome_counter[1]=0;
    outcome_counter[2]=0;
    outcome_counter[3]=0;
    outcome_counter[4]=0;
    
    // Run function that makes the entire session
    build_session(); 
}


void loop() {

    // The program will stay here untill the user starts it by pressing the start button.
    while(!start){
    start = digitalRead(button_one);    
      if(start || auto_start){
        start = true;
        // Start session
        Serial.print("starting..."); 
        Serial.print("\n");
        digitalWrite(running_pin, HIGH);
        start_time = millis(); // This is the start time used by the rest of the program.
        delay(300); //prevent douple response
      }
    }
    
    // This controls the trial interval
    intr_start_time = millis();
    while(millis()<intr_start_time + intervals[trial_counter]-session_structure[trial_counter]){

      // Check if button pressed, if so -> end session
      if(digitalRead(button_one)){
        end_session();
      }

      // Check for licks?
      // .... TODO.....
        
    }


    // Sends information about the trial type in the form of a timed TTL pulse to an external system.
    digitalWrite(BNC_pin,HIGH);
    delay(session_structure[trial_counter]*1000);
    digitalWrite(BNC_pin,LOW);
    Serial.print(millis()-start_time);
    Serial.print("   ");
    Serial.print(session_structure[trial_counter]);
    Serial.print("   ");
    
    // Select trial type and run trial
    if(session_structure[trial_counter] == 1){
      trial_outcome = trial(true, false);
    }else if(session_structure[trial_counter] == 2){
      trial_outcome = trial(false, false);
    }else if(session_structure[trial_counter] == 3) {
      trial_outcome = trial(true, true);
    }else if(session_structure[trial_counter] == 4){
      trial_outcome = trial(false, true);
    }else if (session_structure[trial_counter] == 5){
      delay(1000);
      give_reward();
      trial_outcome = 4;
    }else if (session_structure[trial_counter] == 6){
       digitalWrite(cue_light,HIGH);
       delay(1000);
       digitalWrite(cue_light, LOW);
       delay(1000);
       give_reward();
       trial_outcome = 4;
    }
    
    Serial.print(trial_outcome);
    Serial.print("   ");
 
    // Increment the outcome_counter
    outcome_counter[trial_outcome]++;
    Serial.print(outcome_counter[trial_outcome]);
    Serial.print("\n");
       
    // Increment trial counter
    trial_counter++;

    // End session after maximum number of trials
    if(trial_counter>=nr_trials){
      end_session();
    }
}
