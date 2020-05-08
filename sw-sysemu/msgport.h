#ifndef bemu_msgport_h
#define bemu_msgport_h

#include <cstdint>

namespace bemu {


void set_delayed_msg_port_write(bool);

unsigned get_msg_port_write_width(unsigned thread, unsigned port);

bool get_msg_port_stall(unsigned thread, unsigned port);

void write_msg_port_data(unsigned target_thread, unsigned port, unsigned source_thread, uint32_t* data);
void commit_msg_port_data(unsigned target_thread, unsigned port, unsigned source_thread);

void write_msg_port_data_from_tbox(unsigned target_thread, unsigned port, unsigned tbox, uint32_t* data, uint8_t oob);
void commit_msg_port_data_from_tbox(unsigned target_thread, unsigned port, unsigned tbox);

void write_msg_port_data_from_rbox(unsigned target_thread, unsigned port, unsigned rbox, uint32_t* data, uint8_t oob);
void commit_msg_port_data_from_rbox(unsigned target_thread, unsigned port, unsigned rbox);


} // namespace bemu

#endif // bemu_msgport_h
