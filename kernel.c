// OrvynOS 0.1.1 STABLE - by gabrosito999
// Clean e1000 + ARP + ICMP - MAC autodetect
#include <stdint.h> // vamos a definir nosotros igual por si no tienes libc
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32; typedef unsigned long long u64;

static inline void outl(u16 p, u32 v){ __asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p)); }
static inline u32 inl(u16 p){ u32 r; __asm__ volatile("inl %1,%0":"=a"(r):"Nd"(p)); return r; }

void kprint(char* s,int y,u8 c){ char* v=(char*)0xB8000; int pos=y*80; for(int i=0;s[i];i++){ v[pos*2]=s[i]; v[pos*2+1]=c; pos++; } }
void kprint_hex(u32 val, int y, int x, u8 c){ char* vga=(char*)0xB8000; char* h="0123456789ABCDEF"; for(int i=7;i>=0;i--){ vga[(y*80+x+i)*2]=h[(val>>((7-i)*4))&0xF]; vga[(y*80+x+i)*2+1]=c; } }
u16 iphdr_csum(u8* d,int l){ u32 s=0; for(int i=0;i<l;i+=2) s+=(d[i]<<8)+(i+1<l?d[i+1]:0); while(s>>16) s=(s&0xFFFF)+(s>>16); return (~s)&0xFFFF; }
void kdelay(int c){ for(volatile int i=0;i<c;i++) __asm__ volatile("nop"); }

struct rx_desc{ u64 addr; u16 len; u16 csum; u8 status; u8 err; u16 special; } __attribute__((packed));
struct tx_desc{ u64 addr; u16 len; u8 cso; u8 cmd; u8 status; u8 css; u16 special; } __attribute__((packed));

struct rx_desc* rx_ring = (struct rx_desc*)0x200000;
struct tx_desc* tx_ring = (struct tx_desc*)0x201000;
u8* rx_bufs = (u8*)0x202000;
u8* tx_buf = (u8*)0x300000;
volatile u32* e1000;
u8 mymac[6];
int tx_cur=0;

void e1000_send(u8* data, int len){
    for(int i=0;i<len;i++) tx_buf[i]=data[i];
    tx_ring[tx_cur].addr=(u64)(u32)tx_buf;
    tx_ring[tx_cur].len=len;
    tx_ring[tx_cur].cmd=0x0B; // EOP | IFCS | RS
    tx_ring[tx_cur].status=0;
    int old=tx_cur;
    tx_cur=(tx_cur+1)%8;
    e1000[0x3818/4]=tx_cur; // TDT
    while(!(tx_ring[old].status & 0x01)){}
}

void kernel_main(){
    char* vga=(char*)0xB8000; for(int i=0;i<80*25;i++){ vga[i*2]=' '; vga[i*2+1]=0x07; }
    kprint("OrvynOS 0.1.1 STABLE - ONLINE",0,0x0B);

    u32 bar=0;
    for(int dev=0;dev<32;dev++){
        outl(0xCF8,0x80000000|(dev<<11));
        u32 id=inl(0xCFC);
        if(id==0xFFFFFFFF) continue;
        if((id&0xFFFF)==0x8086 && (id>>16)==0x100E){
            outl(0xCF8,0x80000000|(dev<<11)|0x04); outl(0xCFC,inl(0xCFC)|0x07);
            outl(0xCF8,0x80000000|(dev<<11)|0x10); bar=inl(0xCFC)&~0xF; break;
        }
    }
    if(!bar){ kprint("ERR: e1000 not found",1,0x0C); while(1){} }
    e1000=(volatile u32*)bar;

    // 0.1.1: Auto-detect MAC REAL
    u32 ral=e1000[0x5400/4]; u32 rah=e1000[0x5404/4];
    mymac[0]=ral&0xFF; mymac[1]=(ral>>8)&0xFF; mymac[2]=(ral>>16)&0xFF; mymac[3]=(ral>>24)&0xFF; mymac[4]=rah&0xFF; mymac[5]=(rah>>8)&0xFF;

    kprint("e1000 BAR: 0x",1,0x07); kprint_hex(bar,1,12,0x0F);
    kprint("MAC: : : : : : ",2,0x07);
    for(int i=0;i<6;i++) kprint_hex(mymac[i],2,5+i*4,0x0A);

    // Reset e init rings
    e1000[0]|=0x04000000; kdelay(100000);
    for(int i=0;i<32;i++){ rx_ring[i].addr=(u64)(u32)(rx_bufs+i*2048); rx_ring[i].status=0; }
    e1000[0x2800/4]=(u32)rx_ring; e1000[0x2804/4]=0; e1000[0x2808/4]=32*16; e1000[0x2810/4]=0; e1000[0x2818/4]=31;
    e1000[0x100/4]=0x00008002;
    for(int i=0;i<8;i++){ tx_ring[i].status=1; }
    e1000[0x3800/4]=(u32)tx_ring; e1000[0x3804/4]=0; e1000[0x3808/4]=8*16; e1000[0x3810/4]=0; e1000[0x3818/4]=0;
    e1000[0x400/4]=0x0000000A | (1<<1) | (1<<3);

    // Construir PING
    u8 gw[6]={0x52,0x55,0x0A,0x00,0x02,0x02};
    u8 pkt[60]={0};
    for(int i=0;i<6;i++) pkt[i]=gw[i];
    for(int i=0;i<6;i++) pkt[6+i]=mymac[i];
    pkt[12]=0x08; pkt[13]=0x00; pkt[14]=0x45; pkt[15]=0; pkt[16]=0; pkt[17]=0x1C; pkt[18]=0; pkt[19]=1; pkt[20]=0; pkt[21]=0; pkt[22]=64; pkt[23]=1; pkt[24]=0; pkt[25]=0;
    pkt[26]=10; pkt[27]=0; pkt[28]=2; pkt[29]=15; pkt[30]=10; pkt[31]=0; pkt[32]=2; pkt[33]=2;
    u16 ipc=iphdr_csum(&pkt[14],20); pkt[24]=ipc>>8; pkt[25]=ipc&0xFF;
    pkt[34]=8; pkt[35]=0; pkt[36]=0; pkt[37]=0; pkt[38]=0x12; pkt[39]=0x34; pkt[40]=0; pkt[41]=1;
    u16 icc=iphdr_csum(&pkt[34],8); pkt[36]=icc>>8; pkt[37]=icc&0xFF;

    kprint("PING 10.0.2.2...",3,0x07);
    e1000_send(pkt,42);

    int rx_tail=0; int got=0;
    while(!got){
        if(rx_ring[rx_tail].status & 1){
            u8* eth=rx_bufs+rx_tail*2048;
            u16 type=(eth[12]<<8)|eth[13];
            if(type==0x0806){
                kprint("ARP REQ -> Replying...",4,0x0E);
                u8 rep[42]={0}; for(int i=0;i<6;i++) rep[i]=eth[6+i]; for(int i=0;i<6;i++) rep[6+i]=mymac[i]; rep[12]=0x08; rep[13]=0x06;
                rep[14]=0; rep[15]=1; rep[16]=0x08; rep[17]=0; rep[18]=6; rep[19]=4; rep[20]=0; rep[21]=2;
                for(int i=0;i<6;i++) rep[22+i]=mymac[i]; rep[28]=10; rep[29]=0; rep[30]=2; rep[31]=15; for(int i=0;i<6;i++) rep[32+i]=eth[6+i]; rep[38]=10; rep[39]=0; rep[40]=2; rep[41]=2;
                e1000_send(rep,42); kdelay(8000000); e1000_send(pkt,42);
            } else if(type==0x0800 && eth[23]==1 && eth[34]==0){
                kprint("PONG RECIBIDO DE 10.0.2.2!",4,0x0A);
                kprint("[OK] ORVYNOS 0.1.1 STABLE ONLINE",5,0x0A);
                kprint("TX:1 RX:1 ARP:1 ICMP:1",6,0x07);
                got=1;
            }
            rx_ring[rx_tail].status=0;
            rx_tail=(rx_tail+1)%32;
            e1000[0x2818/4]=(rx_tail+31)%32;
        }
    }
    while(1){}
}