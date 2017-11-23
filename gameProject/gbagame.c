/*
GBA game that mimics old JRPG, based off of Final Fantasy series.
*/

/* First map and tile of the game. */
#include "map1.h"
#include "map1tile.h"
/* Second map and tile of the game. */
//#include "second.h"

/* The size of the GBA screen. */
#define WIDTH 240
#define HEIGHT 160

/* DMA flags. */
#define DMA_ENABLE 0x80000000
#define DMA_16 0x00000000
#define DMA_32 0x04000000

/* DMA registers. */
volatile unsigned int* dma_source = (volatile unsigned int*) 0x40000D4;
volatile unsigned int* dma_destination = (volatile unsigned int*) 0x40000D8;
volatile unsigned int* dma_count = (volatile unsigned int*) 0x40000DC;


/* The three tile modes. */
#define MODE0 0x00
#define MODE1 0x01
#define MODE2 0x02

/* Bits for the four tile layers. */
#define BG0_ENABLE 0x100
#define BG1_ENABLE 0x200
#define BG2_ENABLE 0x400
#define BG3_ENABLE 0x800

/* Control registers for the four tile layers. */
volatile unsigned short* bg0_control = (volatile unsigned short*) 0x4000008;
volatile unsigned short* bg1_control = (volatile unsigned short*) 0x400000a;
volatile unsigned short* bg2_control = (volatile unsigned short*) 0x400000c;
volatile unsigned short* bg3_control = (volatile unsigned short*) 0x400000e;

/* Pallete size. */
#define PALETTE_SIZE 256

/* Display control pointer points to the GBA graphics register. */
volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;

/* Address of the color palette. */
volatile unsigned short* bg_palette = (volatile unsigned short*) 0x5000000;
volatile unsigned short* sprite_palette = (volatile unsigned short*) 0x5000200;

/* Address of the sprite image data. */
volatile unsigned short* sprite_image_memory = (volatile unsigned short*) 0x6010000;

/* Button register. */
volatile unsigned short* buttons = (volatile unsigned short*) 0x04000130;

/* Scrolling background registers. */
volatile short* bg0_x_scroll = (unsigned short*) 0x4000010;
volatile short* bg0_y_scroll = (unsigned short*) 0x4000012;
volatile short* bg1_x_scroll = (unsigned short*) 0x4000014;
volatile short* bg1_y_scroll = (unsigned short*) 0x4000016;
volatile short* bg2_x_scroll = (unsigned short*) 0x4000018;
volatile short* bg2_y_scroll = (unsigned short*) 0x400001a;
volatile short* bg3_x_scroll = (unsigned short*) 0x400001c;
volatile short* bg3_y_scroll = (unsigned short*) 0x400001e;

/* Selects which button based on bit position. */
#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define Button_L (1 << 9)

/* Scanline counter updates how much of the screen has been drawn. */
volatile unsigned short* scanline_counter = (volatile unsigned short*) 0x4000006;

/* This waits for the vblank cycle. */
void wait_vblank()
{
	while (*scanline_counter < 160)
	{
		/* Do nothing. */
	}
}

/* This checks if a button has been pressed and return a true(1) or false(0). */
unsigned char button_pressed(unsigned short button)
{
	unsigned short pressed = *buttons & button;
	return (pressed == 0) ? 1 : 0;
}

/* This returns a pointer to one of the 4 character blocks (0-3). */
volatile unsigned short* char_block(unsigned long block)
{
	return (volatile unsigned short*) (0x6000000 + (block * 0x4000));
}

/* This returns a pointer to one of the 32 screen blocks (0-31). */
volatile unsigned short* screen_block(unsigned long block)
{
	return (volatile unsigned short*) (0x6000000 + (block * 0x800));
}

/* DMA copy. */
void memcpy16(unsigned short* dest, const unsigned short* source, int amount)
{
	*dma_source = (unsigned int) source;
	*dma_destination = (unsigned int) dest;
	*dma_count = amount | DMA_16 |DMA_ENABLE;
}

/* MODULARIZE setup_background() SO IT CAN BE USED FOR MULTIPLE MAPS. (not finished) */
void setup_background(const unsigned char* map_data, const unsigned short* map_palette, unsigned short map_width, unsigned short map_height, const unsigned short* tile, unsigned short tile_width, unsigned short tile_height)
{
	/*
	int i;
	for (i = 0; i < PALETTE_SIZE; i++)
	{
		bg_palette[i] = map_palette[i];
	}
	volatile unsigned short* dest = char_block(0);
	unsigned short* image = (unsigned short*) map_data;
	for (i = 0; i < ((map_width * map_height) / 2); i++)
	{
		dest[i] = image[i];
	}
	*bg0_control = 0 | (0 << 2) | (0 << 6) | (1 << 7) | (16 << 8) | (1 << 13) | (0 << 14);
	dest = screen_block(16);
	for (i = 0; i < (tile_width * tile_height); i++)
	{
		dest[i] = tile[i];
	}
	*/
	memcpy16((unsigned short*) bg_palette, (unsigned short*) map_palette, PALETTE_SIZE);
	volatile unsigned short* dest = char_block(0);
	memcpy16((unsigned short*) dest, (const unsigned short*) map_data, (map_width * map_height) / 2);
	dest = screen_block(16);
	memcpy16((unsigned short*) dest, (const unsigned short*) tile, (tile_width * tile_height));
	*bg0_control = 0 | (0 << 2) | (0 << 6) | (1 << 7) | (16 << 8) | (1 << 13) | (0 << 14);
}

/* This map is drawn on bg1 for playable boundaries. */
void setup_boundary(const unsigned short* tile, unsigned short tile_width, unsigned short tile_height)
{
	int i;
	unsigned short* dest = screen_block(17);
	for (i = 0; i < (tile_width * tile_height); i++)
	{
		dest[i] = tile[i];
	}
	*bg1_control = 1 | (0 << 2) | (0 << 6) | (1 << 7) | (17 << 8) | (1 << 13) | (0 << 14);
}

/* Wait function. */
void delay(unsigned int amount)
{
	for (int i = 0; i < amount * 10; i++);
}

/* Main function. */
int main()
{
	*display_control = MODE0 | BG0_ENABLE | BG1_ENABLE;
	setup_background(map1_data, map1_palette, map1_width, map1_height, map1tile, map1tile_width, map1tile_height);
	setup_boundary(map1boundary, map1boundary_width, map1boundary_height);
	
	int xscroll = 0;
	int yscroll = 0;
	
	while (1)
	{
		if (button_pressed(BUTTON_DOWN))
		{
			yscroll++;
		}
		else if (button_pressed(BUTTON_UP))
		{
			yscroll--;
		}
		else if (button_pressed(BUTTON_RIGHT))
		{
			xscroll++;
		}
		else if (button_pressed(BUTTON_LEFT))
		{
			xscroll--;
		}
		
		wait_vblank();
		*bg0_x_scroll = xscroll;
		*bg0_y_scroll = yscroll;
		delay(50);
	}
}

void interrupt_ignore() 
{
    /* do nothing */
}


/* this table specifies which interrupts we handle which way
 * for now, we ignore all of them */
typedef void (*intrp)();
const intrp IntrTable[13] = {
    interrupt_ignore,   /* V Blank interrupt */
    interrupt_ignore,   /* H Blank interrupt */
    interrupt_ignore,   /* V Counter interrupt */
    interrupt_ignore,   /* Timer 0 interrupt */
    interrupt_ignore,   /* Timer 1 interrupt */
    interrupt_ignore,   /* Timer 2 interrupt */
    interrupt_ignore,   /* Timer 3 interrupt */
    interrupt_ignore,   /* Serial communication interrupt */
    interrupt_ignore,   /* DMA 0 interrupt */
    interrupt_ignore,   /* DMA 1 interrupt */
    interrupt_ignore,   /* DMA 2 interrupt */
    interrupt_ignore,   /* DMA 3 interrupt */
    interrupt_ignore,   /* Key interrupt */
};
