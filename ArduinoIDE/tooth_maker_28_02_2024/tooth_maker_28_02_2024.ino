#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

// last modified: 29th Feb 2024

//define input pins
/////////////////////////////////////////////////////////////////////
#define NUM_OF_IN_PINS 17

#define PRESS_INC 39
#define PRESS_DEC 38

#define LIMIT_SWITCH_KRAJ 37
#define LIMIT_SWITCH_POCETAK 36
#define LIMIT_SWITCH_PRESS 35

#define HOMING 34

#define STEP_DEC 33
#define STEP_INC 32

#define TOOL_SELECT 27

#define STEP_LEFT 9
#define STEP_RIGHT 8

#define CONT_MOVE_LEFT 7
#define CONT_MOVE_RIGHT 6

#define PRESS_GORE 5
#define PRESS_DOLJE 4

#define MANUAL_AUTO 3
#define START_STOP 2

//define output pins
/////////////////////////////////////////////////////////////////////
#define NUM_OF_OUT_PINS 5

#define SOLENOID 46

#define STEPPER_WAGON_PUL 10
#define STEPPER_WAGON_DIR 11

#define STEPPER_TOOL_PUL  31
#define STEPPER_TOOL_DIR  29


//global variables
/////////////////////////////////////////////////////////////////////

//lcd
LiquidCrystal_I2C lcd(0x3F,16,2);



//button control

/*
When a state change is detected, a timer is started.
A debounce-button is considered pressed if
 1) it is pressed,
 2) at least [debounce_delay] has passed since last state change.

For continuous movement of wagon, the debounce logic is executed only 
for [CONT_MOVE_LEFT] and [CONT_MOVE_RIGHT]. This is done because the 
debounce logic is expensive and slows down the continous movement.
*/

const int MAX = 105;
const int number_of_debounced_buttons = 13;
const int debounce_delay = 50;

int button_state[MAX];  // can be HIGH or LOW, is kept up to date with whether button is pressed or not
bool is_button_ok[MAX];  // button is considered ok if at least [debounce_delay] time passed since last button state change
bool is_button_idle[MAX];  // used as a flag for buttons that are not intented to be held down, if held idle becomes false
long long last_debounce_time[MAX];  // time of last button state change
int debounced_buttons_list[number_of_debounced_buttons] = {START_STOP, MANUAL_AUTO, CONT_MOVE_RIGHT, CONT_MOVE_LEFT, PRESS_DOLJE, PRESS_GORE, 
                                                           STEP_RIGHT, STEP_LEFT, STEP_INC, STEP_DEC, HOMING, PRESS_INC, PRESS_DEC};



//wagon motor movement

/*
The motor is moved by pulsing the motor step pin.
The delay between pulses determines the motor speed. 

In each loop iteration, the following happens:
  1) the wagon motor state is determined based on which buttons are pressed,
  2) the wagon is moved based on its current state
    -homing -> move until limit switch is activated, then move back a little, also reset step counter
    -single step -> move just one step in certain direction
    -continuous step -> motor moves continuously with speed depending on how long the button is held
*/

// used to limit acceleration / deceleration
const long long MIN_PULSE_DELAY = 100;
const long long MAX_PULSE_DELAY = 1000;  // if current pulse delay is equal to max, this actually means no movement

// possible modes
const int MOTOR_ACCELERATE = 1;
const int MOTOR_DECELERATE = 2;
const int MOTOR_STEP = 3;
const int MOTOR_HOMING = 4;

long long current_pulse_delay = MAX_PULSE_DELAY;  // controls delay between pulses (motor speed)
int stepper_direction = LOW;  // LOW = LEFT, HIGH = RIGHT
int stepper_state = MOTOR_DECELERATE;  // [MOTOR_DECELERATE] is default state



//step size

// used to limit acceleration / deceleration
const long long MIN_STEP_SIZE = 0;
const long long MAX_STEP_SIZE = 9999;
const long long DEFAULT_STEP_SIZE = 300;  // 3mm

// possible modes
const int STEP_SIZE_ACCELERATE = 1;  // buttons for changing step size are currently being pressed
const int STEP_SIZE_IDLE = 2;  // no buttons are being pressed, the step size should not change

long long current_step_size = DEFAULT_STEP_SIZE;
long long number_of_wagon_motor_pulses = 0;
int step_size_sign = 0;  // used to determine whether step size should increase or decrease
int step_size_at_debounce = 0;
int step_size_state = STEP_SIZE_IDLE;

int step_counter = 0;  // how many single steps have been made relative to home position




//press lcd

// used to limit acceleration / deceleration
const long long MIN_TOOL_LCD = 0;
const long long MAX_TOOL_LCD = 9999;
const long long DEFAULT_PRESS_LCD = 100;  // 1 second
const long long DEFAULT_MOTOR_LCD = 4;  // represents motor angle divided by 90 degrees

// possible modes
const int TOOL_LCD_ACCELERATE = 1;
const int TOOL_LCD_IDLE = 2;

long long current_tool_lcd = DEFAULT_PRESS_LCD;
long long number_of_tool_motor_pulses = 0;
int tool_lcd_sign = 0;  // used to determine whether press delay should increase or decrease
int tool_lcd_at_debounce = 0;
int tool_lcd_state = TOOL_LCD_IDLE;




//mode
const int MANUAL_MODE = 1;  // allows for manual movement of wagon, and manual movement of press
const int AUTOMATIC_MODE_IDLE = 2;  // currently in automatic mode, but stop button was pressed
const int AUTOMATIC_MODE_ACTIVE = 3;  // currently in automatic mode, and is active

const int TOOL_PRESS = 1;  // teeth are punched out using a press
const int TOOL_MOTOR = 2;  // teeth are punched out using a step motor

int current_mode = MANUAL_MODE;
int current_tool = TOOL_PRESS;




// automatic movement
int PRESS_DELAY = 1000;  // 1 second
int TOOL_PULSES_FULL_TURN = 3200;
int wagon_pulses_left = 0;  // used for automatic mode, determines how much the wagon has left to move
int tool_pulses_left = 0;  // used for automatic mode, determines how much the tool motor has left to move
int press_delay_left = 0;  // used for automatic mode, determines how much the press has left to move

/////////////////////////////////////////////////////////////////////

void setup(){
  //input
  int in_pin_list[NUM_OF_IN_PINS] = {START_STOP, MANUAL_AUTO, PRESS_DOLJE, PRESS_GORE, CONT_MOVE_RIGHT, CONT_MOVE_LEFT, STEP_RIGHT, STEP_LEFT, 
                                      STEP_INC, STEP_DEC, HOMING, LIMIT_SWITCH_PRESS, LIMIT_SWITCH_POCETAK, LIMIT_SWITCH_KRAJ, PRESS_INC, PRESS_DEC, TOOL_SELECT};

  for (int i = 0; i < NUM_OF_IN_PINS; i++)
    pinMode(in_pin_list[i], INPUT_PULLUP);
  

  //output
  int out_pin_list[NUM_OF_OUT_PINS] = {STEPPER_WAGON_PUL, STEPPER_WAGON_DIR, STEPPER_TOOL_DIR, STEPPER_TOOL_PUL, SOLENOID};

  for (int i = 0; i < NUM_OF_OUT_PINS; i++)
    pinMode(out_pin_list[i], OUTPUT);
  

  //serial init
  Serial.begin(9600);

  //debounce init
  for (int i = 0; i < number_of_debounced_buttons; i++){
    int button_pin = debounced_buttons_list[i];
    button_state[button_pin] = HIGH;
    is_button_idle[button_pin] = true;
  }

  //solenoid init
  digitalWrite(SOLENOID, LOW);

  //LCD init
  lcd.init();
  lcd.clear();
  lcd.backlight();
  update_lcd();
}

void press_loop(){
  if (current_tool == TOOL_PRESS) {
    if (digitalRead(PRESS_GORE) == LOW)
      digitalWrite(SOLENOID, LOW);
    
    else if (digitalRead(PRESS_DOLJE) == LOW)
      digitalWrite(SOLENOID, HIGH);
  } else {
    if (is_debounced_button_pressed(PRESS_DOLJE) && is_button_idle[PRESS_DOLJE]){
      is_button_idle[PRESS_DOLJE] = false;
      tool_motor_single_step();
    }
  }
}

void output_step_size(){  // debug
  if (current_mode == MANUAL_MODE)
    Serial.print("Manual mode: ");
  
  else if (current_mode == AUTOMATIC_MODE_IDLE)
    Serial.print("Automatic mode idle: ");
  
  else
    Serial.print("Automatic mode active: ");
  

  int br = (int)current_step_size;
  Serial.print(br / 100);
  Serial.print(".");
  if (br % 100 < 10)
    Serial.print("0");
  Serial.print(br % 100);
  Serial.println("");
}

void determine_state_loop(){
  if (is_debounced_button_pressed(MANUAL_AUTO) && is_button_idle[MANUAL_AUTO]){
    is_button_idle[MANUAL_AUTO] = false;
    if (current_mode == MANUAL_MODE) {
      current_mode = AUTOMATIC_MODE_IDLE;
      update_lcd();
    } else{
      launch_idle_mode();
      current_mode = MANUAL_MODE;
      update_lcd();
    }
  }
  if (is_debounced_button_pressed(START_STOP) && is_button_idle[START_STOP]){
    is_button_idle[START_STOP] = false;
    if (current_mode == AUTOMATIC_MODE_IDLE) {
      current_mode = AUTOMATIC_MODE_ACTIVE;
      update_lcd();
    } else if (current_mode == AUTOMATIC_MODE_ACTIVE) {
      current_mode = AUTOMATIC_MODE_IDLE;
      update_lcd();
    }
  }
  if (digitalRead(TOOL_SELECT) == LOW) {
    if (current_tool == TOOL_MOTOR) {
      current_tool_lcd = DEFAULT_PRESS_LCD;
    }
    current_tool = TOOL_PRESS;
  } else {
    if (current_tool == TOOL_PRESS) {
      current_tool_lcd = DEFAULT_MOTOR_LCD;
    }
    current_tool = TOOL_MOTOR;
  }
}


/*
The general loop design is as follows:
  1) read input from buttons and record the state
  2) determine current inner state depending on which buttons are pressed
  3) do certain action depending on current state

Note that the logic for continuous movement is done before everything else 
because it is very time sensitive. 
*/

void loop(){
  // prioritize continuous left and right

  update_debounced_buttons_state(false); // checks only for continuos movement left and right

  if (current_pulse_delay != MAX_PULSE_DELAY) {  // checks if wagon is moving or not
    change_wagon_motor_state_loop(false);
    update_motor();
    return;
  }
    
  //////////////////////////////////////////
  
  update_debounced_buttons_state(true);

  determine_state_loop();

  if (current_mode == MANUAL_MODE){
    change_wagon_motor_state_loop(true);
    update_motor();

    change_step_size_loop();
    update_step_size();

    change_tool_lcd_loop();
    update_tool_lcd();

    press_loop();
  }
  else if (current_mode == AUTOMATIC_MODE_ACTIVE)
    automatic_movement();

  else if (current_mode == AUTOMATIC_MODE_IDLE)
    launch_idle_mode();

  //output_step_size();
}
