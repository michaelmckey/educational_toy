#include "bouc.h"
#include "buho.h"
#include "cabra.h"
#include "canard.h"
#include "cat.h"
#include "cerdo.h"
#include "chat.h"
#include "chien.h"
#include "cochon.h"
#include "conejo.h"
#include "cow.h"
#include "dog.h"
#include "duck.h"
#include "english.h"
#include "espanol.h"
#include "francais.h"
#include "gato.h"
#include "goat.h"
#include "hibou.h"
#include "lapin.h"
#include "mouton.h"
#include "oveja.h"
#include "owl.h"
#include "pato.h"
#include "perro.h"
#include "pig.h"
#include "rabbit.h"
#include "sheep.h"
#include "vache.h"
#include "vaca.h"

#include "auto_detect.h";
#include "brightness_increased.h";
#include "brightness_decreased.h";
#include "calibrate.h";
#include "power_off.h";
#include "power_on.h";
#include "muted.h";
#include "unmuted.h";
#include "volume_increased.h";
#include "volume_decreased.h";

#include "startUp.h"
#include "tada.h"

#include "XT_DAC_Audio.h"

#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

const int KNOWN_RESISTANCE = 2000;//Omhs
const int INPUT_RANGE = 4095;//max analog input value

const int pieces = 9;//number of pieces
const int speaker_pin = 25;
const int red_pin = 21;
const int green_pin = 22;
const int blue_pin = 23;

const int PWMFreq = 5000; /* 5 KHz */
const int PWMResolution = 10;
const int MAX_DUTY_CYCLE = (int)(pow(2, PWMResolution) - 1);

const int redPwmChannel = 0;
const int greenPwmChannel = 1;
const int bluePwmChannel = 2;

const int speaker_timer = 3;//timer used by speaker

int red;
int green;
int blue;

//ldr input pins
const int ldr_pins[pieces] = {32,35,34, 27,26,33, 13,12,14};
//led control pins
const int digital_pins[pieces] = {5,18,19, 4,16,17, 15,2,0};
boolean pieces_in[pieces];//which pins are in
float intensity_coefficients[pieces];//calibrates ldrs
float transparency[pieces];//records transparency of pieces

const int buffer_size = 128;//previous values saved
const float sfi = 100;//how many values to add to rolling average
float values_buffer[pieces][buffer_size];
float totals_buffer[pieces][buffer_size];
float current_values[pieces];//rolling average values
float current_ambient;//rolling average ambient
int iterations = 0;
int part_of_sequence;

boolean pending;//true = piece is pending
int index_pending;//which piece is pending
boolean direction_pending;//0 = out 1 = in
float millis_pending;//time since it was pending

const float just_above_zero = 0.000001;
const int local_average_size = 3;

const int sequence_speed = 8;//should be 2 to the power of something
const int length_of_sequence = 3072;
const int on_brightness = 1023;
const boolean solid_colour = false;
const float entry_delay = 1300;//ms
const float exit_delay = 100;//ms
const float threshold = 0.40;//% above = out, below = in
const float min_transparency = 0.01;

float red_reflected[pieces];
float green_reflected[pieces];
float blue_reflected[pieces];

int readings_till_identified[pieces];

const float time_per_iteration = 4000;//microseconds
float last_time = 0;

float none_readings[pieces];
float red_readings[pieces];
float green_readings[pieces];
float blue_readings[pieces];

const int KNOWN_PIECES = 3;
const float pieces_expected[KNOWN_PIECES][3] = 
{{0.4837, 0.2307, 0.2856}, //red
{0.1672, 0.4360, 0.3968},  //green
{0.2961, 0.0858, 0.6181}}; //blue

//sets up audio class
XT_DAC_Audio_Class DacAudio(speaker_pin,speaker_timer);
XT_Wav_Class *Sound;

int part = 0;
String key = "";
String value = "";

const int volume_step = 10;
const int brightness_step = 10;

boolean is_power_on = true;
boolean is_muted = false;

int brightness = 100;//%
int volume = 100;//%
int language_selected = 0;//0 = auto, 1 = spanish, 2 = english, 3 = french

/*prints message to all outputs*/
void print_all(char message[]){
  Serial.println(message);
  SerialBT.println(message);
}

/*if possible, starts to play the audio file*/
void play_audio(const unsigned char sound[]){
  if(!is_muted){
    Sound = new XT_Wav_Class(sound);
    DacAudio.Play(Sound);
  }
}

/*triggered when the colour of a piece has been identified*/
void pieceIdentified(int piece_number, int piece_colour){
  int language;
  if(language_selected == 0){//if auto detect enabled
    language = piece_colour;
    if(language == 0){
      print_all("Piece is red!");
    }else if(language == 1){
      print_all("Piece is green!");
    }else if(language == 2){
      print_all("Piece is blue!");
    }
  }else{//else uses present language
    language = language_selected - 1;//-1 as first option is auto
  }
  
  if(language == 0){
    print_all("Language is Spanish!");
  }else if(language == 1){
    print_all("Language is English!");
  }else if(language == 2){
    print_all("Language is French!");
  }

  //says and prints animal corresponding to the piece
  if(language == 0){
    //red spanish
    switch(piece_number){
      case 0:
        print_all("gato");
        play_audio(gato);
        break;
      case 1:
        print_all("vaca");
        play_audio(vaca);
        break;
      case 2:
        print_all("pato");
        play_audio(pato);
        break;
        
      case 3:
        print_all("cabra");
        play_audio(cabra);
        break;
      case 4:
        print_all("cerdo");
        play_audio(cerdo);
        break;
      case 5:
        print_all("conejo");
        play_audio(conejo);
        break;
        
      case 6:
        print_all("perro");
        play_audio(perro);
        break;
      case 7:
        print_all("oveja");
        play_audio(oveja);
        break;
      case 8:
        print_all("buho");
        play_audio(buho);
        break;
    }
    
  }else if(language == 1){
    //green english
    switch(piece_number){
      case 0:
        print_all("cat");
        play_audio(cat);
        break;
      case 1:
        print_all("cow");
        play_audio(cow);
        break;
      case 2:
        print_all("duck");
        play_audio(duck);
        break;
        
      case 3:
        print_all("goat");
        play_audio(goat);
        break;
      case 4:
        print_all("pig");
        play_audio(pig);
        break;
      case 5:
        print_all("rabbit");
        play_audio(rabbit);
        break;
        
      case 6:
        print_all("dog");
        play_audio(dog);
        break;
      case 7:
        print_all("sheep");
        play_audio(sheep);
        break;
      case 8:
        print_all("owl");
        play_audio(owl);
        break;
    }
    
  }else if(language == 2){
    //blue french
    switch(piece_number){
      case 0:
        print_all("chat");
        play_audio(chat);
        break;
      case 1:
        print_all("vache");
        play_audio(vache);
        break;
      case 2:
        print_all("canard");
        play_audio(canard);
        break;
        
      case 3:
        print_all("bouc");
        play_audio(bouc);
        break;
      case 4:
        print_all("cochon");
        play_audio(cochon);
        break;
      case 5:
        print_all("lapin");
        play_audio(lapin);
        break;
        
      case 6:
        print_all("chien");
        play_audio(chien);
        break;
      case 7:
        print_all("mouton");
        play_audio(mouton);
        break;
      case 8:
        print_all("hibou");
        play_audio(hibou);
        break;
    }
  }
}

/*triggered when a piece has just been set in*/
void piece_in(int piece_number){
  play_audio(tada);
  float total = 0;
  int count = 0;
  for(int i = 0; i<local_average_size;i++){
    if((iterations - i) > 0){
      total += values_buffer[piece_number][(iterations - i) % buffer_size];
      count++;
    }else{
      break;
    }
  }
  float current_value = total / count;
  transparency[piece_number] = current_value / current_ambient;

  none_readings[piece_number] = current_value;
  readings_till_identified[piece_number] = 4;
}

/*finds the index of the lowest value*/
float min_index(float a[], int size1){
  if(size1 > 0){
    float lowest_value = a[0];
    int lowest_index = 0;
    for(int i = 0; i<size1; i++){
      if(a[i] < lowest_value){
        lowest_value = a[i];
        lowest_index = i;
      }
    }
    return lowest_index;
  }else{
    return 0;
  }
}

/*gets the resistance of the ldr in the specified slot*/
float getLdrResistance(int piece_number){
  int ldr_pin = ldr_pins[piece_number];
  float sensorValue = analogRead(ldr_pin);
  float resistance = ((INPUT_RANGE / sensorValue) - 1) * KNOWN_RESISTANCE;
  return resistance;
}

/*gets the measured brightness in the specified slot(not effected by calibration)*/
float getIntensity(int piece_number){
  float resistance = getLdrResistance(piece_number);
  float base_formula = 16000 * pow(resistance, -2) * 100000;
  float light_intensity = base_formula * intensity_coefficients[piece_number];
  if(light_intensity < 0){
    light_intensity = 0;
  }
  return light_intensity;
}

/* Scales the readings so that they are all the same value.
 * While maintaining the average reading
 */
void calibrate_ldrs(){
  //total of intensities coefficients should equal pieces(so each has a mean of 1 - so no effect from base result)
  float total_base = 0;
  float base_intensity;
  for(int piece_number = 0; piece_number<pieces; piece_number++){
    base_intensity = current_values[piece_number] / intensity_coefficients[piece_number];
    total_base += base_intensity;
  }
  
  float average_base = total_base/pieces;
  for(int piece_number = 0; piece_number<pieces; piece_number++){
    base_intensity = current_values[piece_number] / intensity_coefficients[piece_number];
    intensity_coefficients[piece_number] = float((1 / base_intensity) * average_base);
  }
}

/*checks if the state of any piece has changed*/
void check_piece_in(){
  float now_millis = millis();
  //checks pending piece
  if(pending){
    float percentage = (current_values[index_pending] / current_ambient);
    if(direction_pending == 0){//pending out
      if(percentage >= threshold){
        //expected
        if((now_millis - millis_pending) > exit_delay){
          pieces_in[index_pending] = direction_pending;//direction_pending = 0
          
          pending = false;
          millis_pending = 0;
          //below doesn't matter
          direction_pending = 0;
          index_pending = 0;
        }
      }else{
        pending = false;
        millis_pending = 0;
        //below doesn't matter
        direction_pending = 0;
        index_pending = 0;
      }
    }else{//pending in
      if(percentage <= threshold){
        //expected
        if((now_millis - millis_pending) > entry_delay){
          pieces_in[index_pending] = direction_pending;//direction_pending = 1
          piece_in(index_pending);
          
          pending = false;
          millis_pending = 0;
          //below doesn't matter
          direction_pending = 0;
          index_pending = 0;
        }
      }else{
        pending = false;
        millis_pending = 0;
        //below doesn't matter
        direction_pending = 0;
        index_pending = 0;
      }
    }
  }

  //checks if a piece should be pending
  if(!pending){
    for(int piece_number = 0; piece_number<pieces; piece_number++){
      float percentage = (current_values[piece_number] / current_ambient);
      if(pieces_in[piece_number] == 0){//piece out
        if(percentage <= threshold){
          //piece should be in
          pending = true;
          millis_pending = now_millis;
          //below doesn't matter
          direction_pending = 1;
          index_pending = piece_number;
          break;
        }
      }else{//piece in
        if(percentage >= threshold && readings_till_identified[piece_number] == 0){
          //piece should be out
          pending = true;
          millis_pending = now_millis;
          //below doesn't matter
          direction_pending = 0;
          index_pending = piece_number;
          break;
        }
      }
    }
  }
}

/*gets the measured brightness in the specified slot(effected by calibration)*/
float getRealIntensity(int piece_number){
  float measured_intensity = getIntensity(piece_number);
  if(pieces_in[piece_number]){
    float red_led_effect = (red / 1023.0) * red_reflected[piece_number];
    float green_led_effect = (green / 1023.0) * green_reflected[piece_number];
    float blue_led_effect = (blue / 1023.0) * blue_reflected[piece_number];
    float total_led_effect = red_led_effect + green_led_effect + blue_led_effect;
    float real_intensity = measured_intensity - total_led_effect;
    
    if(real_intensity >= 0){
      return real_intensity;
    }else{
      //less than zero
      return 0;
    }
  }else{
    return measured_intensity;
  }
}

/*gets all ldr readings and updates the rolling average*/
void update_ldr_readings(){
  for(int piece_number = 0; piece_number<pieces; piece_number++){
    values_buffer[piece_number][iterations % buffer_size] = getRealIntensity(piece_number);
  }

  for(int piece_number = 0; piece_number<pieces; piece_number++){
    float latest_value = values_buffer[piece_number][iterations % buffer_size];
    int index_of_last_total = (iterations - 1);
    float last_total = 0;
    if(index_of_last_total >= 0){//no values have been assigned yet
      last_total = totals_buffer[piece_number][index_of_last_total % buffer_size];
    }
    
    int index_of_oldest_value = (iterations - sfi);
    float oldest_value_in_last_total = 0;
    if(index_of_oldest_value >= 0){//no values have been assigned yet
      oldest_value_in_last_total = values_buffer[piece_number][index_of_oldest_value % buffer_size];
    }
  
    totals_buffer[piece_number][iterations % buffer_size] = last_total - oldest_value_in_last_total + latest_value;

    float number_of_elements;
    if(iterations < sfi){
      number_of_elements = iterations + 1;
    }else{
      number_of_elements = sfi;
    }
    current_values[piece_number] = totals_buffer[piece_number][iterations % buffer_size] / number_of_elements;
    if(current_values[piece_number] < 0){
      current_values[piece_number] = 0;
    }
  }
}

/*calculates the average ambient brightness of all pieces were out*/
void getAmbient(){
  int count_in = 0;
  int count_out = 0;
  int count_pending = 0;
  int count_unusable = 0;
  for(int piece_number = 0; piece_number<pieces; piece_number++){
    if(pending && (index_pending == piece_number)){
      count_pending++;
    }else if(pieces_in[piece_number]){
      if((current_values[piece_number] > 0) 
      && (transparency[piece_number] > min_transparency)
      && (readings_till_identified[piece_number] == 0)){
        count_in++;
      }else{
        count_unusable++;
      }
    }else{
      count_out++;
    }
  }
  
  float predictions_in[count_in];
  float predictions_out[count_out];
  int in_i = 0;
  int out_i = 0;
  for(int piece_number = 0; piece_number<pieces; piece_number++){
    if(pending && (index_pending == piece_number)){
      
    }else if(pieces_in[piece_number]){
      if((current_values[piece_number] > 0)
      && (transparency[piece_number] > min_transparency)
      && (readings_till_identified[piece_number] == 0)){
        predictions_in[in_i] = current_values[piece_number] / transparency[piece_number];
        in_i++;
      }
    }else{//piece out
      predictions_out[out_i] = current_values[piece_number];
      out_i++;
    }
  }
  
  if(count_in > 0 || count_out > 0){
    if(count_out >= count_in){//uses whichever category has more readings
      current_ambient = mean(predictions_out, count_out);//mean as no extreme values 
    }else{
      current_ambient = median(predictions_in, count_in);//median as extreme values but middle normally right
    }
  }//otherwise ambient doesn't change as no readings
  
  if(current_ambient <= 0){
    current_ambient = just_above_zero;
  }
}

/*bubble sort*/
void sort(float b[], int sizeb) {
  boolean completely_right;
  for(int i=0; i<(sizeb-1); i++) {
    completely_right = true;
    for(int o=0; o<(sizeb-(i+1)); o++) {
      if(b[o] > b[o+1]) {
        completely_right = false;
        float t = b[o];
        b[o] = b[o+1];
        b[o+1] = t;
      }
    }
    
    if(completely_right){
      break;
    }
  }
}

/*finds the median in an array(if 2 middle elements uses lower one)*/
float median(float a[], int size1){
  if(size1 > 0){
    float result;
    sort(a, size1);
    if(size1 % 2 == 0){
      result = a[size1/2];
    }else{//odd
      float smaller_middle = a[(size1-1)/2];
      return smaller_middle;//smaller value more conservative for this project
    }
    return result;
  }else{
    return 0;
  }
}

/*finds the mean of an array*/
float mean(float a[], int size1) {
  if(size1 > 0){
    float total = 0;
    for(int i=0; i<size1; i++) {
      total += a[i];
    }
    return total / size1;
  }else{
    return 0;
  }
}

/*updates the leds with the latest values*/
void update_leds(){
  if(is_power_on){
    for(int piece_number = 0; piece_number<pieces; piece_number++){
      digitalWrite(digital_pins[piece_number], pieces_in[piece_number]);
    }
    
    int adjusted_red = int(red * (brightness / 100.0));
    int adjusted_green = int(green * (brightness / 100.0));
    int adjusted_blue = int(blue * (brightness / 100.0));

    ledcWrite(redPwmChannel, adjusted_red);
    ledcWrite(greenPwmChannel, adjusted_green);
    ledcWrite(bluePwmChannel, adjusted_blue);
  }else{
    ledcWrite(redPwmChannel, 0);
    ledcWrite(greenPwmChannel, 0);
    ledcWrite(bluePwmChannel, 0);
  }
}

/*uses data about reflected light to determine which piece has been put in*/
void identify_piece(int piece_number){
  float red_change = red_readings[piece_number] - none_readings[piece_number];
  float green_change = green_readings[piece_number] - none_readings[piece_number];
  float blue_change = blue_readings[piece_number] - none_readings[piece_number];
  
  if(red_change < 0){
    red_change = 0;
  }
  if(green_change < 0){
    green_change = 0;
  }
  if(blue_change < 0){
    blue_change = 0;
  }
  red_reflected[piece_number] = red_change;
  green_reflected[piece_number] = green_change;
  blue_reflected[piece_number] = blue_change;

  float red_percent;
  float green_percent;
  float blue_percent;
  float total_change = red_change + green_change + blue_change;
  if(total_change != 0){
    red_percent = red_change / total_change;
    green_percent = green_change / total_change;
    blue_percent = blue_change / total_change;
  }else{
    red_percent = 0;
    green_percent = 0;
    blue_percent = 0;
  }
  
  float distance_to_known[KNOWN_PIECES];
  for(int known_piece = 0; known_piece < KNOWN_PIECES; known_piece++){
    float diff = pow(red_percent - pieces_expected[known_piece][0], 2);
    diff += pow(green_percent - pieces_expected[known_piece][1], 2);
    diff += pow(blue_percent - pieces_expected[known_piece][2], 2);
    distance_to_known[known_piece] = diff;
  }
  int index = min_index(distance_to_known, KNOWN_PIECES);
  pieceIdentified(piece_number, index);
}

/*constructs any commands from recieved characters*/
void process_character(char c){
  Serial.write(c);
  SerialBT.write(c);
  if(c == '\n'){
    if(part==0){// command
      if(key=="button_power"){
        is_power_on = !is_power_on;
        if(is_power_on){
          play_audio(power_on);
        }else{
          play_audio(power_off);
        }
        update_leds();
      }
      if(is_power_on){
        if(key=="button_mute"){
          if(is_muted){
            is_muted = false;
            play_audio(unmuted);
          }else{
            play_audio(muted);
            is_muted = true;
          }
        }else if(key=="button_volume_up"){
          volume += volume_step;
          play_audio(volume_increased);
        }else if(key=="button_volume_down"){
          volume -= volume_step;
          if(volume <= 0){
            //makes sure that the volume can't go down to zero(which would be confusing)
            volume = volume_step;
          }
          play_audio(volume_decreased);
        }else if(key=="button_brightness_up"){
          brightness += brightness_step;
          play_audio(brightness_increased);
        }else if(key=="button_brightness_down"){
          brightness -= brightness_step;
          if(brightness <= 0){
            //makes sure that the volume can't go down to zero(which would be confusing)
            brightness = brightness_step;
          }
          play_audio(brightness_decreased);
        }else if(key=="button_auto"){
          play_audio(auto_detect);
          language_selected = 0;
        }else if(key=="button_spanish"){
          play_audio(espanol);
          language_selected = 1;
        }else if(key=="button_english"){
          play_audio(english);
          language_selected = 2;
        }else if(key=="button_french"){
          play_audio(francais);
          language_selected = 3;
        }else if(key=="button_calibrate"){
          play_audio(calibrate);
          calibrate_ldrs();
        }
      }
    }else if(part==1){//a command with a value
      //For backwards compatibility
      if(key=="volume"){
        int recieved = constrain(value.toInt(),0,255);
        volume = recieved / 2.55;//converts to between 0 - 100%
      }else if(key=="brightness"){
        int recieved = constrain(value.toInt(),0,255);
        brightness = recieved / 2.55;//converts to between 0 - 100%
      }else if(key=="language"){
        language_selected = constrain(value.toInt(),0,3);//0 is auto
        if(language_selected == 0){
          play_audio(auto_detect);
        }else if(language_selected == 1){
          play_audio(espanol);
        }else if(language_selected == 2){
          play_audio(english);
        }else if(language_selected == 3){
          play_audio(francais);
        }
        
      }else if(key=="transmit_data"){
        
      }else if(key=="get_millis"){
        //easier than making new functions
        Serial.print("millis:");
        Serial.println(millis());
        SerialBT.print("millis:");
        SerialBT.println(millis());
      }else if(key=="calibrate_ldrs"){
        calibrate_ldrs();
      }
    }
    
    if(brightness > 100){
      brightness = 100;
    }else if(brightness < 0){
      brightness = 0;
    }
    
    if(volume > 100){
      volume = 100;
    }else if(volume < 0){
      volume = 0;
    }
    
    DacAudio.DacVolume=volume;

    //resets values
    key = "";
    value = "";
    part = 0;
    print_all("OK");
  }else if(c == ':'){
    //at the dividing point
    part++;//changes the part to assemble
  }else if(part==0){
    //part0
    key = key + c;//assembles the key
  }else if(part==1){
    //part1
    value = value + c;//assembles the value
  }else{
    print_all("Invalid part number");
  }
}

/*checks for recieved characters*/
void check_serial(){
  while(SerialBT.available()){
    process_character(SerialBT.read());
  }
  while(Serial.available()){
    process_character(Serial.read());
  }
}

/*initialises device*/
void setup() {
  //initialises arrays
  for(int piece_number = 0; piece_number<pieces; piece_number++){
    pieces_in[piece_number] = 0;
    intensity_coefficients[piece_number] = 1;
  }
  
  Serial.begin(115200);
  SerialBT.begin("Educational Toy");

  ledcSetup(redPwmChannel, PWMFreq, PWMResolution);
  ledcSetup(greenPwmChannel, PWMFreq, PWMResolution);
  ledcSetup(bluePwmChannel, PWMFreq, PWMResolution);

  ledcAttachPin(red_pin, redPwmChannel);
  ledcAttachPin(green_pin, greenPwmChannel);
  ledcAttachPin(blue_pin, bluePwmChannel);
  
  for(int piece_number = 0; piece_number<pieces; piece_number++){
    pinMode(digital_pins[piece_number], OUTPUT);
    digitalWrite(digital_pins[piece_number], pieces_in[piece_number]);
    
    pinMode(ldr_pins[piece_number], INPUT);
  }
  
  red = 0;
  green = 0;
  blue = 0;

  update_leds();

  if(is_muted){
    DacAudio.DacVolume=0;
  }else{
    DacAudio.DacVolume=volume;
  }
  
  Sound = new XT_Wav_Class(startUp);
  DacAudio.Play(Sound);
}

/*main loop*/
void loop() {
  check_serial();
  if(is_power_on){
    if(iterations == 100){
      //calibrates ldrs at start(after enough initial data has been collected)
      calibrate_ldrs();
    }
    
    update_ldr_readings();
    getAmbient();
    check_piece_in();
    
    part_of_sequence = (iterations % (length_of_sequence/sequence_speed)) * sequence_speed;
    
    if(part_of_sequence % 1024 == 0){//about to switch colour
      for(int piece_number = 0; piece_number<pieces; piece_number++){
        if(pieces_in[piece_number]){
          //saves readings so it can be used to identify the piece
          if(part_of_sequence == 1024){
            red_readings[piece_number] = getIntensity(piece_number);
          }else if(part_of_sequence == 2048){
            green_readings[piece_number] = getIntensity(piece_number);
          }else if(part_of_sequence == 0){
            blue_readings[piece_number] = getIntensity(piece_number);
          }

          //checks if a piece has enough data to be identified
          if(readings_till_identified[piece_number] == 1){
            identify_piece(piece_number);
          }
          if(readings_till_identified[piece_number] > 0){
            readings_till_identified[piece_number]--;
          }
        }
      }
    }

    //updates the colours sequence
    red = 0;
    green = 0;
    blue = 0;
    if(solid_colour){
      if(part_of_sequence < 1024){
        red = on_brightness;
      }else if(part_of_sequence < 2048){
        green = on_brightness;
      }else if(part_of_sequence < 3072){
        blue = on_brightness;
      }
    }else{
      if(part_of_sequence < 1024){
        red = (part_of_sequence % 1024 + sequence_speed -1);
      }else if(part_of_sequence < 2048){
        green = (part_of_sequence % 1024 + sequence_speed -1);
      }else if(part_of_sequence < 3072){
        blue = (part_of_sequence % 1024 + sequence_speed -1);
      }
    }
    update_leds();
    
    iterations++;
  }

  if(Sound -> Playing){
    //keeps loading more audio data
    DacAudio.FillBuffer();
  }

  //ensures time per iteration is constant by delaying remaining time
  float time_past = micros() - last_time;
  float delay_time = time_per_iteration - time_past;
  if(delay_time > 0){
    delayMicroseconds(delay_time);
  }
  last_time = micros();
  
}
