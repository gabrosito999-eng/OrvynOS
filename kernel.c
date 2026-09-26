// OrvynOS 0.1.6 SELECTABLE - Apps con teclado
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32; typedef unsigned long long u64;
static inline void outb(u16 p,u8 v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline u8 inb(u16 p){u8 r;__asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p));return r;}

static u16* vga=(u16*)0xB8000; int cx=0,cy=0;
void clear_text(){for(int i=0;i<80*25;i++) vga[i]=0x0F00|' '; cx=0;cy=0;}
void put_char_at(int x,int y,char c,u8 col){if(x<0||x>=80||y<0||y>=25) return; vga[y*80+x]=(col<<8)|c;}
void print(char* s){for(int i=0;s[i];i++){if(s[i]=='\n'){cx=0;cy++; if(cy>=25) cy=24; continue;} put_char_at(cx,cy,s[i],0x0F); cx++; if(cx>=80){cx=0;cy++;}}}

int strcmp(char* a,char* b){int i=0; while(a[i]&&b[i]){if(a[i]!=b[i]) return 0; i++;} return a[i]==b[i];}
int starts_with(char* a,char* b){int i=0; while(b[i]){if(a[i]!=b[i]) return 0; i++;} return 1;}

#include "fs.h"
extern file_t files[16];

u8 get_scancode(){while(!(inb(0x64)&1)) ; return inb(0x60);}
char getc(){
    while(1){
        u8 sc=get_scancode();
        if(sc==0x1C) return '\n';
        if(sc==0x0E) return '\b';
        if(sc==0x01) return 27;
        if(sc&0x80) continue;
        char map[128]={0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,'q','w','e','r','t','y','u','i','o','p','[',']',0,0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,0,0,' ',0};
        if(map[sc]) return map[sc];
    }
}

void draw_box(int x,int y,int w,int h,u8 col){
    for(int i=0;i<w;i++){ put_char_at(x+i,y,i==0?'+':i==w-1?'+':'-',col); put_char_at(x+i,y+h-1,i==0?'+':i==w-1?'+':'-',col); }
    for(int j=1;j<h-1;j++){ put_char_at(x,y+j,'|',col); put_char_at(x+w-1,y+j,'|',col); for(int i=1;i<w-1;i++) put_char_at(x+i,y+j,' ',col); }
}
void draw_title(int x,int y,int w,char* t,u8 col){ for(int i=0;t[i]&&i<w-4;i++) put_char_at(x+2+i,y,t[i],col); }

void desktop_cine(){
    int selected=0;
    char* apps[4]={" Terminal"," Files"," Browser"," Settings"};
    int running=1;
    char welcome_msg[64]="Welcome to OrvynOS 0.1.6 CINE";
    char welcome_sub[64]="Select an App with ARROWS+ENTER";

    while(running){
        // fondo
        for(int yy=0;yy<25;yy++) for(int xx=0;xx<80;xx++) put_char_at(xx,yy,' ',0x1F);
        for(int xx=0;xx<80;xx++) put_char_at(xx,0,' ',0x4F);
        char* top=" OrvynOS 0.1.6 CINE++ - ARROWS:move ENTER:select ESC:exit ";
        for(int i=0;top[i];i++) put_char_at(i,0,top[i],0x4F);

        draw_box(2,2,20,14,0x2F); draw_title(2,2,20," Apps ",0x2F);
        for(int i=0;i<4;i++){
            u8 col = (i==selected)? 0xF2 : 0x2F;
            if(i==selected) for(int xx=3;xx<21;xx++) put_char_at(xx,4+i,' ',col);
            for(int j=0;apps[i][j];j++) put_char_at(4+j,4+i,apps[i][j],col);
        }

        draw_box(25,2,52,18,0x0F); draw_title(25,2,52," Welcome - OrvynOS ",0x0F);
        for(int j=0;welcome_msg[j];j++) put_char_at(27+j,4,welcome_msg[j],0x0F);
        for(int j=0;welcome_sub[j];j++) put_char_at(27+j,6,welcome_sub[j],0x0F);

        // dock
        for(int xx=0;xx<80;xx++) put_char_at(xx,24,' ',0x8F);
        char* dock="[T] Terminal [F] Files [C] Cine";
        for(int i=0;dock[i];i++) put_char_at(2+i,24,dock[i],0x8F);

        u8 sc=get_scancode();
        if(sc&0x80) continue;
        if(sc==0x01){ running=0; } // ESC
        else if(sc==0x48){ if(selected>0) selected--; } // up
        else if(sc==0x50){ if(selected<3) selected++; } // down
        else if(sc==0x1C){ // ENTER
            if(selected==0){ running=0; } // Terminal = volver a shell
            else if(selected==1){
                for(int j=0;j<64;j++){ welcome_msg[j]=0; welcome_sub[j]=0; }
                char* m=" Files:";
                int idx=27;
                for(int i=0;i<16;i++) if(files[i].used){
                    if(idx>70) break;
                    // simple list en welcome
                    for(int k=0;files[i].name[k];k++){ put_char_at(27+k,8+idx-27,files[i].name[k],0x0E); }
                    idx++;
                }
                char* list="readme.txt found - use cat in terminal";
                for(int j=0;list[j];j++) welcome_sub[j]=list[j];
                welcome_sub[32]=0;
                for(int j=0;m[j];j++) welcome_msg[j]=m[j];
            }
            else if(selected==2){
                char* b1="Browser not implemented yet"; char* b2="Coming in 0.1.7";
                int j=0; for(;b1[j];j++) welcome_msg[j]=b1[j]; welcome_msg[j]=0;
                for(j=0;b2[j];j++) welcome_sub[j]=b2[j]; welcome_sub[j]=0;
            }
            else if(selected==3){
                char* s1="Settings - OrvynOS CINE++"; char* s2="Version 0.1.6";
                int j=0; for(;s1[j];j++) welcome_msg[j]=s1[j]; welcome_msg[j]=0;
                for(j=0;s2[j];j++) welcome_sub[j]=s2[j]; welcome_sub[j]=0;
            }
        }
    }
    clear_text();
}

void shell(){
    char buf[64]; int idx=0;
    while(1){
        print("orvyn> "); idx=0;
        while(1){
            char c=getc();
            if(c=='\n'){print("\n"); buf[idx]=0; break;}
            if(c=='\b'){if(idx>0){idx--; if(cx>0){cx--; put_char_at(cx,cy,' ',0x0F);}} continue;}
            if(idx<63){buf[idx++]=c; char s[2]={c,0}; print(s);}
        }
        if(buf[0]==0) continue;
        if(strcmp(buf,"desktop")||strcmp(buf,"boot desktop")||strcmp(buf,"gui")){desktop_cine(); print("Back to shell\n"); continue;}
        if(strcmp(buf,"clear")||strcmp(buf,"cls")){clear_text(); continue;}
        if(strcmp(buf,"help")){print("Commands: help, about, ls, cat <file>, clear, desktop\n"); continue;}
        if(strcmp(buf,"about")){print("OrvynOS 0.1.6 CINE++ - selectable apps\n"); continue;}
        if(strcmp(buf,"ls")){fs_list(); continue;}
        if(starts_with(buf,"cat ")){fs_read(buf+4); continue;}
        if(strcmp(buf,"cine")){print("CINE++ MAX\n"); continue;}
        print("Unknown: "); print(buf); print("\n");
    }
}
void kernel_main(){clear_text(); print("OrvynOS 0.1.6 CINE++ booting...\n"); fs_init(); fs_create("readme.txt"); print("Type 'boot desktop' - ARROWS+ENTER inside\n"); shell();}