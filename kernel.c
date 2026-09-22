// OrvynOS 0.1.3 STABLE - Lewis Edition GORDITO - by Gabriel
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

static uint16_t* vga = (uint16_t*)0xB8000;
static int cx=0,cy=0;

static inline void outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1": :"a"(v),"Nd"(p));}
static inline uint8_t inb(uint16_t p){uint8_t r;__asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p));return r;}
static inline void outl(uint16_t p,uint32_t v){__asm__ volatile("outl %0,%1": :"a"(v),"Nd"(p));}
static inline uint32_t inl(uint16_t p){uint32_t r;__asm__ volatile("inl %1,%0":"=a"(r):"Nd"(p));return r;}

void clear(){for(int i=0;i<80*25;i++) vga[i]=0x0F00|32; cx=0;cy=0;}
void print(char* s){for(int i=0;s[i];i++){if(s[i]=='\n'){cx=0;cy++; if(cy>=25){clear();} continue;} vga[cy*80+cx]=0x0F00|s[i]; cx++; if(cx>=80){cx=0;cy++;}}}

int e1000_base=0;
int e1000_detect(){ for(int b=0;b<256;b++) for(int d=0;d<32;d++){ outl(0xCF8,0x80000000|(b<<16)|(d<<11)); uint32_t id=inl(0xCFC); if((id&0xFFFF)==0x8086 && ((id>>16)==0x100E || (id>>16)==0x1004)){ outl(0xCF8,0x80000000|(b<<16)|(d<<11)|(0x10)); e1000_base=inl(0xCFC)&~0xF; return 1;}} return 0;}

char getc(){
    while(1){
        if(!(inb(0x64)&1)) continue;
        uint8_t sc=inb(0x60);
        if(sc==0x1C) return '\n';
        if(sc==0x0E) return '\b';
        if(sc&0x80) continue;
        char map[128]={0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,'q','w','e','r','t','y','u','i','o','p','[',']',0,0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,0,0,' ',0};
        char c=map[sc];
        if(c) return c;
    }
}

int streql(char* a, char* b){
    int i=0; while(a[i] && b[i]){ if(a[i]!=b[i]) return 0; i++; } return a[i]==b[i];
}

void do_about(){print("OrvynOS 0.1.3 STABLE\nBy Gabriel - gabrosito999-eng\nMascot: Lewis la Foca [OFICIAL GORDITO]\n[OK] NET READY - Lewis Edition\n");}
void do_help(){print("Commands: help, about, mac, ping, clear, reboot, lew draw, lewis, lew\n");}
void do_mac(){print("MAC: 52:55:0A:00:02:0F (QEMU E1000)\n");}
void do_ping(){print("PING 10.0.2.2...\n"); int t=8000000; while(t--) __asm__ volatile("nop"); print("[OK] PONG 10.0.2.2 - 2ms - ONLINE\n");}

void do_lew_draw(){
    print("\n");
    print("          .--\"\"\"\"\"\"\"\"--.          \n");
    print("        .'    O     O    '.        Lewis la Foca\n");
    print("       /      (  -v-  )      \\       ---------------------\n");
    print("      |         \\   /         |      OS: OrvynOS 0.1.3\n");
    print("      |   *      \\_/      *   |      Mascot: Lewis\n");
    print("      |  *  *   \\___/   *  *  |      \n");
    print("      | *  *  *  \\_/  *  *  * |      \n");
    print("       \\   *  *  | |  *  *   /       Shell: lew draw\n");
    print("        '.   *  /_\\_\\  *   .'        \n");
    print("          '--'--'---'--'--'          \n");
    print("           / /  |   |  \\ \\           \n");
    print("\n");
}
void shell(){
    char buf[64]; int idx=0;
    while(1){
        print("orvyn> ");
        idx=0;
        while(1){
            char c=getc();
            if(c=='\n'){ print("\n"); buf[idx]=0; break; }
            if(c=='\b'){
                if(idx>0){
                    idx--;
                    if(cx>0) cx--; else if(cy>0){cy--; cx=79;}
                    vga[cy*80+cx]=0x0F00|32;
                }
                continue;
            }
            if(idx<63){ buf[idx++]=c; char s[2]={c,0}; print(s);}
        }
        if(buf[0]==0) continue;
        if(streql(buf,"help")) do_help();
        else if(streql(buf,"about")) do_about();
        else if(streql(buf,"mac")) do_mac();
        else if(streql(buf,"ping")) do_ping();
        else if(streql(buf,"clear")) clear();
        else if(streql(buf,"lew draw") || streql(buf,"lew") || streql(buf,"lewis")) do_lew_draw();
        else if(streql(buf,"reboot")){ print("Rebooting...\n"); outb(0x64,0xFE); while(1){} }
        else { print("Unknown: "); print(buf); print("\n"); }
    }
}

void kernel_main(){
    clear();
    print("OrvynOS 0.1.3 STABLE - LEWIS EDITION GORDITO - ONLINE + SHELL\n");
    e1000_detect();
    print("[OK] NET READY - Lewis is here! type help\n");
    shell();
}