#ifndef _PS_AUTOCONFIG_H
#define _PS_AUTOCONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define AC_MEM_SIZE_8MB 0
#define AC_MEM_SIZE_64KB 1
#define AC_MEM_SIZE_128KB 2
#define AC_MEM_SIZE_256KB 3
#define AC_MEM_SIZE_512KB 4
#define AC_MEM_SIZE_1MB 5
#define AC_MEM_SIZE_2MB 6
#define AC_MEM_SIZE_4MB 7

void init_autoconfig();
void add_autoconfig_pic(unsigned char *rom, unsigned int rom_size,
                        void (*done_callback)(int configured, unsigned int base));

#ifdef __cplusplus
}
#endif

#endif /* _PS_AUTOCONFIG_H */
