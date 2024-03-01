void stepper_wagon_move(long long number_of_pulses, int pulse_delay, int dir){ // LOW = LEFT, HIGH = RIGHT
  digitalWrite(STEPPER_WAGON_DIR, dir);

  for (long long i = 0; i < number_of_pulses; i++)
  {
    if (digitalRead(LIMIT_SWITCH_PRESS) == LOW)
      break;
    if (dir == LOW && digitalRead(LIMIT_SWITCH_POCETAK) == LOW)
      break;
    if (dir == HIGH && digitalRead(LIMIT_SWITCH_KRAJ) == LOW)
      break;
    digitalWrite(STEPPER_WAGON_PUL, LOW);
    digitalWrite(STEPPER_WAGON_PUL, HIGH);
    delayMicroseconds(pulse_delay);
  }
}

void change_wagon_motor_state_loop(bool check_all){  // determines wagon motor state depending on which button is pressed/held, continuous movement has priority
  if (is_debounced_button_pressed(CONT_MOVE_LEFT)){
    stepper_direction = LOW;
    stepper_state = MOTOR_ACCELERATE;
  }
  else if (is_debounced_button_pressed(CONT_MOVE_RIGHT)){
    stepper_direction = HIGH;
    stepper_state = MOTOR_ACCELERATE;
  }
  else{
    stepper_state = MOTOR_DECELERATE;
  }
  if (!check_all) return;
  if (is_debounced_button_pressed(STEP_LEFT) && is_button_idle[STEP_LEFT]){
    is_button_idle[STEP_LEFT] = false;
    stepper_direction = LOW;
    stepper_state = MOTOR_STEP;
  }
  else if (is_debounced_button_pressed(STEP_RIGHT) && is_button_idle[STEP_RIGHT]){
    is_button_idle[STEP_RIGHT] = false;
    stepper_direction = HIGH;
    stepper_state = MOTOR_STEP;
  }
  if (is_debounced_button_pressed(HOMING) && is_button_idle[HOMING]){
    is_button_idle[HOMING] = false;
    stepper_state = MOTOR_HOMING;
  }
}

void homing(){
  digitalWrite(STEPPER_WAGON_DIR, LOW);
  while (1){
    if (digitalRead(LIMIT_SWITCH_PRESS) == LOW)
      break;
    if (digitalRead(START_STOP) == LOW)
      break;
    if (digitalRead(LIMIT_SWITCH_POCETAK) == LOW)
      break;
    digitalWrite(STEPPER_WAGON_PUL, LOW);
    digitalWrite(STEPPER_WAGON_PUL, HIGH);
    delayMicroseconds(100);
  }
  delay(300);

  digitalWrite(STEPPER_WAGON_DIR, HIGH);
  while (1){
    if (digitalRead(LIMIT_SWITCH_PRESS) == LOW)
      break;
    if (digitalRead(START_STOP) == LOW)
      break;
    if (digitalRead(LIMIT_SWITCH_POCETAK) == HIGH)
      break;
    digitalWrite(STEPPER_WAGON_PUL, LOW);
    digitalWrite(STEPPER_WAGON_PUL, HIGH);
    delayMicroseconds(100);
  }

  step_counter = 0;
  update_lcd();
}

void wagon_motor_single_step () {
  stepper_wagon_move(number_of_wagon_motor_pulses, MIN_PULSE_DELAY, stepper_direction);
  current_pulse_delay = MAX_PULSE_DELAY;  // this means wagon is not moving
  if (stepper_direction == LOW) step_counter--; else step_counter++;
  update_lcd();
}

void wagon_motor_continuous_movement () {
  // wagon motor acceleration logic
  if (stepper_state == MOTOR_ACCELERATE){
    int pin = stepper_direction == LOW ? CONT_MOVE_LEFT : CONT_MOVE_RIGHT;
    current_pulse_delay -= (millis() - last_debounce_time[pin]) / 3;
  }
  else if (stepper_state == MOTOR_DECELERATE){
    int pin = stepper_direction == LOW ? CONT_MOVE_LEFT : CONT_MOVE_RIGHT;
    current_pulse_delay += (millis() - last_debounce_time[pin]);
  }
  current_pulse_delay = max(min(current_pulse_delay, MAX_PULSE_DELAY), MIN_PULSE_DELAY);

  if (current_pulse_delay < MAX_PULSE_DELAY)   // we move the wagon only with strict "<"
    stepper_wagon_move(100, current_pulse_delay, stepper_direction); // the 100 is just a constant related to the motor speed (determined by testing)

}

void update_motor(){
  if (stepper_state == MOTOR_HOMING){
    homing();
    return;
  }

  if (stepper_state == MOTOR_STEP){
    wagon_motor_single_step();
    return;
  }

  wagon_motor_continuous_movement();
}
