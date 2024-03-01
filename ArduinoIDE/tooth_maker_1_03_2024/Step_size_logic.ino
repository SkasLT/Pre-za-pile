void change_step_size_loop(){
  if (is_debounced_button_pressed(STEP_INC)){
    step_size_sign = +1;
    step_size_state = STEP_SIZE_ACCELERATE;
  }
  else if (is_debounced_button_pressed(STEP_DEC)){
    step_size_sign = -1;
    step_size_state = STEP_SIZE_ACCELERATE;
  }
  else
    step_size_state = STEP_SIZE_IDLE;
}

void update_step_size(){
  if (step_size_state == STEP_SIZE_ACCELERATE){
    int pin = step_size_sign == +1 ? STEP_INC : STEP_DEC;
    long long delta = (millis() - last_debounce_time[pin]) / 200;
    delta = max(delta, 1);
    if (delta <= 3)
      current_step_size = step_size_at_debounce + step_size_sign * delta;

    else if (delta <= 7){
      current_step_size += step_size_sign;
      delay(50);
    }
    else if (delta <= 15){
      current_step_size += step_size_sign;
      delay(25);
    }
    else if (delta <= 20){
      current_step_size += step_size_sign;
      delay(5);
    }
    else{
      current_step_size += step_size_sign;
      delay(1);
    }
  }
  current_step_size = max(min(current_step_size, MAX_STEP_SIZE), MIN_STEP_SIZE);
  number_of_wagon_motor_pulses = (current_step_size * 32 + 1) / 3;  // determined by testing and calculations
  if (current_tool == TOOL_MOTOR) number_of_wagon_motor_pulses *= 2;  // when using motor as tool, every other tooth is skipped

  update_lcd();
}