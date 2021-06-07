
///////////////////////////////////////////////////////////////////////////////
// Headers.

#include <stdint.h>
#include "system.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "sprites_idx4.h"

///////////////////////////////////////////////////////////////////////////////
// HW stuff.

#define WAIT_UNITL_0(x) while(x != 0){}
#define WAIT_UNITL_1(x) while(x != 1){}

#define SCREEN_IDX1_W 640
#define SCREEN_IDX1_H 480
#define SCREEN_IDX4_W 320
#define SCREEN_IDX4_H 240
#define SCREEN_RGB333_W 160
#define SCREEN_RGB333_H 120

#define SCREEN_IDX4_W8 (SCREEN_IDX4_W/8)

#define gpu_p32 ((volatile uint32_t*)LPRS2_GPU_BASE)
#define palette_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x1000))
#define unpack_idx1_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x400000))
#define pack_idx1_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x600000))
#define unpack_idx4_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x800000))
#define pack_idx4_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0xa00000))
#define unpack_rgb333_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0xc00000))
#define joypad_p32 ((volatile uint32_t*)LPRS2_JOYPAD_BASE)

typedef struct {
	unsigned a : 1;
	unsigned b : 1;
	unsigned z : 1;
	unsigned start : 1;
	unsigned up : 1;
	unsigned down : 1;
	unsigned left : 1;
	unsigned right : 1;
} bf_joypad;
#define joypad (*((volatile bf_joypad*)LPRS2_JOYPAD_BASE))

typedef struct {
	uint32_t m[SCREEN_IDX1_H][SCREEN_IDX1_W];
} bf_unpack_idx1;
#define unpack_idx1 (*((volatile bf_unpack_idx1*)unpack_idx1_p32))



///////////////////////////////////////////////////////////////////////////////
// Game config.

#define STEP 20



///////////////////////////////////////////////////////////////////////////////
// Game data structures.

typedef struct {
	uint16_t x;
	uint16_t y;
} point_t;

typedef struct {
	point_t pos;
} crosshair_t;

typedef struct {
	point_t pos;
	int visible;
} ships_t;

typedef struct {
	uint8_t grid;
	crosshair_t crosshair;
	ships_t ships;
} game_state_t;


uint32_t* background[40] = {
	grid__p,
};
uint32_t* crosshair[20] = {
	crosshair__p
};
uint32_t* ships[20] = {
	ships__p
};

static inline uint32_t shift_div_with_round_down(uint32_t num, uint32_t shift) {
	uint32_t d = num >> shift;
	return d;
}

static inline uint32_t shift_div_with_round_up(uint32_t num, uint32_t shift) {
	uint32_t d = num >> shift;
	uint32_t mask = (1 << shift) - 1;
	if ((num & mask) != 0) {
		d++;
	}
	return d;
}



static void draw_sprite(
	uint32_t* src_p,
	uint16_t src_w,
	uint16_t src_h,
	uint16_t dst_x,
	uint16_t dst_y
) {


	uint16_t dst_x8 = shift_div_with_round_down(dst_x, 3);
	uint16_t src_w8 = shift_div_with_round_up(src_w, 3);



	for (uint16_t y = 0; y < src_h; y++) {
		for (uint16_t x8 = 0; x8 < src_w8; x8++) {
			uint32_t src_idx = y * src_w8 + x8;
			uint32_t pixels = src_p[src_idx];
			uint32_t dst_idx =
				(dst_y + y) * SCREEN_IDX4_W8 +
				(dst_x8 + x8);
			pack_idx4_p32[dst_idx] = pixels;
		}
	}
}




///////////////////////////////////////////////////////////////////////////////
// Game code.

int main(void) {

	// Setup.
	gpu_p32[0] = 2; // Color bar.
	gpu_p32[1] = 1;
	gpu_p32[0x800] = 0x00ffffff; // Magenta for HUD.

	// Copy palette.
	for (uint8_t i = 0; i < 16; i++) {
		palette_p32[i] = palette[i];
	}


	// Game state.
	game_state_t gs;
	gs.crosshair.pos.x = 0;
	gs.crosshair.pos.y = 0;
	gs.ships.pos.x = 0;
	gs.ships.pos.y = 0;
	int visible1 = 0;
	int visible2 = 0;
	int visible3 = 0;
	int visible4 = 0;
	int visible5 = 0;
	

	memset(&gs, 0, sizeof(gs));

	while (1) {


		/////////////////////////////////////
		// Poll controls.
		int mov_x = 0;
		int mov_y = 0;
		if (joypad.right) {
			mov_x = +1;
		}
		if (joypad.down) {
			mov_y = +1;
		}
		if (joypad.left) {
			mov_x = -1;
		}
		if (joypad.up) {
			mov_y = -1;
		}

		/////////////////////////////////////
		// Gameplay.

		gs.crosshair.pos.x += mov_x * STEP;
		gs.crosshair.pos.y += mov_y * STEP;


		/////////////////////////////////////
		// Drawing.


		// Detecting rising edge of VSync.
		WAIT_UNITL_0(gpu_p32[2]);
		WAIT_UNITL_1(gpu_p32[2]);
		// Draw in buffer while it is in VSync.



		// Black background.
		for (uint16_t r1 = 0; r1 < SCREEN_IDX4_H; r1++) {
			for (uint16_t c8 = 0; c8 < SCREEN_IDX4_W8; c8++) {
				pack_idx4_p32[r1 * SCREEN_IDX4_W8 + c8] = 0x00000000;
			}
		}

		int k = 0;
		int l = 0;
		for (int i = 0; i < 5; i++)
		{
			k = 0;
			for (int j = 0; j < 5; j++)
			{
				draw_sprite(
					background[gs.grid], 40, 40, l, k + 20
				);
				k += 40;
			}
			l += 40;
		}

		draw_sprite(
			crosshair__p, 20, 20, gs.crosshair.pos.x, gs.crosshair.pos.y+20
		);


		if (gs.crosshair.pos.x == 40 && gs.crosshair.pos.y == 20 && joypad.a || visible1 == 1)
		{
			visible1 = 1;
			draw_sprite(
				ships__p, 20, 20, 40, 40
			);
		}

		if (gs.crosshair.pos.x == 60 && gs.crosshair.pos.y == 40 && joypad.a || visible2 == 1)
		{
			visible2 = 1;
			draw_sprite(
				ships__p, 20, 20, 60, 60
			);
		}

		if (gs.crosshair.pos.x == 80 && gs.crosshair.pos.y == 60 && joypad.a || visible3 == 1)
		{
			visible3 = 1;
			draw_sprite(
				ships__p, 20, 20, 80, 80
			);
		}

		if (gs.crosshair.pos.x == 100 && gs.crosshair.pos.y == 80 && joypad.a || visible4 == 1)
		{
			visible4 = 1;
			draw_sprite(
				ships__p, 20, 20, 100, 100
			);
		}

		if (gs.crosshair.pos.x == 120 && gs.crosshair.pos.y == 100 && joypad.a || visible5 == 1)
		{
			visible5 = 1;
			draw_sprite(
				ships__p, 20, 20, 120, 120
			);
		}

	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
