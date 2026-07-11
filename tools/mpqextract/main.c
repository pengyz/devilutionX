#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpqfs/mpqfs.h>

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <mpq_file> <filename> [output]\n", argv[0]);
        return 1;
    }
    mpqfs_archive_t *a = mpqfs_open(argv[1]);
    if (!a) {
        fprintf(stderr, "Cannot open %s: %s\n", argv[1], mpqfs_last_error());
        return 1;
    }
    printf("Opened: %s\n", argv[1]);
    printf("Has file? %d\n", mpqfs_has_file(a, argv[2]));
    
    size_t size;
    void *data = mpqfs_read_file(a, argv[2], &size);
    if (!data) {
        fprintf(stderr, "Not found: %s (%s)\n", argv[2], mpqfs_last_error());
        mpqfs_close(a);
        return 1;
    }
    const char *out = argc > 3 ? argv[3] : "output.bin";
    FILE *f = fopen(out, "wb");
    fwrite(data, 1, size, f);
    fclose(f);
    free(data);
    mpqfs_close(a);
    printf("%s (%zu bytes) -> %s\n", argv[2], size, out);
    return 0;
}
