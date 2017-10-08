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
    unsigned short width = 4;
};

struct ball
{
    unsigned short x, y, size;
    unsigned char color;
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
    for (row = p->y; row < (p->y + p->width); row++) {
        /* loop through each column of the paddle */
        for (col = p->x; col < (p->x + p->length); col++) {
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
            put_pixel(buffer, row, col, p -> color);
        }
    }
}

/* clear the screen right around the paddle */
void update_screen(volatile unsigned short* buffer, unsigned short color, struct paddle* p) {
    short row, col;
    for (row = p->y - 2; row < (p->y + p->width + 2); row++) {
        for (col = p->x - 2; col < (p->x + p->length + 2); col++) {
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
    if (button_pressed(BUTTON_DOWN)) {
        p->y += 2;
    }
    if (button_pressed(BUTTON_UP)) {
        p->y -= 2;
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
        if (p -> y == (HEIGHT - length))
        {
            p -> y -= 2;
            directionTracker = 0;
        }
        else
        {
            p -> y += 2;
        }
    }
    else
    {
        if (p -> y == 0)
        {
            p -> y += 2;
            directionTracker = 1;
        }
        else
        {
            p -> y -= 2;
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

/* the main function */
int main() {
    /* we set the mode to mode 4 with bg2 on */
    *display_control = MODE4 | BG2;

    /* make 2 light slate gray paddles */
    struct paddle player = {35, 75, 15, add_color(119, 136, 153)};
    struct paddle computer = {205, 75, 15, add_color(119,136,153)};
    
    /* make the playing ball for pong */
    struct ball pong = {119, 79, 2, add_color(119, 136, 153)};

    /* add black to the palette */
    unsigned char black = add_color(0, 0, 0);

    /* the buffer we start with */
    volatile unsigned short* buffer = front_buffer;

    /* clear whole screen first */
    clear_screen(front_buffer, black);
    clear_screen(back_buffer, black);
    
    quadrant = rand() % 4;
    switch (quadrant)
    {
        case 0:
            dx = -2;
            dy = 2;
            break;
        case 1:
            dx = 2;
            dy = 2;
            break;
        case 2:
            dx = -2;
            dy = -2;
            break;
        default:
            dx = 2;
            dy = -2;
    }

    /* loop forever */
    while (1) {
        /* clear the screen - only the areas around the paddle! */
        update_screen(buffer, black, &player);
        update_screen(buffer, black, &computer);

        /* draw the paddles and ball */
        draw_paddle(buffer, &player);
        draw_paddle(buffer, &computer);
        draw_ball(buffer, &pong);

        /* handle button input */
        handle_buttons(&player);
        basicAI(&computer);
        ballMove(&pong);

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
3. Ball collision & direction change.
4. Boundary line.
5. Set delay with countdown right after boot.

Extras:
1. Score.
2. AI ball tracking.
3. Ball acceleration.