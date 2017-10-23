/**
 * This is the classic Pong arcade game written for the Game Boy Advance in c.
 **/

#include <stdlib.h>
/* the width and height of the screen */
#define WIDTH 240
#define HEIGHT 160

/* these identifiers define different bit positions of the display control */
#define MODE4 0x0004
#define BG2 0x0400

/* this bit indicates whether to display the front or the back buffer
 * this allows us to refer to bit 4 of the display_control register */
#define SHOW_BACK 0x10;

/* the screen is simply a pointer into memory at a specific address this
 *  * pointer points to 16-bit colors of which there are 240x160 */
volatile unsigned short* screen = (volatile unsigned short*) 0x6000000;

/* the display control pointer points to the gba graphics register */
volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;

/* the address of the color palette used in graphics mode 4 */
volatile unsigned short* palette = (volatile unsigned short*) 0x5000000;

/* pointers to the front and back buffers - the front buffer is the start
 * of the screen array and the back buffer is a pointer to the second half */
volatile unsigned short* front_buffer = (volatile unsigned short*) 0x6000000;
volatile unsigned short* back_buffer = (volatile unsigned short*)  0x600A000;

/* the button register holds the bits which indicate whether each button has
 * been pressed - this has got to be volatile as well
 */
volatile unsigned short* buttons = (volatile unsigned short*) 0x04000130;

/* the bit positions indicate each button - the first bit is for A, second for
 * B, and so on, each constant below can be ANDED into the register to get the
 * status of any one button */
#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define BUTTON_L (1 << 9)

/* the scanline counter is a memory cell which is updated to indicate how
 * much of the screen has been drawn */
volatile unsigned short* scanline_counter = (volatile unsigned short*) 0x4000006;

/* wait for the screen to be fully drawn so we can do something during vblank */
void wait_vblank() {
    /* wait until all 160 lines have been updated */
    while (*scanline_counter < 160) { }
}

/* this function checks whether a particular button has been pressed */
unsigned char button_pressed(unsigned short button) {
    /* and the button register with the button constant we want */
    unsigned short pressed = *buttons & button;

    /* if this value is zero, then it's not pressed */
    if (pressed == 0) {
        return 1;
    } else {
        return 0;
    }
}

/* keep track of the next palette index */
int next_palette_index = 0;

/*
 * function which adds a color to the palette and returns the
 * index to it
 */
unsigned char add_color(unsigned char r, unsigned char g, unsigned char b) {
    unsigned short color = b << 10;
    color += g << 5;
    color += r;

    /* add the color to the palette */
    palette[next_palette_index] = color;

    /* increment the index */
    next_palette_index++;

    /* return index of color just added */
    return next_palette_index - 1;
}

/* a pong paddle */
struct paddle {
    unsigned short x, y, length;
    unsigned char color;
    unsigned short width;
};

struct ball
{
    unsigned short x, y, size;
    unsigned char color;
};

struct playerScore
{
    unsigned short score;
    char player[8];
};

/* put a pixel on the screen in mode 4 */
void put_pixel(volatile unsigned short* buffer, int row, int col, unsigned char color) {
    /* find the offset which is the regular offset divided by two */
    unsigned short offset = (row * WIDTH + col) >> 1;

    /* read the existing pixel which is there */
    unsigned short pixel = buffer[offset];

    /* if it's an odd column */
    if (col & 1) {
        /* put it in the left half of the short */
        buffer[offset] = (color << 8) | (pixel & 0x00ff);
    } else {
        /* it's even, put it in the left half */
        buffer[offset] = (pixel & 0xff00) | color;
    }
}

/* draw a paddle onto the screen */
void draw_paddle(volatile unsigned short* buffer, struct paddle* p) {
    short row, col;
    /* for each row of the paddle */
    for (row = p->y; row < (p->y + p->length); row++) {
        /* loop through each column of the paddle */
        for (col = p->x; col < (p->x + p->width); col++) {
            /* set the screen location to this color */
            put_pixel(buffer, row, col, p->color);
        }
    }
}

/* draw a playing ball onto the screen */
void draw_ball(volatile unsigned short* buffer, struct ball* b)
{
    short row, col;
    for (row = b -> y; row < (b -> y + b -> size); row++)
    {
        for (col = b -> x; col < (b -> x + b -> size); col++)
        {
            put_pixel(buffer, row, col, b -> color);
        }
    }
}

/* clear the screen right around the paddle */
void update_screen(volatile unsigned short* buffer, unsigned short color, struct paddle* p) {
    short row, col;
    for (row = p->y - 2; row < (p->y + p->length + 2); row++) {
        for (col = p->x; col < (p->x + p->width); col++) {
            put_pixel(buffer, row, col, color);
        }
    }
}

void update_ball(volatile unsigned short* buffer, unsigned short color, struct ball* b)
{
    short row, col;
    for (row = b -> y - 2; row < (b -> y + b -> size + 2); row++)
    {
        for (col = b -> x - 2; col < (b -> x + b -> size + 2); col++)
        {
            put_pixel(buffer, row, col, color);
        }
    }
}

/* this function takes a video buffer and returns to you the other one */
volatile unsigned short* flip_buffers(volatile unsigned short* buffer) {
    /* if the back buffer is up, return that */
    if(buffer == front_buffer) {
        /* clear back buffer bit and return back buffer pointer */
        *display_control &= ~SHOW_BACK;
        return back_buffer;
    } else {
        /* set back buffer bit and return front buffer */
        *display_control |= SHOW_BACK;
        return front_buffer;
    }
}

/* handle the buttons which are pressed down */
void handle_buttons(struct paddle* p) {
    /* move the paddle with the arrow keys */	
	if (button_pressed(BUTTON_DOWN) && ((p->y + p->length) < HEIGHT)) {
		p->y++;
	}
	if (button_pressed(BUTTON_UP) && (p->y > 0)) {
		p->y--;
	}
}

/* clear the screen to black */
void clear_screen(volatile unsigned short* buffer, unsigned short color) {
    unsigned short row, col;
    /* set each pixel black */
    for (row = 0; row < HEIGHT; row++) {
        for (col = 0; col < WIDTH; col++) {
            put_pixel(buffer, row, col, color);
        }
    }
}

short directionTracker = 1;
/* basic AI movement of up and down */
void basicAI(struct paddle* p)
{
    if (directionTracker)
    {
        if (p -> y == (HEIGHT - p -> length))
        {
            p -> y -= 1;
            directionTracker = 0;
        }
        else
        {
            p -> y += 1;
        }
    }
    else
    {
        if (p -> y == 0)
        {
            p -> y += 1;
            directionTracker = 1;
        }
        else
        {
            p -> y -= 1;
        }
    }
}

short quadrant, dx, dy;
/* moves the ball */
void ballMove(struct ball* b)
{
    b -> x += dx;
    b -> y += dy;
}

/* records the score for both the player and computer as well as incrementing score. */
void scoreKeeper(struct playerScore* p)
{
    p -> score++;
}

/* resets the paddle and ball position as well as game speed (implement later) after a score. */
void resetState(struct paddle* left, struct paddle* right, struct ball* b, unsigned char color)
{
    left -> x = 35;
    left -> y = 75;
    right -> x = 205;
    right -> y = 75;
    
    quadrant = rand() % 4;
    switch (quadrant)
    {
        case 0:
            dx = -1;
            dy = 1;
            break;
        case 1:
            dx = 1;
            dy = 1;
            break;
        case 2:
            dx = -1;
            dy = -1;
            break;
        default:
            dx = 1;
            dy = -1;
    }
    b -> x = 119;
    b -> y = 79;
    
    clear_screen(front_buffer, color);
    clear_screen(back_buffer, color);
}

/* check the position of the ball and perform an action based on the condition of the ball. */
void ballCollision(struct ball* b, struct playerScore* p1, struct playerScore* p2, struct paddle* left, struct paddle* right, unsigned char color)
{
	int x;
    switch(b -> x)
    {
        case WIDTH:
            scoreKeeper(p1);
            resetState(left, right, b, color);
			for(x = 0; x < 65535; x++)
			{
				//Do nothing.
			}	
            break;
        case 0:
            scoreKeeper(p2);
            resetState(left, right, b, color);
			for(x = 0; x < 65535; x++)
			{
				//Do nothing.
			}	
            break;
    }
    
    switch(b -> y)
    {
        case (HEIGHT - 2):
        case 0:
            dy *= -1;
            break;
    }
    
    // if the ball is in contact with the paddle, reverse the momentum of the ball in the x-axis.
    if(((b -> x == (left -> x + left -> width)) && ((b -> y >= left -> y) && (b -> y <= (left -> y + left -> length)))) || 
    ((b -> x == right -> x) && ((b -> y >= right -> y) && (b -> y <= (right -> y + right -> length))))) dx *= -1;
}

/* spawns the ball in the center of the screen and randomly select a quadrant to play the ball. */
void startBall()
{
    quadrant = rand() % 4;
    switch (quadrant)
    {
        case 0:
            dx = -1;
            dy = 1;
            break;
        case 1:
            dx = 1;
            dy = 1;
            break;
        case 2:
            dx = -1;
            dy = -1;
            break;
        default:
            dx = 1;
            dy = -1;
    }
}

/* the main function */
int main() {
    /* we set the mode to mode 4 with bg2 on */
    *display_control = MODE4 | BG2;

    /* make 2 light slate gray paddles */
    unsigned char slateGray = add_color(119, 136, 153);
    struct paddle player = {35, 75, 15, slateGray, 4};
    struct paddle computer = {205, 75, 15, slateGray, 4};
    
    struct playerScore player1 = {0, "PLAYER 1"};
    struct playerScore player2 = {0, "PLAYER 2"};
    
    /* make the playing ball for pong */
    struct ball pong = {119, 79, 2, slateGray};

    /* add black to the palette */
    unsigned char black = add_color(0, 0, 0);

    /* the buffer we start with */
    volatile unsigned short* buffer = front_buffer;
	
	/* game delay */
	char ballDelay = 0;
	
    /* clear whole screen first */
    clear_screen(front_buffer, black);
    clear_screen(back_buffer, black);
    
    startBall();

    /* loop forever */
    while (1) {
        /* clear the screen - only the areas around the paddle and ball! */
        update_screen(buffer, black, &player);
        update_screen(buffer, black, &computer);
        update_ball(buffer, black, &pong);

        /* draw the paddles and ball */
        draw_paddle(buffer, &player);
        draw_paddle(buffer, &computer);
        draw_ball(buffer, &pong);

        /* handle button input */
		handle_buttons(&player);
		basicAI(&computer);
		if(ballDelay == 5)
        {	
			ballMove(&pong);
			ballDelay = 0;
		}
		ballDelay++;
        ballCollision(&pong, &player1, &player2, &player, &computer, black);

        /* wait for vblank before switching buffers */
        wait_vblank();

        /* swap the buffers */
        buffer = flip_buffers(buffer);
    }
}

/* the game boy advance uses "interrupts" to handle certain situations
 * for now we will ignore these */
void interrupt_ignore() {
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
1. Implement AI movement. ~done
2. Implement ball movement. (Randomize a number for x and y direction %4) ~done
3. Ball collision & direction change. ~done
4. Boundary line. ~done
5. Set delay with countdown right after boot.
6. Reset layout after scoring. ~done

Extras:
1. Score.
2. AI ball tracking.
3. Ball acceleration. (play with the delay loop, this is the for loop that does nothing -> janky wait() used to eat the cycles so that it runs a bit slower)
*/
