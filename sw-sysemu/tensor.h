#ifndef bemu_tensor_h
#define bemu_tensor_h

#include <cstdint>

//namespace bemu {


void tensor_fma_execute();
void tensor_load_execute(bool tenb);
void tensor_quant_execute();
void tensor_reduce_step(unsigned thread);
void tensor_reduce_execute();
void tensor_wait_start(uint64_t value);
void tensor_wait_execute();


//} // namespace bemu

#endif // bemu_tensor_h
