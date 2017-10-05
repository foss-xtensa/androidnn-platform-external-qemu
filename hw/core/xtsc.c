#include "qemu/osdep.h"
#include "hw/xtsc.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void *xtsc_open_shared_memory(const char *ram_name_pattern, size_t ram_size)
{
    const char *pid = getenv("XTSC_PID");
    char ram_name_buf[100];
    const char *ram_name = ram_name_buf;
    void *ram_ptr;
    int fd;
    int rc;
    int i;

    if (pid) {
        snprintf(ram_name_buf, sizeof(ram_name_buf), "%s.%s",
                 ram_name_pattern, pid);
    } else {
        ram_name = ram_name_pattern;
    }

    for (i = 0; i < 10; ++i) {
        fd = shm_open(ram_name, O_RDWR, 0666);
        if (fd < 0) {
            printf("waiting for %s...\n", ram_name);
            sleep(1);
        }
    }
    if (fd < 0) {
        perror("shm_open");
        abort();
    }

    rc = ftruncate(fd, ram_size);
    if (rc < 0) {
        perror("ftruncate");
        abort();
    }

    ram_ptr = mmap(NULL, ram_size,
                   PROT_READ | PROT_WRITE,
                   MAP_SHARED, fd, 0);
    if (ram_ptr == MAP_FAILED) {
        perror("mmap");
        abort();
    }
    return ram_ptr;
}
