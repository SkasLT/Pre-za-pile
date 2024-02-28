#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

// define input pins
/////////////////////////////////////////////////////////////////////
#define PRESA_INC            39
#define PRESA_DEC            38
#define LIMIT_SWITCH_KRAJ    37
#define LIMIT_SWITCH_POCETAK 36
#define LIMIT_SWITCH_PRESA   35

#define HOMING               34

#define STEP_INC             32
#define STEP_DEC             33

#define STEP_LEFT            9
#define STEP_RIGHT           8

#define CONT_MOVE_LEFT       7
#define CONT_MOVE_RIGHT      6

#define PRESA_GORE           5
#define PRESA_DOLJE          4

#define MANUAL_AUTO          3
#define START_STOP           2

// define output pins
/////////////////////////////////////////////////////////////////////
#define SOLENOID    46
#define STEPPER_PUL 10
#define STEPPER_DIR 11

// global variables
/////////////////////////////////////////////////////////////////////

// lcd
LiquidCrystal_I2C lcd(0x3F, 16, 2);

// button control
const int MAX = 105;
const int number_of_debounced_buttons = 11;
const int debounce_delay = 50;

int button_state[MAX];
int is_button_ok[MAX];
int is_button_idle[MAX];
long long last_debounce_time[MAX];
int debounced_buttons_list[number_of_debounced_buttons] = {2, 3, 6, 7, 8, 9, 32, 33, 34, 38, 39};

// motor movement
const long long MIN_STEP_DELAY = 100;
const long long MAX_STEP_DELAY = 1000;

const int MOTOR_ACCELERATE = 1;
const int MOTOR_DECELERATE = 2;
const int MOTOR_STEP = 3;
const int MOTOR_HOMING = 4;

long long current_step_delay = MAX_STEP_DELAY;
int stepper_direction = LOW;
int stepper_state = MOTOR_DECELERATE;

// step size
const long long MIN_STEP_SIZE = 0;
const long long MAX_STEP_SIZE = 9999;
const long long DEFAULT_STEP_SIZE = 300;

const int STEP_SIZE_ACCELERATE = 1;
const int STEP_SIZE_IDLE = 2;

long long current_step_size = DEFAULT_STEP_SIZE;
long long number_of_steps = 0;
int step_size_sign = 0;
int step_size_at_debounce = 0;
int step_size_state = STEP_SIZE_IDLE;

int step_counter = 0;

// presa size
const long long MIN_PRESA_SIZE = 0;
const long long MAX_PRESA_SIZE = 9999;
const long long DEFAULT_PRESA_SIZE = 50;

const int PRESA_SIZE_ACCELERATE = 1;
const int PRESA_SIZE_IDLE = 2;

long long current_presa_size = DEFAULT_PRESA_SIZE;
int presa_size_sign = 0;
int presa_size_at_debounce = 0;
int presa_size_state = PRESA_SIZE_IDLE;

// mode
const int MANUAL_MODE = 1;
const int AUTOMATIC_MODE_IDLE = 2;
const int AUTOMATIC_MODE_ACTIVE = 3;
int PRESA_DELAY = 5000;

int current_mode = MANUAL_MODE;
int steps_left = 0;
int presa_delay_left = 0;

/////////////////////////////////////////////////////////////////////

void display_num(int val, int digits)
{
    char s[digits];
    for(int i = 0; i < digits; i++)
        s[i] = ' ';
    int sign = 1;
    if(val < 0)
    {
        sign = -1;
        val = -val;
    }
    for(int i = 0; i < digits; i++)
    {
        if(val == 0 && i > 0)
        {
            if(sign == 1)
            {
                s[digits - 1 - i] = '#';
            }
            else if(sign == -1)
            {
                s[digits - 1 - i] = '-';
                s[digits - 2 - i] = '#';
            }
            break;
        }
        s[digits - 1 - i] = (char)('0' + val % 10);
        val /= 10;
    }
    lcd.print(s);
}

void update_lcd()
{
    lcd.setCursor(0, 0);
    lcd.print("Step: ");
    lcd.print((int)(current_step_size / 100));
    lcd.print(".");
    lcd.print((int)(current_step_size % 100 / 10));
    lcd.print((int)(current_step_size % 10));
    lcd.print(' ');

    lcd.setCursor(0, 1);
    lcd.print("Delay: ");
    lcd.print((int)(current_presa_size / 100));
    lcd.print(".");
    lcd.print((int)(current_presa_size % 100 / 10));
    lcd.print((int)(current_presa_size % 10));
    lcd.print(' ');

    if(current_mode == MANUAL_MODE)
    {
        lcd.setCursor(12, 0);
        lcd.print(" MAN");
    }
    else
    {
        lcd.setCursor(12, 0);
        lcd.print("AUTO");
    }

    lcd.setCursor(11, 1);
    display_num(step_counter, 5);
}

void setup()
{
    // input
    int num_of_in_pins = 16;
    int in_pin_list[num_of_in_pins] = {2, 3, 4, 5, 6, 7, 8, 9, 32, 33, 34, 35, 36, 37, 38, 39};

    for(int i = 0; i < num_of_in_pins; i++)
        pinMode(in_pin_list[i], INPUT_PULLUP);


    // output
    int num_of_out_pins = 3;
    int out_pin_list[num_of_out_pins] = {10, 11, 46};

    for(int i = 0; i < num_of_out_pins; i++)
        pinMode(out_pin_list[i], OUTPUT);


    // serial init
    Serial.begin(9600);

    // debounce init
    for(int i = 0; i < number_of_debounced_buttons; i++)
    {
        int button_pin = debounced_buttons_list[i];
        button_state[button_pin] = HIGH;
        is_button_idle[button_pin] = true;
    }

    // solenoid init
    digitalWrite(SOLENOID, LOW);

    // LCD init
    lcd.init();
    lcd.clear();
    lcd.backlight();
    update_lcd();
}

void stepper_move(long long steps, int delay_time, int dir)
{     // LOW = LEFT, HIGH = RIGHT
    digitalWrite(STEPPER_DIR, dir);

    for(long long i = 0; i < steps; i++)
    {
        if(digitalRead(LIMIT_SWITCH_PRESA) == LOW)
            break;
        if(dir == LOW && digitalRead(LIMIT_SWITCH_POCETAK) == LOW)
            break;
        if(dir == HIGH && digitalRead(LIMIT_SWITCH_KRAJ) == LOW)
            break;
        digitalWrite(STEPPER_PUL, LOW);
        digitalWrite(STEPPER_PUL, HIGH);
        delayMicroseconds(delay_time);
    }
}

bool is_debounced_button_pressed(int pin)
{
    return button_state[pin] == LOW && is_button_ok[pin];
}

void update_single_debounced_button(int button_pin)
{
    int reading = digitalRead(button_pin);
    if(reading != button_state[button_pin])
    {
        last_debounce_time[button_pin] = millis();
        if(button_pin == STEP_INC || button_pin == STEP_DEC)
            step_size_at_debounce = current_step_size;
        if(button_pin == PRESA_INC || button_pin == PRESA_DEC)
            presa_size_at_debounce = current_presa_size;
    }
    button_state[button_pin] = reading;

    if((millis() - last_debounce_time[button_pin]) > debounce_delay)
    {
        is_button_ok[button_pin] = true;
        if(button_state[button_pin] == HIGH)
            is_button_idle[button_pin] = true;
    }
}

void update_debounced_buttons_state(bool check_all)
{
    update_single_debounced_button(CONT_MOVE_LEFT);
    update_single_debounced_button(CONT_MOVE_RIGHT);
    if(!check_all)
        return;
    for(int i = 0; i < number_of_debounced_buttons; i++)
    {
        int button_pin = debounced_buttons_list[i];
        update_single_debounced_button(button_pin);
    }
}

void change_motor_state_loop(bool check_all)
{
    if(is_debounced_button_pressed(CONT_MOVE_LEFT))
    {
        stepper_direction = LOW;
        stepper_state = MOTOR_ACCELERATE;
    }
    else if(is_debounced_button_pressed(CONT_MOVE_RIGHT))
    {
        stepper_direction = HIGH;
        stepper_state = MOTOR_ACCELERATE;
    }
    else
    {
        stepper_state = MOTOR_DECELERATE;
    }
    if(!check_all)
        return;
    if(is_debounced_button_pressed(STEP_LEFT) && is_button_idle[STEP_LEFT])
    {
        is_button_idle[STEP_LEFT] = 0;
        stepper_direction = LOW;
        stepper_state = MOTOR_STEP;
    }
    else if(is_debounced_button_pressed(STEP_RIGHT) && is_button_idle[STEP_RIGHT])
    {
        is_button_idle[STEP_RIGHT] = 0;
        stepper_direction = HIGH;
        stepper_state = MOTOR_STEP;
    }
    if(is_debounced_button_pressed(HOMING) && is_button_idle[HOMING])
    {
        is_button_idle[HOMING] = 0;
        stepper_state = MOTOR_HOMING;
    }
}

void homing()
{
    digitalWrite(STEPPER_DIR, LOW);
    while(1)
    {
        if(digitalRead(LIMIT_SWITCH_PRESA) == LOW)
            break;
        if(digitalRead(START_STOP) == LOW)
            break;
        if(digitalRead(LIMIT_SWITCH_POCETAK) == LOW)
            break;
        digitalWrite(STEPPER_PUL, LOW);
        digitalWrite(STEPPER_PUL, HIGH);
        delayMicroseconds(100);
    }
    delay(300);

    digitalWrite(STEPPER_DIR, HIGH);
    while(1)
    {
        if(digitalRead(LIMIT_SWITCH_PRESA) == LOW)
            break;
        if(digitalRead(START_STOP) == LOW)
            break;
        if(digitalRead(LIMIT_SWITCH_POCETAK) == HIGH)
            break;
        digitalWrite(STEPPER_PUL, LOW);
        digitalWrite(STEPPER_PUL, HIGH);
        delayMicroseconds(100);
    }

    step_counter = 0;
    update_lcd();
}

void update_motor()
{
    if(stepper_state == MOTOR_HOMING)
    {
        homing();
        return;
    }

    if(stepper_state == MOTOR_STEP)
    {
        stepper_move(number_of_steps, MIN_STEP_DELAY, stepper_direction);
        current_step_delay = MAX_STEP_DELAY;
        if(stepper_direction == LOW)
            step_counter--;
        else
            step_counter++;
        update_lcd();
        return;
    }

    if(stepper_state == MOTOR_ACCELERATE)
    {
        int pin = stepper_direction == LOW ? CONT_MOVE_LEFT : CONT_MOVE_RIGHT;
        current_step_delay -= (millis() - last_debounce_time[pin]) / 3;
    }
    else if(stepper_state == MOTOR_DECELERATE)
    {
        int pin = stepper_direction == LOW ? CONT_MOVE_LEFT : CONT_MOVE_RIGHT;
        current_step_delay += (millis() - last_debounce_time[pin]);
    }
    current_step_delay = max(min(current_step_delay, MAX_STEP_DELAY), MIN_STEP_DELAY);

    if(current_step_delay < MAX_STEP_DELAY)
        stepper_move(100, current_step_delay, stepper_direction);
}

void change_step_size_loop()
{
    if(is_debounced_button_pressed(STEP_INC))
    {
        step_size_sign = +1;
        step_size_state = STEP_SIZE_ACCELERATE;
    }
    else if(is_debounced_button_pressed(STEP_DEC))
    {
        step_size_sign = -1;
        step_size_state = STEP_SIZE_ACCELERATE;
    }
    else
        step_size_state = STEP_SIZE_IDLE;
}

void update_step_size()
{
    if(step_size_state == STEP_SIZE_ACCELERATE)
    {
        int pin = step_size_sign == +1 ? STEP_INC : STEP_DEC;
        long long delta = (millis() - last_debounce_time[pin]) / 200;
        delta = max(delta, 1);
        if(delta <= 3)
            current_step_size = step_size_at_debounce + step_size_sign * delta;

        else if(delta <= 7)
        {
            current_step_size += step_size_sign;
            delay(50);
        }
        else if(delta <= 15)
        {
            current_step_size += step_size_sign;
            delay(25);
        }
        else if(delta <= 20)
        {
            current_step_size += step_size_sign;
            delay(5);
        }
        else
        {
            current_step_size += step_size_sign;
            delay(1);
        }
    }
    current_step_size = max(min(current_step_size, MAX_STEP_SIZE), MIN_STEP_SIZE);
    number_of_steps = (current_step_size * 32 + 1) / 3;

    update_lcd();
}

void change_presa_size_loop()
{
    if(is_debounced_button_pressed(PRESA_INC))
    {
        presa_size_sign = +1;
        presa_size_state = PRESA_SIZE_ACCELERATE;
    }
    else if(is_debounced_button_pressed(PRESA_DEC))
    {
        presa_size_sign = -1;
        presa_size_state = PRESA_SIZE_ACCELERATE;
    }
    else
        presa_size_state = PRESA_SIZE_IDLE;
}

void update_presa_size()
{
    if(presa_size_state == PRESA_SIZE_ACCELERATE)
    {
        int pin = presa_size_sign == +1 ? PRESA_INC : PRESA_DEC;
        long long delta = (millis() - last_debounce_time[pin]) / 200;
        delta = max(delta, 1);
        if(delta <= 3)
            current_presa_size = presa_size_at_debounce + presa_size_sign * delta;

        else if(delta <= 7)
        {
            current_presa_size += presa_size_sign;
            delay(50);
        }
        else if(delta <= 15)
        {
            current_presa_size += presa_size_sign;
            delay(25);
        }
        else if(delta <= 20)
        {
            current_presa_size += presa_size_sign;
            delay(5);
        }
        else
        {
            current_presa_size += presa_size_sign;
            delay(1);
        }
    }
    current_presa_size = max(min(current_presa_size, MAX_PRESA_SIZE), MIN_PRESA_SIZE);
    PRESA_DELAY = current_presa_size * 10;

    update_lcd();
}

void presa_loop()
{
    if(digitalRead(PRESA_GORE) == LOW)
        digitalWrite(SOLENOID, LOW);

    else if(digitalRead(PRESA_DOLJE) == LOW)
        digitalWrite(SOLENOID, HIGH);
}

void output_step_size()
{
    if(current_mode == MANUAL_MODE)
        Serial.print("Manual mode: ");

    else if(current_mode == AUTOMATIC_MODE_IDLE)
        Serial.print("Automatic mode idle: ");

    else
        Serial.print("Automatic mode active: ");


    int br = (int)current_step_size;
    Serial.print(br / 100);
    Serial.print(".");
    if(br % 100 < 10)
        Serial.print("0");
    Serial.print(br % 100);
    Serial.println("");
}

void automatic_movement()
{
    if(steps_left == 0)
    {
        if(presa_delay_left == 0)
        {
            steps_left = number_of_steps;
            presa_delay_left = PRESA_DELAY;
            step_counter++;
            update_lcd();
        }
        else
        {
            if(PRESA_DELAY / 2 <= presa_delay_left)
                digitalWrite(SOLENOID, HIGH);

            else
                digitalWrite(SOLENOID, LOW);

            presa_delay_left -= min(presa_delay_left, 50);
            delay(50);
        }
    }
    else
    {
        long long delta = min(steps_left, 100);
        steps_left -= delta;
        stepper_move(delta, MIN_STEP_DELAY, HIGH);
    }
}

void launch_idle_mode()
{
    stepper_move(steps_left, MIN_STEP_DELAY, HIGH);
    steps_left = 0;
    presa_delay_left = PRESA_DELAY;
}

void manual_auto_loop()
{
    if(is_debounced_button_pressed(MANUAL_AUTO) && is_button_idle[MANUAL_AUTO])
    {
        is_button_idle[MANUAL_AUTO] = 0;
        if(current_mode == MANUAL_MODE)
        {
            current_mode = AUTOMATIC_MODE_IDLE;
            step_counter = 0;
            update_lcd();
        }
        else
        {
            launch_idle_mode();
            current_mode = MANUAL_MODE;
            step_counter = 0;
            update_lcd();
        }
    }
    if(is_debounced_button_pressed(START_STOP) && is_button_idle[START_STOP])
    {
        is_button_idle[START_STOP] = 0;
        if(current_mode == AUTOMATIC_MODE_IDLE)
        {
            current_mode = AUTOMATIC_MODE_ACTIVE;
            update_lcd();
        }
        else if(current_mode == AUTOMATIC_MODE_ACTIVE)
        {
            current_mode = AUTOMATIC_MODE_IDLE;
            update_lcd();
        }
    }
}

void loop()
{
    // prioritize continuous left and right

    update_debounced_buttons_state(0);     // checks only for continuos movement left and right

    if(current_step_delay != MAX_STEP_DELAY)
    {
        change_motor_state_loop(0);
        update_motor();
        return;
    }

    //////////////////////////////////////////

    update_debounced_buttons_state(1);

    manual_auto_loop();

    if(current_mode == MANUAL_MODE)
    {
        change_motor_state_loop(1);
        update_motor();

        change_step_size_loop();
        update_step_size();

        change_presa_size_loop();
        update_presa_size();

        presa_loop();
    }
    else if(current_mode == AUTOMATIC_MODE_ACTIVE)
        automatic_movement();

    else if(current_mode == AUTOMATIC_MODE_IDLE)
        launch_idle_mode();

    // output_step_size();
}
