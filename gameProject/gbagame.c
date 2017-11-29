/*
GBA game that mimics old JRPG, based off of Final Fantasy series.
*/

/* First map and tile of the game. */
#include "map1.h"
#include "map1tile.h"
#include "map1boundary.h"
/* Second map and tile of the game. */
#include "map2.h"
#include "map2tile.h"
#include "map2boundary.h"

/* Sprites for the game. */
#include "sprites.h"

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

/* Flags for sprite handling in display control register. */
#define SPRITE_MAP_2D 0x0
#define SPRITE_MAP_1D 0x40
#define SPRITE_ENABLE 0x1000

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

/* Sprite size. */
#define NUM_SPRITES 128

/* A sprite struct containing attributes needed to draw the sprite on screen. */
struct Sprite
{
	unsigned short attribute0;
	unsigned short attribute1;
	unsigned short attribute2;
	unsigned short attribute3;
};

/* An enum type containing the different sizes of sprites for the game. */
enum SpriteSize
{
	SIZE_16_16,
	SIZE_16_32,
	SIZE_64_64,
};

/* An array of all sprites to be used for the game. */
struct Sprite sprites[NUM_SPRITES];
int next_sprite_index = 0;

/* Display control pointer points to the GBA graphics register. */
volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;

/* Address of the color palette. */
volatile unsigned short* bg_palette = (volatile unsigned short*) 0x5000000;
volatile unsigned short* sprite_palette = (volatile unsigned short*) 0x5000200;

/* Address of the sprite image data. */
volatile unsigned short* sprite_image_memory = (volatile unsigned short*) 0x6010000;

/* Address of the sprite attributes. */
volatile unsigned short* sprite_attribute_memory = (volatile unsigned short*) 0x7000000;

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

/* This map is drawn on bg1 for visible game map. */
void setup_background(const unsigned char* map_data, const unsigned short* map_palette, unsigned short map_width, unsigned short map_height, const unsigned short* tile, unsigned short tile_width, unsigned short tile_height)
{
	memcpy16((unsigned short*) bg_palette, (unsigned short*) map_palette, PALETTE_SIZE);
	volatile unsigned short* dest = char_block(0);
	memcpy16((unsigned short*) dest, (const unsigned short*) map_data, (map_width * map_height) / 2);
	dest = screen_block(16);
	memcpy16((unsigned short*) dest, (const unsigned short*) tile, (tile_width * tile_height));
	*bg1_control = 1 | (0 << 2) | (0 << 6) | (1 << 7) | (17 << 8) | (1 << 13) | (0 << 14);
}

/* This map is drawn on bg0 for playable boundaries. */
void setup_boundary(const unsigned short* tile, unsigned short tile_width, unsigned short tile_height)
{
	volatile unsigned short* dest = screen_block(17);
	memcpy16((unsigned short*) dest, (const unsigned short*) tile, (tile_width * tile_height));
	*bg0_control = 0 | (0 << 2) | (0 << 6) | (1 << 7) | (16 << 8) | (1 << 13) | (0 << 14);
}

/*
Looks up what kind of tile it is and return a number that will determine what action to perform:
	0 - Normal movement.
	1 - boundary tile, stop character movement.
	2 - Interaction tile, perform specific action.
	3 - New map tile, load new map into screen.
 */
unsigned short tile_interact(int x_coord, int y_coord, int xscroll, int yscroll, const unsigned short* boundary, int boundary_width, int boundary_height, unsigned short new_map_tile, unsigned short interaction_tile)
{
	x_coord += xscroll;
	y_coord += yscroll;
	x_coord >>= 3;
	y_coord >>= 3;
	
	while (x_coord >= boundary_width) x_coord -= boundary_width;
	while (y_coord >= boundary_height) y_coord -= boundary_height;
	while (x_coord < 0) x_coord += boundary_width;
	while (y_coord < 0) y_coord += boundary_height;
	int index = y_coord * boundary_width + x_coord;
	
	switch (boundary[index])
	{
		case new_map_tile:
			return 3;
			break;
		case interaction_tile:
			return 2;
			break;
		case 149:
			return 1;
			break;
		default:
			return 0;
			break;
	}
}

/* Setup the sprite image and palette. */
void setup_sprite_image()
{
	memcpy16((unsigned short*) sprite_palette, (unsigned short*) sprites_palette, PALETTE_SIZE);
	memcpy16((unsigned short*) sprite_image_memory, (unsigned short*) sprites_data, (sprites_width * sprites_height) / 2);
}

/* Initialize a sprite with necessary properties and return a pointer. */
struct Sprite* sprite_init(int x_coord, int y_coord, enum SpriteSize dimension, int tile_index, int priority)
{
	int index = next_sprite_index++;
	int size, shape;
	switch (dimension)
	{
		case SIZE_16_16:
			size = 1;
			shape = 0;
			break;
		case SIZE_16_32:
			size = 2;
			shape = 2;
			break;
		case SIZE_64_64:
			size = 3;
			shape = 0;
			break;
	}
	sprites[index].attribute0 = y_coord | (0 << 8) | (0 << 10) | (0 << 12) | (1 << 13) | (shape << 14);
	sprites[index].attribute1 = x_coord | (0 << 9) | (0 << 12) | (0 << 13) | (size << 14);
	sprites[index].attribute2 = tile_index | (priority << 10) | (0 << 12);
	return &sprites[index];
}

/* Set a sprite's position. */
void sprite_position(struct Sprite* sprite, int x_coord, int y_coord)
{
	sprite->attribute0 &= 0xff00;
	sprite->attribute0 |= (y_coord & 0xff);
	sprite->attribute1 &= 0xfe00;
	sprite->attribute1 |= (x_coord & 0x1ff);
}

/* Moves a sprite in a direction. */
void sprite_move(struct Sprite* sprite, int dx, int dy)
{
	int y = sprite->attribute0 & 0xff;
	int x = sprite->attribute1 & 0x1ff;
	sprite_position(sprite, x + dx, y + dy);
}

/* Changes the tile offset of a sprite. */
void sprite_set_offset(struct Sprite* sprite, int offset)
{
	sprite->attribute2 &= 0xfc00;
	sprite->attribute2 |= (offset & 0x03ff);
}

/* This updates all of the sprites on the screen. */
void sprite_update_all()
{
	memcpy16((unsigned short*) sprite_attribute_memory, (unsigned short*) sprites, NUM_SPRITES * 4);
}

/* This clears all sprite from visible map and moves them offscreen. */
void sprite_clear()
{
	next_sprite_index = 0;
	for (int i = 0; i < NUM_SPRITES; i++)
	{
		sprites[i].attribute0 = HEIGHT;
		sprites[i].attribute1 = WIDTH;
	}
}

/* A struct for each character's logic and behavior. */
struct Character
{
	struct Sprite* sprite;
	int x, y, frame, animation_delay, counter, move, direction, border;
};

/* Initializes a character. */
void character_init(struct Character* character, int x_coord, int y_coord, enum SpriteSize size)
{
	character->x = x_coord;
	character->y = y_coord;
	character->border = 40;
	character->frame = 0;
	character->move = 0;
	switch (character->direction)
	{
		case 0:
			character->frame = 0;
			break;
		case 1:
			character->frame = 32;
			break;
		case 2:
			character->frame = 16;
			break;
		case 3:
			character->frame = 48;
			break;
	}
	character->counter = 4;
	character->animation_delay = 5;
	character->sprite = sprite_init(x_coord, y_coord, size, 0, 0);
}

/* Moves the character left and if at the border limit, move map instead. */
int character_left(struct Character* character)
{
	if (character->move == 0)
	{
		character->frame = 32;
	}
	character->move = 1;
	character->direction = 1;
	if (character->x < character->border)
	{
		return 1;
	}
	else
	{
		character->x--;
		return 0;
	}
}

/* Moves the character right and if at the border limit, move map instead. */
int character_right(struct Character* character)
{
	if (character->move == 0)
	{
		character->frame = 48;
	}
	character->move = 1;
	character->direction = 3;
	if (character->x > (WIDTH - character->border))
	{
		return 1;
	}
	else
	{
		character->x++;
		return 0;
	}
}

/* Moves the character up and if at the border limit, move map instead. */
int character_up(struct Character* character)
{
	if (character->move == 0)
	{
		character->frame = 16;
	}
	character->move = 1;
	character->direction = 2;
	if (character->y < character->border)
	{
		return 1;
	}
	else
	{
		character->y--;
		return 0;
	}
}

/* Moves the character down and if a t the border limit, move map instead. */
int character_down(struct Character* character)
{
	if (character->move == 0)
	{
		character->frame = 0;
	}
	character->move = 1;
	character->direction = 0;
	if (character->y > (HEIGHT - character->border))
	{
		return 1;
	}
	else
	{
		character->y++;
		return 0;
	}
}

/* If no input detected, stops character sprite animation, and reset frame back to 0. */
void character_stop(struct Character* character)
{
	character->move = 0;
	character->counter = 4;
	switch (character->direction)
	{
		case 0:
			character->frame = 0;
			break;
		case 1:
			character->frame = 32;
			break;
		case 2:
			character->frame = 16;
			break;
		case 3:
			character->frame = 48;
			break;
	}
	sprite_set_offset(character->sprite, character->frame);
}

/* Updates the character sprite. */
void character_update(struct Character* character)
{
	if (character->move)
	{
		character->counter++;
		if (character->counter >= character->animation_delay)
		{
			switch (character->direction)
			{
				case 0:		
					character->frame += 8;
					if (character->frame > 8)
					{
						character->frame = 0;
					}
					break;
				case 1:
					character->frame += 8;
					if (character->frame > 40)
					{
						character->frame = 32;
					}
					break;
				case 2:
					character->frame += 8;
					if (character->frame > 24)
					{
						character->frame = 16;
					}
					break;
				case 3:
					character->frame += 8;
					if (character->frame > 56)
					{
						character->frame = 48;
					}
					break;
			}
			sprite_set_offset(character->sprite, character->frame);
			character->counter = 0;
		}
	}
	sprite_position(character->sprite, character->x, character->y);
}

/* Wait function. */
void delay(unsigned int amount)
{
	for (int i = 0; i < amount * 10; i++);
}

/* Main function. */
int main()
{
	*display_control = MODE0 | BG0_ENABLE | BG1_ENABLE | SPRITE_ENABLE | SPRITE_MAP_1D;
	setup_background(map1_data, map1_palette, map1_width, map1_height, map1tile, map1tile_width, map1tile_height);
	setup_boundary(map1boundary, map1boundary_width, map1boundary_height);
	setup_sprite_image();
	sprite_clear();

	struct Character cainWorld;
	character_init(&cainWorld, 120, 80, SIZE_16_16);

	int xscroll = 0;
	int yscroll = 0;
	unsigned short tileCheck;
	unsigned short mapTracker = 1;
	
	while (1)
	{
		if (mapTracker == 1 || mapTracker == 2)
		{
			character_update(&cainWorld);
			if (button_pressed(BUTTON_DOWN))
			{
				if (character_down(&cainWorld))
				{
					// yscroll++;
					if (mapTracker == 1)
					{
						tileCheck = tile_interact((cainWorld->x >> 8) + 8, (cainWorld->y >> 8) + 32, xscroll, yscroll, map1boundary, map1boundary_width, map1boundary_height, 34, 999);
					}
					else if (mapTracker == 2)
					{
						// Fix new map tile and map interact tile to link to map1 and for fight map.
						tileCheck = tile_interact((cainWorld->x >> 8) + 8, (cainWorld->y >> 8) + 32, xscroll, yscroll, map2boundary, map2boundary_width, map2boundary_height, 34, 999);
					}
					switch (tileCheck)
					{
						case 0:
							yscroll++;
							break;
						case 1:
							character_stop(&cainWorld);
							break;
						case 2:
							// TO DO.
							break;
						case 3:
							if (mapTracker == 1)
							{
								setup_background(map2_data, map2_palette, map2_width, map2_height, map2tile, map2tile_width, map2tile_height);
								setup_boundary(map2boundary, map2boundary_width, map2boundary_height);
								mapTracker = 2;
							}
							else if (mapTracker == 2)
							{
								setup_background(map1_data, map1_palette, map1_width, map1_height, map1tile, map1tile_width, map1tile_height);
								setup_boundary(map1boundary, map1boundary_width, map1boundary_height);
								mapTracker = 1;
							}
							sprite_clear();
							cainWorld->x = 120;
							cainWorld->y = 80;
							break;
					}
				}
			}
			else if (button_pressed(BUTTON_UP))
			{
				if (character_up(&cainWorld))
				{
					// yscroll--;
					if (mapTracker == 1)
					{
						tileCheck = tile_interact((cainWorld->x >> 8) + 8, (cainWorld->y >> 8) + 32, xscroll, yscroll, map1boundary, map1boundary_width, map1boundary_height, 34, 999);
					}
					else if (mapTracker == 2)
					{
						// Fix new map tile and map interact tile to link to map1 and for fight map.
						tileCheck = tile_interact((cainWorld->x >> 8) + 8, (cainWorld->y >> 8) + 32, xscroll, yscroll, map2boundary, map2boundary_width, map2boundary_height, 34, 999);
					}
					switch (tileCheck)
					{
						case 0:
							yscroll--;
							break;
						case 1:
							character_stop(&cainWorld);
							break;
						case 2:
							// TO DO.
							break;
						case 3:
							if (mapTracker == 1)
							{
								setup_background(map2_data, map2_palette, map2_width, map2_height, map2tile, map2tile_width, map2tile_height);
								setup_boundary(map2boundary, map2boundary_width, map2boundary_height);
								mapTracker = 2;
							}
							else if (mapTracker == 2)
							{
								setup_background(map1_data, map1_palette, map1_width, map1_height, map1tile, map1tile_width, map1tile_height);
								setup_boundary(map1boundary, map1boundary_width, map1boundary_height);
								mapTracker = 1;
							}
							sprite_clear();
							cainWorld->x = 120;
							cainWorld->y = 80;
							break;
					}
				}
			}
			else if (button_pressed(BUTTON_RIGHT))
			{
				if (character_right(&cainWorld))
				{
					// xscroll++;
					if (mapTracker == 1)
					{
						tileCheck = tile_interact((cainWorld->x >> 8) + 8, (cainWorld->y >> 8) + 32, xscroll, yscroll, map1boundary, map1boundary_width, map1boundary_height, 34, 999);
					}
					else if (mapTracker == 2)
					{
						// Fix new map tile and map interact tile to link to map1 and for fight map.
						tileCheck = tile_interact((cainWorld->x >> 8) + 8, (cainWorld->y >> 8) + 32, xscroll, yscroll, map2boundary, map2boundary_width, map2boundary_height, 34, 999);
					}
					switch (tileCheck)
					{
						case 0:
							xscroll++;
							break;
						case 1:
							character_stop(&cainWorld);
							break;
						case 2:
							// TO DO.
							break;
						case 3:
							if (mapTracker == 1)
							{
								setup_background(map2_data, map2_palette, map2_width, map2_height, map2tile, map2tile_width, map2tile_height);
								setup_boundary(map2boundary, map2boundary_width, map2boundary_height);
								mapTracker = 2;
							}
							else if (mapTracker == 2)
							{
								setup_background(map1_data, map1_palette, map1_width, map1_height, map1tile, map1tile_width, map1tile_height);
								setup_boundary(map1boundary, map1boundary_width, map1boundary_height);
								mapTracker = 1;
							}
							sprite_clear();
							cainWorld->x = 120;
							cainWorld->y = 80;
							break;
					}
				}
			}
			else if (button_pressed(BUTTON_LEFT))
			{
				if (character_left(&cainWorld))
				{
					// xscroll--;
					if (mapTracker == 1)
					{
						tileCheck = tile_interact((cainWorld->x >> 8) + 8, (cainWorld->y >> 8) + 32, xscroll, yscroll, map1boundary, map1boundary_width, map1boundary_height, 34, 999);
					}
					else if (mapTracker == 2)
					{
						// Fix new map tile and map interact tile to link to map1 and for fight map.
						tileCheck = tile_interact((cainWorld->x >> 8) + 8, (cainWorld->y >> 8) + 32, xscroll, yscroll, map2boundary, map2boundary_width, map2boundary_height, 34, 999);
					}
					switch (tileCheck)
					{
						case 0:
							xscroll--;
							break;
						case 1:
							character_stop(&cainWorld);
							break;
						case 2:
							// TO DO.
							break;
						case 3:
							if (mapTracker == 1)
							{
								setup_background(map2_data, map2_palette, map2_width, map2_height, map2tile, map2tile_width, map2tile_height);
								setup_boundary(map2boundary, map2boundary_width, map2boundary_height);
								mapTracker = 2;
							}
							else if (mapTracker == 2)
							{
								setup_background(map1_data, map1_palette, map1_width, map1_height, map1tile, map1tile_width, map1tile_height);
								setup_boundary(map1boundary, map1boundary_width, map1boundary_height);
								mapTracker = 1;
							}
							sprite_clear();
							cainWorld->x = 120;
							cainWorld->y = 80;
							break;
					}
				}
			}
			else
			{
				character_stop(&cainWorld);
			}
		}
		else if (mapTracker == 3)
		{
			// TO DO fight map logic.
		}
		wait_vblank();
		*bg0_x_scroll = xscroll;
		*bg0_y_scroll = yscroll;
		sprite_update_all();
		delay(500);
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

/*
TO DO:
~ Put all .h into structs so it can be passed as an argument into functions to make things simpler.
~ Modularize code into multiple files to make it cleaner.
~ Work on map1. (done)
~ Work on map1 boundary. (done)
~ Work on map2.
~ Work on map2 boundary.
~ Work on sprites. (done)
~ Sprite movement. (done)
~ Sprite direction. (done)
~ Sprite collision. (done)
~ Battle map.
~ Fight menu.
~ Convert 3 functions into assembly.
~ Smooth screen (map) transition.
*/
