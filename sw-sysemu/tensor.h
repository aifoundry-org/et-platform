#ifndef bemu_tensor_h
#define bemu_tensor_h

#include <cstdint>

namespace bemu {


void tensor_fma_execute(Hart& cpu);
void tensor_load_execute(Hart& cpu, bool tenb);
void tensor_quant_execute(Hart& cpu);
void tensor_reduce_step(Hart& rcv_cpu, Hart& snd_cpu);
void tensor_reduce_execute(Hart& cpu);
void tensor_wait_start(Hart& cpu, uint64_t value);
void tensor_wait_execute(Hart& cpu);


} // namespace bemu

#endif // bemu_tensor_h
