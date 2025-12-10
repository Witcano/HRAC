#include "hrac.h"
#include <cstdio>
#include <algorithm>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

const int little_endian = 1, elem_len = 1;
#define dtype uint8_t
#define compress_type fits_kcomp_u8
#define decompress_type fits_kdecomp_u8

bool check_same(size_t len, uint8_t a[], uint8_t b[]) {
    for(uint32_t i = 0; i<len; i++) {
        if(a[i]!=b[i]) {
            printf("%d: %d %d\n", i, a[i],b[i]);
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("usage: %s <input> <blk> <nsblk> <inner>\n"
            "  input    : path to raw astronomical data file\n"
            "  blk      : block size, default 64\n"
            "  nsblk    : the length of the dimension with the lowest variability\n"
            "  inner    : the actual storage-address distance between two adjacent elements along the dimension with the lowest variability.\n",
            argv[0]
        );
        return -1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open file failed");
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat failed");
        close(fd);
        return -1;
    }

    size_t iLen = st.st_size;
    size_t oLen = (size_t)(iLen*1.1);

    

    uint8_t *out;
    dtype *in, *r_in;
    out = (uint8_t*)aligned_alloc(64, oLen);
    r_in = (dtype*)aligned_alloc(64, iLen);
    in = (dtype*)mmap(NULL, iLen, PROT_READ, MAP_SHARED, fd, 0);
    if (in == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return -1;
    }
	close(fd);

    

    if (!little_endian && elem_len > 1) {
        uint8_t *p = (uint8_t*)in;
        if (elem_len == 4)
            for (int a = 0; a<iLen; a+=4) {
                std::swap(p[a], p[a+3]);
                std::swap(p[a+1], p[a+2]);
            }
        else if (elem_len == 2)
            for (int a = 0; a<iLen; a+=2) {
                std::swap(p[a], p[a+1]);
            }
    }

    

    struct timespec t1, t2;
    long seconds, nanoseconds;
    double elapsed;

    uint32_t blk = std::atoi(argv[2]);
    uint32_t nsblk = std::atoi(argv[3]);
    uint32_t inner = std::atoi(argv[4]);

    size_t tile_size = nsblk*inner;
    size_t tile_numb = iLen/elem_len/(nsblk*inner);


    printf("# %s\n", argv[1]);
    for(int c=0; c<tile_numb; c++) {
        dtype* cpos = in + c*tile_size;

        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1);
        size_t v_oLen = compress_type(cpos, tile_size, out, oLen, blk, nsblk, inner);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t2);
        seconds = t2.tv_sec - t1.tv_sec;
        nanoseconds = t2.tv_nsec - t1.tv_nsec;
        elapsed = seconds + nanoseconds * 1e-9;

        printf("%lu %lu %lf ", tile_size*elem_len, v_oLen, elapsed);

        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1);
        size_t r_iLen = decompress_type(out, v_oLen, r_in, tile_size, blk, nsblk, inner);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t2);
        seconds = t2.tv_sec - t1.tv_sec;
        nanoseconds = t2.tv_nsec - t1.tv_nsec;
        elapsed = seconds + nanoseconds * 1e-9;
        printf("%lf\n", elapsed);



        if(tile_size == r_iLen && check_same(r_iLen*elem_len, (uint8_t*)cpos, (uint8_t*)r_in));
        else {
            printf("error: inconsistent: %lu %lu\n", tile_size, r_iLen);
        }
    }


    munmap(in, iLen);
    free(out);
    free(r_in);
    return 0;
}