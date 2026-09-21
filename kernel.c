typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32; typedef unsigned long long u64;
static inline void outl(u16 p, u32 v){ __asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p)); }
static inline u32 inl(u16 p){ u32 r; __asm__ volatile("inl %1,%0":"=a"(r):"Nd"(p)); return r; }
void print(char* s,int y,u8 c){ char* v=(char*)0xB8000; int pos=y*80; for(int i=0;s[i];i++){ v[pos*2]=s[i]; v[pos*2+1]=c; pos++; } }
u16 checksum(u8* d,int l){ u32 s=0; for(int i=0;i<l;i+=2) s+=(d[i]<<8)+(i+1<l?d[i+1]:0); while(s>>16) s=(s&0xFFFF)+(s>>16); return ~s&0xFFFF; }
struct rx_desc{ u64 addr; u16 len; u16 csum; u8 status; u8 err; u16 special; } __attribute__((packed));
struct tx_desc{ u64 addr; u16 len; u8 cso; u8 cmd; u8 status; u8 css; u16 special; } __attribute__((packed));
struct rx_desc* rx_ring = (struct rx_desc*)0x200000;
struct tx_desc* tx_ring = (struct tx_desc*)0x201000;
u8* rx_bufs = (u8*)0x202000;
u8* tx_buf = (u8*)0x300000;
volatile u32* e1000;
void send_pkt(u8* data, int len, int idx){
    for(int i=0;i<len;i++) tx_buf[i]=data[i];
    tx_ring[idx].addr=(u64)(u32)tx_buf;
    tx_ring[idx].len=len;
    tx_ring[idx].cmd=0x0B;
    tx_ring[idx].status=0;
    e1000[0x3818/4]= (idx+1)%8;
    while(!(tx_ring[idx].status & 1)){}
}
void kernel_main(){
    char* vga=(char*)0xB8000; for(int i=0;i<80*25;i++){ vga[i*2]=' '; vga[i*2+1]=0x07; }
    print("OrvynOS 0.1.0 - V4.2 PONG FINAL",0,0x0A);
    u32 bar=0;
    for(int dev=0;dev<32;dev++){ outl(0xCF8,0x80000000|(dev<<11)); u32 id=inl(0xCFC); if(id==0xFFFFFFFF) continue; if((id&0xFFFF)==0x8086 && (id>>16)==0x100E){ outl(0xCF8,0x80000000|(dev<<11)|0x04); outl(0xCFC,inl(0xCFC)|0x07); outl(0xCF8,0x80000000|(dev<<11)|0x10); bar=inl(0xCFC)&~0xF; break; } }
    if(!bar){ print("No e1000",1,0x0C); while(1){} }
    e1000=(volatile u32*)bar;
    e1000[0]|=0x04000000; for(int i=0;i<100000;i++) __asm__ volatile("nop");
    for(int i=0;i<32;i++){ rx_ring[i].addr=(u64)(u32)(rx_bufs+i*2048); rx_ring[i].status=0; }
    e1000[0x2800/4]=(u32)rx_ring; e1000[0x2804/4]=0; e1000[0x2808/4]=32*16; e1000[0x2810/4]=0; e1000[0x2818/4]=31;
    e1000[0x100/4]=0x00008002;
    for(int i=0;i<8;i++){ tx_ring[i].addr=0; tx_ring[i].status=1; tx_ring[i].len=0; }
    e1000[0x3800/4]=(u32)tx_ring; e1000[0x3804/4]=0; e1000[0x3808/4]=8*16; e1000[0x3810/4]=0; e1000[0x3818/4]=0;
    e1000[0x400/4]=0x0000000A | (1<<1) | (1<<3);
    u8 mymac[6]={0x52,0x54,0x00,0x12,0x34,0x56};
    u8 gw[6]={0x52,0x55,0x0A,0x00,0x02,0x02};
    u8 pkt[60]={0}; for(int i=0;i<6;i++) pkt[i]=gw[i]; for(int i=0;i<6;i++) pkt[6+i]=mymac[i]; pkt[12]=0x08; pkt[13]=0x00;
    pkt[14]=0x45; pkt[15]=0; pkt[16]=0; pkt[17]=0x1C; pkt[18]=0; pkt[19]=1; pkt[20]=0; pkt[21]=0; pkt[22]=64; pkt[23]=1; pkt[24]=0; pkt[25]=0;
    pkt[26]=10; pkt[27]=0; pkt[28]=2; pkt[29]=15; pkt[30]=10; pkt[31]=0; pkt[32]=2; pkt[33]=2;
    u16 ip_c=checksum(&pkt[14],20); pkt[24]=ip_c>>8; pkt[25]=ip_c&0xFF;
    pkt[34]=8; pkt[35]=0; pkt[36]=0; pkt[37]=0; pkt[38]=0x12; pkt[39]=0x34; pkt[40]=0; pkt[41]=1;
    u16 ic_c=checksum(&pkt[34],8); pkt[36]=ic_c>>8; pkt[37]=ic_c&0xFF;
    print("Enviando PING a 10.0.2.2...",1,0x07);
    send_pkt(pkt,42,0);
    print("TX OK! Esperando...",2,0x0A);
    int rx_idx=0;
    while(1){
        if(rx_ring[rx_idx].status & 1){
            u8* eth = rx_bufs + rx_idx*2048;
            u16 et = (eth[12]<<8)|eth[13];
            if(et==0x0806){
                print("ARP reply OK, reenviando PING",3,0x0E);
                u8 rep[42]={0}; for(int i=0;i<6;i++) rep[i]=eth[6+i]; for(int i=0;i<6;i++) rep[6+i]=mymac[i]; rep[12]=0x08; rep[13]=0x06;
                rep[14]=0; rep[15]=1; rep[16]=0x08; rep[17]=0; rep[18]=6; rep[19]=4; rep[20]=0; rep[21]=2;
                for(int i=0;i<6;i++) rep[22+i]=mymac[i]; rep[28]=10; rep[29]=0; rep[30]=2; rep[31]=15;
                for(int i=0;i<6;i++) rep[32+i]=eth[6+i]; rep[38]=10; rep[39]=0; rep[40]=2; rep[41]=2;
                send_pkt(rep,42,1);
                for(volatile int d=0; d<8000000; d++) __asm__ volatile("nop");
                send_pkt(pkt,42,0);
                print("PING reenviado!",4,0x07);
            } else if(et==0x0800 && eth[23]==1 && eth[34]==0){
                print("PONG RECIBIDO DE 10.0.2.2!",5,0x0A);
                print("[OK] ORVYNOS ONLINE! 0.1.0",6,0x0C);
                print("CINE WEON",7,0x0E);
                break;
            }
            rx_ring[rx_idx].status=0;
            rx_idx=(rx_idx+1)%32;
            e1000[0x2818/4]= (rx_idx+31)%32;
        }
    }
    while(1){}
}