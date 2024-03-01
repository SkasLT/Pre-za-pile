void change_tool_lcd_loop(){
  if (is_debounced_button_pressed(PRESS_INC)){
    tool_lcd_sign = +1;
    tool_lcd_state = TOOL_LCD_ACCELERATE;
  }
  else if (is_debounced_button_pressed(PRESS_DEC)){
    tool_lcd_sign = -1;
    tool_lcd_state = TOOL_LCD_ACCELERATE;
  }
  else
    tool_lcd_state = TOOL_LCD_IDLE;
}

void update_tool_lcd(){
  if (tool_lcd_state == TOOL_LCD_ACCELERATE){
    int pin = tool_lcd_sign == +1 ? PRESS_INC : PRESS_DEC;
    long long delta = (millis() - last_debounce_time[pin]) / 200;
    delta = max(delta, 1);
    if (delta <= 3)
      current_tool_lcd = tool_lcd_at_debounce + tool_lcd_sign * delta;

    else if (delta <= 7){
      current_tool_lcd += tool_lcd_sign;
      delay(50);
    }
    else if (delta <= 15){
      current_tool_lcd += tool_lcd_sign;
      delay(25);
    }
    else if (delta <= 20){
      current_tool_lcd += tool_lcd_sign;
      delay(5);
    }
    else{
      current_tool_lcd += tool_lcd_sign;
      delay(1);
    }
  }
  current_tool_lcd = max(min(current_tool_lcd, MAX_TOOL_LCD), MIN_TOOL_LCD);
  PRESS_DELAY = current_tool_lcd * 10;
  number_of_tool_motor_pulses = current_tool_lcd * 800;  // 360 degrees is 3200 pulses

  update_lcd();
}