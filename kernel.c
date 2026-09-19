static inline unsigned char inb(unsigned short p){unsigned char r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r;}
static inline void outb(unsigned short p, unsigned char v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}

char* vga = (char*)0xB8000;
int cursor=0;
int color=0x07;

void scroll(){
 if(cursor<80*25) return;
 for(int i=0;i<80*24;i++){vga[i*2]=vga[(i+80)*2]; vga[i*2+1]=vga[(i+80)*2+1];}
 for(int i=80*24;i<80*25;i++){vga[i*2]=' '; vga[i*2+1]=color;}
 cursor=80*24;
}
void putc(char c){
 if(c=='\n'){cursor=(cursor/80+1)*80; scroll(); return;}
 if(c=='\b'){if(cursor>0){cursor--; vga[cursor*2]=' '; } return;}
 vga[cursor*2]=c; vga[cursor*2+1]=color; cursor++; scroll();
}
void print(char* s){for(int i=0;s[i];i++) putc(s[i]);}
void clear(){for(int i=0;i<80*25;i++){vga[i*2]=' '; vga[i*2+1]=0x07;} cursor=0;}

int strcmp(char* a,char* b){int i=0;while(a[i]&&b[i]&&a[i]==b[i])i++;return a[i]-b[i];}
int strncmp(char* a,char* b,int n){for(int i=0;i<n;i++){if(a[i]!=b[i])return a[i]-b[i]; if(!a[i])return 0;}return 0;}
void strcpy(char* d,char* s){int i=0;while(s[i]){d[i]=s[i];i++;}d[i]=0;}

void cmd_browser(char* url){
 clear();
 color=0x1F; print(" ");
 print(" Orvyn Browser 0.0.5 - [X] ");
 print(" URL: "); if(url[0]) print(url); else print("orvyn://home");
 print(" ");
 color=0x07;
 print("\n\n");
 if(!url[0] || strncmp(url,"home",4)==0){
   print(" Welcome to OrvynOS Web\n");
   print(" ----------------------\n");
   print(" Try: browser help | browser about | browser google.com\n\n");
   print(" [Orvyn Search] > Type something and press ENTER is not working yet\n");
 } else if(strncmp(url,"help",4)==0){
   print(" Browser Commands:\n browser <url> - visit url\n browser home - homepage\n");
 } else if(strncmp(url,"about",5)==0){
   print(" OrvynOS 0.0.5 Text Browser\n No TCP/IP yet, this is a mock renderer.\n Next: add RTL8139 driver.\n");
 } else {
   print(" Loading "); print(url); print("...\n\n");
   print(" +------------------------------------------------+\n");
   print(" | This is a simulated page for "); print(url); print("\n");
   print(" | Content would be rendered here in text mode. |\n");
   print(" | No internet yet - OrvynOS is offline. |\n");
   print(" +------------------------------------------------+\n");
 }
 print("\n Press 'q' to exit browser\n");
}

char input_buf[128];
int input_len=0;

void exec_cmd(){
 print("\n");
 if(input_len==0) return;
 input_buf[input_len]=0;
 if(strcmp(input_buf,"clear")==0){clear();}
 else if(strcmp(input_buf,"help")==0){
   print("Commands: clear, help, echo <text>, browser <url>, about, reboot\n");
 }
 else if(strncmp(input_buf,"echo ",5)==0){print(input_buf+5); print("\n");}
 else if(strncmp(input_buf,"browser",7)==0){
   char* url=input_buf+7; while(*url==' ')url++;
   char c=0; int waiting=1;
   cmd_browser(url);
   while(waiting){
     if(inb(0x64)&1){
       unsigned char sc=inb(0x60);
       if(sc==0x10) waiting=0; // q
     }
   }
   clear();
 }
 else if(strcmp(input_buf,"about")==0){print("OrvynOS 0.0.4 -> 0.0.5 by Gabriel\n");}
 else if(strcmp(input_buf,"reboot")==0){outb(0x64,0xFE); while(1);}
 else {print("Unknown: "); print(input_buf); print("\n");}
}

void kernel_main(){
 clear();
 color=0x0A; print("OrvynOS 0.0.5 [SHELL READY]\n"); color=0x07;
 print("Type help for commands\n\n> ");
 char map[128]={0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,'q','w','e','r','t','y','u','i','o','p','[',']',0,0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,0,0,' '};
 while(1){
   if(inb(0x64)&1){
     unsigned char sc=inb(0x60);
     if(sc==0x1C){ // enter
       exec_cmd(); print("\n> "); input_len=0;
     } else if(sc==0x0E){ // backspace
       if(input_len>0){input_len--; putc('\b');}
     } else if(sc<128 && map[sc]){
       if(input_len<127){input_buf[input_len++]=map[sc]; putc(map[sc]);}
     }
   }
 }
}