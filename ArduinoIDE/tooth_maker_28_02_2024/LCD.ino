void display_num (int val, int digits) {
  char s[digits];
  for (int i = 0; i < digits; i++) {
    s[i] = ' ';
  }
  int sign = 1;
  if (val < 0) {
    sign = -1;
    val = -val;
  }
  for (int i = 0; i < digits; i++) {
    if (val == 0 && i > 0) {
      if (sign == 1) {
        s[digits - 1 - i] = '#';
      } else if (sign == -1) {
        s[digits - 1 - i] = '-';
        s[digits - 2 - i] = '#';
      }
      break;
    }
    s[digits - 1 - i] = (char) ('0' + val % 10);
    val /= 10;
  }
  lcd.print(s);
}

void update_lcd () {
  lcd.setCursor(0, 0);
  lcd.print("Step: ");
  lcd.print((int) (current_step_size / 100));
  lcd.print(".");
  lcd.print((int)(current_step_size % 100 / 10));
  lcd.print((int)(current_step_size % 10));
  lcd.print(' ');

  if (current_tool == TOOL_PRESS) {
    lcd.setCursor(0, 1);
    lcd.print("Delay: ");
    lcd.print((int) (current_tool_lcd / 100));
    lcd.print(".");
    lcd.print((int)(current_tool_lcd % 100 / 10));
    lcd.print((int)(current_tool_lcd % 10));
    lcd.print(' ');
  } else {
    lcd.setCursor(0, 1);
    lcd.print("Angle: ");
    lcd.print((int) (current_tool_lcd * 90));
    lcd.print(' ');
  }

  if (current_mode == MANUAL_MODE) {
    lcd.setCursor(12, 0);
    lcd.print(" MAN");
  } else {
    lcd.setCursor(12, 0);
    lcd.print("AUTO");
  }

  lcd.setCursor(11, 1);
  display_num(step_counter, 5);
}
