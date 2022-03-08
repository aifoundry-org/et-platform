#include "etsoc/isa/hart.h"
#include "etsoc/isa/cacheops-umode.h"
#include "etsoc/common/utils.h"

inline int min(int a, int b) { return a < b ? a : b; }

typedef struct {
  int* a;
  int* b;
  int* result;
  int numElements;
} MyVectors;

// use only one shire to exec this simple test
int main(const MyVectors* const vectors) {

    /**************************/
    /* Shire services testing */
    /**************************/
    int tid = (int)get_thread_id();
    if (tid == 0) return 0;
    int mid = (int)get_minion_id();
    int numWorkers = SOC_MINIONS_PER_SHIRE;
    int elemsPerWorker = (vectors->numElements + numWorkers - 1) / numWorkers;
    if (elemsPerWorker % 16) {
        elemsPerWorker += 16 - (elemsPerWorker % 16);
    }
    int init = elemsPerWorker * mid;
    int end = min(elemsPerWorker * (mid + 1), vectors->numElements);

    for (int i = init; i < end; ++i) {
        vectors->result[i] = vectors->a[i] + vectors->b[i];
    }

    /************************/
    /* U-mode utils testing */
    /************************/
    int64_t status;

    /* Only a single thread in kernel */
    if((get_shire_id() == 0) && ((get_minion_id() & 0x1f) == 0) && (get_thread_id() == 1))
    {
        /* Test memset */
        et_memset(&vectors->a[0], 1, 8);

        /* Test memcpy */
        et_memcpy(&vectors->b[0], &vectors->a[0], 8);

        /* Test memcpy */
        et_memcmp(&vectors->a[0], &vectors->b[0], 8);

        /* Test strlen */
        et_strlen((char*)&vectors->a[0]);

        /****************************************/
        /* Priviledged Cache Ops U-mode testing */
        /****************************************/

        status = cache_ops_priv_l1_cache_lock_sw(0, 0x8102000000);
        et_assert(status == 0)

        /* L1 D-cache is in split with scratchpad enabled. unlock set 14 and 15 associated with thread 1 */
        status = cache_ops_priv_l1_cache_unlock_sw(0, 14);
        et_assert(status == 0)
        status = cache_ops_priv_l1_cache_unlock_sw(0, 15);
        et_assert(status == 0)
    }

    /* use_tmask=0, dst=1 (L2/SP_RAM), set=0, way=0, num_lines=15 */
    status = cache_ops_priv_evict_sw(0, to_L2, 0, 0, 15);
    et_assert(status == 0)

    status = cache_ops_priv_cache_invalidate(1, 0);
    et_assert(status == 0)

#if 0 /* flush_sw not available on SysEmu */
    /* use_tmask=0, dst=1 (L2/SP_RAM), set=0, way=0, num_lines=15 */
    status = cache_ops_priv_flush_sw(0, to_L2, 0, 0, 15);
    et_assert(status == 0)
#endif

    return 0;
}
