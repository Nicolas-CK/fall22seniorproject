#include <stdlib.h>
#include <string.h>

// include NESLIB header
#include "neslib.h"

// include CC65 NES Header (PPU)
#include <nes.h>

// BCD arithmetic support
#include "bcd.h"
//#link "bcd.c"

// VRAM update buffer
#include "vrambuf.h"
//#link "vrambuf.c"

//defining
#define FP_BITS 4


// CHR Data
//#resource "tileset.chr"
//#link "tileset.s"

// link title and level screen palette and RLE nametable
//#link "title_screen.s"
//#link "level.s"

// include nametables for title screen and level
#include "level_nam.h"
#include "title_nam.h"

// Palletes Data
const unsigned char palTitle[16]={ 0x0f,0x03,0x15,0x30,0x0f,0x01,0x21,0x31,0x0f,0x06,0x30,0x26,0x0f,0x09,0x19,0x29 };
const unsigned char palLevel[16]={ 0x0f,0x03,0x15,0x30,0x0f,0x01,0x21,0x31,0x0f,0x06,0x30,0x26,0x0f,0x09,0x19,0x29 };

//making characters
static unsigned char wait;
static unsigned char frame_cnt;
static int iy,dy;

///// METASPRITES

// Player MetaSprite
const unsigned char sprPlayer[]={
  0,-1,0x49,0,
  8,-1,0x4a,0,
  0, 7,0x4b,0,
  8, 7,0x4c,0,
  128
};

const unsigned char sprGhost[]={
  0,-1,0x49,0,
  8,-1,0x4a,0,
  0, 7,0x4b,0,
  8, 7,0x4c,0,
  128
};

/*{pal:"nes",layout:"nes"}*/
const char PALETTE[32] = { 
  0x03,			// screen color

  0x11,0x30,0x27,0x00,	// background palette 0
  0x1C,0x20,0x2C,0x00,	// background palette 1
  0x00,0x10,0x20,0x00,	// background palette 2
  0x06,0x16,0x26,0x00,   // background palette 3

  0x0F,0x12,0x20,0x00,	// sprite palette 0
  0x00,0x37,0x25,0x00,	// sprite palette 1
  0x0D,0x2D,0x3A,0x00,	// sprite palette 2
  0x0D,0x27,0x2A	// sprite palette 3
};

// setup PPU and tables
void setup_graphics() {
  // clear sprites
  oam_hide_rest(0);
  // set palette colors
  pal_all(PALETTE);
}

// Player x/y coordinates
byte actor_x = 128;
byte actor_y = 64;

byte ghost_x = 128;
byte ghost_y = 168;

// Player x/y velocity
sbyte actor_dx = 0;
sbyte actor_dy = 0;

sbyte ghost_dx = 0;
sbyte ghost_dy = 0;

// Function for moving player
void move_player() {
  char i;
  char pad;
  
  for(i = 0; i < 1; i++)
  {
  pad = pad_poll(i);
  // X Coordinates
  if (pad & PAD_LEFT && actor_x > 0) actor_dx = -2;
  else if (pad & PAD_RIGHT && actor_x<240) actor_dx = 2;
  else actor_dx = 0;
  
  // Y Coordinates
  if (pad&PAD_UP && actor_y>0) actor_dy=-2;
  else if (pad&PAD_DOWN && actor_y<212) actor_dy=2;
  else actor_dy=0;
  }
 
  // draw player
  oam_meta_spr(actor_x, actor_y, 0, sprPlayer);
  oam_meta_spr(ghost_x, ghost_y, 16, sprGhost);
  
  actor_x += actor_dx;
  actor_y += actor_dy;
  
  ghost_dx = (actor_x - ghost_x);
  ghost_dy = (actor_y - ghost_y);

  ghost_x += ghost_dx/20;
  ghost_y += ghost_dy/20;
}

// Draw sprites
void draw_sprites() {
  oam_clear();
  oam_meta_spr(128, 64, 0, sprPlayer);
  oam_meta_spr(128, 168, 16, sprGhost);
}


// Function to fade out screen
void fade_out() {
  byte vb;
  for (vb=4; vb!=0; vb--) {
    // Set brightness value
    pal_bright(vb);
    // wait for 4/60 sec
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
  }
 
  pal_bright(0);
  set_vram_update(NULL);
}

// Function to fade in screen
void fade_in() {
  byte vb;
  for(vb=0; vb<=4; vb++) {
    // Set brighness value
    pal_bright(vb);
    // wait for 4/60 sec
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
  }
}

// Shows title screen
void show_title() {
  // Disable Rendering
  ppu_off();

  // Set pallete to our title screen pallete
  pal_bg(palTitle);
  
  // Unpack nametable into VRAM
  vram_adr(NAMETABLE_A);
  vram_unrle(title_nam);
  
  // Enable Rendering
  ppu_on_all();
  wait = 0;
  frame_cnt=0;
  iy=220<<FP_BITS;
  
  while(1) 
  {
    ppu_wait_frame();
    
    if (pad_trigger(0)&PAD_START) break;
    
    iy+=dy;
    
    if(iy<0)
    {
      iy = 0;
      dy=-dy>>1;
    }
    
    if(dy>(-8<<FP_BITS)) dy-=2;
    
    if(wait)
    {
      --wait;
    }
    else
    {
      pal_col(10,(frame_cnt&32)?0x0f:0x20);
      ++frame_cnt;
    }
  }
  
  fade_out();
}


// Shows level screen
void show_level() {
  // Disable Rendering
  ppu_off();

  // Set pallete to our level screen pallete
  pal_bg(palLevel);
  
  // Unpack nametable into VRAM
  vram_adr(NAMETABLE_A);
  vram_unrle(level_nam);
  
  // Enable Rendering
  ppu_on_all();
  
  fade_in();
}

void game_loop()
{
  // Show level
  show_level();
  
  // Draw Sprites
  draw_sprites();
  
  
  while(1) {    
    // Function for determining player movement
    move_player();
    
    // Wait for the next frame
    ppu_wait_frame();
  }
}

void main(void)
{ 
  setup_graphics();
  
  // infinite loop
  while(1) {
    
    // Show Title
    show_title();
    
    // Play Game
    game_loop();
  }
}
