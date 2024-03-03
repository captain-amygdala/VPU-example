#ifndef _PS_PROTOCOL_H
#define _PS_PROTOCOL_H

unsigned int ps_read_8(unsigned int address);
unsigned int ps_read_16(unsigned int address);
unsigned int ps_read_32(unsigned int address);

void ps_write_8(unsigned int address, unsigned int data);
void ps_write_16(unsigned int address, unsigned int data);
void ps_write_32(unsigned int address, unsigned int data);

unsigned int ps_read_status_reg();
void ps_write_status_reg(unsigned int value);

void ps_setup_protocol();
void ps_reset_state_machine();
void ps_pulse_reset();

unsigned int ps_get_ipl_zero();

#endif /* _PS_PROTOCOL_H */
