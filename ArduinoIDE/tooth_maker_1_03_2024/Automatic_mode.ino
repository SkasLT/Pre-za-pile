// each loop iteration [wagon_pulses_left] decreases by little until it hits zero, it determines the wagon movement
// after that each loop iteration decreases [press_delay_left] by a little until it hits zero
// after that these values are reset and the next cycle can start

void automatic_movement(){
  if (wagon_pulses_left == 0){
    if (current_tool == TOOL_PRESS) {
      if (press_delay_left == 0){
        wagon_pulses_left = number_of_wagon_motor_pulses;
        press_delay_left = PRESS_DELAY;
        step_counter++;
        update_lcd();
      }
      else{
        if (PRESS_DELAY / 2 <= press_delay_left)  // half the time is spent moving down, and half for up
          digitalWrite(SOLENOID, HIGH);
        
        else
          digitalWrite(SOLENOID, LOW);
        
        press_delay_left -= min(press_delay_left, 50);
        delay(50);
      }
    } else {
      if (tool_pulses_left == 0) {
        wagon_pulses_left = number_of_wagon_motor_pulses;
        tool_pulses_left = TOOL_PULSES_FULL_TURN;
        update_lcd();
      } else {
        long long delta = min(tool_pulses_left, 100);
        tool_pulses_left -= delta;
        stepper_tool_move(delta, MIN_PULSE_DELAY, HIGH);
      }
    }
  }
  else{
    long long delta = min(wagon_pulses_left, 100);
    wagon_pulses_left -= delta;
    stepper_wagon_move(delta, MIN_PULSE_DELAY, HIGH);
  }
}

void launch_idle_mode(){
  stepper_wagon_move(wagon_pulses_left, MIN_PULSE_DELAY, HIGH);
  //stepper_tool_move(tool_pulses_left, MIN_PULSE_DELAY, HIGH);
  wagon_pulses_left = 0;
  press_delay_left = PRESS_DELAY;
  tool_pulses_left = TOOL_PULSES_FULL_TURN;
}