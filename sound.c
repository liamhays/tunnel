// I edit this file in fundamental mode because it makes it a lot
// easier.

void a_out_to_7f() __naked {
__asm
  out (#0x7f), a
  ret
__endasm;
}

void a_out_to_06() __naked {
__asm
  out (#0x06), a
  ret
__endasm;
}

#pragma save
void reset_stereo() {
  // just write 0xff to port 0x06
__asm
  ld a, #0xff
  call _a_out_to_06
__endasm;
}
#pragma restore

#pragma save
#pragma disable_warning 85 // unused argument

// these would appear to only work on channel 0.
void /*unsigned char*/ mute_channel(unsigned char channel) __z88dk_fastcall {
  // __z88dk_fastcall means the input arg is passed the
  // way we would return a value (i.e., channel is char, so
  // it's in L).

__asm
  ld a, l
  ; sla is a once-and-done instruction; we have to run it 5 times
  ; to push five zeros to the right of our number
  sla a
  sla a
  sla a
  sla a
  sla a
  ld b, #0b10011111 ; latch/data, empty channel slot, volume 0xf
  or b
  ; this actually works!
  call _a_out_to_7f
  ;ld l, a
__endasm;
}
#pragma restore

// this doesn't work for channel 3 (noise)
#pragma save
#pragma disable_warning 85 // unused argument
void /*unsigned char*/ max_channel(unsigned char channel) __z88dk_fastcall {
__asm
  ld a, l
  sla a
  sla a
  sla a
  sla a
  sla a
  ld b, #0b10010000 ; latch/data, empty channel slot, volume 0x0
  or b
  call _a_out_to_7f
  ;ld l, a
__endasm;
}
#pragma restore

#pragma save
void select_sound() {
  // when this is called, everything should be muted
  // let's start by trying A6, or 0x040.
__asm
  ld a, #0xff
  call _a_out_to_06
__endasm;

  max_channel(0);
__asm
  ld a, #0b10000000
  call _a_out_to_7f
  ld a, #0b00000100
  call _a_out_to_7f

__endasm;
  for (unsigned int i=0; i<8000; i++) continue;
  mute_channel(0);
}
#pragma restore


#define PAUSE_LEN 8000
#pragma save
void pause_sound() {
// let's make this play 440 Hz, stop, then play 900 Hz.
// I don't know why SDCC won't parse inline labels.

  max_channel(0);
__asm

  ; begin actual sound data
  ; A4 is 0xXE0F accourding to Sega docs (how cool that we have them!)
  ; however, the order is wrong: this is actually 0xFE, because
  ; 0x0F is the lower bits
  ; this gives us 0011111110

  ; left side
  ld a, #0b11110000
  out (#0x06), a
  
  ld a, #0b10001110
  call _a_out_to_7f
  ld a, #0b00001111
  call _a_out_to_7f
__endasm;

  for (unsigned long i = 0; i < PAUSE_LEN; i++) continue;
  
__asm
  ; now for 900 Hz.
  ; the closest we have to 900 hertz in the Sega table is A5, at 880.
  ; A5 is stored with XF and 07, so its data is 0x7F, or 01111111.

  ; right side
  ld a, #0b00001111
  out (#0x06), a
  
  ld a, #0b10000111
  call _a_out_to_7f
  ld a, #0b00000111
  call _a_out_to_7f
__endasm;

  for (unsigned int j = 0; j < PAUSE_LEN; j++) continue;
  mute_channel(0);
}
#pragma restore

#pragma save
void pause_unlock_sound() {
// let's make this play 440 Hz, stop, then play 900 Hz.
// I don't know why SDCC won't parse inline labels.

  max_channel(0);
__asm

  ; 900 Hz
  ld a, #0b00001111
  call _a_out_to_06
  
  ld a, #0b10000111
  call _a_out_to_7f
  ld a, #0b00000111
  call _a_out_to_7f
__endasm;

  for (unsigned long i = 0; i < PAUSE_LEN; i++) continue;

__asm
  ; 440 Hz
  ld a, #0b11110000
  call _a_out_to_06
  
  ld a, #0b10001110
  call _a_out_to_7f
  ld a, #0b00001111
  call _a_out_to_7f
__endasm;
  
  for (unsigned int j = 0; j < PAUSE_LEN; j++) continue;
  
  mute_channel(0);
}
#pragma restore

unsigned char low_dat;
unsigned char high_dat;
//unsigned char __at(0xD000) mute_value;

#pragma save
void start_sound() {

// a rise from E3 to E4. We could do anything, now that we have
// a working algorithm
  reset_stereo();
  max_channel(0);
  // this is producing the right numbers, the problem is that the sound isn't coming out
  for (unsigned int i = 0x2A7; i >= 0x071; i--) {
    low_dat = (i & 0b1111) | 0b10000000;
    high_dat = (i & 0b1111110000) >> 4;
    // It looks like the counter is stored in BC
    __asm
	ld a, (_low_dat)
    	call _a_out_to_7f
    	ld a, (_high_dat)
    	call _a_out_to_7f
    __endasm;
    // 50 is a good length for this
    for (unsigned int j = 0; j < 50; j++) continue;
  }

// now play a C chord
// apparently A4 on a guitar is open 5th string
// that means a C is C5, A6, and D6

  mute_channel(0);
  mute_channel(1);
  mute_channel(2);

  // C5 is 0x0D6, A6 is 0x03C, D7 is 0x30
  // 11010110, 111100, 110000
  __asm
    ld a, #0b10000110
    call _a_out_to_7f
    ld a, #0b00001101
    call _a_out_to_7f

    ld a, #0b10101100
    call _a_out_to_7f
    ld a, #0b00000011
    call _a_out_to_7f

    ld a, #0b11010000
    call _a_out_to_7f
    ld a, #0b00000011
    call _a_out_to_7f

  __endasm;

  max_channel(0);
  max_channel(1);
  max_channel(2);

  // this makes the chord hold for a good length of time
  for (unsigned long counter = 0; counter < 40000; counter++) continue;

  mute_channel(0);
  mute_channel(1);
  mute_channel(2);
}
#pragma restore

/*#pragma save
void game_over_jingle() {
// this should be a "boouup-bou-bou-boouu-boouu-boouu" sound. A descending
// set of notes.
__asm
  ld a,*/

/*unsigned char __at(0xD000) max;
unsigned char __at(0xD002) mute;*/
#pragma save
void ship_noise() {
  reset_stereo();
  /*max = */max_channel(3);
__asm
  ; this is a reasonably course noise, though we might want to change
  ; it as the ship rises and falls, or something else entirely.
  ld a, #0b11100111
  call _a_out_to_7f
__endasm;
}
#pragma restore


#pragma save
void game_over_noise_burst() {
  reset_stereo();
;  max_channel(3);
;__asm
;  ld a, #0b11100101
;  ;        ||||````- Noise characteristic 0101
;  ;        |||`----- Data type byte
;  ;        |``------ Channel 3
;  ;        `-------- latch/data byte
;  call _a_out_to_7f
;__endasm;
;  mute_channel(3);

  max_channel(3);
__asm
  ld a, #0b11100100
  call _a_out_to_7f
__endasm;
  for (unsigned int i = 0; i < 11000; i++) continue;
  mute_channel(3);
}
#pragma restore