#include <stdint.h>

#include "etlibdevice.h"
#include "kernel_args.h"


#define KERNEL_ENTRY extern "C" __attribute__((visibility("default")))


namespace quantization {
inline int8_t clip_i32_i8(int32_t in) {
  return device_max(-128, device_min(127, in));
}

inline int8_t quantize(float input, float scale, int32_t offset) {
    float result = input / scale + offset;
    return quantization::clip_i32_i8(device_roundf(result));
}

inline float dequantize(int8_t input, float scale, int32_t offset) {
    return scale * (input - offset);
}
} // namespace quantization


template<typename T>
static void blendResult(T alpha_val, T result, T beta_val, T &dst)
{
    // if β = 0 then dst does not have to be a valid input
    dst = alpha_val * result + (beta_val == (T)0 ? (T)0 : (beta_val * dst));
}

template<typename T>
static T &matEl(T *X, int ldx, etblasOperation_t op, int i, int j)
{
    if (op == ETBLAS_OP_N) {
        return X[i + j*ldx];
    } else {
        return X[j + i*ldx];
    }
}

template<typename T>
static T &matElRM(T *X, int ldx, bool is_transx, int i, int j)
{
    if (!is_transx) {
        return X[j + i*ldx];
    } else {
        return X[i + j*ldx];
    }
}

template<typename T>
static T &tensor4dEl(T *tensor, int n, int c, int h, int w, int ns, int cs, int hs, int ws )
{
    return tensor[n*ns + c*cs + h*hs + w*ws];
}

template<typename T>
T* tensor4dElPtr(T *tensor, int n, int c, int h, int w, int ns, int cs, int hs, int ws )
{
    return &tensor[n*ns + c*cs + h*hs + w*ws];
}


static inline unsigned get_global_id_x(KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    return wi_state_p->block_id_x * state_p->block_size_x + wi_state_p->local_id_x;
}

KERNEL_ENTRY void CopyKernel_Int32_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        unsigned len = *reinterpret_cast<unsigned*>(args + 0);
        int32_t* src = *reinterpret_cast<int32_t**>(args + 8);
        int32_t* dst = *reinterpret_cast<int32_t**>(args + 16);

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            dst[global_id] = src[global_id];
        }
    }
}

KERNEL_ENTRY void CopyKernel_Int8_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        unsigned len = *reinterpret_cast<unsigned*>(args + 0);
        int8_t* src = *reinterpret_cast<int8_t**>(args + 8);
        int8_t* dst = *reinterpret_cast<int8_t**>(args + 16);

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            dst[global_id] = src[global_id];
        }
    }
}

KERNEL_ENTRY void SetKernel_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        unsigned len = *reinterpret_cast<unsigned*>(args + 0);
        float val = *reinterpret_cast<float*>(args + 4);
        float* devPtr = *reinterpret_cast<float**>(args + 8);

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            devPtr[global_id] = val;
        }
    }
}

KERNEL_ENTRY void SetKernel_Int32_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        unsigned len = *reinterpret_cast<unsigned*>(args + 0);
        int32_t val = *reinterpret_cast<int32_t*>(args + 4);
        int32_t* devPtr = *reinterpret_cast<int32_t**>(args + 8);

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            devPtr[global_id] = val;
        }
    }
}

KERNEL_ENTRY void SetKernel_Int8_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        unsigned len = *reinterpret_cast<unsigned*>(args + 0);
        int8_t val = *reinterpret_cast<int8_t*>(args + 4);
        int8_t* devPtr = *reinterpret_cast<int8_t**>(args + 8);

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            devPtr[global_id] = val;
        }
    }
}

KERNEL_ENTRY void SigmoidKernel_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        unsigned len = *reinterpret_cast<unsigned*>(args + 0);
        float* devPtrX = *reinterpret_cast<float**>(args + 8);
        float* devPtrY = *reinterpret_cast<float**>(args + 16);

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            devPtrY[global_id] = device_rcpf(1.0f + device_expf(-devPtrX[global_id]));
        }
    }
}

KERNEL_ENTRY void TanhKernel_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        unsigned len = *reinterpret_cast<unsigned*>(args + 0);
        float* devPtrX = *reinterpret_cast<float**>(args + 8);
        float* devPtrY = *reinterpret_cast<float**>(args + 16);

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            devPtrY[global_id] = 2.0f * device_rcpf(1.0f + device_expf(-2.0f * devPtrX[global_id])) - 1.0f;
        }
    }
}

KERNEL_ENTRY void MulBroadcast2Kernel_Float_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        float* a = *reinterpret_cast<float**>(args + 0);
        float* b = *reinterpret_cast<float**>(args + 8);
        float* out = *reinterpret_cast<float**>(args + 16);
        unsigned pre = *reinterpret_cast<unsigned*>(args + 24);
        unsigned n = *reinterpret_cast<unsigned*>(args + 28);
        unsigned post = *reinterpret_cast<unsigned*>(args + 32);
        unsigned len = n * pre * post;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            out[global_id] = a[global_id]*b[(global_id / post) % n];
        }
    }
}

KERNEL_ENTRY void BinAddKernelBroadcast_False_Float_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        float* a = *reinterpret_cast<float**>(args + 0);
        float* b = *reinterpret_cast<float**>(args + 8);
        float* r = *reinterpret_cast<float**>(args + 16);
        unsigned pre = *reinterpret_cast<unsigned*>(args + 24);
        unsigned post = *reinterpret_cast<unsigned*>(args + 28);
        unsigned n = *reinterpret_cast<unsigned*>(args + 32);
        unsigned len = n * pre * post;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            r[global_id] = a[global_id] + b[(global_id / post) % n];
        }
    }
}

KERNEL_ENTRY void AddKernel_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        unsigned n = *reinterpret_cast<unsigned*>(args + 0);
        float* a = *reinterpret_cast<float**>(args + 8);
        float* b = *reinterpret_cast<float**>(args + 16);
        float* y = *reinterpret_cast<float**>(args + 24);
        unsigned len = n;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            y[global_id] = a[global_id] + b[global_id];
        }
    }
}

KERNEL_ENTRY void SubKernel_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        unsigned n = *reinterpret_cast<unsigned*>(args + 0);
        float* a = *reinterpret_cast<float**>(args + 8);
        float* b = *reinterpret_cast<float**>(args + 16);
        float* y = *reinterpret_cast<float**>(args + 24);
        unsigned len = n;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            y[global_id] = a[global_id] - b[global_id];
        }
    }
}

KERNEL_ENTRY void MaxKernel_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        unsigned n = *reinterpret_cast<unsigned*>(args + 0);
        float* a = *reinterpret_cast<float**>(args + 8);
        float* b = *reinterpret_cast<float**>(args + 16);
        float* y = *reinterpret_cast<float**>(args + 24);
        unsigned len = n;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            y[global_id] = (a[global_id] >= b[global_id]) ? a[global_id] : b[global_id];
        }
    }
}

KERNEL_ENTRY void MinKernel_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        unsigned n = *reinterpret_cast<unsigned*>(args + 0);
        float* a = *reinterpret_cast<float**>(args + 8);
        float* b = *reinterpret_cast<float**>(args + 16);
        float* y = *reinterpret_cast<float**>(args + 24);
        unsigned len = n;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            y[global_id] = (a[global_id] <= b[global_id]) ? a[global_id] : b[global_id];
        }
    }
}

KERNEL_ENTRY void MulKernel_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        unsigned n = *reinterpret_cast<unsigned*>(args + 0);
        float* a = *reinterpret_cast<float**>(args + 8);
        float* b = *reinterpret_cast<float**>(args + 16);
        float* y = *reinterpret_cast<float**>(args + 24);
        unsigned len = n;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            y[global_id] = a[global_id] * b[global_id];
        }
    }
}

KERNEL_ENTRY void DivKernel_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        unsigned n = *reinterpret_cast<unsigned*>(args + 0);
        float* a = *reinterpret_cast<float**>(args + 8);
        float* b = *reinterpret_cast<float**>(args + 16);
        float* y = *reinterpret_cast<float**>(args + 24);
        unsigned len = n;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            y[global_id] = a[global_id] / b[global_id];
        }
    }
}

KERNEL_ENTRY void CmpLTEKernel_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        unsigned n = *reinterpret_cast<unsigned*>(args + 0);
        float* a = *reinterpret_cast<float**>(args + 8);
        float* b = *reinterpret_cast<float**>(args + 16);
        float* y = *reinterpret_cast<float**>(args + 24);
        unsigned len = n;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            y[global_id] = (a[global_id] <= b[global_id]) ? 1.0f : 0.0f;
        }
    }
}

KERNEL_ENTRY void CmpEQKernel_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        unsigned n = *reinterpret_cast<unsigned*>(args + 0);
        float* a = *reinterpret_cast<float**>(args + 8);
        float* b = *reinterpret_cast<float**>(args + 16);
        float* y = *reinterpret_cast<float**>(args + 24);
        unsigned len = n;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            y[global_id] = (a[global_id] == b[global_id]) ? 1.0f : 0.0f;
        }
    }
}

KERNEL_ENTRY void PowKernel_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        unsigned n = *reinterpret_cast<unsigned*>(args + 0);
        float* a = *reinterpret_cast<float**>(args + 8);
        float* b = *reinterpret_cast<float**>(args + 16);
        float* y = *reinterpret_cast<float**>(args + 24);
        unsigned len = n;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            y[global_id] = device_powf(a[global_id], b[global_id]);
        }
    }
}

KERNEL_ENTRY void SelectKernel_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        unsigned n = *reinterpret_cast<unsigned*>(args + 0);
        float* a = *reinterpret_cast<float**>(args + 8);
        float* b = *reinterpret_cast<float**>(args + 16);
        float* cond = *reinterpret_cast<float**>(args + 24);
        float* y = *reinterpret_cast<float**>(args + 32);
        unsigned len = n;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            y[global_id] = (cond[global_id] != 0.0f) ? a[global_id] : b[global_id];
        }
    }
}

KERNEL_ENTRY void Sgemm_v2_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        Sgemm_v2_KernelArgs_t* args_p = reinterpret_cast<Sgemm_v2_KernelArgs_t*>(args);
        unsigned len = args_p->n*args_p->m;

        // This is technically 1D Kernel. TODO: use 2D for simplicity.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        {
            if ( global_id < len )
            {
                int i = global_id % args_p->m;
                int j = global_id / args_p->m;
                float &cEl = matEl(args_p->C, args_p->ldc, ETBLAS_OP_N, i, j);
                float sum = 0.0f;
                for (int l = 0; l < args_p->k; l++)
                {
                    sum += matEl(args_p->A, args_p->lda, args_p->transa, i, l) * matEl(args_p->B, args_p->ldb, args_p->transb, l, j);
                }
                blendResult(args_p->alpha_val, sum, args_p->beta_val, cEl);
            }
        }
    }
}

struct TensorData {
    static constexpr unsigned MS = 16;
    float m[MS][MS];

    void load(const float *mem, size_t stride, int rows, bool is_transpose) {
        int ldx = stride / sizeof(float);
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < MS; j++) {
                if (!is_transpose) {
                    m[i][j] = mem[i*ldx + j];
                } else {
                    m[i][j] = mem[j*ldx + i];
                }
            }
        }
    }

    void store(float *mem, size_t stride, int rows, int cols, bool is_transpose) {
        int ldx = stride / sizeof(float);
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (!is_transpose) {
                    mem[i*ldx + j] = m[i][j];
                } else {
                    mem[j*ldx + i] = m[i][j];
                }
            }
        }
    }

    void fma(bool is_first_pass, const TensorData &a, const TensorData &b, int arows, int acols, int bcols) {
        for (int i = 0; i < arows; i++) {
            for (int j = 0; j < bcols; j++) {
                if (is_first_pass) {
                    m[i][j] = 0.0f;
                }
                for (int l = 0; l < acols; l++) {
                    m[i][j] += a.m[i][l] * b.m[l][j];
                }
            }
        }
    }
};

KERNEL_ENTRY void MatMulTensor_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    MatMulTensor_KernelArgs_t* args_p = reinterpret_cast<MatMulTensor_KernelArgs_t*>(args);
    size_t stridea = args_p->lda * sizeof(float);
    size_t strideb = args_p->ldb * sizeof(float);
    size_t stridec = args_p->ldc * sizeof(float);
    bool is_transa = args_p->is_transa;
    bool is_transb = args_p->is_transb;
    TensorLoadTrans transa = !is_transa ? TensorLoadTrans::None : TensorLoadTrans::Transpose32;
    TensorLoadTrans transb = !is_transb ? TensorLoadTrans::None : TensorLoadTrans::Transpose32;
    static constexpr unsigned BS = 16; // BLOCK_SIZE
    unsigned blocks_y = (args_p->m + BS - 1) / BS;
    unsigned blocks_x = (args_p->n + BS - 1) / BS;
    unsigned blocks_k = (args_p->k + BS - 1) / BS;

    // This is technically 1D Kernel. Each work item compute 16x16 block of result matrix.
    unsigned global_id = get_global_id_x(state_p, wi_state_p);

    if ( global_id < blocks_x * blocks_y )
    {
        int block_x = global_id % blocks_x;
        int block_y = global_id / blocks_x;

        float *blockC = &args_p->C[args_p->ldc*block_y*BS + block_x*BS];
        int blockYelems = device_min(args_p->m - block_y*BS, BS);
        int blockXelems = device_min(args_p->n - block_x*BS, BS);

        // tensor_fma requires bcols must be dividable by 4
        // tensor_store requires cols must be dividable by 4
#if 0
        TensorData temp_a;
        TensorData temp_b;
        TensorData res;
        bool is_first_pass = true;
        for (unsigned block_k = 0; block_k < blocks_k; block_k++)
        {
            const float *blockA = !is_transa ? &args_p->A[args_p->lda*block_y*BS + block_k*BS] : &args_p->A[args_p->lda*block_k*BS + block_y*BS];
            const float *blockB = !is_transb ? &args_p->B[args_p->ldb*block_k*BS + block_x*BS] : &args_p->B[args_p->ldb*block_x*BS + block_k*BS];
            int blockKelems = device_min(args_p->k - block_k*BS, BS);

            temp_a.load(blockA, stridea, blockYelems, is_transa);
            temp_b.load(blockB, strideb, blockKelems, is_transb);

            res.fma(is_first_pass, temp_a, temp_b, blockYelems, blockKelems, blockXelems);

            is_first_pass = false;
        }
        res.store(blockC, stridec, blockYelems, blockXelems, false);
#else
        bool is_first_pass = true;
        for (unsigned block_k = 0; block_k < blocks_k; block_k++)
        {
            const float *blockA = !is_transa ? &args_p->A[args_p->lda*block_y*BS + block_k*BS] : &args_p->A[args_p->lda*block_k*BS + block_y*BS];
            const float *blockB = !is_transb ? &args_p->B[args_p->ldb*block_k*BS + block_x*BS] : &args_p->B[args_p->ldb*block_x*BS + block_k*BS];
            int blockKelems = device_min(args_p->k - block_k*BS, BS);

            tensor_load(0,  blockA, stridea, blockYelems, transa);
            tensor_load(16, blockB, strideb, blockKelems, transb);

            tensor_fma(is_first_pass, 0, 16, blockYelems, blockKelems, blockXelems, TensorFmaType::F32);

            is_first_pass = false;
        }
        if (blockXelems % 4 == 0) {
            tensor_store(blockC, stridec, blockYelems, blockXelems);
        } else {
            tensor_store_emulation(blockC, stridec, blockYelems, blockXelems);
        }
#endif
    }
}

KERNEL_ENTRY void MatMulTensorInt8To32_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    MatMulTensorInt8To32_KernelArgs_t* args_p = reinterpret_cast<MatMulTensorInt8To32_KernelArgs_t*>(args);
    size_t stridea = args_p->lda * sizeof(int8_t);
    size_t strideb = args_p->ldb * sizeof(int8_t);
    size_t stridec = args_p->ldc * sizeof(int32_t);
    bool is_transa = args_p->is_transa;
    bool is_transb = args_p->is_transb;
    TensorLoadTrans transa = !is_transa ? TensorLoadTrans::None        : TensorLoadTrans::Transpose8;
    TensorLoadTrans transb = !is_transb ? TensorLoadTrans::Interleave8 : TensorLoadTrans::TransposeInterleave8;
    static constexpr unsigned BS = 16; // BLOCK_SIZE
    static constexpr unsigned BSK = BS * 4;
    unsigned blocks_y = (args_p->m + BS - 1) / BS;
    unsigned blocks_x = (args_p->n + BS - 1) / BS;
    unsigned blocks_k = (args_p->k + BSK - 1) / BSK;

    // This is technically 1D Kernel. Each work item compute 16x16 block of result matrix.
    unsigned global_id = get_global_id_x(state_p, wi_state_p);

    if ( global_id < blocks_x * blocks_y )
    {
        int block_x = global_id % blocks_x;
        int block_y = global_id / blocks_x;

        int32_t *blockC = &args_p->C[args_p->ldc*block_y*BS + block_x*BS];
        int blockYelems = device_min(args_p->m - block_y*BS, BS);
        int blockXelems = device_min(args_p->n - block_x*BS, BS);

        bool is_first_pass = true;
        for (unsigned block_k = 0; block_k < blocks_k; block_k++)
        {
            const int8_t *blockA = !is_transa ? &args_p->A[args_p->lda*block_y*BS + block_k*BSK] : &args_p->A[args_p->lda*block_k*BSK + block_y*BS];
            const int8_t *blockB = !is_transb ? &args_p->B[args_p->ldb*block_k*BSK + block_x*BS] : &args_p->B[args_p->ldb*block_x*BS + block_k*BSK];
            int blockKelems = device_min(args_p->k - block_k*BSK, BSK) / 4; // k is always dividable by 4!!

            tensor_load(0,  blockA, stridea, blockYelems, transa);
            tensor_load(16, blockB, strideb, blockKelems, transb);

            tensor_fma(is_first_pass, 0, 16, blockYelems, blockKelems, blockXelems, TensorFmaType::I8I32);

            is_first_pass = false;
        }
        if (blockXelems % 4 == 0) {
            tensor_store(blockC, stridec, blockYelems, blockXelems);
        } else {
            tensor_store_emulation(blockC, stridec, blockYelems, blockXelems);
        }
    }
}

KERNEL_ENTRY void QuantizedMatMul_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    QuantizedMatMul_KernelArgs_t* args_p = reinterpret_cast<QuantizedMatMul_KernelArgs_t*>(args);
    unsigned len = args_p->n*args_p->m;

    // This is technically 1D Kernel.
    unsigned global_id = get_global_id_x(state_p, wi_state_p);

    if ( global_id < len )
    {
        int j = global_id % args_p->n;
        int i = global_id / args_p->n;
        int8_t &cEl = matElRM(args_p->C, args_p->ldc, false, i, j);
        int32_t sum = 0;
        for (int l = 0; l < args_p->k; l++)
        {
            int32_t aEl = matElRM(args_p->A, args_p->lda, args_p->is_transa, i, l);
            int32_t bEl = matElRM(args_p->B, args_p->ldb, args_p->is_transb, l, j);
            sum += (aEl - args_p->offseta) * (bEl - args_p->offsetb);
        }
        cEl = quantization::clip_i32_i8(device_roundf(args_p->invScale * sum + args_p->offsetc));
    }
}

/*
 * 2nd part of implementation of Convolution via matrix mul.
 * On the 1st step calculated a dot product of all channels of individual "pixel" on all possible filter channels.
 * On the 2nd step we should sum filter_h*filter_w elements of tempory matrix from the 1st step.
 */
KERNEL_ENTRY void ConvTailNHWC_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    ConvTailNHWC_KernelArgs_t* args_p = reinterpret_cast<ConvTailNHWC_KernelArgs_t*>(args);
    unsigned len = args_p->batch_size*args_p->out_c*args_p->out_h*args_p->out_w;
    float* dest = (float*)args_p->y;
    const float* ws = (float*)args_p->workspace; // Workspace = intermediate matrix after MatMul
    const float* bias = (float*)args_p->b;
    int ldws = args_p->filter_h * args_p->filter_w * args_p->out_c;

    // This is 1D Kernel.
    unsigned global_id = get_global_id_x(state_p, wi_state_p);

    if ( global_id < len )
    {
        // global_id -> fully packed NHWC
        int c = global_id % args_p->out_c;
        int w = (global_id / args_p->out_c) % args_p->out_w;
        int h = ((global_id / args_p->out_c) / args_p->out_w) % args_p->out_h;
        int n = ((global_id / args_p->out_c) / args_p->out_w) / args_p->out_h;

        int y = (h * args_p->u) - args_p->pad_h;
        int x = (w * args_p->v) - args_p->pad_w;

        // Apply 2d filter
        float conv_sum = 0.0f;
        for ( int fh = 0 ; fh < args_p->filter_h ; fh++ )
        {
            for ( int fw = 0 ; fw < args_p->filter_w ; fw++ )
            {
                int ih = y + fh * args_p->dilation_h;
                int iw = x + fw * args_p->dilation_w;

                if ( ih >= 0 && iw >= 0 && ih < args_p->in_h && iw < args_p->in_w )
                {
                    int ws_y = iw + args_p->in_w * (ih + args_p->in_h * n);
                    int ws_x = fw + args_p->filter_w * (fh + args_p->filter_h * c);

                    conv_sum += ws[ws_y * ldws + ws_x];
                }
            }
        }

        if ( bias )
        {
            conv_sum += bias[c];
        }

        dest[global_id] = conv_sum;
    }
}

KERNEL_ENTRY void ConvTailNHWC_Int8Q_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    ConvTailNHWC_KernelArgs_t* args_p = reinterpret_cast<ConvTailNHWC_KernelArgs_t*>(args);
    unsigned len = args_p->batch_size*args_p->out_c*args_p->out_h*args_p->out_w;
    int8_t* dest = (int8_t*)args_p->y;
    const int32_t* ws = (int32_t*)args_p->workspace; // Workspace = intermediate matrix after MatMul
    const int32_t* bias = (int32_t*)args_p->b;
    int ldws = args_p->filter_h * args_p->filter_w * args_p->out_c;

    // Calculate the scale of the values that come out of the matrix
    // multiplication part of the calculation.
    float matMulScale = args_p->inScale * args_p->filterScale;

    // This is 1D Kernel.
    unsigned global_id = get_global_id_x(state_p, wi_state_p);

    if ( global_id < len )
    {
        // global_id -> fully packed NHWC
        int c = global_id % args_p->out_c;
        int w = (global_id / args_p->out_c) % args_p->out_w;
        int h = ((global_id / args_p->out_c) / args_p->out_w) % args_p->out_h;
        int n = ((global_id / args_p->out_c) / args_p->out_w) / args_p->out_h;

        int y = (h * args_p->u) - args_p->pad_h;
        int x = (w * args_p->v) - args_p->pad_w;

        // Apply 2d filter
        int32_t conv_sum = 0;
        for ( int fh = 0 ; fh < args_p->filter_h ; fh++ )
        {
            for ( int fw = 0 ; fw < args_p->filter_w ; fw++ )
            {
                int ih = y + fh * args_p->dilation_h;
                int iw = x + fw * args_p->dilation_w;

                if ( ih >= 0 && iw >= 0 && ih < args_p->in_h && iw < args_p->in_w )
                {
                    int ws_y = iw + args_p->in_w * (ih + args_p->in_h * n);
                    int ws_x = fw + args_p->filter_w * (fh + args_p->filter_h * c);

                    conv_sum += ws[ws_y * ldws + ws_x];
                }
            }
        }

        if ( bias )
        {
            // Scale the bias to match the scale of the matrix multiplication.
            int32_t B = device_roundf(float(bias[c] - args_p->biasOffset) *
                                      (args_p->biasScale / matMulScale));
            conv_sum += B;
        }

        // Scale the result back to the expected destination scale.
        dest[global_id] = quantization::clip_i32_i8(
            device_roundf(float(conv_sum) * (matMulScale / args_p->outScale) + args_p->outOffset));
    }
}

KERNEL_ENTRY void ConvolutionForward_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
#define ELEM_IN(_n, _c, _h, _w) tensor4dEl((const float*)args_p->x, _n, _c, _h, _w, args_p->in_nStride, args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_FILTER1(_n, _c, _h, _w) tensor4dEl((const float*)args_p->w, _n, _c, _h, _w, args_p->filter_w * args_p->filter_h * args_p->filter_c, args_p->filter_w * args_p->filter_h, args_p->filter_w, 1)
#define ELEM_FILTER2(_n, _c, _h, _w) tensor4dEl((const float*)args_p->w, _n, _c, _h, _w, args_p->filter_w * args_p->filter_h * args_p->filter_c, 1, args_p->filter_c*args_p->filter_w, args_p->filter_c)
#define ELEM_OUT(_n, _c, _h, _w) tensor4dEl((float*)args_p->y, _n, _c, _h, _w, args_p->out_nStride, args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
        ConvolutionForward_KernelArgs_t* args_p = reinterpret_cast<ConvolutionForward_KernelArgs_t*>(args);
        unsigned len = args_p->out_n*args_p->out_c*args_p->out_h*args_p->out_w;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        {
            if ( global_id < len )
            {
                int w = global_id % args_p->out_w;
                int h = (global_id / args_p->out_w) % args_p->out_h;
                int c = ((global_id / args_p->out_w) / args_p->out_h) % args_p->out_c;
                int n = ((global_id / args_p->out_w) / args_p->out_h) / args_p->out_c;
                int y = (h * args_p->u) - args_p->pad_h;
                int x = (w * args_p->v) - args_p->pad_w;

                // Apply 2d filter
                float conv_sum = 0.0f;
                for ( int fh = 0 ; fh < args_p->filter_h ; fh++ )
                {
                    for ( int fw = 0 ; fw < args_p->filter_w ; fw++ )
                    {
                        int ih = y + fh * args_p->dilation_h;
                        int iw = x + fw * args_p->dilation_w;

                        if ( ih >= 0 && iw >= 0 && ih < args_p->in_h && iw < args_p->in_w )
                        {
                            /**
                             * Each input channel is filtered by channel filter and added to resulting sum
                             * In the end all channels of input image folded into one
                             */
                            for ( int fc = 0 ; fc < args_p->filter_c ; fc++ )
                            {
                                if ( args_p->filter_format == ETDNN_TENSOR_NCHW )
                                {
                                    conv_sum += ELEM_IN( n, fc, ih, iw ) * ELEM_FILTER1( c, fc, fh, fw );
                                } else
                                {
                                    conv_sum += ELEM_IN( n, fc, ih, iw ) * ELEM_FILTER2( c, fc, fh, fw );
                                }
                            }
                        }
                    }
                }

                if ( args_p->b )
                {
                    conv_sum += ((float*)args_p->b)[c];
                }

                float &oelem = ELEM_OUT( n, c, h, w );
                blendResult(args_p->alpha.f, conv_sum, args_p->beta.f, oelem);
            }
        }
#undef ELEM_OUT
#undef ELEM_FILTER2
#undef ELEM_FILTER1
#undef ELEM_IN
    }
}

KERNEL_ENTRY void ConvolutionForward_Int8Q_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
#define ELEM_IN(_n, _c, _h, _w) tensor4dEl((const int8_t*)args_p->x, _n, _c, _h, _w, args_p->in_nStride, args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_FILTER1(_n, _c, _h, _w) tensor4dEl((const int8_t*)args_p->w, _n, _c, _h, _w, args_p->filter_w * args_p->filter_h * args_p->filter_c, args_p->filter_w * args_p->filter_h, args_p->filter_w, 1)
#define ELEM_FILTER2(_n, _c, _h, _w) tensor4dEl((const int8_t*)args_p->w, _n, _c, _h, _w, args_p->filter_w * args_p->filter_h * args_p->filter_c, 1, args_p->filter_c*args_p->filter_w, args_p->filter_c)
#define ELEM_OUT(_n, _c, _h, _w) tensor4dEl((int8_t*)args_p->y, _n, _c, _h, _w, args_p->out_nStride, args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
        ConvolutionForward_KernelArgs_t* args_p = reinterpret_cast<ConvolutionForward_KernelArgs_t*>(args);
        unsigned len = args_p->out_n*args_p->out_c*args_p->out_h*args_p->out_w;
        // Calculate the scale of the values that come out of the matrix
        // multiplication part of the calculation.
        float matMulScale = args_p->inScale * args_p->filterScale;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        {
            if ( global_id < len )
            {
                int w = global_id % args_p->out_w;
                int h = (global_id / args_p->out_w) % args_p->out_h;
                int c = ((global_id / args_p->out_w) / args_p->out_h) % args_p->out_c;
                int n = ((global_id / args_p->out_w) / args_p->out_h) / args_p->out_c;
                int y = (h * args_p->u) - args_p->pad_h;
                int x = (w * args_p->v) - args_p->pad_w;

                // Apply 2d filter
                int32_t conv_sum = 0;
                for ( int fh = 0 ; fh < args_p->filter_h ; fh++ )
                {
                    for ( int fw = 0 ; fw < args_p->filter_w ; fw++ )
                    {
                        int ih = y + fh * args_p->dilation_h;
                        int iw = x + fw * args_p->dilation_w;

                        if ( ih >= 0 && iw >= 0 && ih < args_p->in_h && iw < args_p->in_w )
                        {
                            /**
                             * Each input channel is filtered by channel filter and added to resulting sum
                             * In the end all channels of input image folded into one
                             */
                            for ( int fc = 0 ; fc < args_p->filter_c ; fc++ )
                            {
                                int32_t I = ELEM_IN( n, fc, ih, iw );
                                int32_t F = (args_p->filter_format == ETDNN_TENSOR_NCHW)
                                            ? ELEM_FILTER1( c, fc, fh, fw )
                                            : ELEM_FILTER2( c, fc, fh, fw );
                                conv_sum += (I - args_p->inOffset) * (F - args_p->filterOffset);
                            }
                        }
                    }
                }

                if ( args_p->b )
                {
                    // Scale the bias to match the scale of the matrix multiplication.
                    int32_t B = device_roundf(float(((int32_t*)args_p->b)[c] - args_p->biasOffset) *
                                              (args_p->biasScale / matMulScale));
                    conv_sum += B;
                }

                int8_t &oelem = ELEM_OUT( n, c, h, w );
                // Scale the result back to the expected destination scale.
                oelem = quantization::clip_i32_i8(
                    device_roundf(float(conv_sum) * (matMulScale / args_p->outScale) + args_p->outOffset));
            }
        }
#undef ELEM_OUT
#undef ELEM_FILTER2
#undef ELEM_FILTER1
#undef ELEM_IN
    }
}

KERNEL_ENTRY void AddTensor_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
#define ELEM_IN(_n, _c, _h, _w) tensor4dEl((const float*)args_p->A, _n, _c, _h, _w, args_p->in_nStride, args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_OUT(_n, _c, _h, _w) tensor4dEl((float*)args_p->C, _n, _c, _h, _w, args_p->out_nStride, args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
        AddTensor_KernelArgs_t* args_p = reinterpret_cast<AddTensor_KernelArgs_t*>(args);
        unsigned len = args_p->out_n*args_p->out_c*args_p->out_h*args_p->out_w;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        {
            if ( global_id < len )
            {
                int w = global_id % args_p->out_w;
                int h = (global_id / args_p->out_w) % args_p->out_h;
                int c = ((global_id / args_p->out_w) / args_p->out_h) % args_p->out_c;
                int n = ((global_id / args_p->out_w) / args_p->out_h) / args_p->out_c;
                int iw = (args_p->in_w == args_p->out_w) ? w : 0;
                int ih = (args_p->in_h == args_p->out_h) ? h : 0;
                int ic = (args_p->in_c == args_p->out_c) ? c : 0;
                int in = (args_p->in_n == args_p->out_n) ? n : 0;

                const float in_elem = ELEM_IN( in, ic, ih, iw );
                float& out_elem = ELEM_OUT( n, c, h, w );

                blendResult(args_p->alpha.f, in_elem, args_p->beta.f, out_elem);
            }
        }
#undef ELEM_OUT
#undef ELEM_IN
    }
}

KERNEL_ENTRY void ActivationForward_Sigmoid_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
#define ELEM_IN_P(_n, _c, _h, _w) tensor4dElPtr((const float*)args_p->x, _n, _c, _h, _w, args_p->in_nStride, args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_OUT_P(_n, _c, _h, _w) tensor4dElPtr((float*)args_p->y, _n, _c, _h, _w, args_p->out_nStride, args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
        ActivationForward_KernelArgs_t* args_p = reinterpret_cast<ActivationForward_KernelArgs_t*>(args);
        unsigned len = args_p->n*args_p->c*args_p->h*args_p->w;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        {
            if ( global_id < len )
            {
                int w = global_id % args_p->w;
                int h = (global_id / args_p->w) % args_p->h;
                int c = ((global_id / args_p->w) / args_p->h) % args_p->c;
                int n = ((global_id / args_p->w) / args_p->h) / args_p->c;

                const float* in_elem_p = ELEM_IN_P( n, c, h, w );
                float* out_elem_p = ELEM_OUT_P( n, c, h, w );

                float res = device_rcpf(1.0f + device_expf(-(*in_elem_p)));
                blendResult(args_p->alpha.f, res, args_p->beta.f, *out_elem_p);
            }
        }
#undef ELEM_OUT_P
#undef ELEM_IN_P
    }
}

KERNEL_ENTRY void ActivationForward_Relu_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
#define ELEM_IN_P(_n, _c, _h, _w) tensor4dElPtr((const float*)args_p->x, _n, _c, _h, _w, args_p->in_nStride, args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_OUT_P(_n, _c, _h, _w) tensor4dElPtr((float*)args_p->y, _n, _c, _h, _w, args_p->out_nStride, args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
        ActivationForward_KernelArgs_t* args_p = reinterpret_cast<ActivationForward_KernelArgs_t*>(args);
        unsigned len = args_p->n*args_p->c*args_p->h*args_p->w;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        {
            if ( global_id < len )
            {
                int w = global_id % args_p->w;
                int h = (global_id / args_p->w) % args_p->h;
                int c = ((global_id / args_p->w) / args_p->h) % args_p->c;
                int n = ((global_id / args_p->w) / args_p->h) / args_p->c;

                const float* in_elem_p = ELEM_IN_P( n, c, h, w );
                float* out_elem_p = ELEM_OUT_P( n, c, h, w );

                float res = device_maxf(0.0f, *in_elem_p);
                blendResult(args_p->alpha.f, res, args_p->beta.f, *out_elem_p);
            }
        }
#undef ELEM_OUT_P
#undef ELEM_IN_P
    }
}

KERNEL_ENTRY void ActivationForward_Tanh_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
#define ELEM_IN_P(_n, _c, _h, _w) tensor4dElPtr((const float*)args_p->x, _n, _c, _h, _w, args_p->in_nStride, args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_OUT_P(_n, _c, _h, _w) tensor4dElPtr((float*)args_p->y, _n, _c, _h, _w, args_p->out_nStride, args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
        ActivationForward_KernelArgs_t* args_p = reinterpret_cast<ActivationForward_KernelArgs_t*>(args);
        unsigned len = args_p->n*args_p->c*args_p->h*args_p->w;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        {
            if ( global_id < len )
            {
                int w = global_id % args_p->w;
                int h = (global_id / args_p->w) % args_p->h;
                int c = ((global_id / args_p->w) / args_p->h) % args_p->c;
                int n = ((global_id / args_p->w) / args_p->h) / args_p->c;

                const float* in_elem_p = ELEM_IN_P( n, c, h, w );
                float* out_elem_p = ELEM_OUT_P( n, c, h, w );

                float res = 2.0f * device_rcpf(1.0f + device_expf(-2.0f * (*in_elem_p))) - 1.0f;
                blendResult(args_p->alpha.f, res, args_p->beta.f, *out_elem_p);
            }
        }
#undef ELEM_OUT_P
#undef ELEM_IN_P
    }
}

static void PoolingForward_Float_Impl(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
#define ELEM_IN(_n, _c, _h, _w) tensor4dEl((const float*)args_p->x, _n, _c, _h, _w, args_p->in_nStride, args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_OUT(_n, _c, _h, _w) tensor4dEl((float*)args_p->y, _n, _c, _h, _w, args_p->out_nStride, args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
        PoolingForward_KernelArgs_t* args_p = reinterpret_cast<PoolingForward_KernelArgs_t*>(args);
        unsigned len = args_p->out_n*args_p->out_c*args_p->out_h*args_p->out_w;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            int w = global_id % args_p->out_w;
            int h = (global_id / args_p->out_w) % args_p->out_h;
            int c = ((global_id / args_p->out_w) / args_p->out_h) % args_p->out_c;
            int n = ((global_id / args_p->out_w) / args_p->out_h) / args_p->out_c;
            int y = (h * args_p->vertical_stride) - args_p->vertical_pad;
            int x = (w * args_p->horizontal_stride) - args_p->horizontal_pad;
            int start_x = device_max(x, 0);
            int start_y = device_max(y, 0);
            int limit_x = device_min(x + args_p->window_w, args_p->in_w);
            int limit_y = device_min(y + args_p->window_h, args_p->in_h);

            float max_val = ELEM_IN(n, c, start_y, start_x);
            float sum = 0.0f;

            for( int i = start_y; i < limit_y; i++)
            {
                for( int j = start_x; j < limit_x; j++)
                {
                    const float cur_val = ELEM_IN(n, c, i, j);
                    max_val = device_maxf(max_val, cur_val);
                    sum += cur_val;
                }
            }

            float result;
            switch (args_p->pooling_function)
            {
              case ETDNN_POOLING_MAX:
                result = max_val;
                break;
              case ETDNN_POOLING_AVERAGE_COUNT_INCLUDE_PADDING:
                result = sum / (args_p->window_w * args_p->window_h);
                break;
              case ETDNN_POOLING_AVERAGE_COUNT_EXCLUDE_PADDING:
                result = sum / ((limit_x - start_x) * (limit_y - start_y));
                break;
              default:
                assert_unreachable();
            }
            float& out_elem = ELEM_OUT( n, c, h, w );
            blendResult(args_p->alpha.f, result, args_p->beta.f, out_elem);
        }
#undef ELEM_OUT
#undef ELEM_IN
    }
}

KERNEL_ENTRY void PoolingForward_Max_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    PoolingForward_Float_Impl(args, state_p, wi_state_p);
}

KERNEL_ENTRY void PoolingForward_AvgIn_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    PoolingForward_Float_Impl(args, state_p, wi_state_p);
}

KERNEL_ENTRY void PoolingForward_AvgEx_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    PoolingForward_Float_Impl(args, state_p, wi_state_p);
}

static void PoolingForward_Int8Q_Impl(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
#define ELEM_IN(_n, _c, _h, _w) tensor4dEl((const int8_t*)args_p->x, _n, _c, _h, _w, args_p->in_nStride, args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_OUT(_n, _c, _h, _w) tensor4dEl((int8_t*)args_p->y, _n, _c, _h, _w, args_p->out_nStride, args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
        PoolingForward_KernelArgs_t* args_p = reinterpret_cast<PoolingForward_KernelArgs_t*>(args);
        unsigned len = args_p->out_n*args_p->out_c*args_p->out_h*args_p->out_w;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            int w = global_id % args_p->out_w;
            int h = (global_id / args_p->out_w) % args_p->out_h;
            int c = ((global_id / args_p->out_w) / args_p->out_h) % args_p->out_c;
            int n = ((global_id / args_p->out_w) / args_p->out_h) / args_p->out_c;
            int y = (h * args_p->vertical_stride) - args_p->vertical_pad;
            int x = (w * args_p->horizontal_stride) - args_p->horizontal_pad;
            int start_x = device_max(x, 0);
            int start_y = device_max(y, 0);
            int limit_x = device_min(x + args_p->window_w, args_p->in_w);
            int limit_y = device_min(y + args_p->window_h, args_p->in_h);

            int32_t max_val = ELEM_IN(n, c, start_y, start_x);
            int32_t sum = 0;

            for( int i = start_y; i < limit_y; i++)
            {
                for( int j = start_x; j < limit_x; j++)
                {
                    const int32_t cur_val = ELEM_IN(n, c, i, j);
                    max_val = device_max(max_val, cur_val);
                    sum += cur_val - args_p->inOffset;
                }
            }

            int32_t int_val;
            float invScale;
            switch (args_p->pooling_function)
            {
              case ETDNN_POOLING_MAX:
                int_val = max_val - args_p->inOffset;
                invScale = args_p->inScale / args_p->outScale;
                break;
              case ETDNN_POOLING_AVERAGE_COUNT_INCLUDE_PADDING:
                int_val = sum;
                invScale = args_p->inScale / args_p->outScale / (args_p->window_w * args_p->window_h);
                break;
              case ETDNN_POOLING_AVERAGE_COUNT_EXCLUDE_PADDING:
                int_val = sum;
                invScale = args_p->inScale / args_p->outScale / ((limit_x - start_x) * (limit_y - start_y));
                break;
              default:
                assert_unreachable();
            }
            int8_t& out_elem = ELEM_OUT( n, c, h, w );
            out_elem = quantization::clip_i32_i8(
                    device_roundf(float(int_val) * invScale + args_p->outOffset));

        }
#undef ELEM_OUT
#undef ELEM_IN
    }
}

KERNEL_ENTRY void PoolingForward_Max_Int8Q_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    PoolingForward_Int8Q_Impl(args, state_p, wi_state_p);
}

KERNEL_ENTRY void PoolingForward_AvgIn_Int8Q_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    PoolingForward_Int8Q_Impl(args, state_p, wi_state_p);
}

KERNEL_ENTRY void PoolingForward_AvgEx_Int8Q_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    PoolingForward_Int8Q_Impl(args, state_p, wi_state_p);
}

KERNEL_ENTRY void BatchNormalizationForwardInference_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
#define ELEM_IN_P(_n, _c, _h, _w) tensor4dElPtr((const float*)args_p->x, _n, _c, _h, _w, args_p->in_nStride, args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_SCALE_P(_n, _c, _h, _w) tensor4dElPtr((const float*)args_p->bnScale, _n, _c, _h, _w, args_p->bn_nStride, args_p->bn_cStride, args_p->bn_hStride, args_p->bn_wStride)
#define ELEM_BIAS_P(_n, _c, _h, _w) tensor4dElPtr((const float*)args_p->bnBias, _n, _c, _h, _w, args_p->bn_nStride, args_p->bn_cStride, args_p->bn_hStride, args_p->bn_wStride)
#define ELEM_MEAN_P(_n, _c, _h, _w) tensor4dElPtr((const float*)args_p->estimatedMean, _n, _c, _h, _w, args_p->bn_nStride, args_p->bn_cStride, args_p->bn_hStride, args_p->bn_wStride)
#define ELEM_VAR_P(_n, _c, _h, _w) tensor4dElPtr((const float*)args_p->estimatedVariance, _n, _c, _h, _w, args_p->bn_nStride, args_p->bn_cStride, args_p->bn_hStride, args_p->bn_wStride)
#define ELEM_OUT_P(_n, _c, _h, _w) tensor4dElPtr((float*)args_p->y, _n, _c, _h, _w, args_p->out_nStride, args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
        BatchNormalizationForwardInference_KernelArgs_t* args_p = reinterpret_cast<BatchNormalizationForwardInference_KernelArgs_t*>(args);
        unsigned len = args_p->n*args_p->c*args_p->h*args_p->w;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        {
            if ( global_id < len )
            {
                int w = global_id % args_p->w;
                int h = (global_id / args_p->w) % args_p->h;
                int c = ((global_id / args_p->w) / args_p->h) % args_p->c;
                int n = ((global_id / args_p->w) / args_p->h) / args_p->c;
                int bw = (args_p->mode == ETDNN_BATCHNORM_PER_ACTIVATION) ? w : 0;
                int bh = (args_p->mode == ETDNN_BATCHNORM_PER_ACTIVATION) ? h : 0;
                int bc = c;
                int bn = 0;

                const float* in_elem_p = ELEM_IN_P( n, c, h, w );
                float* out_elem_p = ELEM_OUT_P( n, c, h, w );
                const float* scale_p = ELEM_SCALE_P( bn, bc, bh, bw );
                const float* bias_p = ELEM_BIAS_P( bn, bc, bh, bw );
                const float* mean_p = ELEM_MEAN_P( bn, bc, bh, bw );
                const float* var_p = ELEM_VAR_P( bn, bc, bh, bw );

                float in = *in_elem_p;
                float scale = *scale_p;
                float bias = *bias_p;
                float mean = *mean_p;
                float var = *var_p;
                float epsilon = args_p->epsilon;

                float res = ((scale*(in-mean))/device_sqrtf(epsilon+var))+bias;
                blendResult(args_p->alpha.f, res, args_p->beta.f, *out_elem_p);
            }
        }
#undef ELEM_OUT_P
#undef ELEM_VAR_P
#undef ELEM_MEAN_P
#undef ELEM_BIAS_P
#undef ELEM_SCALE_P
#undef ELEM_IN_P
    }
}

KERNEL_ENTRY void SoftmaxForward_Instance_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
#define ELEM_IN_P(_n, _c, _h, _w) tensor4dElPtr((const float*)args_p->x, _n, _c, _h, _w, args_p->in_nStride, args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_OUT_P(_n, _c, _h, _w) tensor4dElPtr((float*)args_p->y, _n, _c, _h, _w, args_p->out_nStride, args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
        SoftmaxForward_KernelArgs_t* args_p = reinterpret_cast<SoftmaxForward_KernelArgs_t*>(args);
        unsigned reduce_len = args_p->c*args_p->h*args_p->w;

        float *smem_p  = (float *)__et_get_smem_vars_storage(state_p, wi_state_p);

        // This is 1D Kernel.
        unsigned local_id = wi_state_p->local_id_x;
        unsigned block_size = state_p->block_size_x;
        unsigned block_id = wi_state_p->block_id_x;

        // Number of elements 'reduce_len' which are should be reduced in this block may be bigger then block size,
        // so, each thread linearly reduces their own part into 'partial_sum'.
        float partial_sum = 0.0f;
        for (unsigned lin_id = local_id; lin_id < reduce_len; lin_id += block_size)
        {
            int w = lin_id % args_p->w;
            int h = (lin_id / args_p->w) % args_p->h;
            int c = (lin_id / args_p->w) / args_p->h;
            int n = block_id;

            partial_sum += device_expf(*ELEM_IN_P(n, c, h, w));
        }
        // Each thread in block has their own 'partial_sum' now.
        // We are going to reduce them into one value in 2 steps.

        // Step 1. intra-shire reduce.
        float shire_sum = intra_shire_treduce_fadd(partial_sum);
        if (get_lane_id() == 0) {
            // Only first minion from each shire has meaningful 'shire_sum '.
            // Put this value from each shire into separate cell of block shared memory.
            unsigned local_shire_id = local_id / MINIONS_PER_SHIRE;
            smem_p[local_shire_id] = shire_sum;
        }

        // __syncthreads(): wait all shires put their values into smem
        llvm_nvvm_barrier0(state_p, wi_state_p);

        // Step 2. reduce shire values reading they from smem sequentally in first thread of block.
        if (local_id == 0) {
            float sum = 0.0f;
            for (unsigned i = 0; i < wi_state_p->shires_per_block; i++)
            {
                sum += smem_p[i];
            }
            // put total sum into smem_p[0]
            smem_p[0] = sum;
        }

        // __syncthreads(): ensure total sum in smem_p[0] will be visible to all threads in block
        llvm_nvvm_barrier0(state_p, wi_state_p);

        /*
         * Total sum is calculated here. Only elementwise part of Softmax remains.
         */
        for (unsigned lin_id = local_id; lin_id < reduce_len; lin_id += block_size)
        {
            int w = lin_id % args_p->w;
            int h = (lin_id / args_p->w) % args_p->h;
            int c = (lin_id / args_p->w) / args_p->h;
            int n = block_id;

            float result = device_expf(*ELEM_IN_P(n, c, h, w)) / smem_p[0];

            float *out_elem_p = ELEM_OUT_P(n, c, h, w);
            blendResult(args_p->alpha.f, result, args_p->beta.f, *out_elem_p);
        }
#undef ELEM_OUT_P
#undef ELEM_IN_P
    }
}

KERNEL_ENTRY void SoftmaxForward_Channel_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        // FIXME: implementation needed
        assert_unreachable();
    }
}

KERNEL_ENTRY void TransformTensor_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
#define ELEM_IN(_n, _c, _h, _w) tensor4dEl((const float*)args_p->x, _n, _c, _h, _w, args_p->in_nStride, args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_OUT(_n, _c, _h, _w) tensor4dEl((float*)args_p->y, _n, _c, _h, _w, args_p->out_nStride, args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
        TransformTensor_KernelArgs_t* args_p = reinterpret_cast<TransformTensor_KernelArgs_t*>(args);
        unsigned len = args_p->n*args_p->c*args_p->h*args_p->w;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            int w = global_id % args_p->w;
            int h = (global_id / args_p->w) % args_p->h;
            int c = ((global_id / args_p->w) / args_p->h) % args_p->c;
            int n = ((global_id / args_p->w) / args_p->h) / args_p->c;

            float elem_in = ELEM_IN(n, c, h, w);
            float &elem_out = ELEM_OUT(n, c, h, w);
            blendResult(args_p->alpha.f, elem_in, args_p->beta.f, elem_out);
        }
#undef ELEM_OUT
#undef ELEM_IN
    }
}

KERNEL_ENTRY void TransformTensor_Int8_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
#define ELEM_IN(_n, _c, _h, _w) tensor4dEl((const int8_t*)args_p->x, _n, _c, _h, _w, args_p->in_nStride, args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_OUT(_n, _c, _h, _w) tensor4dEl((int8_t*)args_p->y, _n, _c, _h, _w, args_p->out_nStride, args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
        TransformTensor_KernelArgs_t* args_p = reinterpret_cast<TransformTensor_KernelArgs_t*>(args);
        unsigned len = args_p->n*args_p->c*args_p->h*args_p->w;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            int w = global_id % args_p->w;
            int h = (global_id / args_p->w) % args_p->h;
            int c = ((global_id / args_p->w) / args_p->h) % args_p->c;
            int n = ((global_id / args_p->w) / args_p->h) / args_p->c;

            ELEM_OUT(n, c, h, w) = ELEM_IN(n, c, h, w);
        }
#undef ELEM_OUT
#undef ELEM_IN
    }
}

KERNEL_ENTRY void BatchedAdd_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    BatchedAdd_KernelArgs_t* args_p = reinterpret_cast<BatchedAdd_KernelArgs_t*>(args);
    unsigned len = args_p->n*args_p->slice_size;
    float* dest = args_p->dest;
    const float* batch = args_p->batch;
    const float* slice = args_p->slice;

    // This is 1D Kernel.
    unsigned global_id = get_global_id_x(state_p, wi_state_p);

    if ( global_id < len )
    {
        int s = global_id % args_p->slice_size;

        dest[global_id] = batch[global_id] + slice[s];
    }
}

KERNEL_ENTRY void QuantizedBatchedAdd_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    QuantizedBatchedAdd_KernelArgs_t* args_p = reinterpret_cast<QuantizedBatchedAdd_KernelArgs_t*>(args);
    unsigned len = args_p->n*args_p->slice_size;
    int8_t* dest = args_p->dest;
    const int8_t* batch = args_p->batch;
    const int8_t* slice = args_p->slice;

    // This is 1D Kernel.
    unsigned global_id = get_global_id_x(state_p, wi_state_p);

    if ( global_id < len )
    {
        int i = global_id;
        int s = global_id % args_p->slice_size;

        float batchVal = quantization::dequantize(batch[i], args_p->batchScale, args_p->batchOffset);
        float sliceVal = quantization::dequantize(slice[s], args_p->sliceScale, args_p->sliceOffset);
        float destVal = batchVal + sliceVal;
        dest[i] = quantization::quantize(destVal, args_p->destScale, args_p->destOffset);
    }
}

KERNEL_ENTRY void Insert_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
#define ELEM_IN( _d0, _d1, _d2, _d3) tensor4dEl((const float*)args_p->x, _d0, _d1, _d2, _d3, args_p->x_strides[0], args_p->x_strides[1], args_p->x_strides[2], args_p->x_strides[3])
#define ELEM_OUT( _d0, _d1, _d2, _d3) tensor4dEl((float*)args_p->y, _d0, _d1, _d2, _d3, args_p->y_strides[0], args_p->y_strides[1], args_p->y_strides[2], args_p->y_strides[3])
        Insert_KernelArgs_t* args_p = reinterpret_cast<Insert_KernelArgs_t*>(args);
        unsigned len = args_p->sizes[3]*args_p->sizes[2]*args_p->sizes[1]*args_p->sizes[0];

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        {
            if ( global_id < len )
            {
                int d3 = global_id % args_p->sizes[3];
                int d2 = (global_id / args_p->sizes[3]) % args_p->sizes[2];
                int d1 = ((global_id / args_p->sizes[3]) / args_p->sizes[2]) % args_p->sizes[1];
                int d0 = ((global_id / args_p->sizes[3]) / args_p->sizes[2]) / args_p->sizes[1];

                ELEM_OUT(d0 + args_p->y_offsets[0], d1 + args_p->y_offsets[1], d2 + args_p->y_offsets[2], d3 + args_p->y_offsets[3]) = ELEM_IN(d0 + args_p->x_offsets[0], d1 + args_p->x_offsets[1], d2 + args_p->x_offsets[2], d3 + args_p->x_offsets[3]);
            }
        }
#undef ELEM_IN
#undef ELEM_OUT
    }
}

KERNEL_ENTRY void Insert_Int64_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
#define ELEM_IN( _d0, _d1, _d2, _d3) tensor4dEl((const int64_t*)args_p->x, _d0, _d1, _d2, _d3, args_p->x_strides[0], args_p->x_strides[1], args_p->x_strides[2], args_p->x_strides[3])
#define ELEM_OUT( _d0, _d1, _d2, _d3) tensor4dEl((int64_t*)args_p->y, _d0, _d1, _d2, _d3, args_p->y_strides[0], args_p->y_strides[1], args_p->y_strides[2], args_p->y_strides[3])
        Insert_KernelArgs_t* args_p = reinterpret_cast<Insert_KernelArgs_t*>(args);
        unsigned len = args_p->sizes[3]*args_p->sizes[2]*args_p->sizes[1]*args_p->sizes[0];

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        {
            if ( global_id < len )
            {
                int d3 = global_id % args_p->sizes[3];
                int d2 = (global_id / args_p->sizes[3]) % args_p->sizes[2];
                int d1 = ((global_id / args_p->sizes[3]) / args_p->sizes[2]) % args_p->sizes[1];
                int d0 = ((global_id / args_p->sizes[3]) / args_p->sizes[2]) / args_p->sizes[1];

                ELEM_OUT(d0 + args_p->y_offsets[0], d1 + args_p->y_offsets[1], d2 + args_p->y_offsets[2], d3 + args_p->y_offsets[3]) = ELEM_IN(d0 + args_p->x_offsets[0], d1 + args_p->x_offsets[1], d2 + args_p->x_offsets[2], d3 + args_p->x_offsets[3]);
            }
        }
#undef ELEM_IN
#undef ELEM_OUT
    }
}

KERNEL_ENTRY void Gather_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        Gather_KernelArgs_t* args_p = reinterpret_cast<Gather_KernelArgs_t*>(args);
        unsigned len = args_p->indices_n;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            float* dest_p = &(((float*)args_p->dest)[args_p->data_ydim * global_id]);
            const float* data_p = &(((const float *)args_p->data)[args_p->indices[global_id] * args_p->data_ydim]);

            for ( int i = 0 ; i < args_p->data_ydim ; i++ )
            {
                dest_p[i] = data_p[i];
            }
        }
    }
}

KERNEL_ENTRY void TopK_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        // FIXME: Done on host machine
        assert_unreachable();
    }
}

KERNEL_ENTRY void LRN_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
#define ELEM_IN_P(_n, _c, _h, _w) tensor4dElPtr((const float*)args_p->x, _n, _c, _h, _w, args_p->in_nStride, args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_OUT_P(_n, _c, _h, _w) tensor4dElPtr((float*)args_p->y, _n, _c, _h, _w, args_p->out_nStride, args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
#define ELEM_SCALE_P(_n, _c, _h, _w) tensor4dElPtr((float*)args_p->scale, _n, _c, _h, _w, args_p->scale_nStride, args_p->scale_cStride, args_p->scale_hStride, args_p->scale_wStride)
        LRN_KernelArgs_t* args_p = reinterpret_cast<LRN_KernelArgs_t*>(args);
        unsigned len = args_p->n*args_p->c*args_p->h*args_p->w;
        float window_size_half = args_p->halfWindowSize;
        float window_size = window_size_half * 2 + 1;
        float normed_alpha = args_p->alpha.f / window_size;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        {
            if ( global_id < len )
            {
                int w = global_id % args_p->w;
                int h = (global_id / args_p->w) % args_p->h;
                int c = ((global_id / args_p->w) / args_p->h) % args_p->c;
                int n = ((global_id / args_p->w) / args_p->h) / args_p->c;
                int c_min = (c >= window_size_half) ? (c - window_size_half) : 0;
                int c_max = c + window_size_half;
                int c_limit = args_p->c - 1;
                float square_sum = 0.0;

                c_max = c_max < c_limit ? c_max : c_limit;

                for ( int c_win = c_min ; c_win <= c_max; c_win++ )
                {
                    float in_value = *ELEM_IN_P(n, c_win, h, w);

                    square_sum += in_value * in_value;
                }

                const float* in_elem_p = ELEM_IN_P( n, c, h, w );
                float* out_elem_p = ELEM_OUT_P( n, c, h, w );
                float* scale_elem_p = ELEM_OUT_P( n, c, h, w );
                float scale = normed_alpha * square_sum + args_p->k.f;

                *scale_elem_p = scale;

                *out_elem_p = (*in_elem_p) * device_powf(scale, -args_p->beta.f);
            }
        }
#undef ELEM_SCALE_P
#undef ELEM_OUT_P
#undef ELEM_IN_P
    }
}

KERNEL_ENTRY void LogKernel_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        unsigned len = *reinterpret_cast<unsigned*>(args + 0);
        float* devPtrX = *reinterpret_cast<float**>(args + 8);
        float* devPtrY = *reinterpret_cast<float**>(args + 16);

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            devPtrY[global_id] = device_logf(devPtrX[global_id]);
        }
    }
}

KERNEL_ENTRY void QuantizeKernel_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    unsigned len = *reinterpret_cast<unsigned*>(args + 0);
    float* devPtrX = *reinterpret_cast<float**>(args + 8);
    int8_t* devPtrY = *reinterpret_cast<int8_t**>(args + 16);
    float scale = *reinterpret_cast<float*>(args + 24);
    int32_t offset = *reinterpret_cast<int32_t*>(args + 28);

    // This is 1D Kernel.
    unsigned global_id = get_global_id_x(state_p, wi_state_p);

    if ( global_id < len )
    {
        devPtrY[global_id] = quantization::quantize(devPtrX[global_id], scale, offset);
    }
}

KERNEL_ENTRY void DequantizeKernel_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    unsigned len = *reinterpret_cast<unsigned*>(args + 0);
    int8_t* devPtrX = *reinterpret_cast<int8_t**>(args + 8);
    float* devPtrY = *reinterpret_cast<float**>(args + 16);
    float scale = *reinterpret_cast<float*>(args + 24);
    int32_t offset = *reinterpret_cast<int32_t*>(args + 28);

    // This is 1D Kernel.
    unsigned global_id = get_global_id_x(state_p, wi_state_p);

    if ( global_id < len )
    {
        devPtrY[global_id] = quantization::dequantize(devPtrX[global_id], scale, offset);
    }
}

KERNEL_ENTRY void RescaleQuantizedKernel_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    unsigned len = *reinterpret_cast<unsigned*>(args + 0);
    int8_t* devPtrX = *reinterpret_cast<int8_t**>(args + 8);
    int8_t* devPtrY = *reinterpret_cast<int8_t**>(args + 16);
    float xScale = *reinterpret_cast<float*>(args + 24);
    int32_t xOffset = *reinterpret_cast<int32_t*>(args + 28);
    float yScale = *reinterpret_cast<float*>(args + 32);
    int32_t yOffset = *reinterpret_cast<int32_t*>(args + 36);

    // This is 1D Kernel.
    unsigned global_id = get_global_id_x(state_p, wi_state_p);

    if ( global_id < len )
    {
        float val = quantization::dequantize(devPtrX[global_id], xScale, xOffset);
        devPtrY[global_id] = quantization::quantize(val, yScale, yOffset);
    }
}

KERNEL_ENTRY void Splat_Float_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        Splat_KernelArgs_t* args_p = reinterpret_cast<Splat_KernelArgs_t*>(args);
        unsigned len = args_p->count;
        float* dest = (float*)args_p->y;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            dest[global_id] = args_p->data.f;
        }
    }
}

KERNEL_ENTRY void Splat_Int8_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        Splat_KernelArgs_t* args_p = reinterpret_cast<Splat_KernelArgs_t*>(args);
        unsigned len = args_p->count;
        signed char* dest = (signed char*)args_p->y;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            dest[global_id] = args_p->data.i8;
        }
    }
}

KERNEL_ENTRY void Splat_Int32_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        Splat_KernelArgs_t* args_p = reinterpret_cast<Splat_KernelArgs_t*>(args);
        unsigned len = args_p->count;
        int* dest = (int*)args_p->y;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            dest[global_id] = args_p->data.i32;
        }
    }
}

KERNEL_ENTRY void Splat_Int64_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    {
        Splat_KernelArgs_t* args_p = reinterpret_cast<Splat_KernelArgs_t*>(args);
        unsigned len = args_p->count;
        int64_t* dest = (int64_t*)args_p->y;

        // This is 1D Kernel.
        unsigned global_id = get_global_id_x(state_p, wi_state_p);

        if ( global_id < len )
        {
            dest[global_id] = args_p->data.i64;
        }
    }
}

static void ArithOp_Int8Q_Impl(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    QuantizedArithOp_KernelArgs_t* args_p = reinterpret_cast<QuantizedArithOp_KernelArgs_t*>(args);
    unsigned len = args_p->n;

    // This is 1D Kernel.
    unsigned global_id = get_global_id_x(state_p, wi_state_p);

    if ( global_id < len )
    {
        float aVal = quantization::dequantize(args_p->a[global_id], args_p->aScale, args_p->aOffset);
        float bVal = quantization::dequantize(args_p->b[global_id], args_p->bScale, args_p->bOffset);
        float yVal;
        switch (args_p->op)
        {
            case ETDNN_TENSOR_ARITH_OP_ADD: yVal = aVal + bVal; break;
            case ETDNN_TENSOR_ARITH_OP_SUB: yVal = aVal - bVal; break;
            case ETDNN_TENSOR_ARITH_OP_MUL: yVal = aVal * bVal; break;
            case ETDNN_TENSOR_ARITH_OP_DIV: yVal = aVal / bVal; break;
            case ETDNN_TENSOR_ARITH_OP_MAX: yVal = device_maxf(aVal, bVal); break;
            case ETDNN_TENSOR_ARITH_OP_MIN: yVal = device_minf(aVal, bVal); break;
            default:
                assert_unreachable();
        }
        args_p->y[global_id] = quantization::quantize(yVal, args_p->yScale, args_p->yOffset);
    }
}

KERNEL_ENTRY void AddKernel_Int8Q_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    ArithOp_Int8Q_Impl(args, state_p, wi_state_p);
}

KERNEL_ENTRY void SubKernel_Int8Q_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    ArithOp_Int8Q_Impl(args, state_p, wi_state_p);
}

KERNEL_ENTRY void MulKernel_Int8Q_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    ArithOp_Int8Q_Impl(args, state_p, wi_state_p);
}

KERNEL_ENTRY void DivKernel_Int8Q_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    ArithOp_Int8Q_Impl(args, state_p, wi_state_p);
}

KERNEL_ENTRY void MaxKernel_Int8Q_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    ArithOp_Int8Q_Impl(args, state_p, wi_state_p);
}

KERNEL_ENTRY void MinKernel_Int8Q_ETKERNEL_entry_point(char* args, KernelRuntimeState_t* state_p, KernelWorkItemState_t* wi_state_p)
{
    ArithOp_Int8Q_Impl(args, state_p, wi_state_p);
}

