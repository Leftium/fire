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

// D E F I N E S ////////////////////////////////////////////////////////////
#define FIRE_W 640      // width of fire
#define FIRE_H 240      // height of fire

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
    if (x < 0 || x >= FIRE_W || y < 0 || y >= FIRE_H) {
        return 0;
    } else {
        return fire[x + (FIRE_W * y)];
    }
}

inline void set_fire(USINT *fire, int x, int y, USINT value)
{
    if (x >= 0 && x < FIRE_W && y >= 0 && y < FIRE_H) {
        fire[x + (FIRE_W * y)] = value;
    }
}

inline void add_fire(USINT *fire, int x, int y, USINT value)
{
    if (x >= 0 && x < FIRE_W && y >= 0 && y < FIRE_H) {
        fire[x + (FIRE_W * y)] += (value % 256);
    };
}

// find_fire /////////////////////////////////////////////////////////////////
// -returns the proper color at (x, y) in the next frame of fire
// -needs prev frame of fire untouched, so put new value in a diff fire array
inline USINT fire_math(int x, int y, USINT *fire)
{
    USINT total = 0;

    total += get_fire(fire, (x + 0), (y - 1));

    total += get_fire(fire, (x - 1), (y + 0));
    total += get_fire(fire, (x + 0), (y + 0)) * 2;
    total += get_fire(fire, (x + 1), (y + 0));

    total += get_fire(fire, (x + 0), (y + 1));

    total += get_fire(fire, (x - 1), (y + 2));
    total += get_fire(fire, (x + 1), (y + 2));

    total += get_fire(fire, (x + 0), (y + 3));

    total += get_fire(fire, (x - 1), (y + 4));
    total += get_fire(fire, (x + 0), (y + 4));
    total += get_fire(fire, (x + 1), (y + 4));

    return (total / 12);
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
    for (int y = 0; y < FIRE_H; y++) {
        for (int x = 0; x < FIRE_W; x++) {
            putpixel(bitmap, x, y, palette_color[get_fire(fire, x, y)]);
        }
    }
}

// Z E R O //////////////////////////////////////////////////////////////////
void zero(USINT *array)
{
    for (int i = 0; i < (FIRE_W * FIRE_H); i++) {
        array[i] = 0;
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

        if (get_fire(fire, hotspot_x, hotspot_y) < 160) {
            set_fire(fire, hotspot_x, hotspot_y, 160);
        }
        add_fire(fire, hotspot_x, hotspot_y, 4);
        if (get_fire(fire, hotspot_x, hotspot_y) > 255) {
            set_fire(fire, hotspot_x, hotspot_y, 255);
        }
    }
}

// D O  F I R E  ////////////////////////////////////////////////////////////
void do_fire()
{
    USINT *prev, *curr, *temp;;

    prev = (USINT*) malloc( FIRE_W * FIRE_H * sizeof(USINT));
    curr = (USINT*) malloc( FIRE_W * FIRE_H * sizeof(USINT));

    zero(prev);
    zero(curr);

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

    while(!keypressed())
    {
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

                add_hotspot(prev, hotspots[i].x, hotspots[i].y, 8, 50);
            }

            mouse_projection_x = FIRE_W * mouse_x / SCREEN_W;
            mouse_projection_y = FIRE_H * mouse_y / SCREEN_H;

            add_hotspot(prev, mouse_projection_x, mouse_projection_y, 8, 200);

            calc_fire(prev, curr);

            temp = curr;
            curr = prev;
            prev = temp;
        }
		draw_fire(prev, buf);

		acquire_screen();
        stretch_blit(buf, screen, 0, 0, FIRE_W, FIRE_H, 0, 0, SCREEN_W, SCREEN_H);
        textprintf_ex(screen, font, 10, 10, makecol(255, 255, 255), -1, "fire by wonsungi [frames/sec:%3d]", fps);
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
        pal[i].r = -637+(i<<2);    // orange   (63, 32, 0)
        pal[i].g = -318+(i<<1);
        pal[i].b = 0;
    }
    set_palette(pal);
}

int gfx_card = GFX_AUTODETECT_WINDOWED;
int gfx_w = 800;
int gfx_h = 600;
int gfx_bpp = 8;


int main(void)
{
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
    if (!gfx_mode_select_ex(&gfx_card, &gfx_w, &gfx_h, &gfx_bpp)) {
        return -1;
    }

    if (gfx_bpp != 0) {
        set_color_depth(gfx_bpp);
    }
    if (set_gfx_mode(gfx_card, gfx_w, gfx_h, 0, 0) != 0) return 1;

    PALETTE fire;
    init_fire_palette(fire);
    do_fire();

    acquire_screen();
    save_pcx("fire.pcx", screen, fire);
    release_screen();

    return 0;
}
END_OF_MAIN()
