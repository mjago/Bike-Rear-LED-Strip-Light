/*
  Bike Rear LED Strip Light (with battery monitor)

  Processor:      Arduino Pro Mini (3v3)
  Battery:        Li-Ion 18650
  LEDS:           WS1812B (8 Pixels)
  Battery Status: Display sequence changes to indicate
                  Charge Status

  Author:  MJago
  Date:    22 Oct' 2023
  License: MIT
*/

/*
  Define DEBUG for serial debug info...
  #define DEBUG
*/

#include <Adafruit_NeoPixel.h>
#include <LowPower.h>

#define PIXEL_PIN      4
#define NUM_PIXELS     8
#define BAT_READ_PIN   A3
#define ADC_STEPS      1023
#define LED_FULL_RED   0xFF0000
#define LED_FULL_GREEN 0x00FF00
#define LED_FULL_BLUE  0x0000FF
#define LED_OFF        0x000000
#define MAX_RED        32
#define INC_RED        8
#define MIN_RED        INC_RED
#define BAT_LVL_NORMAL 3600
#define BAT_LVL_3560   3560
#define BAT_LVL_3520   3520
#define BAT_LVL_3480   3480
#define BAT_LVL_3440   3440
#define BAT_LVL_3400   3400
#define BAT_LVL_EMPTY  3350
#define BAT_HYST       0050
#define VREF           1.1

#ifdef DEBUG

const char * state_str[] =
  {
    "BAT_STATE_NORMAL",
    "BAT_STATE_3560",
    "BAT_STATE_3520",
    "BAT_STATE_3480",
    "BAT_STATE_3440",
    "BAT_STATE_3400",
    "BAT_STATE_EMPTY",
    "BAT_STATE_COUNT"
  };

#define PRINTLN(x) Serial.println(x)
#define PRINT(x)   Serial.print(x)

#else /* DEBUG */

#define PRINTLN(x)
#define PRINT(x)

#endif /* DEBUG */

const uint32_t bat_lvls[] =
  {
    BAT_LVL_NORMAL,
    BAT_LVL_3560,
    BAT_LVL_3520,
    BAT_LVL_3480,
    BAT_LVL_3440,
    BAT_LVL_3400,
    BAT_LVL_EMPTY
  };

typedef enum BAT_STATE
  {
    BAT_STATE_NORMAL,
    BAT_STATE_3560,
    BAT_STATE_3520,
    BAT_STATE_3480,
    BAT_STATE_3440,
    BAT_STATE_3400,
    BAT_STATE_EMPTY,
    BAT_STATE_SIZE
  } bat_state_t;

typedef enum COLOUR_STATE
  {
    COLOUR_STATE_0,
    COLOUR_STATE_1,
    COLOUR_STATE_2,
    COLOUR_STATE_SIZE
  } colour_state_t;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS,
                                            PIXEL_PIN,
                                            NEO_GRB +
                                            NEO_KHZ800);

void set_colour(colour_state_t * colour_state,
                unsigned long * colour,
                int32_t * colour_offset,
                bool toggle);
uint32_t v_conversion(uint16_t value);
void read_voltage(uint32_t * bat_lvl);
void flash_strip_4(uint32_t width,
                   unsigned long colour,
                   bool toggle);
void flash_alternate_strip(uint32_t  width,
                           unsigned long colour);
void flash_strip(uint32_t  width,
                 unsigned long colour,
                 bool toggle);
void led_sweep_L(uint16_t cycles,
                 uint16_t speed,
                 uint8_t width,
                 uint32_t color);
void led_sweep_R(uint16_t cycles,
                 uint16_t speed,
                 uint8_t width,
                 uint32_t color);
void led_sweep(uint16_t cycles,
                 uint16_t speed,
                 uint8_t width,
                 uint32_t color,
                 bool toggle);
void clear_strip(void);
uint32_t bat_level(bat_state_t state);
void check_lower_state(bat_state_t * bat_state,
                       uint32_t bat_lvl,
                       bat_state_t lower);
void check_higher_state(bat_state_t * bat_state,
                        uint32_t bat_lvl,
                        bat_state_t higher);
void set_bat_state(bat_state_t * bat_state, uint32_t bat_lvl);

void setup(void)
{
  uint32_t dummy;
  uint32_t x;

#ifdef DEBUG

  Serial.begin(115200);

#endif /* DEBUG */

  strip.begin();
  clear_strip();
  analogReference(INTERNAL);

  for(x = 0; x < 5; x++)
    {
      read_voltage(&dummy);
      delay(100);
    }
}

void loop(void)
{
  colour_state_t colour_state = COLOUR_STATE_0;
  bat_state_t bat_state = BAT_STATE_SIZE;
  unsigned long colour;
  bool toggle = true;
  int32_t colour_offset = 0;
  uint32_t pixels = NUM_PIXELS;
  uint32_t bat_lvl;
  uint32_t x;

  read_voltage(&bat_lvl);
  set_bat_state(&bat_state, bat_lvl);

  for(;;)
    {
      set_colour(&colour_state,
                 &colour,
                 &colour_offset,
                 toggle);

      read_voltage(&bat_lvl);
      toggle = ! toggle;
      set_bat_state(&bat_state, bat_lvl);
      PRINTLN(state_str[bat_state]);

      switch(bat_state)
        {
        case BAT_STATE_NORMAL:
          led_sweep(2, 48, 2, colour, toggle);
          flash_strip_4(NUM_PIXELS, colour, toggle);
          break;

        case BAT_STATE_3560:
          pixels = NUM_PIXELS - 0;
          break;

        case BAT_STATE_3520:
          pixels = NUM_PIXELS - 1;
          break;

        case BAT_STATE_3480:
          pixels = NUM_PIXELS - 2;
          break;

        case BAT_STATE_3440:
        case BAT_STATE_3400:
          pixels = NUM_PIXELS - 3;
          break;

        default:
          clear_strip();
          LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
          break;
        }

      if(bat_state > BAT_STATE_NORMAL && bat_state < BAT_STATE_EMPTY)
        {
          for(x = 0; x < 4; x++)
            {
              flash_alternate_strip(pixels, colour);
            }
        }
    }
}

void set_colour(colour_state_t * colour_state,
                unsigned long * colour,
                int32_t * colour_offset,
                bool toggle)
{
  static bool colour_dir = true;

  *colour = LED_FULL_RED;

  if(colour_dir)
    {
      *colour_offset += INC_RED;
    }
  else
    {
      *colour_offset -= INC_RED;
    }

  switch(*colour_state)
    {
    case COLOUR_STATE_0:
      *colour += (0x100 * *colour_offset);
      break;

    case COLOUR_STATE_1:
      *colour += *colour_offset;
      break;

    case COLOUR_STATE_2:
      *colour += (0x100 * *colour_offset) + *colour_offset;
      break;

    default:
      break;
    }

  PRINT("colour_offset: ");
  PRINTLN(*colour_offset);

  if(*colour_offset > MAX_RED || *colour_offset < MIN_RED)
    {
      colour_dir = ! colour_dir;
      *colour_offset = *colour_offset > MAX_RED ? MAX_RED : MIN_RED;
      if(colour_dir)
        {
          *colour_state = (*colour_state >= 2)
            ? COLOUR_STATE_0
            : (colour_state_t)(*colour_state + 1);
        }
    }
}

uint32_t v_conversion(uint16_t value)
{
  return ((((value + 0.5) * VREF) / 1024) * 5.7) * 1000.0;
}

void read_voltage(uint32_t * bat_lvl)
{
  uint16_t value;
  value = analogRead(BAT_READ_PIN);
  PRINT(value);
  *bat_lvl = v_conversion(value);
  PRINT(", Voltage: ");
  PRINTLN(*bat_lvl);
}

void flash_strip_4(uint32_t width,
                   unsigned long colour,
                   bool toggle)
{
  bool dir;
  uint32_t x;

  for(x = 0; x < 4; x++)
    {
      dir = (x < 2) ? ! toggle : toggle;
      flash_strip(width, colour, dir);
    }
}

void flash_alternate_strip(uint32_t width, unsigned long colour)
{
  uint32_t i;
  bool dir;

  for(i = 0; i < 4; i++)
    {
      dir = (i & 2) ? false : true;
      flash_strip(width, colour, dir);
    }
}

void flash_strip(uint32_t width, unsigned long colour, bool toggle)
{
  uint32_t min_0 = NUM_PIXELS - width;
  uint32_t max_0 = NUM_PIXELS / 2;
  uint32_t min_1 = NUM_PIXELS / 2;
  uint32_t max_1 = width;
  uint32_t max;
  uint32_t min;
  unsigned long col;
  uint32_t i;

  for(i = 0; i<NUM_PIXELS; i++)
    {
      max = toggle ? max_0 : max_1;
      min = toggle ? min_0 : min_1;

      if(i >= min && i < max)
        {
          col = colour;
        }
      else
        {
          col = LED_OFF;
        }
      strip.setPixelColor(i, col);
      strip.show();
    }
  delay(150);
  clear_strip();
  delay(150);
}

void led_sweep_L(uint16_t cycles,
                 uint16_t speed,
                 uint8_t width,
                 uint32_t color)
{
  uint32_t previous[NUM_PIXELS];
  int32_t count;
  int32_t x;
  for (count = 1; count<NUM_PIXELS; count++)
    {
      strip.setPixelColor(count, color);
      previous[count] = color;
      for(x = count; x>0; x--)
        {
          previous[x-1] = led_dim(previous[x-1], width);
          strip.setPixelColor(x-1, previous[x-1]);
        }
      strip.show();
      delay(speed);
    }
  for (count = NUM_PIXELS-1; count >= 0; count--)
    {
      strip.setPixelColor(count, color);
      previous[count] = color;
      for(x = count; x<=NUM_PIXELS ;x++)
        {
          previous[x-1] = led_dim(previous[x-1], width);
          strip.setPixelColor(x+1, previous[x+1]);
        }
      strip.show();
      delay(speed);
    }
}

void led_sweep_R(uint16_t cycles,
                 uint16_t speed,
                 uint8_t width,
                 uint32_t color)
{
  uint32_t previous[NUM_PIXELS];
  int32_t count;
  int32_t x;

  for (count = NUM_PIXELS-1; count>=0; count--)
    {
      strip.setPixelColor(count, color);
      previous[count] = color;
      for(x = count; x<=NUM_PIXELS ;x++)
        {
          previous[x-1] = led_dim(previous[x-1], width);
          strip.setPixelColor(x+1, previous[x+1]);
        }
      strip.show();
      delay(speed);
    }
  for (count = 1; count<NUM_PIXELS; count++)
    {
      strip.setPixelColor(count, color);
      previous[count] = color;
      for(x = count; x>0; x--)
        {
          previous[x-1] = led_dim(previous[x-1], width);
          strip.setPixelColor(x-1, previous[x-1]);
        }
      strip.show();
      delay(speed);
    }
}

void led_sweep(uint16_t cycles,
               uint16_t speed,
               uint8_t width,
               uint32_t color,
               bool toggle)
{
  uint32_t i;

  for(i = 0; i < cycles; i++)
    {
      if(toggle)
        {
          led_sweep_L(cycles, speed, width, color);
        }
      else
        {
          led_sweep_R(cycles, speed, width, color);
        }
    }
}

void clear_strip(void)
{
  uint32_t i;

  for(i = 0; i<NUM_PIXELS; i++)
    {
      strip.setPixelColor(i, 0x000000); strip.show();
    }
}

uint32_t led_dim(uint32_t color, uint8_t width)
{
  uint32_t return_val;

  return_val =
    (((color & LED_FULL_RED)   / width) & LED_FULL_RED)   +
    (((color & LED_FULL_GREEN) / width) & LED_FULL_GREEN) +
    (((color & LED_FULL_BLUE)  / width) & LED_FULL_BLUE);

    return return_val;
}

uint32_t bat_level(bat_state_t state)
{
  uint32_t return_val = BAT_LVL_3400;

  if(state < BAT_STATE_SIZE)
    {
      return_val = bat_lvls[(uint32_t) state];
    }

  return return_val;
}

void check_lower_state(bat_state_t * bat_state,
                       uint32_t bat_lvl,
                       bat_state_t lower)
{
  if(bat_lvl < bat_level(*bat_state))
    {
      *bat_state = lower;
    }
}

void check_higher_state(bat_state_t * bat_state,
                        uint32_t bat_lvl,
                        bat_state_t higher)
{
  if(bat_lvl > bat_level(*bat_state) + BAT_HYST)
    {
      *bat_state = higher;
    }
}

void set_bat_state(bat_state_t * bat_state, uint32_t bat_lvl)
{
  bat_state_t state = BAT_STATE_SIZE;

  for(;;)
    {
      switch(*bat_state)
        {
        case BAT_STATE_NORMAL:
          check_lower_state(bat_state, bat_lvl, BAT_STATE_3560);
          break;

        case BAT_STATE_3560:
          check_lower_state(bat_state, bat_lvl, BAT_STATE_3520);
          check_higher_state(bat_state, bat_lvl, BAT_STATE_NORMAL);
          break;

        case BAT_STATE_3520:
          check_lower_state(bat_state, bat_lvl, BAT_STATE_3480);
          check_higher_state(bat_state, bat_lvl, BAT_STATE_3560);
          break;

        case BAT_STATE_3480:
          check_lower_state(bat_state, bat_lvl, BAT_STATE_3440);
          check_higher_state(bat_state, bat_lvl, BAT_STATE_3520);
          break;

        case BAT_STATE_3440:
          check_lower_state(bat_state, bat_lvl, BAT_STATE_3400);
          check_higher_state(bat_state, bat_lvl, BAT_STATE_3480);
          break;

        case BAT_STATE_3400:
          check_lower_state(bat_state, bat_lvl, BAT_STATE_EMPTY);
          check_higher_state(bat_state, bat_lvl, BAT_STATE_3440);
          break;

        case BAT_STATE_EMPTY:
          check_higher_state(bat_state, bat_lvl, BAT_STATE_3440);
          break;

        default:
          *bat_state = BAT_STATE_3440;
        }
      if(state == *bat_state)
        {
          break;
        }
      state = *bat_state;
    }
}
