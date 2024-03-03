#ifndef _PS_MAPPINGS_H
#define _PS_MAPPINGS_H

#ifdef __cplusplus
extern "C" {
#endif

struct ps_device {
    unsigned int (*read_8)(unsigned int address);
    unsigned int (*read_16)(unsigned int address);
    unsigned int (*read_32)(unsigned int address);
    void (*write_8)(unsigned int address, unsigned int value);
    void (*write_16)(unsigned int address, unsigned int value);
    void (*write_32)(unsigned int address, unsigned int value);
};

int ps_add_device(struct ps_device *device);
void ps_add_range(unsigned char devno, unsigned int base, unsigned int size);

void init_mappings();

#ifdef __cplusplus
}
#endif

#endif /* _PS_MAPPINGS_H */
