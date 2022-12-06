#define TARGET_GG // you can change this in SMSlib.h, but it's easier to see here

#include "SMSlib.h"
#include <stdbool.h>
#include <string.h>

#include "images_h/title.h"
#include "images_h/ship.h"
#include "images_h/empty_screen.h"
#include "images_h/text.h"
#include "images_h/explosion.h"
#include "images_h/tower2.h" // tower2 has the multiple colors and looks nicer
#include "images_h/enemy_ship.h"

#include "sound.c"
// We have the hardware to do some interesting flying effects, so
// maybe some stars scrolling at different speeds (like that screen
// after you choose a character in Mega Man). We could do this by
// scrolling two or three sprites (no more, to prevent tearing), with
// different size points and tiles.

SMS_EMBED_SEGA_ROM_HEADER(1, 1);
SMS_EMBED_SDSC_HEADER_AUTO_DATE(1, 0, "Liam Hays", "Tunnel", "Fly through the tunnel and don't hit the sides!");

void erase_screen();
unsigned char prng();
unsigned char run_prng();
void initialize_game();
void game_over(bool ceiling);
void update_ship_position();
unsigned char gen_stalactite_height();
void update_tunnel();
void start_button_loop();

const unsigned int
bg_palette[16] = {0x000, RGBHTML(0x5742F5), 0x0FF, 0xFFF, 0x11E, 0x06F, 0x0BF,
//              black          purple     yellow white  red   orange  lighter orange
		  RGBHTML(0xFFBF00), RGBHTML(0xFFE100), RGBHTML(0xFFF700), RGBHTML(0xA6B238), 
		// seven           eight               nine               ten
		  0, 0, 0, 0, 0};

// we need to include black, the ship's purple, and the explosion
// colors, because they're all used as sprites.
const unsigned int
sprite_palette[16] = {0x000, // black
		    RGBHTML(0x5742F5), 0x0FF, 0xFFF, 0x11E, 0x06F, 0x0BF, RGBHTML(0x1FA100), RGBHTML(0xCE00FF)};



void erase_screen() {
  SMS_loadTileMap(0, 0, empty_screen, sizeof(empty_screen));
}

// initalizing the seed from r only works when seed is pre-initialized. why?
// interestingly, the PRNG also needs random to be initialized to function.
unsigned char seed = 0;

#pragma save
#pragma disable_warning 59
unsigned char get_register_r() __preserves_regs(b, c, d, e, h, iyh, iyl) {
  // Z80 only lets you copy r to a, so we have to then copy a to l for
  // the return.  It's important to note that only seven bits of r are
  // ever changed (the eighth is only configurable by an ld r,a
  // instruction), so we can load it and shift it to multiply it by 2.
  
  __asm__("ld a,r\nsla a\nld l,a");
}
#pragma restore

// random has to be initialized to keep the PRNG stable. (see notes just above)
unsigned char random = 0;

/* 
 * This is a simple two-byte PRNG, from
 * https://wikiti.brandonw.net/index.php?title=Z80_Routines:Math:Random
 * I moved it to an SDCC function. However, it is quite touchy, and
 * circumstances I don't really seem to be able to control can cause
 * the whole tunnel to crash. As one might anticipate, pragma save
 * and restore help and appear to be vital.
 * (A few seconds later, I found the issue; see the declaration for
 *  random above).
 */
// this preserves all registers except af; and it's worth noting that
// the __preserves_regs doesn't make any changes. however, I still
// think it's a good idea.
#pragma save
#pragma disable_warning 59
unsigned char prng() __preserves_regs(b, c, d, e, h, iyh, iyl) {
__asm
        push    hl
        push    de
        ld      hl,(_seed)
        ld      a,r
        ld      d,a
        ld      e,(hl)
        add     hl,de
        add     a,l
        xor     h
        ld      (_seed),hl
        pop     de
        pop     hl
  
        ld      l,a
__endasm;
}
#pragma restore

unsigned char run_prng() {
  random = prng();
  seed = random; // update to make the next random a new value
  return random;
}

void start_button_loop() {
  unsigned int keys;

  while (true) {
    keys = SMS_getKeysStatus();
    if (keys & GG_KEY_START) {
      break;
    }
  }
}

signed char ship_sprites[12];


// center on the x axis
unsigned int ship_x = 12*8 + 192/8 - 8;
unsigned int ship_y = (224 - 144);

// 36 is old but works
#define divider 36

// turn to true so that we don't add a million sprites (though it might not actually matter)
// bool ship_already_added = false;

unsigned char stalactite_heights[20]; // one for each column
unsigned char stalagmite_heights[20];
// not only does making this global reduce the code size, it
// significantly reduces the number of instructions needed for the
// memcpy()s.
unsigned char new_heights[20];

// how many tiles have advanced (increase this in the array left-shift
// loop)
unsigned long passed_tile_count = 1;
unsigned char max_height = 6; // increase as we go
unsigned long high_score = 0; // update at each game over if score > high_score

bool hardmode = false;
unsigned char enemy_ship_sprites[1];
unsigned char __at(0xD000) enemy_x = 0;
unsigned char __at(0xD002) enemy_y = 0;
/* ********************
 * Each screen update function is responsible for writing new data.
 */

void update_ship_position() {
  SMS_updateSpritePosition(ship_sprites[0], ship_x, ship_y);
  SMS_updateSpritePosition(ship_sprites[1], ship_x + 8, ship_y);
  SMS_updateSpritePosition(ship_sprites[2], ship_x, ship_y + 8);
  SMS_updateSpritePosition(ship_sprites[3], ship_x + 8, ship_y + 8);
  SMS_updateSpritePosition(ship_sprites[4], ship_x + 16, ship_y + 8);
  SMS_updateSpritePosition(ship_sprites[5], ship_x + 24, ship_y + 8);
  SMS_updateSpritePosition(ship_sprites[6], ship_x, ship_y + 16);
  SMS_updateSpritePosition(ship_sprites[7], ship_x + 8, ship_y + 16);
  SMS_updateSpritePosition(ship_sprites[8], ship_x + 16, ship_y + 16);
  SMS_updateSpritePosition(ship_sprites[9], ship_x + 24, ship_y + 16);
  SMS_updateSpritePosition(ship_sprites[10], ship_x, ship_y + 24);
  SMS_updateSpritePosition(ship_sprites[11], ship_x + 8, ship_y + 24);
  UNSAFE_SMS_copySpritestoSAT(); // we're in VBlank, this is okay
}

unsigned char display_frames = 0;
unsigned char max_display_frames = 15; // when we reach this number, redraw
// unsigned char ship_fall_rate = 1;
// so it's actually not that great to control the fall speed of the
// ship. not only does it not really affect the challenge, it also
// seems to break the explosion drawing routine.


/* ******* 4 is the minimum height a tower can be to show on the screen. */

unsigned char gen_stalactite_height() {
  // remember, 4 is the minimum height a column has to be to display
  // on the screen (they're one tile tall)---but I'm not sure why,
  // because it seems like my code (in update_tunnel()) shouldn't
  // cause that
  
  // local variable produces one less line than global.
  unsigned char out = 4 + run_prng()/divider;
  if (out >= max_height) out = max_height; // difficulty control
  return out;
}


// there's no reason we /have/ to use hex
unsigned int game_over_text[] = {30, 50, 62, 54, 0, 38, 71, 54, 67};
unsigned int score_text[] = {42, 52, 64, 67, 54};
unsigned int press_start_text[] = {39, 41, 28, 42, 42, 0, 42, 43, 24, 41, 43};
unsigned int level_text[] = {35, 28, 45, 28, 35};
unsigned int high_text[] = {31, 58, 56, 57};
// still have to use hex here for priority (well, not really, but it's easier this way)
// unsigned int pause_text[] = {0x101E, 0x1018, 0x1022, 0x1020, 0x1019};

// the highest this can be is 31*8, which fits in an unsigned char.
unsigned char scroll_value = 30*8;
// you would think this would have to be 27, but we have to be one
// more, apparently, to make the writing offscreen work.

unsigned char write_column = 26;
// signed char star;
// unsigned char x
void update_tunnel() {
  
  /* This pair of memcpy()s is an alterative approach to shifting an
   * array left to a for loop, which is the same code I use in the
   * "game" of SCEPPU. Adding one causes memcpy to copy the last, not
   * the first nineteen elements (it seems confusing, but think about
   * it).
   *
   * Source for that idea is https://stackoverflow.com/a/1163630
   *
   * Using memcpy() should also be faster than the for loop, because
   * it's optimized with the z80's ldir (load, increment, repeat)
   * instruction. I think it does make the game run a little
   * smoother. In addition, the +1 doesn't cause any problems, because
   * it's just added to the address of the array by the compiler and
   * assembled into an extra nibble (or whatever) by the
   * assembler. Other words: not a performance problem whatsoever.
   */

  // we are only drawing one column of tiles, so we can do a lot during VBlank.
  memcpy(new_heights, stalactite_heights + 1, 19 * sizeof(unsigned char));
  // we have to do this in here, otherwise nothing changes
  new_heights[19] = gen_stalactite_height();
  memcpy(stalactite_heights, new_heights, 20 * sizeof(unsigned char));

  memcpy(new_heights, stalagmite_heights + 1, 19 * sizeof(unsigned char));
  // we have to do this in here, otherwise nothing changes
  // gen_stalactite_height();
  new_heights[19] = gen_stalactite_height();
  memcpy(stalagmite_heights, new_heights, 20 * sizeof(unsigned char));

  // star = SMS_addSprite(16, 8,
  /* SMS_setNextTileatXY(write_column, 15);
     SMS_setTile(56);*/
  SMS_setBGScrollX(scroll_value);
  // lower and upper limits (inclusive) of 0 and 31 make the scrolling
  // run around the whole screen.
  
  // Both a switch () and an if () create the same assembly here. I
  // keep an if because it's easier to read.
  if (scroll_value == 0) scroll_value = 31*8;
  else scroll_value -= 8;
  
  passed_tile_count++;
  // write to all tiles in that column

  // we should probably be updating in scroll_value+1
  // write_column = scroll_value + 1;
  if (write_column == 31) write_column = 0; /* This now works! */
  else write_column++;
  // at write_column of 18, after drawing one tile farther to the
  // right, it decides to draw a row of individual tiles. It doesn't
  // stop until it reaches column 0, where it actually draws in the
  // next row (probably) because it's trying to draw at column 32,
  // which does not exist.

  // erase the column, which works very well.
  for (unsigned char row = 3; row <= 20; row++) {
    SMS_setNextTileatXY(write_column, row);
    SMS_setTile(0x00);
  }

  for (unsigned char height = 3;
       height < stalactite_heights[19];
       height++) {
    SMS_setNextTileatXY(write_column, height);
    SMS_setTile(0x0D);
  }

  // 21 makes it draw the right amount, to the bottom of the screen
  // as does 24 for the correct height
  for (unsigned char oppheight = 24 - stalagmite_heights[19];
       oppheight < 21;
       oppheight++) {
    SMS_setNextTileatXY(write_column, oppheight);
    SMS_setTile(0x0D);
  }
}

bool y_change_direction; // false for subtract, true for add
void update_enemies() {
  enemy_x -= 3;
  
  if (enemy_y >= (20*8 - 1)) y_change_direction = false;
  else if (enemy_y <= (3*8)) y_change_direction = true;

  if (y_change_direction) enemy_y += 2;
  else enemy_y -= 2;

  SMS_updateSpritePosition(enemy_ship_sprites[0], enemy_x, enemy_y);
  UNSAFE_SMS_copySpritestoSAT();
}
// this holds the values for the extra sprites we have to draw to have
// a complete explosion
unsigned char explosion_extrasprites[2];

void game_over(bool ceiling) { // ceiling is whether or not we hit the ceiling
  // I'm not very worried about performance in this function. As long
  // as it appears to show up instantly, it's not an issue.

  /* ******* There's no reason we couldn't just update the ship's
     * tiles to be the explosion. However, to do this, we have to
     * create two new sprites and keep track of them. Not a problem,
     * but worth noting.*/
  game_over_noise_burst();
#define EXPLOSION_START 77
  // SMS_initSprites(); // effectively clears SAT
  SMS_updateSpriteImage(enemy_ship_sprites[0], 0);
  SMS_updateSpriteImage(ship_sprites[2], 0);
  SMS_updateSpriteImage(ship_sprites[3], EXPLOSION_START);
  SMS_updateSpriteImage(ship_sprites[4], EXPLOSION_START + 1);
  SMS_updateSpriteImage(ship_sprites[6], EXPLOSION_START + 2);
  SMS_updateSpriteImage(ship_sprites[7], EXPLOSION_START + 3);
  SMS_updateSpriteImage(ship_sprites[8], EXPLOSION_START + 4);
  SMS_updateSpriteImage(ship_sprites[9], EXPLOSION_START + 5);
  SMS_updateSpriteImage(ship_sprites[10], EXPLOSION_START + 6);
  SMS_updateSpriteImage(ship_sprites[11], EXPLOSION_START + 7);

  unsigned char ex0_x = ship_x + 16;
  unsigned char ex0_y = ship_y + 24;
  unsigned char ex1_x = ship_x + 8;
  unsigned char ex1_y = ship_y + 32;
  // for whatever reason, at least in Emulicious, we have to have an
  // offset depending on where the ship hit. these are numbers that
  // seem to work.

  if (ceiling) {
    ex0_y++; // = ship_y + 25;
    ex1_y++; // = ship_y + 33;
  } else {
    ex0_y--; // = ship_y + 23;
    ex1_y--; // = ship_y + 31;
  }



  explosion_extrasprites[0] = SMS_addSprite(ex0_x, ex0_y, 85);
  explosion_extrasprites[1] = SMS_addSprite(ex1_x, ex1_y, 86);


  // hide these three sprites from the ship that we don't use
  SMS_hideSprite(ship_sprites[0]);
  SMS_hideSprite(ship_sprites[1]);
  SMS_hideSprite(ship_sprites[5]);
  SMS_copySpritestoSAT();

  // hold the explosion on screen for a bit
  for (unsigned int i = 0; i < 5000; i++) continue;

  for (unsigned long stop_frame = 0; stop_frame < 90000; stop_frame++) continue;

  erase_screen(); // do this first to reduce artifacting
  SMS_setBGScrollX(0); // reset scroll for the next round

  // erase_screen() only erases the tiles, so we have to erase the ship too.
  for (unsigned char i = 0; i < 12; i++) {
    SMS_hideSprite(ship_sprites[i]);
  }
  SMS_hideSprite(explosion_extrasprites[0]);
  SMS_hideSprite(explosion_extrasprites[1]);
  SMS_copySpritestoSAT();



  // we don't want to use autoSetUpTextRenderer() here because we have
  // a specific set of tiles to load (to save memory, because why not)

  // an unsigned long can hold ten digits at most.
  // score_text and high_text are aligned so that there is enough space
  SMS_loadTileMapArea(10, 6, game_over_text, 9, 1);
  unsigned char level = 0;
  switch (max_height) {
  case 6:
    level = 1;
    break;
  case 7:
    level = 2;
    break;
  case 8:
    level = 3;
    break;
  case 9:
    level = 4;
    break;
  }
  
#define NUMBERS_START 0x0E
  SMS_loadTileMapArea(10, 8, level_text, 5, 1);
  SMS_setNextTileatXY(16, 8);
  SMS_setTile(NUMBERS_START + level);
  SMS_loadTileMapArea(10, 10, score_text, 5, 1);
  SMS_loadTileMapArea(10, 11, high_text, 4, 1);
  unsigned char digit;
  // we fill in from the right because that's how our digit reading loop works
  unsigned char write_col = 25; // fill in from the right, and support the full length of a long
  if (passed_tile_count > high_score) {
    high_score = passed_tile_count;
  }
  // this is probably very fast---just a few mathematical operations and no printf().
  // it works great, which is really cool too.
  

  do {
    // we have to iterate over each digit here
    digit = passed_tile_count % 10;
    SMS_setNextTileatXY(write_col, 10);
    SMS_setTile(NUMBERS_START + digit); // our number tiles start at index 0x0E.
    write_col--;
  } while (passed_tile_count /= 10);

  // put high score on screen
  // a temporary variable because we're dividing it and want to keep the old one
  unsigned long tmp_high_score = high_score;
  write_col = 25; // reset for next loop
  do {
    // we have to iterate over each digit here
    digit = tmp_high_score % 10;
    SMS_setNextTileatXY(write_col, 11);
    SMS_setTile(NUMBERS_START + digit); // our number tiles start at index 0x0E.
    write_col--;
  } while (tmp_high_score /= 10);

  
  SMS_loadTileMapArea(10, 16, press_start_text, 11, 1);
  // game_over_jingle();
  start_button_loop();
  for (unsigned int j = 0; j < 7000; j++) continue;
  initialize_game();
}

void initialize_game() {
  // control the display to help reduce color dots or whatever Emulicious is showing
  SMS_displayOff();

  // we have to put these here so that they occur only when the screen
  // is off, making it look more polished
  
  // get some nice hardware semi-randomness.
  seed = get_register_r();

  // bg_palette[1] = run_prng() + 0xF;
  // sprite_palette[1] = run_prng() + 0xF;
  GG_loadBGPalette(bg_palette);
  GG_loadSpritePalette(sprite_palette);
  
  SMS_loadTiles(ship_tiles, 1, sizeof(ship_tiles));
  SMS_loadTiles(tower2, 13, sizeof(tower2));
  // load these to prepare for later
  SMS_load1bppTiles(text_tiles_1bpp, 0x0E, sizeof(text_tiles_1bpp), 0x00, 0x03);
  SMS_loadTiles(explosion_tiles, 77, sizeof(explosion_tiles));
  SMS_loadTiles(enemy_ship_tile, 87, sizeof(enemy_ship_tile));
  // SMS_loadTiles(spike_tower, 56, sizeof(spike_tower));
  // SMS_loadTiles(small_star, 56, sizeof(small_star));
  // (3,6) in tile format is start of GG screen (loadTileMap takes (6, 3))
  
  // we're still reading from the first half


  // X scroll is already set to 0 from startup or game_over()
  /* for (unsigned char i = 0; i < 20; i++) {
    // start with an empty screen to ease the player in
    stalactite_heights[i] = 0; // gen_stalactite_height(); // 0;
    }*/
  // memset seems to be somewhat optimized (using djnz), and this
  // takes much less produced assembly than a for loop.
  memset(stalactite_heights, 0, 20);
  memset(stalagmite_heights, 0, 20);
  passed_tile_count = 1;
  max_height = 6;
  display_frames = 0;
  max_display_frames = 15;
  
  ship_y = (224 - 144);
  scroll_value = 30*8;
  write_column = 27;
  
  erase_screen();

  // (44,24) should be the first pixel the GG screen shows

  /* ***** I don't think it's a good idea to add new sprites
   * (hideSprite just moves them) every time we initialize a new
   * game. A simple check fixes that problem, in case it is one. */
  /* if (ship_already_added) {
    // update_ship_position(); // safe to call because display is off
    SMS_updateSpriteImage(ship_sprites[2], 3);
    SMS_updateSpriteImage(ship_sprites[3], 4);
    SMS_updateSpriteImage(ship_sprites[4], 5);
    SMS_updateSpriteImage(ship_sprites[6], 7);
    SMS_updateSpriteImage(ship_sprites[7], 8);
    SMS_updateSpriteImage(ship_sprites[8], 9);
    SMS_updateSpriteImage(ship_sprites[9], 10);
    SMS_updateSpriteImage(ship_sprites[10], 11);
    SMS_updateSpriteImage(ship_sprites[11], 12);
    UNSAFE_SMS_copySpritestoSAT();
    update_ship_position();
    // extra explosion sprites are already hidden once we're here.
    } else {*/
  SMS_initSprites();
  ship_sprites[0] = SMS_addSprite(ship_x, ship_y, 1);
  ship_sprites[1] = SMS_addSprite(ship_x + 8, ship_y, 2);
  ship_sprites[2] = SMS_addSprite(ship_x, ship_y + 8, 3);
  ship_sprites[3] = SMS_addSprite(ship_x + 8, ship_y + 8, 4);
  ship_sprites[4] = SMS_addSprite(ship_x + 16, ship_y + 8, 5);
  ship_sprites[5] = SMS_addSprite(ship_x + 24, ship_y + 8, 6);
  ship_sprites[6] = SMS_addSprite(ship_x, ship_y + 16, 7);
  ship_sprites[7] = SMS_addSprite(ship_x + 8, ship_y + 16, 8);
  ship_sprites[8] = SMS_addSprite(ship_x + 16, ship_y + 16, 9);
  ship_sprites[9] = SMS_addSprite(ship_x + 24, ship_y + 16, 10);
  ship_sprites[10] = SMS_addSprite(ship_x, ship_y + 24, 11);
  ship_sprites[11] = SMS_addSprite(ship_x + 8, ship_y + 24, 12);

  if (hardmode) {
    enemy_x = run_prng();
    // enemy_y = 90;
    enemy_y = 30 + (run_prng() >> 6);
    while (enemy_x < 6*8 || enemy_x > 25*8) enemy_x = run_prng();
    enemy_ship_sprites[0] = SMS_addSprite(enemy_x, enemy_y, 87);
  }
  UNSAFE_SMS_copySpritestoSAT();


  SMS_displayOn();
}

void title_screen() {
  GG_loadBGPalette(title_palette);
  GG_loadSpritePalette(title_palette);
  SMS_useFirstHalfTilesforSprites(true);
  // using aPLib data (compressed with apultra, of course) decreases
  // the size by a good 3K or so.
  UNSAFE_SMS_loadaPLibcompressedTiles(title_tiles_apultra, 1);
  SMS_load1bppTiles(text_tiles_1bpp, 108, sizeof(text_tiles_1bpp), 0x00, 0x03);
  // sometimes this causes some junk (extra tiles from the
  // decompression that aren't supposed to be there) to show up on the
  // screen. I hope it's not an issue with the
  // decompression. empty_screen() doesn't seem to affect it, though
  // it might just be the emulator (there's no restart key on a real
  // GG)
  SMS_loadTileMapArea(6, 3, title_tilemap_tunnel_text_row1, 18, 1);
  SMS_loadTileMapArea(9, 4, title_tilemap_tunnel_text_row2, 14, 1);
  SMS_loadTileMapArea(8, 5, title_tilemap_tunnel_text_row3, 15, 1);
  SMS_loadTileMapArea(8, 6, title_tilemap_tunnel_text_row4, 17, 1);
  SMS_loadTileMapArea(13, 10, title_tilemap_normal_text, 6, 1);
  SMS_loadTileMapArea(13, 12, title_tilemap_hard_text, 4, 1);
  SMS_loadTileMapArea(6, 16, title_tilemap_psb_text_row1, 20, 1);
  SMS_loadTileMapArea(6, 17, title_tilemap_psb_text_row2, 20, 1);
  SMS_loadTileMapArea(13, 19, title_tilemap_copyright_text_row1, 4, 1);
  SMS_loadTileMapArea(13, 20, title_tilemap_copyright_text_row2, 4, 1);

  // we should have a menu here for "Normal" and "Hard"

  SMS_initSprites();
  signed char arrow = SMS_addSprite(12*8, 10*8 - 1, 170);
  UNSAFE_SMS_copySpritestoSAT();
  
  SMS_displayOn();
  unsigned int pressed;
  unsigned char position = 0;
  while (true) {
    SMS_waitForVBlank();
    pressed = SMS_getKeysStatus();
    if (pressed & PORT_A_KEY_UP) {
      if (position == 1) {
	select_sound();
	SMS_updateSpritePosition(arrow, 12*8, 10*8 - 1);
	UNSAFE_SMS_copySpritestoSAT();
	position = 0;
      }
    } else if (pressed & PORT_A_KEY_DOWN) {
      if (position == 0) {
	select_sound();
	SMS_updateSpritePosition(arrow, 12*8, 12*8 - 1);
	UNSAFE_SMS_copySpritestoSAT();
	position = 1;
      }
    } else if (pressed & GG_KEY_START) {
      hardmode = (bool)position;
      break;
    }
  }

  for (unsigned int i = 0; i < 7500; i++) continue; // debounce
  start_sound();
}

void clearVRAM() {
  // from Maxim's tutorial
__asm
	push af
	push bc
    
    ; 1. Set VRAM write address to 0 by outputting $4000 ORed with $0000
    ld a,#0x00
    out (0xbf),a
    ld a,#0x40
    out (0xbf),a
    ; 2. Output 16KB of zeroes
    ld bc,#0x4000    ; Counter for 16KB of VRAM
    
    loop:
        ld a,#0x00    ; Value to write
        out (0xbe),a ; Output to VRAM
        dec bc
        ld a,b
	or c
        jp nz,loop

     pop bc
     pop af
__endasm;
}

void main() {
  clearVRAM();
  erase_screen();
  // load the title screen, wait for START press
  title_screen();
  initialize_game();

  unsigned int pressed;

  // ship_noise();
  while (true) {
    SMS_waitForVBlank();
    
    if (hardmode) {
      if ((SMS_VDPFlags & 0b00100000) != 0) {
	// collision with the little green men
	game_over(false);
      }
      update_enemies(); // should definitely be every frame
    }
    pressed = SMS_getKeysStatus();
    // doing the tunnel first helps reduce, but not eliminate, tearing
    if (display_frames == max_display_frames) {
      update_tunnel();

      display_frames = 0;
    } else {
      display_frames++;
    }
    
    update_ship_position();

    if (pressed & PORT_A_KEY_2) {
      if (ship_y >= 24) {
	ship_y--;
      }
    } else if (pressed & GG_KEY_START) {
      // 7500 and 3500 are magic numbers that seem to work pretty
      // well.  if they change, this comment will be out of date, and
      // I'll have to adjust the loop in game_over() as well (who
      // knows if they work on real hardware...)

      // the reason they have to be here is that the processor is fast
      // enough that it will return to the loop, having detected a
      // button press, sooner than we want it too.

      // I'm not sure how to fix the pause issue. We could use
      // sprites, though it wouldn't be very effictive. Probably
      // something with the wrapping on the write column would be
      // useful, or we could just freeze the frame.
      // SMS_loadTileMapArea(write_column - 10, 13, pause_text, 5, 1);
      // I wonder if these work on real hardware
      pause_sound();
      for (unsigned int i = 0; i < 7500; i++) continue;

      start_button_loop();
      pause_unlock_sound();
      // SMS_loadTileMapArea(write_column - 10, 13, five_empty_tiles, 5, 1);
      for (unsigned int j = 0; j < 3500; j++) continue;
    } else {
      if (ship_y <= 144 - 8 - 1) {
	ship_y++;
      }
    }

    // making it move a lot faster actually makes it /easier/.
    switch (passed_tile_count) {
    case 50:
      max_height = 7;
      seed = get_register_r();
      break;
    case 90:
      max_height = 8;
      max_display_frames--;
      seed = get_register_r();
      break;
    case 120:
      max_height = 9;
      seed = get_register_r();
      break;
    }

    /* ************ COLLISION 

     * This is an array system, where each frame, we generate an array
     * of the points to check, then check them against the
     * stalagmites/stalactites. It seems to work pretty darn well.*/
     
    
    // This check pair works on the back sprites of the ship.

    
    // If you look at the tiles that build up the ship, using 16 seems
    // incredibly dumb. Somehow, it works, though, and I suspect the
    // issue is in the test in the for loop below.
    unsigned char
      ship_low_slant[4] = {ship_y + 1 + 24 - 2, // doesn't make sense, but works well
			   ship_y + 1 + 16 + 2, // does make sense, looks good
			   ship_y + 1 + 16 - 2,
			   // why is it 8 and not 16? I have no idea
			   // add 2 to offset the height of the tile
			   ship_y + 8 + 2};

    // we could use i=0--4, but that requires more arithmetic than i=7--11.
    for (unsigned char i = 7; i < 11; i++) {
      // for these, we are subtracting from 23 because (I think) there's
      // some compensation in the generation algorithm.
      // that said, 23 and no adjustments does seem to work.
      // has to be i+7 (not i+8) because we're working one column ahead in the scrolling.

      
      if (ship_low_slant[i - 7] >= (unsigned char)(23 - stalagmite_heights[i]) * 8) {
	// add 6 because column is absolute
	// subtract one to center (on both x and y)
	game_over(false);
      }
    }
    // we should only need to test this once, at the back of the ship
    // (as opposed to four times in the loop above)

    // Sometimes, 153 works, other times, 20*8 (160, the value that
    // makes sense) works. Sometimes it's even 20*8-1, which also
    // seems to be the most reliable. It could be down to inaccuracies
    // in Emulicious, though I would be surprised.
    if (ship_low_slant[0] == 20*8 - 1) game_over(false);

    unsigned int
      ship_high_slant[4] = {ship_y + 1,
			    ship_y + 1 + 5,
			    ship_y + 8 + 2,
			    ship_y + 8 + 6};

    for (unsigned char j = 7; j < 11; j++) {
      if (ship_high_slant[j - 7] <= (unsigned char)(stalactite_heights[j] * 8)) {
	  game_over(true);
      }
    }
    // not 3 because it shouldn't show all the way
    if (ship_high_slant[0] == 24) game_over(true); // 7 + 6, 2);
  }
}
