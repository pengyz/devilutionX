#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpqfs/mpqfs.h>

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <mpq_file> <filename> [output]\n", argv[0]);
        return 1;
    }

    mpqfs_archive_t *a = NULL;
    mpqfs_error_code rc = mpqfs_open(argv[1], &a);
    if (rc != MPQFS_OK) {
        fprintf(stderr, "Cannot open %s: %s\n", argv[1], mpqfs_error_message(rc));
        return 1;
    }
    printf("Opened: %s\n", argv[1]);
    printf("Has file? %d\n", (int)mpqfs_has_file(a, argv[2]));

    void *data = NULL;
    size_t size = 0;
    rc = mpqfs_read_file(a, argv[2], &data, &size);
    if (rc != MPQFS_OK) {
        fprintf(stderr, "Cannot read %s: %s\n", argv[2], mpqfs_error_message(rc));
        mpqfs_close(a);
        return 1;
    }

    const char *out = argc > 3 ? argv[3] : "output.bin";
    FILE *f = fopen(out, "wb");
    if (f == NULL) {
        fprintf(stderr, "Cannot write %s\n", out);
        free(data);
        mpqfs_close(a);
        return 1;
    }
    fwrite(data, 1, size, f);
    fclose(f);
    free(data);
    mpqfs_close(a);
    printf("%s (%zu bytes) -> %s\n", argv[2], size, out);
    return 0;
}
