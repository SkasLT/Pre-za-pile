void stepper_tool_move(long long number_of_pulses, int pulse_delay, int dir){ // LOW = LEFT, HIGH = RIGHT
  digitalWrite(STEPPER_TOOL_DIR, dir);

  for (long long i = 0; i < number_of_pulses; i++)
  {
    digitalWrite(STEPPER_TOOL_PUL, LOW);
    digitalWrite(STEPPER_TOOL_PUL, HIGH);
    delayMicroseconds(pulse_delay);
  }
}

void tool_motor_single_step () {
  stepper_tool_move(number_of_tool_motor_pulses, MIN_PULSE_DELAY, stepper_direction);
}

