#include <time.h>
#include <allegro.h>

// Add a framerate counter //////////////////////////////////////////////////
// based on: http://wiki.allegro.cc/index.php?title=Timers#FPS
int fps = 0;
int frames_done = 0;
int old_ticks = 0;

volatile int ticks = 0;
void ticker()
{
    ticks++;
}
END_OF_FUNCTION(ticker);

// Handle the close button
volatile int done = FALSE;
void close_button_handler(void)
{
    done = TRUE;
}
END_OF_FUNCTION(close_button_handler)


PALETTE fire_palette, gray_palette;
PALETTE *palette = &fire_palette;

int fire_colors[4096];
int gray_colors[4096];

int *colors = fire_colors;

bool frame_by_frame = FALSE;
bool calculate_fire = TRUE;
bool truecolor      = TRUE;

// D E F I N E S ////////////////////////////////////////////////////////////
#define FIRE_W 800      // width of fire
#define FIRE_H 600      // height of fire

#define NUM_HOTSPOTS 240
#define HOTSPOT_REGION 20

typedef int USINT;

// R N D ////////////////////////////////////////////////////////////////////
inline int rnd(int max)
{
    return rand() % (max + 1);
}

inline USINT get_fire(USINT *fire, int x, int y)
{
    return fire[x+2 + (FIRE_W * (y+2))];
}

inline void set_fire(USINT *fire, int x, int y, USINT value)
{
    fire[x+2 + (FIRE_W * (y+2))] = value;
}


// find_fire /////////////////////////////////////////////////////////////////
// -returns the proper color at (x, y) in the next frame of fire
// -needs prev frame of fire untouched, so put new value in a diff fire array
inline USINT fire_math(int x, int y, USINT *fire)
{
    return (get_fire(fire, (x + 0), (y + 1)) +
            get_fire(fire, (x + 0), (y + 2)) +
            get_fire(fire, (x + 0), (y + 3)) +
            get_fire(fire, (x + 0), (y + 4)) +
            get_fire(fire, (x - 1), (y + 5)) +
            get_fire(fire, (x + 0), (y + 5)) +
            get_fire(fire, (x + 1), (y + 5)) +
            get_fire(fire, (x + 0), (y + 6))
           ) >> 3;
}

// C A L C  F I R E  //////////////////////////////////////////////////////////
inline void calc_fire(USINT *prev, USINT *curr)
{
    for (int y = 0; y < FIRE_H; y++) {
        for (int x = 0; x < FIRE_W; x++) {
            set_fire(curr, x, y, fire_math(x, y, prev));
        }
    }
}

// D R A W  F I R E  ////////////////////////////////////////////////////////
inline void draw_fire(USINT *fire, BITMAP *bitmap)
{
    if (truecolor) {
        for (int y = 0; y < FIRE_H; y++) {
            for (int x = 0; x < FIRE_W; x++) {
                putpixel(bitmap, x, y, colors[int(get_fire(fire, x, y))]);
            }
        }
    } else {
        for (int y = 0; y < FIRE_H; y++) {
            for (int x = 0; x < FIRE_W; x++) {
                putpixel(bitmap, x, y, palette_color[int(get_fire(fire, x, y)) >> 4]);
            }
        }
    }
}

void add_hotspot(USINT *fire, int x, int y, int hotspot_radius, int num_spots)
{
    for (int j= 0; j < num_spots; j++)
    {
        int hotspot_x = rnd(hotspot_radius * 2) - hotspot_radius;
        int hotspot_y = rnd(hotspot_radius * 2) - hotspot_radius;

        if (((hotspot_x * hotspot_x) + (hotspot_y * hotspot_y)) > (hotspot_radius * hotspot_radius))
        {
            hotspot_x /= 2;
            hotspot_y /= 2;
        }

        hotspot_x += x;
        hotspot_y += y;

        if (hotspot_x >= 0 && hotspot_x < FIRE_W &&
            hotspot_y >= 0 && hotspot_y < FIRE_H) {

            set_fire(fire, hotspot_x, hotspot_y,
                     CLAMP(2560,
                           get_fire(fire, hotspot_x, hotspot_y) + 64,
                           4095));
        }
    }
}

// D O  F I R E  ////////////////////////////////////////////////////////////
void do_fire()
{
    USINT *prev, *curr, *temp;;

    prev = (USINT*) malloc( (FIRE_W + 4) * (FIRE_H + 10) * sizeof(USINT));
    curr = (USINT*) malloc( (FIRE_W + 4) * (FIRE_H + 10) * sizeof(USINT));

    BITMAP *buf;
    buf = create_bitmap(FIRE_W, FIRE_H);
    clear(buf);

    int mouse_projection_x;
    int mouse_projection_y;

    struct Hotspot
    {
        int x, y, speed;
    };

    Hotspot hotspots[NUM_HOTSPOTS];

    for (int i=0; i<NUM_HOTSPOTS; i++)
    {
        hotspots[i].x = rnd(FIRE_W);
        hotspots[i].y = FIRE_H-rnd(HOTSPOT_REGION);
        hotspots[i].speed = rnd(20);
    }

    while(!done)
    {
        if (frame_by_frame) {
            // Pause until keypress or close icon pressed
            while(!keypressed() && !done);
        }

        // Handle keypresses
        if (keypressed())
        {
            switch(readkey() >> 8)
            {
                case KEY_ESC:
                    done = TRUE;
                    break;

                case KEY_PRTSCR:
                case KEY_F12:
                    acquire_screen();
                    save_pcx("fire.pcx", screen, *palette);
                    release_screen();
                    break;

                case KEY_1: // toggle grayscale palette
                    colors = (colors == fire_colors ? gray_colors : fire_colors);
                    palette = (palette == &fire_palette ? &gray_palette : &fire_palette);
                    set_palette(*palette);
                    break;

                case KEY_2: // toggle frame-by-frame mode
                    frame_by_frame = !frame_by_frame;
                    break;

                case KEY_3: // toggle fire calculations
                    calculate_fire = !calculate_fire;
                    break;
                case KEY_4: // toggle truecolor
                    truecolor = !truecolor;
                    break;
            }
        }

        for (int r = 0; r < 3; r++)
        {
            for (int i = 0; i < NUM_HOTSPOTS; i++)
            {
                hotspots[i].x += rnd(hotspots[i].speed*2) - hotspots[i].speed;
                if (hotspots[i].x <  0     ) hotspots[i].x += FIRE_W;
                if (hotspots[i].x >= FIRE_W) hotspots[i].x -= FIRE_W;

                hotspots[i].y += rnd(hotspots[i].speed*2) - hotspots[i].speed;
                if (hotspots[i].y <  FIRE_H-HOTSPOT_REGION) hotspots[i].y += HOTSPOT_REGION;
                if (hotspots[i].y >= FIRE_H               ) hotspots[i].y -= HOTSPOT_REGION;

                add_hotspot(prev, hotspots[i].x, hotspots[i].y, 8, 100);
            }

            mouse_projection_x = FIRE_W * mouse_x / SCREEN_W;
            mouse_projection_y = FIRE_H * mouse_y / SCREEN_H;

            if (mouse_b & 1) {
                add_hotspot(prev, mouse_projection_x, mouse_projection_y, 8, 400);
            }

            if (calculate_fire) {
                calc_fire(prev, curr);
            }

            temp = curr;
            curr = prev;
            prev = temp;
        }
        draw_fire(prev, buf);

        if (!calculate_fire) {
            memset(prev, 0, (FIRE_W + 4) * (FIRE_H + 10) * sizeof(USINT));
        }

        acquire_screen();
        stretch_blit(buf, screen, 0, 0, FIRE_W, FIRE_H, 0, 0, SCREEN_W, SCREEN_H);
        textprintf_ex(screen, font, 10, 10, makecol(255, 255, 255), -1, "Fire! by Leftium [FPS:%3d]", fps);

        release_screen();

        frames_done++;

        if(ticks - old_ticks >= 10)//i.e. a second has passed since we last measured the frame rate
        {
            fps = frames_done;
            //fps now holds the the number of frames done in the last second
            //you can now output it using textout_ex et al.

            //reset for the next second
            frames_done = 0;
            old_ticks = ticks;
        }
    }

    free(prev);
    free(curr);
}


// I N I T  F I R E  P A L E T T E //////////////////////////////////////////
void init_fire_palette(PALETTE pal)
{
    for (int i = 255; i > -1; i--)
    {
        pal[i].r = 0;           // blk (0, 0, 0)
        pal[i].g = 0;
        pal[i].b = 0;
    }

    for (int i = 255; i > 223; i--)
    {
        pal[i].r = 63;          // white (63, 63, 63)
        pal[i].g = 63;
        pal[i].b = 63;
    }

    for (int i = 223; i > 191; i--)
    {
        pal[i].r = 63;          // yellow   (63, 63, 0)
        pal[i].g = 63;
        pal[i].b = -383+(i<<1); // 2(32 - (oi - i))
    }

    for (int i = 191; i > 175; i--)
    {
        pal[i].r = 63;          // orange   (63, 32, 0)
        pal[i].g = -319+(i<<1);
        pal[i].b = 0;
    }

    for (int i = 175; i > 159; i--)
    {
        pal[i].r = -637+(i<<2);    // black   (0, 0, 0)
        pal[i].g = -318+(i<<1);
        pal[i].b = 0;
    }
}

void init_fire_colors()
{
    for (int x = 0; x < 256; x++)
    {
        // black (0, 0, 0)
        fire_colors[x] = makecol(0, 0, 0);
    }

    int i = 4095;
    for (int x = 0; x < 512; x++)
    {
        // white (255, 255, 255)
        fire_colors[i] = makecol(255, 255, 255);
        i--;
    }

    for (int x = 0; x < 512; x++)
    {
        // yellow (255, 255, 0)
        fire_colors[i] = makecol(255, 255, 255 - (x/2));
        i--;
    }

    for (int x = 0; x < 256; x++)
    {
        // orange (255, 128, 0)
        fire_colors[i] = makecol(255, 255 - (x/2), 0);
        i--;
    }

    for (int x = 0; x < 256; x++)
    {
        // black (0, 0, 0)
        fire_colors[i] = makecol(255 - x, 128 - (x/2), 0);
        i--;
    }
}

// I N I T  G R A Y  P A L E T T E //////////////////////////////////////////
void init_gray_palette(PALETTE pal)
{
    for (int i = 0; i < 256; i++)
    {
        pal[i].r = i / 4;
        pal[i].g = i / 4;
        pal[i].b = i / 4;
    }
}

void init_gray_colors()
{
    for (int i = 0; i < 4096; i++)
    {
        gray_colors[i] = makecol(i/16, i/16, i/16);
    }

}

int gfx_card = GFX_AUTODETECT_WINDOWED;
int gfx_w = 800;
int gfx_h = 600;
int gfx_bpp = 8;


int main(void)
{
    srand(time(NULL));

    /* you should always do this at the start of Allegro programs */
    if (allegro_init() != 0)
        return 1;

    install_timer();

    LOCK_VARIABLE(ticks);
    LOCK_FUNCTION(ticker);
    install_int_ex(ticker, BPS_TO_TIMER(10));//i.e. game time is in tenths of seconds

    /* set up the keyboard handler */
    install_keyboard();
    install_timer();
    install_mouse();

    LOCK_FUNCTION(close_button_handler);
    set_close_button_callback(close_button_handler);

    /* set a graphics mode sized 320x200 */
    if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0)!=0) {
        set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
        allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
        return 1;
    }

    RGB grey = { 52, 51, 49 };

    vsync();
    set_color(0, &grey);
    set_color(1, &black_palette[0]);

    gui_fg_color = 1;
    gui_bg_color = 0;

    gfx_bpp = desktop_color_depth();

    //  If CTRL key pressed bring up graphics mode selection dialog only
    if (key_shifts & KB_CTRL_FLAG) {
        if (!gfx_mode_select_ex(&gfx_card, &gfx_w, &gfx_h, &gfx_bpp)) {
            return -1;
        }
    }

    if (gfx_bpp != 0) {
        set_color_depth(gfx_bpp);
    }
    if (set_gfx_mode(gfx_card, gfx_w, gfx_h, 0, 0) != 0) return 1;

    // Try to enable running in background, if possible
    if (set_display_switch_mode(SWITCH_BACKGROUND) != 0) {
        set_display_switch_mode(SWITCH_BACKAMNESIA);
    }

    enable_hardware_cursor();
    select_mouse_cursor(MOUSE_CURSOR_ARROW);
    show_mouse(screen);

    init_fire_palette(fire_palette);
    init_gray_palette(gray_palette);
    init_fire_colors();
    init_gray_colors();
    set_palette(*palette);

    do_fire();

    return 0;
}
END_OF_MAIN()
