#include <dirent.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 2)
        return 2;
    DIR *d = opendir(argv[1]);
    if (d == NULL) {
        printf("opendir failed: %s\n", argv[1]);
        return 1;
    }
    int seen = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        printf("entry: %s\n", ent->d_name);
        seen++;
    }
    closedir(d);
    char path[1024];
    snprintf(path, sizeof path, "%s/001_init.sql", argv[1]);
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        printf("fopen failed: %s\n", path);
        return 1;
    }
    char buf[16];
    size_t n = fread(buf, 1, sizeof buf, f);
    fclose(f);
    printf("opendir ok (%d entries), fopen ok, first %zu bytes: %.*s\n", seen, n, (int)n, buf);
    return 0;
}
