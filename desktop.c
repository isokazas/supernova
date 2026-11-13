#include "desktop.h"
#include "graphics.h"
#include "input.h"
#include "terminal_gui.h"
#include "ports.h"

#define WINDOW_COUNT 4
#define TASKBAR_H 16
typedef enum { APP_FILES, APP_INFO, APP_CLOCK, APP_TERMINAL } app_id_t;
typedef struct { int x,y,w,h; const char *title; uint8_t color; int visible; } window_t;
static window_t windows[WINDOW_COUNT] = {
    {8,26,250,142,"FILES",COLOR_LIGHT_RED,1},
    {40,34,250,130,"SYSTEM INFO",COLOR_LIGHT_GREEN,1},
    {56,38,220,110,"CLOCK",COLOR_LIGHT_CYAN,0},
    {8,22,304,160,"TERMINAL",COLOR_LIGHT_BLUE,0}
};
static int z_order[WINDOW_COUNT]={0,1,2,3}, focused=APP_FILES;
static int cursor_x=GFX_WIDTH/2, cursor_y=GFX_HEIGHT/2;
static int dragging,drag_dx,drag_dy,start_open;
static uint8_t previous_buttons;
static unsigned char last_clock_second=0xFF;

static int clamp(int v,int lo,int hi){return v<lo?lo:(v>hi?hi:v);}
static int inside(int x,int y,int rx,int ry,int rw,int rh){return x>=rx&&x<rx+rw&&y>=ry&&y<ry+rh;}
static void front(int app){int p=0;while(p<WINDOW_COUNT&&z_order[p]!=app)++p;if(p==WINDOW_COUNT)return;for(int i=p;i<WINDOW_COUNT-1;++i)z_order[i]=z_order[i+1];z_order[WINDOW_COUNT-1]=app;focused=app;}
static void focus_next(void){for(int n=1;n<=WINDOW_COUNT;++n){int a=(focused+n)%WINDOW_COUNT;if(windows[a].visible){front(a);return;}}}
static void open_app(int app){windows[app].visible=1;front(app);start_open=0;if(app==APP_TERMINAL)terminal_gui_init();}
static unsigned char rtc(uint8_t r){port_byte_out(0x70,(uint8_t)(r|0x80));return port_byte_in(0x71);}
static unsigned char bcd(unsigned char x){return (unsigned char)((x&15)+(x>>4)*10);}

static void draw_cursor(void){for(int r=0;r<10;++r)for(int c=0;c<=r/2;++c)gfx_put_pixel(cursor_x+c,cursor_y+r,(r==9||c==0||c==r/2)?COLOR_BLACK:COLOR_WHITE);}
static void draw_clock(window_t*w){unsigned char sb=rtc(0x0B),h=rtc(4),m=rtc(2),s=rtc(0);int pm=(h&0x80)!=0;h&=0x7f;if(!(sb&4)){h=bcd(h);m=bcd(m);s=bcd(s);}if(!(sb&2)){h=(unsigned char)(h%12);if(pm)h+=12;}last_clock_second=s;char t[9]={(char)('0'+h/10),(char)('0'+h%10),':',(char)('0'+m/10),(char)('0'+m%10),':',(char)('0'+s/10),(char)('0'+s%10),0};gfx_draw_string(w->x+70,w->y+42,t,COLOR_WHITE,COLOR_BLACK);gfx_draw_string(w->x+42,w->y+63,"QEMU CMOS CLOCK",COLOR_LIGHT_CYAN,COLOR_BLACK);}
static void draw_window(int a){window_t*w=&windows[a];if(!w->visible)return;gfx_draw_rect(w->x,w->y,w->w,w->h,a==focused?COLOR_YELLOW:COLOR_WHITE,COLOR_DARK_GRAY);gfx_fill_rect(w->x+1,w->y+1,w->w-2,14,w->color);gfx_draw_string(w->x+6,w->y+4,w->title,COLOR_WHITE,w->color);gfx_fill_rect(w->x+w->w-12,w->y+3,8,9,COLOR_RED);gfx_draw_string(w->x+w->w-11,w->y+4,"X",COLOR_WHITE,COLOR_RED);gfx_fill_rect(w->x+3,w->y+18,w->w-6,w->h-21,COLOR_BLACK);
    if(a==APP_FILES){gfx_draw_string(w->x+12,w->y+27,"IN MEMORY FILE DEMO",COLOR_YELLOW,COLOR_BLACK);gfx_draw_string(w->x+12,w->y+45,"DESKTOP",COLOR_WHITE,COLOR_BLACK);gfx_draw_string(w->x+12,w->y+60,"TERMINAL",COLOR_WHITE,COLOR_BLACK);gfx_draw_string(w->x+12,w->y+75,"README TXT",COLOR_WHITE,COLOR_BLACK);}
    else if(a==APP_INFO){gfx_draw_string(w->x+12,w->y+30,"SUPERNOVA OS",COLOR_WHITE,COLOR_BLACK);gfx_draw_string(w->x+12,w->y+45,"VGA 320 X 200",COLOR_LIGHT_CYAN,COLOR_BLACK);gfx_draw_string(w->x+12,w->y+60,"TAB: FOCUS WINDOWS",COLOR_WHITE,COLOR_BLACK);gfx_draw_string(w->x+12,w->y+75,"ARROWS: MOVE POINTER",COLOR_WHITE,COLOR_BLACK);}
    else if(a==APP_CLOCK)draw_clock(w);else terminal_gui_draw(w->x+6,w->y+20,48,17);
}
static void draw_menu(void){if(!start_open)return;gfx_draw_rect(4,92,102,88,COLOR_WHITE,COLOR_DARK_GRAY);gfx_draw_string(12,99,"OPEN APP",COLOR_YELLOW,COLOR_DARK_GRAY);gfx_draw_string(12,114,"FILES",COLOR_WHITE,COLOR_DARK_GRAY);gfx_draw_string(12,128,"TERMINAL",COLOR_WHITE,COLOR_DARK_GRAY);gfx_draw_string(12,142,"INFO",COLOR_WHITE,COLOR_DARK_GRAY);gfx_draw_string(12,156,"CLOCK",COLOR_WHITE,COLOR_DARK_GRAY);}
static void render(void){gfx_clear(COLOR_BLUE);for(int y=20;y<GFX_HEIGHT-TASKBAR_H;y+=16)for(int x=(y/16)%2?8:16;x<GFX_WIDTH;x+=32)gfx_put_pixel(x,y,COLOR_LIGHT_BLUE);for(int i=0;i<WINDOW_COUNT;++i)draw_window(z_order[i]);gfx_fill_rect(0,GFX_HEIGHT-TASKBAR_H,GFX_WIDTH,TASKBAR_H,COLOR_DARK_GRAY);gfx_draw_rect(3,GFX_HEIGHT-14,44,12,COLOR_WHITE,COLOR_LIGHT_BLUE);gfx_draw_string(8,GFX_HEIGHT-11,"START",COLOR_WHITE,COLOR_LIGHT_BLUE);for(int i=0;i<WINDOW_COUNT;++i){int a=z_order[i];if(windows[a].visible){int bx=51+i*65;gfx_fill_rect(bx,GFX_HEIGHT-13,61,10,a==focused?COLOR_LIGHT_BLUE:COLOR_DARK_GRAY);gfx_draw_string(bx+2,GFX_HEIGHT-11,windows[a].title,COLOR_WHITE,a==focused?COLOR_LIGHT_BLUE:COLOR_DARK_GRAY);}}draw_menu();draw_cursor();gfx_present();}
static void click(void){if(inside(cursor_x,cursor_y,3,GFX_HEIGHT-14,44,12)){start_open=!start_open;return;}if(start_open){if(inside(cursor_x,cursor_y,4,108,102,14))open_app(APP_FILES);else if(inside(cursor_x,cursor_y,4,122,102,14))open_app(APP_TERMINAL);else if(inside(cursor_x,cursor_y,4,136,102,14))open_app(APP_INFO);else if(inside(cursor_x,cursor_y,4,150,102,14))open_app(APP_CLOCK);else start_open=0;return;}for(int i=0;i<WINDOW_COUNT;++i){int a=z_order[i],bx=51+i*65;if(windows[a].visible&&inside(cursor_x,cursor_y,bx,GFX_HEIGHT-16,61,16)){front(a);return;}}for(int p=WINDOW_COUNT-1;p>=0;--p){int a=z_order[p];window_t*w=&windows[a];if(!w->visible||!inside(cursor_x,cursor_y,w->x,w->y,w->w,16))continue;front(a);if(cursor_x>=w->x+w->w-14){w->visible=0;focus_next();return;}dragging=1;drag_dx=cursor_x-w->x;drag_dy=cursor_y-w->y;return;}window_t*f=&windows[APP_FILES];if(f->visible&&inside(cursor_x,cursor_y,f->x+6,f->y+54,f->w-12,16))open_app(APP_TERMINAL);}
static void mouse(const input_event_t*e){cursor_x=clamp(cursor_x+e->mouse_dx,0,GFX_WIDTH-1);cursor_y=clamp(cursor_y+e->mouse_dy,0,GFX_HEIGHT-1);int down=(e->mouse_buttons&1)!=0;if(down&&!(previous_buttons&1))click();if(!down)dragging=0;if(dragging&&down){window_t*w=&windows[focused];w->x=clamp(cursor_x-drag_dx,0,GFX_WIDTH-w->w);w->y=clamp(cursor_y-drag_dy,18,GFX_HEIGHT-TASKBAR_H-w->h);}previous_buttons=e->mouse_buttons;}
static void key(const input_event_t*e){if(e->key==KEY_TAB){focus_next();return;}if(e->key==KEY_UP){cursor_y=clamp(cursor_y-4,0,GFX_HEIGHT-1);return;}if(e->key==KEY_DOWN){cursor_y=clamp(cursor_y+4,0,GFX_HEIGHT-1);return;}if(e->key==KEY_LEFT){cursor_x=clamp(cursor_x-4,0,GFX_WIDTH-1);return;}if(e->key==KEY_RIGHT){cursor_x=clamp(cursor_x+4,0,GFX_WIDTH-1);return;}if(e->key==KEY_CLOSE){windows[focused].visible=0;focus_next();return;}if(focused!=APP_TERMINAL)return;if(e->key==KEY_BACKSPACE)terminal_gui_input('\b');else if(e->key==KEY_ENTER)terminal_gui_input('\n');else if(e->character)terminal_gui_input(e->character);}
void desktop_init(void){focused=APP_FILES;start_open=dragging=0;previous_buttons=0;cursor_x=GFX_WIDTH/2;cursor_y=GFX_HEIGHT/2;terminal_gui_init();input_init();render();}
void desktop_run(void){input_event_t e;for(;;){int changed=0;if(input_poll(&e)){if(e.type==INPUT_MOUSE)mouse(&e);else if(e.type==INPUT_KEY)key(&e);changed=1;}if(windows[APP_CLOCK].visible&&rtc(0)!=last_clock_second)changed=1;if(changed)render();}}
