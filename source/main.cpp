#include <cstdio>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <orbis/Pad.h>
#include <orbis/UserService.h>
#include "graphics.h"
#include "log.h"

static const int W = 1920;
static const int H = 1080;
std::stringstream debugLogStream;

static Color rgb(uint8_t r, uint8_t g, uint8_t b) { return Color{r,g,b}; }

static const uint8_t FONT[36][5] = {
 {0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},
 {0x3E,0x41,0x41,0x41,0x22},{0x7F,0x41,0x41,0x22,0x1C},
 {0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},
 {0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},
 {0x00,0x41,0x7F,0x41,0x00},{0x20,0x40,0x41,0x3F,0x01},
 {0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
 {0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},
 {0x3E,0x41,0x41,0x41,0x3E},{0x7F,0x09,0x09,0x09,0x06},
 {0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
 {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},
 {0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},
 {0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,0x08,0x14,0x63},
 {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},
 {0x3E,0x45,0x49,0x51,0x3E},{0x00,0x42,0x7F,0x40,0x00},
 {0x62,0x51,0x49,0x49,0x46},{0x22,0x41,0x49,0x49,0x36},
 {0x18,0x14,0x12,0x7F,0x10},{0x2F,0x49,0x49,0x49,0x31},
 {0x3E,0x49,0x49,0x49,0x32},{0x01,0x71,0x09,0x05,0x03},
 {0x36,0x49,0x49,0x49,0x36},{0x26,0x49,0x49,0x49,0x3E}
};

static int glyphIndex(char c) {
 if(c>='A'&&c<='Z') return c-'A';
 if(c>='0'&&c<='9') return 26+c-'0';
 return -1;
}
static void rect(Scene2D* s,int x,int y,int w,int h,Color c) {
 if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;}
 if(x+w>W)w=W-x; if(y+h>H)h=H-y;
 if(w>0&&h>0)s->DrawRectangle(x,y,w,h,c);
}
static void text(Scene2D* s,const char* str,int x,int y,int scale,Color c) {
 int ox=x;
 for(;*str;++str) {
  if(*str==' '){x+=6*scale;continue;}
  if(*str=='\n'){y+=9*scale;x=ox;continue;}
  int idx=glyphIndex(*str);
  if(idx>=0) for(int col=0;col<5;++col)
   for(int row=0;row<7;++row)
    if(FONT[idx][col]&(1<<row)) rect(s,x+col*scale,y+row*scale,scale,scale,c);
  else if(*str==':'||*str=='.') {
   rect(s,x+2*scale,y+2*scale,scale,scale,c);
   rect(s,x+2*scale,y+6*scale,scale,scale,c);
  } else if(*str=='-') rect(s,x,y+3*scale,4*scale,scale,c);
  x+=6*scale;
 }
}
static void drawBackground(Scene2D* s,int frame) {
 s->FrameBufferFill(rgb(5,8,13));
 for(int y=0;y<H;++y) {
  int glow=(y<650)?(650-y)/18:0;
  rect(s,0,y,W,1,rgb(5+glow/4,8+glow/3,13+glow/2));
 }
 // Silhouette of a deserted industrial corridor.
 rect(s,0,700,W,380,rgb(7,10,14));
 for(int i=0;i<8;++i) {
  int x=170+i*230;
  rect(s,x,280,12,520,rgb(18,25,31));
  rect(s,x+12,280,150,8,rgb(20,28,34));
  rect(s,x+150,288,8,420,rgb(13,19,25));
 }
 rect(s,0,820,W,3,rgb(32,42,48));
 for(int y=0;y<H;y+=4) rect(s,0,y,W,1,rgb(8,12,17));
 // Red emergency light and subtle pulse.
 int pulse=20+(frame%60<30?frame%30:60-frame%60);
 rect(s,1510,150,180,5,rgb(90+pulse,12,15));
 rect(s,1510,155,180,2,rgb(35,8,10));
}
static void drawMenu(Scene2D* s,int selected,int frame) {
 drawBackground(s,frame);
 text(s,"BLACKOUT",145,180,12,rgb(225,232,235));
 text(s,"SIGNAL LOST",155,330,4,rgb(177,190,198));
 rect(s,150,390,560,3,rgb(150,26,31));
 text(s,"A SURVIVAL HORROR EXPERIENCE",155,420,2,rgb(111,129,139));
 const char* items[]={"NEW GAME","CONTINUE","OPTIONS","CREDITS","QUIT"};
 for(int i=0;i<5;++i) {
  int y=565+i*78;
  if(i==selected) {
   rect(s,140,y-12,540,58,rgb(105,18,25));
   rect(s,140,y-12,5,58,rgb(235,55,60));
   text(s,">",165,y,4,rgb(255,255,255));
   text(s,items[i],210,y,4,rgb(255,255,255));
  } else text(s,items[i],210,y,4,rgb(137,148,155));
 }
 rect(s,145,1000,1630,2,rgb(38,48,55));
 text(s,"D PAD  SELECT",150,1020,2,rgb(111,126,135));
 text(s,"X  CONFIRM",1510,1020,2,rgb(155,165,171));
 text(s,"BUILD  0.2  /  PS4",1500,70,1,rgb(80,94,103));
}
static void drawNotice(Scene2D* s,const char* title,const char* body,int frame) {
 drawBackground(s,frame);
 rect(s,330,330,1260,420,rgb(13,19,25));
 rect(s,330,330,1260,4,rgb(170,28,35));
 text(s,title,410,405,5,rgb(235,239,241));
 text(s,body,410,520,2,rgb(170,182,190));
 text(s,"CIRCLE  BACK TO MENU",410,660,2,rgb(220,70,75));
}
int main(void) {
 setvbuf(stdout,nullptr,_IONBF,0);
 DEBUGLOG << "BLACKOUT: SIGNAL LOST - graphical menu boot";
 Scene2D scene(W,H,4);
 if(!scene.Init(0xC000000,2)){DEBUGLOG<<"Video initialization failed";for(;;)sceKernelUsleep(1000000);}
 scene.SetActiveFrameBuffer(0);
 if(scePadInit()!=0){DEBUGLOG<<"Pad initialization failed";for(;;)sceKernelUsleep(1000000);}
 OrbisUserServiceInitializeParams userParams{};
 userParams.priority=ORBIS_KERNEL_PRIO_FIFO_LOWEST;
 sceUserServiceInitialize(&userParams);
 int userId=-1;
 sceUserServiceGetInitialUser(&userId);
 int pad=scePadOpen(userId,0,0,nullptr);
 if(pad<0){DEBUGLOG<<"DualShock 4 unavailable";for(;;)sceKernelUsleep(1000000);}
 int selected=0,screen=0,frame=0;
 uint32_t previous=0;
 for(;;) {
  OrbisPadData data{};
  if(scePadReadState(pad,&data)==0) {
   uint32_t now=data.buttons;
   uint32_t pressed=now&~previous;
   previous=now;
   if(screen==0) {
    if(pressed&ORBIS_PAD_BUTTON_UP) selected=(selected+4)%5;
    if(pressed&ORBIS_PAD_BUTTON_DOWN) selected=(selected+1)%5;
    if(pressed&ORBIS_PAD_BUTTON_CROSS) {
     if(selected==0)screen=1;
     else if(selected==1)screen=2;
     else if(selected==2)screen=3;
     else if(selected==3)screen=4;
     else screen=5;
    }
   } else if(pressed&ORBIS_PAD_BUTTON_CIRCLE) screen=0;
  }
  if(screen==0)drawMenu(&scene,selected,frame);
  else if(screen==1)drawNotice(&scene,"NEW GAME","THE FIRST CHAPTER IS UNDER DEVELOPMENT.",frame);
  else if(screen==2)drawNotice(&scene,"CONTINUE","NO SAVE DATA FOUND.",frame);
  else if(screen==3)drawNotice(&scene,"OPTIONS","GAMEPLAY SETTINGS WILL BE ADDED HERE.",frame);
  else if(screen==4)drawNotice(&scene,"CREDITS","A RIDAGAMES PRODUCTION.",frame);
  else drawNotice(&scene,"QUIT","PRESS CIRCLE TO RETURN.",frame);
  scene.SubmitFlip(frame);
  scene.FrameWait(frame);
  scene.FrameBufferSwap();
  ++frame;
  sceKernelUsleep(16000);
 }
 return 0;
}