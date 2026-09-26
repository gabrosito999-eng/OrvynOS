#ifndef FS_H
#define FS_H

typedef struct {
    char name[32];
    char data[1024];
    int size;
    int used;
} file_t;

void fs_init();
int fs_create(char* name);
void fs_list();
void fs_read(char* name);
int fs_write(char* name, char* data);

#endif