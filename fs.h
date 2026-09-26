#define FS_MAX_FILES 16
#define FS_MAX_NAME 32
#define FS_MAX_DATA 1024

typedef struct {
    char name[32];
    char data[1024];
    int size;
    int used;
} file_t;

void fs_init();
int fs_create(char* n);
int fs_write(char* n, char* d);
int fs_read(char* n, char* b);
int fs_delete(char* n);
void fs_list();