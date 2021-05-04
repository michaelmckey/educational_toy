//8 letters max for SD card filename(12 if including ".wav") this is why shut_down didn't work but shutDown did!!
//SAME FOR SD CARD LOGS COLOUR_DATA.CSV DOESNT WORK BUT DATALOG5.CSV DOES

//MAKE SO THAT IT CAN DETECT PIECE TAKEN OUT AT ANY TIME WITHOUT waiting ON NO BRIGHTNESS reading
//make expected different with different lightning conditions(use previous green readings to tell what expected green is)
//basically make expected brightness a function that uses more data e.g. what colour of led is used. readings for other pieces in/other pieces out, previous calibration readings
//When shadow over ldr red didnt trigger piece inserted(at 60% threshhold) unless 250lux provided
//add sleeping with //#include <avr/sleep.h>
//DATA logging cant open file

//by only using the initial brightness of the piece it works great(if no brightness change) and no flicking back and forth if no piece inserted
#include <SD.h>                           //include SD module library
#include <TMRpcm.h>                       //include speaker control library

#define SD_ChipSelectPin 53                //define CS pin
float reading;
TMRpcm audio;                            //crete an object for speaker library

const int vol_pin = 7;

int vol = 255;
int brightness = 255;
int language_selected = 2;//1 = red, 2=green, 3=blue (0 only occurs if error)

char w = ' ';
boolean NL = true;
int part = 0;
String key = "";
String value = "";

/*
TMRpcm errors:
The program can't take file names as strings(only as char*)
can't write to sd card when audio playing can solve this with audio.pause();

The circuit:
   SD card attached to SPI bus as follows:
 ** GND - GND
 ** VCC - 5V
 ** MOSI - pin 51
 ** MISO - pin 50
 ** SCK/CLK - pin 52
 ** CS - pin 53 (for MKRZero SD: SDCARD_SS_PIN)

The WAV file:
 ** bit resolution: 8 bit
 ** sample rate: 16000Hz
 ** audio channel: mono
  */

/*
float red_light_reflected[3] = {146.75,2.59,25.67};//r,g,b
float green_light_reflected[3] = {50.02,13.02,69.26};//r,g,b
float blue_light_reflected[3] = {86.87,9.13,59.41};//r,g,b
*/

float red_light_reflected[3] = {146.75,2.59,25.67};//r,g,b
float green_light_reflected[3] = {50.02,19.02,69.26};//r,g,b
float blue_light_reflected[3] = {86.87,9.13,59.41};//r,g,b

int supply_voltage = 5;//V
int known_resistance = 2200;//Omhs
int readings = 250;//roughly 250 readings per second per ldr

const int red_pin = 2;
const int green_pin = 3;
const int blue_pin = 4;

int red = 0;
int green = 0;
int blue = 0;

const int pieces = 9;
int ldr_pins[pieces] = {A1,A2,A3, A4,A5,A6, A7,A8,A9};
int digital_pins[pieces] = {22, 23, 24, 25, 26, 27, 28, 29, 30};
boolean pieces_in[pieces] = {0,0,0, 0,0,0, 0,0,0};

float none_readings[pieces];
float red_readings[pieces];
float green_readings[pieces];
float blue_readings[pieces];

float current_brightness_sums[pieces] = {0,0,0, 0,0,0, 0,0,0};

float current_piece_readings[pieces];

//100% is pieces out
//50% is arm over
//30% is pieces in
float percentage_threshhold = 70;

float initial_piece_readings[pieces];

char* spanish_pieces[pieces] = {"gato.wav", "vaca.wav", "pato.wav", "cabra.wav", "cerdo.wav", "conejo.wav", "perro.wav", "oveja.wav", "buho.wav"};//red
char* english_pieces[pieces] = {"cat.wav", "cow.wav", "duck.wav", "goat.wav", "pig.wav", "rabbit.wav", "dog.wav", "sheep.wav", "owl.wav"};//green
char* french_pieces[pieces] = {"chat.wav", "vache.wav", "canard.wav", "bouc.wav", "cochon.wav", "lapin.wav", "chien.wav", "mouton.wav", "hibou.wav"};//blue

float get_current_light_intensity(int piece_number){
  //NEED to improve formula by making dark box and adding phone and cycling through led intensities(record on another device e.g. ipad) then replace phone with ldr
  int ldr_pin = ldr_pins[piece_number];
  int sensorValue = analogRead(ldr_pin);
  float voltage = sensorValue * (5.0 / 1023);
  float resistance = (voltage * known_resistance)/(supply_voltage-voltage);
  //formula derived by doing collecting data from 6 light levels on my phone(using a lux meter app)
  //then verfifying results by testing with the top 4 apps on google play
  //old LDR values: float light_intensity = 460814690 * pow(resistance,-1.632);//lumens
  float light_intensity = 30000000 * pow(resistance,-1.542);
  //float light_intensity = -0.981 * log(resistance) + 11.977;//to_lux_coefficient * pow(resistance, to_lux_power);//lumens
  return light_intensity;//resistance;//
}

void error_out(String msg){
  while(true){
    Serial.println(msg);
  }
}

void update_leds(){
  for(int piece_number = 0; piece_number<pieces; piece_number++){
    digitalWrite(digital_pins[piece_number], pieces_in[piece_number]);
  }
  //255 - constrain(red,0,255)
  analogWrite(red_pin, red);
  analogWrite(green_pin, green);
  analogWrite(blue_pin, blue);
  //Serial.println("updated");
}

void piece_inserted(int piece_number){
  pieces_in[piece_number] = true;
  Serial.print("Piece ");
  Serial.print(piece_number);
  Serial.println("inserted!");
  play_audio("tada.wav");
  int colour = language_selected;//get_colour_inserted(ldr_number);
  char* animal;
  if(colour == 0){
    Serial.println("error in colour detection");
  }else if(colour == 1){
    Serial.println("red piece detected");
    animal = spanish_pieces[piece_number];
    //Serial.println(animal_on_a_string);
  }else if(colour == 2){
    Serial.println("green piece detected");
    animal = english_pieces[piece_number];
  }else if(colour == 3){
    Serial.println("blue piece detected");
    animal = french_pieces[piece_number];
  }else{
    Serial.println("colour detected outside of known colours");
  }
  Serial.println(String(animal));
  play_audio(animal);
  log_piece_change(piece_number, language_selected, String(animal), 1);
  //delay(1000);//gives time for hand to be removed
}

void piece_removed(int piece_number){
  pieces_in[piece_number] = false;
  Serial.print("Piece ");
  Serial.print(piece_number);
  Serial.println("removed!");
  int colour = language_selected;//get_colour_inserted(ldr_number);
  String animal = "UNKNOWN";
  if(colour == 0){
    Serial.println("error in colour detection");
  }else if(colour == 1){
    animal = spanish_pieces[piece_number];
    //Serial.println(animal_on_a_string);
  }else if(colour == 2){
    animal = english_pieces[piece_number];
  }else if(colour == 3){
    animal = french_pieces[piece_number];
  }
  log_piece_change(piece_number, language_selected, animal, 0);
  
}

void play_audio(char* file_name){//plays audio and waits for it to stop
  audio.pause();//unpauses audio
  analogWrite(vol_pin,vol);
  audio.play(file_name);
  while(audio.isPlaying()){
    ;
  }
  analogWrite(vol_pin, 0);//prevents buzzing on lower volume levels and saves power
  audio.pause();//pauses audio allowing sd card to be used
}

void calibrate_ldrs(){
  //gets ldr data
  float current_brightness_sums[pieces] = {0,0,0, 0,0,0, 0,0,0};
  for(int i = 0;i<readings;i++){
    for(int piece_number = 0; piece_number<pieces; piece_number++){//loops through all pieces and measures brightness
      current_brightness_sums[piece_number] += get_current_light_intensity(piece_number);
    }
  }
  
  //updates ldr readings
  for(int piece_number = 0; piece_number<pieces; piece_number++){
    current_piece_readings[piece_number] = current_brightness_sums[piece_number] / readings;
    current_brightness_sums[piece_number] = 0;
    initial_piece_readings[piece_number] = current_piece_readings[piece_number];
  }
}

void setup() {
  Serial.begin(9600);//usb connection
  Serial1.begin(9600);//bluetooth connection
  audio.speakerPin = 11;                  //define speaker pin.
                                          //the library is using this pin
  pinMode(vol_pin, OUTPUT);
  
  analogWrite(vol_pin,vol);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("loading...");
  
  if(!SD.begin(SD_ChipSelectPin)) {      //see if the card is present and can be initialized
    Serial.println("Failed");
  }

  //delete old session data
  SD.remove("session.csv");
  
  audio.setVolume(5);                    //5 has the highest volume/interferance
  audio.pause();//turns off audio so allow sd card to be used(play_audio() function only turns it on when using it)
  //char fileName[] = "log_on.wav";
  //LOOK UP myString.toCharArray() arduino.cc
  play_audio("startUp.wav");         //the sound file will play each time the arduino powers up, or is reset
  
  for(int piece_number = 0;piece_number<pieces;piece_number++){
    pinMode(digital_pins[piece_number], OUTPUT);
  }
  
  pinMode(red_pin, OUTPUT);
  pinMode(green_pin, OUTPUT);
  pinMode(blue_pin, OUTPUT);
  
  update_leds();//turns LEDs off

  calibrate_ldrs();
  Serial.println("starting...");
}

void log_piece_change(int piece_number, int language, String animal, int current_state){
  String dataString = String(piece_number) + "," + String(language) + "," + String(animal) + "," + String(current_state) + "," + String(millis()) + "\n";
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("session.csv", FILE_WRITE);
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    //Serial.println("piece data saved!");
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening pieces.csv");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  for(int sequence_position = 0; sequence_position<4; sequence_position++){
    //delay(2000);//makes the readings more accurate
    int sequence_position_before = ((sequence_position -1)+4)%4;
    if(sequence_position == 0){
      //check for changes in pieces in
      //Serial.println("Readings for piece 1");
      //Serial.print(none_readings[5]);
      //Serial.print(",");
      /*
      Serial.print(red_readings[5] - none_readings[5]);
      Serial.print(",");
      Serial.print(green_readings[5] - none_readings[5]);
      Serial.print(",");
      Serial.println(blue_readings[5] - none_readings[5]);*/
      ;
    }
    //resets measurements
    for(int piece_number = 0; piece_number<pieces; piece_number++){
      current_brightness_sums[piece_number] = 0;
    }
    
    //records sum of readings
    for(int reading_n = 0; reading_n<readings;reading_n++){
      for(int piece_number = 0; piece_number<pieces; piece_number++){//loops through all pieces and measures brightness
        float brightness = get_current_light_intensity(piece_number);
        current_brightness_sums[piece_number] += brightness;
      }
    }
    
    //get ldr values and add to database
    for(int piece_number = 0; piece_number<pieces; piece_number++){
      current_piece_readings[piece_number] = current_brightness_sums[piece_number] / readings;
      
      if(sequence_position_before == 0){
        none_readings[piece_number] = current_piece_readings[piece_number];
      }else if(sequence_position_before == 1){
        red_readings[piece_number] = current_piece_readings[piece_number];
      }else if(sequence_position_before == 2){
        green_readings[piece_number] = current_piece_readings[piece_number];
      }else if(sequence_position_before == 3){
        blue_readings[piece_number] = current_piece_readings[piece_number];
      }
    }
    
    //checks if piece inserted or not
    for(int piece_number = 0; piece_number<pieces; piece_number++){//i=1 as first piece is calibration ldr
      //expected_brightness = initial_piece_readings[piece_number];//* average_percentage_change;//ambient_brightness_ratio[piece_number] * calibration_ldr_reading;//start piece brightness / calibration ratio * calibration now ~ ambient piece brightnes
      float predicted_reflected_light;
      if(pieces_in[piece_number]){
        if(sequence_position_before == 0){
          predicted_reflected_light = 0;//as no light on
        }else if(sequence_position_before == 1){
          predicted_reflected_light = red_light_reflected[language_selected - 1];//as no light on
        }else if(sequence_position_before == 2){
          predicted_reflected_light = green_light_reflected[language_selected - 1];//as no light on
        }else if(sequence_position_before == 3){
          predicted_reflected_light = blue_light_reflected[language_selected - 1];//as no light on
        }
      }else{
        predicted_reflected_light = 0;
      }
      //float scaled_reading = current_piece_readings[piece_number];
      float percent_of_expected = (current_piece_readings[piece_number] / initial_piece_readings[piece_number])*100;//2 times margin of effor on reflected
      Serial.print(percent_of_expected);
      Serial.print(" : ");
      if(!pieces_in[piece_number]){//if piece not in
        if(percent_of_expected < percentage_threshhold){//and piece should be in
          piece_inserted(piece_number);
        }
      }else{//if piece in
        //if no_light reading too low
        if((percent_of_expected > percentage_threshhold) && (sequence_position_before == 0)){
          piece_removed(piece_number);
        }
      }
    }
    
    Serial.println("");
    //creates bluetooth connection to phone
    while(Serial1.available()){
      w = Serial1.read();
      Serial.write(w);
      
      if(w == '\n'){
        //end of message so key and value fully assembled
        if(key=="volume"){
          vol = constrain(value.toInt(),0,255);
        }else if(key=="brightness"){
          brightness = constrain(value.toInt(),0,255);
        }else if(key=="language"){
          language_selected = constrain(value.toInt(),0,3);//0 is an error
          if(language_selected == 0){
            Serial.println("error in bluetooth language selection");
          }else if(language_selected == 1){
            play_audio("espanol.wav");
          }else if(language_selected == 2){
            play_audio("english.wav");
          }else if(language_selected == 3){
            play_audio("francais.wav");
          }
        }else if(key=="transmit_data"){
          File dataFile = SD.open("session.csv");
        
          // if the file is available, write to it:
          if (dataFile) {
            Serial.println("TRansmitting data...");
            while (dataFile.available()) {
              Serial1.write(dataFile.read());
            }
            dataFile.close();
          }else {
            Serial.println("error opening datalog.txt");
          }
        }else if(key=="get_millis"){
          Serial.println("returning millis");
          Serial1.println(String(millis()));
        }else if(key=="calibrate_ldrs"){
          calibrate_ldrs();
        }
        
        //resets values
        key = "";
        value = "";
        part = 0;
        Serial1.println("OK");
      }else if(w == ':'){
        //at the dividing point
        part++;//changes the part to assemble
      }else if(part==0){
        //part0
        key = key + w;//assembles the key
      }else if(part==1){
        //part1
        value = value + w;//assembles the value
      }else{
        Serial.println("Invalid part number");
      }
    }
    
    
    if(Serial.available()){//copys communications between serial ports 0(computer) & 1(bluetooth module)
      w = Serial.read();
      Serial1.write(w);
      if(NL){
        Serial.print(">");
        NL = false;
      }
      Serial.write(w);
      if(w==10){
        NL = true;
      }
    }
    
    //updates LEDs
    red = 0;
    green = 0;
    blue = 0;
    if(sequence_position == 0){
      ;//none on
    }else if(sequence_position == 1){
      red = brightness;//red on
    }else if(sequence_position == 2){
      green = brightness;//green on
    }else if(sequence_position == 3){
      blue = brightness;//blue on
    }else{
      //error_out("error with sequence change");
    }

    update_leds();
  }
}
