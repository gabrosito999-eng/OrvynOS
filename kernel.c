// OrvynOS 0.3 alpha - SNAKE EDITION LENTO - Gabriel Ancud
#include <stdint.h>
#include <stdbool.h>
static inline void outb(uint16_t p, uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p){ uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }
static inline void outl(uint16_t p, uint32_t v){ __asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p)); }
static inline uint32_t inl(uint16_t p){ uint32_t r; __asm__ volatile("inl %1,%0":"=a"(r):"Nd"(p)); return r; }

static uint16_t* VGA=(uint16_t*)0xB8000;
static uint16_t shadow[80*25];
int mouse_x=40, mouse_y=12; bool mouse_left=false; bool need_redraw=true; int selected=7; int last_mx=-1, last_my=-1; int acc_x=0, acc_y=0; bool in_desktop=false; char term_buf[64]; int term_len=0; char term_lines[10][80]; int term_line_count=0;

void put_at(int x,int y,uint8_t c,char ch){ if(x<0||x>=80||y<0||y>=25) return; shadow[y*80+x]=(c<<8)|ch; }
void put_at_real(int x,int y,uint8_t c,char ch){ if(x<0||x>=80||y<0||y>=25) return; VGA[y*80+x]=(c<<8)|ch; }
void clear_shadow(uint8_t c){ for(int i=0;i<80*25;i++) shadow[i]=(c<<8)|' '; }
void flush_shadow(){ for(int i=0;i<80*25;i++) VGA[i]=shadow[i]; }
void draw_rect(int x,int y,int w,int h,uint8_t col){ for(int j=y;j<y+h;j++) for(int i=x;i<x+w;i++) put_at(i,j,col,' '); }
void draw_window(int x,int y,int w,int h,uint8_t bg,uint8_t bd){ draw_rect(x,y,w,h,bg); for(int i=x;i<x+w;i++){ put_at(i,y,bd,'-'); put_at(i,y+h-1,bd,'-'); } for(int j=y;j<y+h;j++){ put_at(x,j,bd,'|'); put_at(x+w-1,j,bd,'|'); } put_at(x,y,bd,'+'); put_at(x+w-1,y,bd,'+'); put_at(x,y+h-1,bd,'+'); put_at(x+w-1,y+h-1,bd,'+'); }
void puts_at(int x,int y,uint8_t c,const char* s){ for(int i=0;s[i];i++) put_at(x+i,y,c,s[i]); }

void sleep_ms_nopoll(int ms){ for(volatile int i=0;i<ms*25000;i++) __asm__ volatile("nop"); }
void pc_beep(int f){ if(f==0){ outb(0x61,inb(0x61)&0xFC); return; } int d=1193180/f; outb(0x43,0xB6); outb(0x42,d&0xFF); outb(0x42,(d>>8)&0xFF); outb(0x61,inb(0x61)|3); }
void no_beep(){ pc_beep(0); }
void lew_boot_sound(){ pc_beep(523); sleep_ms_nopoll(120); pc_beep(659); sleep_ms_nopoll(120); pc_beep(784); sleep_ms_nopoll(120); pc_beep(1046); sleep_ms_nopoll(250); no_beep(); sleep_ms_nopoll(50); pc_beep(784); sleep_ms_nopoll(80); pc_beep(1046); sleep_ms_nopoll(350); no_beep(); }
void click_sound(){ pc_beep(1800); sleep_ms_nopoll(12); no_beep(); }
void eat_sound(){ pc_beep(1200); sleep_ms_nopoll(30); pc_beep(1800); sleep_ms_nopoll(30); no_beep(); }
void die_sound(){ pc_beep(400); sleep_ms_nopoll(100); pc_beep(200); sleep_ms_nopoll(200); no_beep(); }

void mouse_wait(uint8_t t){ int to=100000; while(to--){ if((t==0&&!(inb(0x64)&1))||(t==1&&!(inb(0x64)&2))) return; } }
void mouse_write(uint8_t a){ mouse_wait(1); outb(0x64,0xD4); mouse_wait(1); outb(0x60,a); }
uint8_t mouse_read(){ mouse_wait(0); return inb(0x60); }
void mouse_init(){ mouse_wait(1); outb(0x64,0xA8); mouse_wait(1); outb(0x64,0x20); mouse_wait(0); uint8_t s=inb(0x60)|2; mouse_wait(1); outb(0x64,0x60); mouse_wait(1); outb(0x60,s); mouse_write(0xF6); mouse_read(); mouse_write(0xF4); mouse_read(); }
int mouse_cyc=0; uint8_t mouse_packet[3];
void mouse_poll(){ while(inb(0x64)&1){ if(!(inb(0x64)&0x20)){ inb(0x60); continue; } uint8_t b=inb(0x60); if(mouse_cyc==0 &&!(b&0x08)) continue; mouse_packet[mouse_cyc]=b; mouse_cyc++; if(mouse_cyc==3){ mouse_cyc=0; uint8_t status=mouse_packet[0]; if(status&0xC0) continue; int dx=(int8_t)mouse_packet[1]; int dy=(int8_t)mouse_packet[2]; bool nl=status&1; acc_x+=dx; acc_y+=dy; int mx=acc_x/2; int my=acc_y/2; if(mx!=0||my!=0){ acc_x-=mx*2; acc_y-=my*2; int nx=mouse_x+mx; int ny=mouse_y-my; if(nx<0) nx=0; if(nx>=80) nx=79; if(ny<1) ny=1; if(ny>=25) ny=24; if(nx!=mouse_x||ny!=mouse_y){ mouse_x=nx; mouse_y=ny; need_redraw=true; } } if(nl!=mouse_left){ mouse_left=nl; need_redraw=true; } } } }

uint32_t e1000_base=0; bool e1000_found=false; uint32_t pci_read(uint8_t b,uint8_t s,uint8_t f,uint8_t o){ outl(0xCF8,(1<<31)|(b<<16)|(s<<11)|(f<<8)|o); return inl(0xCFC); }
void e1000_init(){ for(int bus=0;bus<8;bus++) for(int slot=0;slot<32;slot++){ uint32_t id=pci_read(bus,slot,0,0); if((id&0xFFFF)==0x8086 && ((id>>16)==0x100E || (id>>16)==0x1004 || (id>>16)==0x100F)){ uint32_t bar=pci_read(bus,slot,0,0x10)&0xFFFFFFF0; if(bar==0) continue; e1000_base=bar; e1000_found=true; uint32_t cmd=pci_read(bus,slot,0,4); cmd|=0x7; outl(0xCF8,(1<<31)|(bus<<16)|(slot<<11)|4); outl(0xCFC,cmd); return; } } }

char kbd_get_char(){ if(!(inb(0x64)&1)) return 0; if(inb(0x64)&0x20) return 0; uint8_t sc=inb(0x60); if(sc&0x80) return 0; if(sc==0x01) return 27; if(sc==0x1C) return '\n'; if(sc==0x0E) return '\b'; if(sc==0x39) return ' '; switch(sc){ case 0x02: return '1'; case 0x03: return '2'; case 0x04: return '3'; case 0x05: return '4'; case 0x06: return '5'; case 0x07: return '6'; case 0x08: return '7'; case 0x09: return '8'; case 0x0A: return '9'; case 0x0B: return '0'; case 0x10: return 'q'; case 0x11: return 'w'; case 0x12: return 'e'; case 0x13: return 'r'; case 0x14: return 't'; case 0x15: return 'y'; case 0x16: return 'u'; case 0x17: return 'i'; case 0x18: return 'o'; case 0x19: return 'p'; case 0x1E: return 'a'; case 0x1F: return 's'; case 0x20: return 'd'; case 0x21: return 'f'; case 0x22: return 'g'; case 0x23: return 'h'; case 0x24: return 'j'; case 0x25: return 'k'; case 0x26: return 'l'; case 0x2C: return 'z'; case 0x2D: return 'x'; case 0x2E: return 'c'; case 0x2F: return 'v'; case 0x30: return 'b'; case 0x31: return 'n'; case 0x32: return 'm'; default: return 0; } }
char kbd_poll(){ if(!(inb(0x64)&1)) return 0; if(inb(0x64)&0x20) return 0; uint8_t sc=inb(0x60); if(sc==0x48) return 'w'; if(sc==0x50) return 's'; if(sc==0x1E) return 'a'; if(sc==0x20) return 'd'; if(sc==0x13) return 'r'; if(sc==0x01) return 27; if(sc&0x80) return 0; return 0; }
bool str_eq(const char* a,const char* b){ int i=0; while(a[i]&&b[i]){ if(a[i]!=b[i]) return false; i++; } return a[i]==b[i]; }

// ===== SNAKE LENTO =====
#define SNAKE_W 32
#define SNAKE_H 14
int snake_x[128]; int snake_y[128]; int snake_len=4; int snake_dir=1;
int food_x=10, food_y=5; int score=0; bool game_over=false; int tick=0;
uint32_t rnd_seed=12345;
int rnd(int max){ rnd_seed=rnd_seed*1103515245+12345; return (rnd_seed>>16)%max; }
void snake_reset(){ snake_len=4; score=0; game_over=false; snake_dir=1; for(int i=0;i<4;i++){ snake_x[i]=8-i; snake_y[i]=7; } food_x=15; food_y=7; tick=0; }
void snake_spawn_food(){ food_x=2+rnd(SNAKE_W-4); food_y=2+rnd(SNAKE_H-4); }
void snake_update(){
    if(game_over) return;
    tick++;
    int speed = 28 - (score/30); // lento al inicio, se acelera de a poco
    if(speed<8) speed=8;
    if(tick<speed) return;
    tick=0;
    int nx=snake_x[0]; int ny=snake_y[0];
    if(snake_dir==0) ny--; if(snake_dir==1) nx++; if(snake_dir==2) ny++; if(snake_dir==3) nx--;
    if(nx<0||nx>=SNAKE_W||ny<0||ny>=SNAKE_H){ game_over=true; die_sound(); return; }
    for(int i=0;i<snake_len;i++) if(snake_x[i]==nx && snake_y[i]==ny){ game_over=true; die_sound(); return; }
    bool ate = (nx==food_x && ny==food_y);
    for(int i=snake_len;i>0;i--){ snake_x[i]=snake_x[i-1]; snake_y[i]=snake_y[i-1]; }
    snake_x[0]=nx; snake_y[0]=ny;
    if(ate){ snake_len++; score+=10; eat_sound(); if(snake_len>120) snake_len=120; snake_spawn_food(); }
}

void render_terminal(){ clear_shadow(0x07); puts_at(0,0,0x1F," OrvynOS 0.3 Snake LENTO - Terminal - Ancud "); puts_at(0,1,0x07," OrvynOS [Version 0.3]"); for(int i=0;i<term_line_count && i<10;i++) puts_at(0,4+i,0x07,term_lines[i]); char prompt[80]=" root@orvyn:~$ "; int l=0; while(prompt[l]) l++; for(int i=0;i<term_len;i++) prompt[l+i]=term_buf[i]; prompt[l+term_len]='_'; prompt[l+term_len+1]=0; puts_at(0,4+term_line_count,0x07,prompt); puts_at(0,23,0x0F,"Escribe 'boot desktop' - SNAKE! lento"); flush_shadow(); }

void build_desktop(){
    clear_shadow(0x1F); draw_rect(0,0,80,1,0x4F); puts_at(0,0,0x4F," OrvynOS 0.3 Snake LENTO - ESC=terminal - Ancud ");
    draw_window(2,2,28,16,0x2F,0x2F); draw_rect(4,2,24,1,0x70); puts_at(4,2,0x70," Orvyn Menu ");
    draw_window(35,2,42,20,0x0F,0x0F); draw_rect(37,2,38,1,0x70);
    const char* titles[]={"Terminal","File Manager","Browser","Settings","Lew :3","ifconfig","ping","SNAKE - Lew Game"};
    puts_at(37,2,0x70,titles[selected]);
    for(int i=0;i<8;i++){ int y=4+i; const char* its[]={"Terminal","Files","Browser","Settings","Lew :3","ifconfig","ping","SNAKE!"}; if(i==selected){ draw_rect(3,y,24,1,0xF2); puts_at(5,y,0xF2,its[i]); } else { draw_rect(3,y,24,1,0x2F); puts_at(5,y,0x2F,its[i]); } }
    draw_rect(36,4,40,17,0x0F);
    if(selected==7){
        draw_rect(37,4,34,16,0x00);
        puts_at(37,4,0x0F," SNAKE - Score: "); char sc[10]; int s=score; for(int i=4;i>=0;i--){ sc[i]='0'+s%10; s/=10; } sc[5]=0; puts_at(53,4,0x0E,sc);
        if(game_over){ puts_at(40,11,0x4F," GAME OVER - R para retry "); }
        for(int x=0;x<SNAKE_W;x++){ put_at(37+x,5,0x08,'#'); put_at(37+x,5+SNAKE_H,0x08,'#'); }
        for(int y=0;y<SNAKE_H+1;y++){ put_at(37,y+5,0x08,'#'); put_at(37+SNAKE_W-1,y+5,0x08,'#'); }
        put_at(37+food_x,6+food_y,0x0C,'*');
        for(int i=0;i<snake_len;i++){ uint8_t col = (i==0)?0x0A:0x02; char ch=(i==0)?'O':'o'; put_at(37+snake_x[i],6+snake_y[i],col,ch); }
        puts_at(37,21,0x07,"WASD mover - R retry - Lento Edition");
    } else {
        if(selected==0){ puts_at(37,4,0x0F,"Terminal v0.3 Lento"); }
        else if(selected==1){ puts_at(37,4,0x0F,"File Manager"); }
        else if(selected==2){ puts_at(37,4,0x0F,"Browser - Online"); }
        else if(selected==3){ puts_at(37,4,0x0F,"Settings"); puts_at(37,6,0x07,"Snake Speed: SLOW"); }
        else if(selected==4){ puts_at(37,4,0x0F,".--\"\"\"--. / _ _ \\ Lew"); }
        else if(selected==5){ puts_at(37,4,0x0F,"ifconfig - 10.0.2.15"); }
        else if(selected==6){ puts_at(37,4,0x0F,"ping - 32ms"); }
    }
}
void render_desktop(bool force){ if(!force &&!need_redraw){ if(selected==7){ build_desktop(); flush_shadow(); put_at_real(mouse_x,mouse_y,0x0E,0x10); need_redraw=false; return; } if(last_mx>=0) VGA[last_my*80+last_mx]=shadow[last_my*80+last_mx]; put_at_real(mouse_x,mouse_y,0x0E,0x10); last_mx=mouse_x; last_my=mouse_y; return; } build_desktop(); flush_shadow(); put_at_real(mouse_x,mouse_y,0x0E,0x10); last_mx=mouse_x; last_my=mouse_y; need_redraw=false; }
void add_term_line(const char* s){ if(term_line_count<10){ int i=0; while(s[i]&&i<79){ term_lines[term_line_count][i]=s[i]; i++; } term_lines[term_line_count][i]=0; term_line_count++; } else { for(int i=0;i<9;i++){ int j=0; while(term_lines[i+1][j]){ term_lines[i][j]=term_lines[i+1][j]; j++; } term_lines[i][j]=0; } int i=0; while(s[i]&&i<79){ term_lines[9][i]=s[i]; i++; } term_lines[9][i]=0; } }

void kernel_main(){
    mouse_init(); e1000_init(); snake_reset(); term_line_count=0; add_term_line("Snake lento ON"); render_terminal(); bool was_down=false;
    while(1){
        char c=kbd_get_char();
        if(c){ if(!in_desktop){ if(c=='\b'){ if(term_len>0) term_len--; render_terminal(); } else if(c=='\n'){ term_buf[term_len]=0; add_term_line(term_buf); if(str_eq(term_buf,"boot desktop")){ in_desktop=true; lew_boot_sound(); while(inb(0x64)&1) inb(0x60); need_redraw=true; render_desktop(true); } else if(term_len>0){ add_term_line("Command not found"); render_terminal(); } term_len=0; if(!in_desktop) render_terminal(); } else if(term_len<60){ term_buf[term_len++]=c; render_terminal(); } } else { if(c==27){ in_desktop=false; add_term_line("Exited"); render_terminal(); } if(selected==7){ if(c=='w'&&snake_dir!=2) snake_dir=0; if(c=='s'&&snake_dir!=0) snake_dir=2; if(c=='a'&&snake_dir!=1) snake_dir=3; if(c=='d'&&snake_dir!=3) snake_dir=1; if(c=='r'){ snake_reset(); need_redraw=true; } } } }
        if(in_desktop){
            mouse_poll(); char k=kbd_poll();
            if(selected!=7){ if(k=='w'){ if(selected>0){ selected--; need_redraw=true; click_sound(); }} if(k=='s'){ if(selected<7){ selected++; need_redraw=true; click_sound(); }} }
            else { if(k=='w'&&snake_dir!=2) snake_dir=0; if(k=='s'&&snake_dir!=0) snake_dir=2; if(k=='a'&&snake_dir!=1) snake_dir=3; if(k=='d'&&snake_dir!=3) snake_dir=1; if(k=='r'){ snake_reset(); need_redraw=true; } snake_update(); need_redraw=true; }
            if(mouse_left &&!was_down){ if(mouse_x>=2 && mouse_x<30 && mouse_y>=4 && mouse_y<=11){ int ns=mouse_y-4; if(ns>=0&&ns<8){ selected=ns; need_redraw=true; click_sound(); if(selected==7) snake_reset(); } } }
            was_down=mouse_left; render_desktop(false);
        }
        for(volatile int i=0;i<25000;i++) __asm__ volatile("nop");
    }
}