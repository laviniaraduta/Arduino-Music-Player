#ifndef EXTRAS_H
#define EXTRAS_H

#define SHUFFLE_C 0
#define SOUND_FULL_C 1
#define NEXT_C 2
#define PREV_C 3
#define PAUSE_C 4
#define SOUND_EMPTY_C 5
#define FULL_C 6
#define NULL_C 7




//////////////////////////////////////////// Custom chars //////////////////////////////////////////////////
byte shuffle_char[] = {
  B00000,
  B01010,
  B01010,
  B01010,
  B01010,
  B00100,
  B01010,
  B01010
};

byte sound_full_char[] = {
  B00001,
  B00011,
  B11111,
  B11111,
  B11111,
  B11111,
  B00011,
  B00001
};

byte sound_empty_char[] = {
  B00001,
  B00011,
  B11101,
  B10001,
  B10001,
  B11101,
  B00011,
  B00001
};

byte next_char[] = {
  B10001,
  B11001,
  B11101,
  B11111,
  B11101,
  B11001,
  B10001,
  B00000
};


byte prev_char[] = {
  B10001,
  B10011,
  B10111,
  B11111,
  B10111,
  B10011,
  B10001,
  B00000
};

byte full_char[] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

byte pause_char[] = {
  B10000,
  B11000,
  B11100,
  B11110,
  B11100,
  B11000,
  B10000,
  B00000
};


byte null_char[] = {
  B00000,
  B00000,
  B10001,
  B01010,
  B00100,
  B01010,
  B10001,
  B00000
};

#endif