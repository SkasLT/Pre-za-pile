bool is_debounced_button_pressed(int pin){
  return button_state[pin] == LOW && is_button_ok[pin];
}

void update_single_debounced_button (int button_pin) {
  int reading = digitalRead(button_pin);
  if (reading != button_state[button_pin]){
    last_debounce_time[button_pin] = millis();
    if (button_pin == STEP_INC || button_pin == STEP_DEC)
      step_size_at_debounce = current_step_size;
    if (button_pin == PRESS_INC || button_pin == PRESS_DEC)
      tool_lcd_at_debounce = current_tool_lcd;
  }
  button_state[button_pin] = reading;

  if ((millis() - last_debounce_time[button_pin]) > debounce_delay){
    is_button_ok[button_pin] = true;
    if (button_state[button_pin] == HIGH)
      is_button_idle[button_pin] = true;
  }
}

void update_debounced_buttons_state(bool check_all){
  update_single_debounced_button(CONT_MOVE_LEFT);
  update_single_debounced_button(CONT_MOVE_RIGHT);
  if (!check_all) return;
  for (int i = 0; i < number_of_debounced_buttons; i++){
    int button_pin = debounced_buttons_list[i];
    update_single_debounced_button(button_pin);
  }
}
