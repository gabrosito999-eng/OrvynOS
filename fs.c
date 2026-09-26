// OrvynOS 0.1.6 - fs.c COMPLETO CINE++
#include "fs.h"

file_t files[16];
int file_count = 0;

void fs_init(){
    for(int i=0;i<16;i++){
        files[i].used = 0;
        files[i].size = 0;
        for(int j=0;j<32;j++) files[i].name[j]=0;
        for(int j=0;j<1024;j++) files[i].data[j]=0;
    }
    file_count = 0;
}

int fs_create(char* name){
    if(file_count >= 16) return -1;
    for(int i=0;i<16;i++){
        if(!files[i].used){
            files[i].used = 1;
            int j=0;
            while(name[j] && j<31){ files[i].name[j]=name[j]; j++; }
            files[i].name[j]=0;
            files[i].size = 0;
            file_count++;
            return i;
        }
    }
    return -1;
}

// interno para print - lo usa kernel.c
extern void print(char* s);
void fs_list(){
    print("Files:\n");
    int found=0;
    for(int i=0;i<16;i++){
        if(files[i].used){
            print(" - ");
            print(files[i].name);
            print("\n");
            found=1;
        }
    }
    if(!found) print(" (no files)\n");
}

void fs_read(char* name){
    for(int i=0;i<16;i++){
        if(files[i].used){
            int match=1;
            int j=0;
            while(name[j] && files[i].name[j]){
                if(name[j]!=files[i].name[j]){ match=0; break; }
                j++;
            }
            if(name[j]!=files[i].name[j]) match=0;
            if(match){
                if(files[i].size==0){
                    print("[empty file: ");
                    print(files[i].name);
                    print("]\n");
                } else {
                    print(files[i].data);
                    print("\n");
                }
                return;
            }
        }
    }
    print("File not found: ");
    print(name);
    print("\n");
}

int fs_write(char* name, char* data){
    for(int i=0;i<16;i++){
        if(files[i].used){
            int match=1;
            int j=0;
            while(name[j] && files[i].name[j]){
                if(name[j]!=files[i].name[j]){ match=0; break; }
                j++;
            }
            if(name[j]!=files[i].name[j]) match=0;
            if(match){
                int k=0;
                while(data[k] && k<1023){ files[i].data[k]=data[k]; k++; }
                files[i].data[k]=0;
                files[i].size=k;
                return 0;
            }
        }
    }
    return -1;
}