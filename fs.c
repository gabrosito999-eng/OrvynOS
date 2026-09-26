#include "fs.h"
extern void print(char*); extern void print_int(int);
int sl2(char* s){int i=0; while(s[i]) i++; return i;}
int sc2(char* a,char* b){int i=0; while(a[i]&&b[i]&&a[i]==b[i]) i++; return a[i]-b[i];}
void scpy2(char* d,char* s){int i=0; while(s[i]){d[i]=s[i]; i++;} d[i]=0;}
file_t files[16];
void fs_init(){for(int i=0;i<16;i++) files[i].used=0;}
int fs_create(char* n){for(int i=0;i<16;i++) if(!files[i].used){scpy2(files[i].name,n); files[i].size=0; files[i].used=1; files[i].data[0]=0; return 0;} return -1;}
int fs_write(char* n,char* d){for(int i=0;i<16;i++) if(files[i].used&&sc2(files[i].name,n)==0){scpy2(files[i].data,d); files[i].size=sl2(d); return 0;} fs_create(n); return fs_write(n,d);}
int fs_read(char* n,char* b){for(int i=0;i<16;i++) if(files[i].used&&sc2(files[i].name,n)==0){scpy2(b,files[i].data); return files[i].size;} return -1;}
int fs_delete(char* n){for(int i=0;i<16;i++) if(files[i].used&&sc2(files[i].name,n)==0){files[i].used=0; return 0;} return -1;}
void fs_list(){print("Files:\n"); int e=1; for(int i=0;i<16;i++) if(files[i].used){e=0; print(" - "); print(files[i].name); print("\n");} if(e) print(" (empty)\n");}