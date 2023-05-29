#include <Wire.h>
#include <Arduino.h>
#include <TMRpcm.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>  
#include "extras.h"

LiquidCrystal_I2C lcd(0x27, 16, 2); 
TMRpcm audio;

///////////////////////////////// Counters for time between button press ////////////////////////////////////
volatile unsigned long last_debounce_next = 0; 
volatile unsigned long last_debounce_prev = 0; 
volatile unsigned long last_debounce_pause = 0; 
volatile unsigned long last_debounce_shuffle = 0; 
volatile unsigned long last_debounce_up = 0; 
volatile unsigned long last_debounce_down = 0; 

volatile unsigned long current_millis = millis();
const unsigned long debounce_delay = 500;

//////////////////////////////////////////////// Volume settings ///////////////////////////////////////////
const int max_volume = 11;
int volume_level = 9;

/////////////////////////////////////////// Booleans for player behavior //////////////////////////////////
unsigned long paused_time = 0;
unsigned long start_time = 0;
bool paused = false;
bool shuffle = false;
bool new_song = false;
bool start = false;

////////////////////////////////////////// Song related variables /////////////////////////////////////////
int song_number = 14;
char filename[7];
char title[32];
int track_number = 0;
int prev_song = 0;

////////////////////////////////////// Buttons Control Booleans ////////////////////////////////////////////
volatile bool next_press = false;
volatile bool prev_press = false;
volatile bool up_vol_press = false;
volatile bool down_vol_press = false;
volatile bool pause_press = false;
volatile bool shuffle_press = false;


///////////////////////////////////////////// BUTTON INTERUPTS ////////////////////////////////////////////
// next button - PD3 - INT1
ISR(INT0_vect) {
  next_press = true;
}

// PCINT2: shuffle - PD4, prev - PD5, down - PD6 and up - PD7
ISR(PCINT2_vect) {
  if ((PIND & (1 << PD4)) == 0) {
    shuffle_press = true;
  } else if ((PIND & (1 << PD5)) == 0) {
    prev_press = true;
  } else if ((PIND & (1 << PD6)) == 0) {
    up_vol_press = true;
  } else if ((PIND & (1 << PD7)) == 0) {
    down_vol_press = true;
  }
}

// PCINT0: pause/play - PB0
ISR(PCINT0_vect) {
  if ((PINB & (1 << PB0)) == 0) {
    pause_press = true;
  }
}


void interrupts_setup() {
  cli();

  // input pins
  DDRD &= ~(1 << PD5) & ~(1 << PD4) & ~(1 << PD6) & ~(1 << PD7) & ~(1 << PD2);
  DDRB &= ~(1 << PB0);

  // input pullup
  PORTD |= (1 << PD4) | (1 << PD5) | (1 << PD6) | (1 << PD7) | (1 << PD2);
  PORTB |= (1 << PB0);

  // external INTERRUPT
  EIMSK |=  (1 << INT0);

  // Interrupt constrol register
  PCICR |= (1 << PCIE2) | (1 << PCIE0);

  // interrupt PIN CHANGE
  PCMSK2 |= (1 << PCINT20) | (1 << PCINT21) | (1 << PCINT22) | (1 << PCINT23);
  PCMSK0 |= (1 << PCINT0);

  // falling edge of INT0
  EICRA |= (1 << ISC01);

  sei();
}


void get_song_titles() {
  for (int i = 1; i <= song_number; i++) {
    sprintf(filename, "%02d.wav", i);
    Serial.print("Name of ");
    Serial.print(filename);
    Serial.print(" is ");
    audio.listInfo(filename, title, 0);
    Serial.println(title);
  }
}

// get the index of the next song, circular
int get_next_track() {
  if (track_number == song_number) {
    return 1;
  } else {
    return track_number + 1;
  }
}

// get the index of the grevious song, circular
int get_prev_track() {
  if (track_number == 1) {
    return song_number;
  } else {
    return track_number - 1;
  }
}

void display_title() {
  lcd.setCursor(0, 0);
  lcd.clear();
  lcd.print(track_number);
  lcd.print(".");
  lcd.print(title);
  audio.play(filename);
}

// plays the next song and prints the name of the song on the lcd
void play_next_song() {
  audio.stopPlayback();
  new_song = true;
  prev_song = track_number;
  if (shuffle) {
    randomSeed(analogRead(0));

    // thy to get a random index, that is not the next in line
    int new_track = random(1, song_number);
    while (new_track == track_number || new_track == get_next_track()) {
      new_track = random(1, song_number);
    }

    track_number = new_track;
  } else {
    track_number = get_next_track();
  }

  // all songs are named index.wav
  sprintf(filename, "%02d.wav", track_number);

  Serial.print("next - Will play: ");
  Serial.println(filename);

  // get the title of the song from the song metadata
  audio.listInfo(filename, title, 0);
  display_title();
  
} 

// plays the previous song and prints the name of the song on the lcd
void play_prev_song() {
  audio.stopPlayback();
  new_song = true;

  // when shuffling the last song index is kept
  // when trying to prev more than 2 times, the songs will just be played in order
  if (prev_song == track_number) {
    track_number = get_prev_track();
  } else {
    track_number = prev_song;
  }
  prev_song = track_number;

  sprintf(filename, "%02d.wav", track_number);
  Serial.print("prev - Will play: ");
  Serial.println(filename);

  audio.listInfo(filename, title, 0);

  display_title();
}

void setup() {
  // lcd initialization
  lcd.init();
  lcd.clear();
  lcd.backlight();
  Serial.begin(9600);
  pinMode(3, OUTPUT);

  // create custom chars
  lcd.createChar(SHUFFLE_C, shuffle_char);
  lcd.createChar(NEXT_C, next_char);
  lcd.createChar(PREV_C, prev_char);
  lcd.createChar(SOUND_FULL_C, sound_full_char);
  lcd.createChar(SOUND_EMPTY_C, sound_empty_char);
  lcd.createChar(FULL_C, full_char);
  lcd.createChar(NULL_C, null_char);
  lcd.createChar(PAUSE_C, pause_char);


  lcd.setCursor(0, 0);
  lcd.clear();
  lcd.print("Music player");
  lcd.setCursor(0, 1);
  lcd.print("starting...");
  delay(1000);
  
  lcd.setCursor(0, 0);
  lcd.clear();
  lcd.print("Initializing SD card...");
  while (!SD.begin(10)) {}
  lcd.setCursor(0, 1);
  lcd.print("Done :)");

  get_song_titles();

  audio.setVolume(4);
  audio.speakerPin = 9;

  interrupts_setup();

  lcd.setCursor(1, 0);
  lcd.clear();
  lcd.print("Press play to");
  lcd.setCursor(1, 1);
  lcd.print("feel nostalgic");
}

void loop() {
  if (start) {
    if (!audio.isPlaying()) {
      play_next_song();
      start_time = millis();
      paused_time = 0;
    } else if (!paused) {
      int analogValue = analogRead(A1);  // read value from A1
      int intensity = map(analogValue, 0, 1023, 0, 255);  // map value between 0 and 255
      if (analogValue > 700) {
        analogWrite(3, intensity);
      } else {
        analogWrite(3, 0);  // turn off LED when analogValue under threshold
      }

      unsigned long curr_time = millis();
      unsigned long elapsed_time = curr_time - start_time - paused_time;
      unsigned int mins = elapsed_time / 60000;
      unsigned int secs = (elapsed_time % 60000) / 1000;
      lcd.setCursor(0, 1);
      lcd.print(String(mins, DEC));
      lcd.print(":");
      if (secs < 10) {
        lcd.print("0");
      }
      lcd.print(String(secs, DEC));
      lcd.setCursor(7, 1);
      lcd.print(" ");
    } else if (paused) {
      lcd.setCursor(7, 1);
      lcd.write((byte)PAUSE_C);
    }
  }

  if (new_song) {
    if (shuffle) {
      if (shuffle) {
        lcd.setCursor(12, 1);
        lcd.write(SHUFFLE_C);
      } else {
        lcd.setCursor(12, 1);
        lcd.print(" ");
      }
    }
    new_song = false;
  }

  if (next_press) {
    unsigned long current_millis = millis();
    if (current_millis - last_debounce_next < debounce_delay) {
      next_press = false;
    } else {
      // button pressed => next song
      next_press = false;
      last_debounce_next = current_millis;
      play_next_song();
      start_time = millis();

      // print small animation on lcd
      lcd.setCursor(13, 1);
      lcd.write(NEXT_C);
      delay(200);
      lcd.setCursor(13, 1);
      lcd.print(" ");
    }
  }

  if (prev_press) {
    current_millis = millis();
    if (current_millis - last_debounce_prev < debounce_delay) {
      prev_press = false;
    } else {
      // button pressed => next song
      Serial.println("PREV");
      last_debounce_prev = current_millis;
      play_prev_song();
      start_time = millis();
      
      lcd.setCursor(13, 1);
      lcd.write(PREV_C);
      delay(200);
      lcd.setCursor(13, 1);
      lcd.print(" ");
    }
  }

  if (down_vol_press) {
    current_millis = millis();
    if (current_millis - last_debounce_down < debounce_delay) {
      down_vol_press = false;
    } else {
      down_vol_press = false;
      // button pressed => next song
      Serial.println("DOWN");
      last_debounce_down = current_millis;
      volume_level--;
      if (volume_level <= 0) {
        volume_level = 0;
        lcd.clear();
        lcd.setCursor(5, 0);
        lcd.print("MUTED");
        lcd.setCursor(2, 1);
        lcd.write((byte)SOUND_EMPTY_C);
        lcd.write((byte)NULL_C);
      } else {
        lcd.clear();
        lcd.setCursor(5, 0);
        lcd.print("VOLUME ");
        lcd.print(volume_level);
        lcd.setCursor(2, 1);
        lcd.write(SOUND_FULL_C);
        for (int i = 1; i <= volume_level; i++) {
          lcd.write(FULL_C);
        }
        audio.volume(0);
      }
      delay(1000);
      lcd.setCursor(0, 0);
      lcd.clear();
      lcd.print(track_number);
      lcd.print(".");
      lcd.print(title);
      new_song = true;
    }
  }

  if (up_vol_press) {
    current_millis = millis();
    if (current_millis - last_debounce_up < debounce_delay) {
      up_vol_press = false;
    } else {
      up_vol_press = false;
      Serial.println("UP");

      volume_level++;
      // do not get over the max level
      if (volume_level > max_volume) {
        volume_level = max_volume;
      } else {
        audio.volume(1);
      }

      // display the level as a bar and as a number
      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print("VOLUME ");
      lcd.print(volume_level);
      lcd.setCursor(2, 1);
      lcd.write(SOUND_FULL_C);
      for (int i = 1; i <= volume_level; i++) {
        lcd.write(FULL_C);
      }
      delay(1000);
      lcd.setCursor(0, 0);
      lcd.clear();
      lcd.print(track_number);
      lcd.print(".");
      lcd.print(title);

      // this is just to print the shuffle symbol
      new_song = true;
    }
  }

  if (pause_press) {
    current_millis = millis();
    if (current_millis - last_debounce_pause < debounce_delay) {
      pause_press = false;
    } else {
      Serial.println("PAUSE");
      last_debounce_pause = current_millis;

      if (!start) {
        // first time the player is started pause acts as next
        start = true;
        play_next_song();
      } else {
        audio.pause();
        paused = !paused;

        if (paused) {
          // paused became true => now the song is paused so keep the moment it was paused
          paused_time += millis() - start_time;
        } else {
          // the song is replayed, start the counter again from the pause time
          start_time = millis() - paused_time;
          paused_time = 0;
        }
      }
    }
  }

  if (shuffle_press) {
    current_millis = millis();
    if (current_millis - last_debounce_shuffle < debounce_delay) {
      shuffle_press = false;
    } else {
      Serial.println("SHUFFLE");
      last_debounce_shuffle = current_millis;
      shuffle = !shuffle;

      // when shuffling is activated a symbol is displayed
      if (shuffle) {
        lcd.setCursor(12, 1);
        lcd.write(SHUFFLE_C);
      } else {
        lcd.setCursor(12, 1);
        lcd.print(" ");
      }
      
    }
  }
}
