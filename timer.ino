#include <Nokia_LCD.h>

#define MAX_RANGE(x) x + 10
#define MIN_RANGE(x) x - 10
#define CHECK_BUTTON(x, y) MAX_RANGE(y) >= x && MIN_RANGE(y) <= x

const int MAX_ROW_STR = 14;
const int PAYLOAD_PIN = 8;
const int POWER = 1023;
const int MENU = 740;
const int UP = 486;
const int DOWN = 244;

const int MAX_HOURS = 99;
const int MAX_MINUTES = 59;
const int MAX_SECONDS = 59;
const int MAX_MILLISECONDS = 999;

typedef enum selected_element_menu {
  BEGIN,
  HOURS,
  MINUTES,
  SECONDS,
  MILLISECONS,
  PAYLOAD,
  END
} selected_element_menu;

typedef enum button_type {
  NULL_BUTTON,
  POWER_BUTTON,
  MENU_BUTTON,
  UP_BUTTON,
  DOWN_BUTTON
} button_type;

typedef struct time {
  short hours;
  short minutes;
  short seconds;
  short milliseconds;
} time;

typedef struct button_state {
  button_type pressed_button;
  button_type next_pressed;
  unsigned long begin_pressed_time;
  unsigned long present_pressed_time;
  bool holded;
} button_state;

struct state_timer {
  unsigned long frame_time;
  unsigned long delay_time;
  unsigned long begin_time;
  bool isFirstTick;
  bool payload_state;
  bool update;
  bool timer_run;
  time prev_timer_view;
  time timer_view;
  selected_element_menu choosen_menu;
  button_state button;
} state_timer;

Nokia_LCD display_hardware (7 /* CLK */, 6 /* DIN */, 5 /* DC */, 4 /* CE */, 3 /* RST */);

void setup() 
{
  pinMode(PAYLOAD_PIN, OUTPUT);
  state_timer.update = true;
  state_timer.isFirstTick = false;
  display_hardware.begin();
  display_hardware.clear();
}

void loop() 
{
  state_timer.frame_time = millis();
  read_button();
  handle_button();

  if (state_timer.timer_run && (!state_timer.isFirstTick))
  { 
    calculate_time();
  }

  if (state_timer.update) 
  {
    state_timer.update = false;
    draw(); 
  }
}

void zero_voltage_tick() 
{ 

  if (state_timer.timer_run) 
  {
    unsigned long current_time = millis();

    if (state_timer.isFirstTick) 
    {
      state_timer.begin_time = current_time;
      digitalWrite(PAYLOAD_PIN, state_timer.payload_state);
      state_timer.delay_time = state_timer.delay_time + current_time;
      state_timer.isFirstTick = false;
    }

    if (current_time < state_timer.delay_time)
    {
      return;
    }
    else
    {
      digitalWrite(PAYLOAD_PIN, false);
      state_timer.timer_run = false;
      state_timer.payload_state = false;
      state_timer.update = true;
      state_timer.timer_view = state_timer.prev_timer_view;
    }
  }
  else
  {
    digitalWrite(PAYLOAD_PIN, state_timer.payload_state);
  }

  detachInterrupt(0);
}

void read_button() 
{
  unsigned int value_buttons = analogRead(A0);
  unsigned long pressed_time = state_timer.frame_time;
  button_state* button = &state_timer.button;
  button->pressed_button = button->next_pressed;

  if (CHECK_BUTTON(POWER, value_buttons)) 
  {
    button->next_pressed = POWER_BUTTON;
  } 
  else if (CHECK_BUTTON(MENU, value_buttons)) 
  {
    button->next_pressed = MENU_BUTTON;
  } 
  else if (CHECK_BUTTON(UP, value_buttons)) 
  {
    button->next_pressed = UP_BUTTON;
  }
  else if (CHECK_BUTTON(DOWN, value_buttons)) 
  {
    button->next_pressed = DOWN_BUTTON;
  }
  else
  {
    button->next_pressed = NULL_BUTTON;
  }

  if (button->next_pressed != button->pressed_button && button->next_pressed)
  {
    button->begin_pressed_time = pressed_time;
  } 
  else if (button->next_pressed == button->pressed_button)
  {
    button->present_pressed_time = pressed_time;
  }
  else
  {
    button->begin_pressed_time = 0;
    button->present_pressed_time = 0;
  }

  if (button->present_pressed_time - button->begin_pressed_time > 500) 
  {
    button->holded = true;
  }
  else
  {
    button->holded = false;
  }
}

void handle_button() 
{
  button_state* button = &state_timer.button;

  if (button->pressed_button == NULL)
  {
    return;
  }
  
  if (button->pressed_button != POWER_BUTTON && state_timer.timer_run)
  {
    return;
  }
  
  if (button->pressed_button == button->next_pressed && !button->holded) 
  {
    return;
  }

  if (button->pressed_button == UP_BUTTON)
  {
    up_button();
  }  
  else if (button->pressed_button == DOWN_BUTTON)
  {
    down_button();
  }
  else if (button->pressed_button == POWER_BUTTON && !button->holded)
  {
    power_button();
  } 
  else if (button->pressed_button == MENU_BUTTON && !button->holded && !state_timer.timer_run) 
  {
    switch_menu();
  }

  state_timer.update = true;
}

void calculate_time() 
{
  time deduct_time;
  msec_to_time(state_timer.frame_time - state_timer.begin_time, &deduct_time);
  remaining_time(state_timer.prev_timer_view, deduct_time, &state_timer.timer_view);
  state_timer.update = true;
}

void msec_to_time(unsigned long msec_time, time* default_time) 
{
  default_time->milliseconds = msec_time % 1000;
  default_time->seconds = (msec_time / 1000) % 60;
  default_time->minutes = (msec_time / 1000) / 60;
  
  if (default_time->minutes > 0)
  {
    default_time->hours = default_time->minutes / 60;
  }
  else
  {
    default_time->hours = 0;
  }
}

void down_button() 
{
  int down_value = state_timer.button.holded ? 10 : 1;
  selected_element_menu choosen_menu = state_timer.choosen_menu;
  time* time = &state_timer.timer_view;

  if (choosen_menu == HOURS && time->hours > 0) 
  {
    int downed_value = time->hours - down_value;
    time->hours = downed_value > 0 ? downed_value : 0; 
  } 
  else if (choosen_menu == MINUTES && time->minutes > 0) 
  {
    int downed_value = time->minutes - down_value;
    time->minutes = downed_value > 0 ? downed_value : 0; 
  }
  else if (choosen_menu == SECONDS && time->seconds > 0)
  {
    int downed_value = time->seconds - down_value;
    time->seconds = downed_value > 0 ? downed_value : 0; 
  }
  else if (choosen_menu == MILLISECONS && time->milliseconds > 0)
  {
    int downed_value = time->milliseconds - down_value;
    time->milliseconds = downed_value > 0 ? downed_value : 0; 
  }
}

void up_button() 
{
  int up_value = state_timer.button.holded ? 10 : 1;
  selected_element_menu choosen_menu = state_timer.choosen_menu;
  time* time = &state_timer.timer_view;

  if (choosen_menu == HOURS && time->hours < MAX_HOURS) 
  {
    unsigned short uped_value = time->hours + up_value;
    time->hours = uped_value > MAX_HOURS ? MAX_HOURS : uped_value; 
  } 
  else if (choosen_menu == MINUTES && time->minutes < MAX_MINUTES) 
  {
    unsigned short uped_value = time->minutes + up_value;
    time->minutes = uped_value > MAX_MINUTES ? MAX_MINUTES : uped_value;
  }
  else if (choosen_menu == SECONDS && time->seconds < MAX_SECONDS)
  {
     unsigned short uped_value = time->seconds + up_value;
    time->seconds = uped_value > MAX_SECONDS ? MAX_SECONDS : uped_value;
  }
  else if (choosen_menu == MILLISECONS && time->milliseconds < MAX_MILLISECONDS)
  {
     unsigned short uped_value = time->milliseconds + up_value;
    time->milliseconds = uped_value > MAX_MILLISECONDS ? MAX_MILLISECONDS : uped_value; ;
  }
}

void power_button()
{
  selected_element_menu menu = state_timer.choosen_menu;
  time* timer = &state_timer.timer_view;

  if (menu == PAYLOAD) 
  {
    state_timer.payload_state = !state_timer.payload_state;
  } 
  else if (isZeroTime(timer)) 
  {
    return;
  }
  else if (state_timer.timer_run) 
  {
    state_timer.payload_state = false;
    state_timer.timer_run = false;
    state_timer.timer_view = state_timer.prev_timer_view;
  } 
  else
  {
    state_timer.choosen_menu = END;
    state_timer.timer_run =  true;
    state_timer.payload_state = true;
    state_timer.isFirstTick = true;
    state_timer.delay_time = time_to_msec(&state_timer.timer_view);
    state_timer.prev_timer_view = state_timer.timer_view;
  }

  attachInterrupt(0, zero_voltage_tick, RISING);
}

void switch_menu() 
{
  if (state_timer.choosen_menu > (END - 1))
  {
    state_timer.choosen_menu = BEGIN;
  }

  state_timer.choosen_menu = state_timer.choosen_menu + 1;
}

void draw() 
{
  display_hardware.setCursor(0, 0);
  time* draw_time = &state_timer.timer_view;
  String str_payload_state(state_timer.payload_state ? " [ON]" : "[OFF]");

  print_row_element(&display_hardware, "HR", draw_time->hours, state_timer.choosen_menu == HOURS);
  print_row_element(&display_hardware, "MIN", draw_time->minutes, state_timer.choosen_menu == MINUTES);
  print_row_element(&display_hardware, "SEC", draw_time->seconds, state_timer.choosen_menu == SECONDS);
  print_row_element(&display_hardware, "MSEC", draw_time->milliseconds, state_timer.choosen_menu == MILLISECONS);

  display_hardware.setCursor(0, 5);
  print_row_element(&display_hardware, "PAYLOAD", str_payload_state, state_timer.choosen_menu == PAYLOAD);
}

void print_row_element(Nokia_LCD* display, String name_unit, unsigned long value, bool selected)
 {
  String str_value(value);
  print_row_element(display, name_unit, str_value, selected);
}

void print_row_element(Nokia_LCD* display, String name_unit, String value, bool selected) 
{
  name_unit.concat(": ");
  name_unit.concat(value);

  for (int i = name_unit.length(); i < MAX_ROW_STR; i++)
    name_unit.concat(' ');

  display->setInverted(selected);
  display->println(name_unit.c_str());
}

bool isZeroTime(time* timer) 
{
  return timer->hours == 0 && timer->minutes == 0 && timer->seconds == 0 && timer->milliseconds == 0;
}

bool isMicroTimer(time* timer) 
{
  return timer->hours == 0 && timer->minutes == 0 && timer->seconds == 0 && timer->milliseconds < 500;
}

unsigned long time_to_msec(time* convert_time)
{
  unsigned long result = 0;
  unsigned long hours = convert_time->hours;
  unsigned long minutes = convert_time->minutes;
  unsigned long seconds = convert_time->seconds;

  result += hours * 60 * 60 * 1000;
  result += minutes * 60 * 1000;
  result += seconds * 1000;
  result += convert_time->milliseconds;
  return result;
}

bool remaining_time(time begin_time, time deduct_time, time* result) 
{
  if (deduct_time.milliseconds > begin_time.milliseconds) 
  {
    --begin_time.seconds;
    begin_time.milliseconds += 1000;
  }

  if (deduct_time.seconds > begin_time.seconds) 
  {
    --begin_time.minutes;
    begin_time.seconds += 60;
  }

  if (deduct_time.minutes > begin_time.minutes) 
  {
    --begin_time.hours;
    begin_time.minutes += 60;
  } 

  result->hours = begin_time.hours - deduct_time.hours;
  result->minutes = begin_time.minutes - deduct_time.minutes;
  result->seconds = begin_time.seconds - deduct_time.seconds;
  result->milliseconds = begin_time.milliseconds - deduct_time.milliseconds;
}
