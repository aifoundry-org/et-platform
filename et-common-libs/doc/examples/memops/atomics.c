/*
  Example of using atomics apis.
  Below is an example which demonstrates how to use atomics apis to perform atomic operations.
*/

/* Include api specific header */
#include <etsoc/isa/atomics.h>
#include "utils.h"

/* Declare global variables */
uint8_t g_tmp_data_8 = 100;
uint8_t g_tmp_data_16 = 200;
uint8_t g_tmp_data_32 = 300;
uint8_t g_tmp_data_64 = 400;

int main(void)
{
    /* Declare a test variable */
    uint8_t tmp_data_8 = 100;
    uint8_t tmp_res_8;

    /* Load one byte data using atomics*/
    tmp_res_8 = atomic_load_local_8(&test_data_8);
    et_printf("atomic_load_local_8 data: %d\n", tmp_res_8);

    /* Declare a test variable */
    uint8_t tmp_data_16 = 100;
    uint8_t tmp_res_16;

    /* Load 2-byte data using atomics*/
    tmp_res_16 = atomic_load_local_16(&tmp_data_16);
    et_printf("atomic_load_local_16 data: %d\n", tmp_res_16);

    /* Declare a test variable */
    uint8_t tmp_data_32 = 100;
    uint8_t tmp_res_32;

    /* Load 4-byte data using atomics*/
    tmp_res_32 = atomic_load_local_32(&tmp_data_32);
    et_printf("atomic_load_local_32 data: %d\n", tmp_res_32);

    /* Declare a test variable */
    uint8_t tmp_data_64 = 100;
    uint8_t tmp_res_64;

    /* Load 8-byte data using atomics*/
    tmp_res_64 = atomic_load_local_64(&tmp_data_64);
    et_printf("atomic_load_local_64 data: %d\n", tmp_res_64);

    /* Load one byte data using atomics*/
    tmp_res_8 = atomic_load_global_8(&g_tmp_data_8);
    et_printf("atomic_load_global_8 data: %d\n", tmp_res_8);

    /* Load 2-byte data using atomics*/
    tmp_res_16 = atomic_load_global_16(&g_tmp_data_16);
    et_printf("atomic_load_global_16 data: %d\n", tmp_res_16);

    /* Load 4-byte data using atomics*/
    tmp_res_32 = atomic_load_global_32(&g_tmp_data_32);
    et_printf("atomic_load_global_32 data: %d\n", tmp_res_32);

    /* Load 8-byte data using atomics*/
    tmp_res_64 = atomic_load_global_64(&g_tmp_data_64);
    et_printf("atomic_load_global_64 data: %d\n", tmp_res_64);

    /* Load one byte data using atomics*/
    tmp_res_8 = atomic_load_signed_local_8(&test_data_8);
    et_printf("atomic_load_signed_local_8 data: %d\n", tmp_res_8);

    /* Load 2-byte data using atomics*/
    tmp_res_16 = atomic_load_signed_local_16(&tmp_data_16);
    et_printf("atomic_load_signed_local_16 data: %d\n", tmp_res_16);

    /* Load one 4-byte data using atomics*/
    tmp_res_32 = atomic_load_signed_local_32(&tmp_data_32);
    et_printf("atomic_load_signed_local_32 data: %d\n", tmp_res_32);

    /* Load 8-byte data using atomics*/
    tmp_res_64 = atomic_load_signed_local_64(&tmp_data_64);
    et_printf("atomic_load_signed_local_64 data: %d\n", tmp_res_64);

    /* Load one byte data using atomics*/
    tmp_res_8 = atomic_load_signed_global_8(&g_tmp_data_8);
    et_printf("atomic_load_signed_global_8 data: %d\n", tmp_res_8);

    /* Load 2-byte data using atomics*/
    tmp_res_16 = atomic_load_signed_global_16(&g_tmp_data_16);
    et_printf("atomic_load_signed_global_16 data: %d\n", tmp_res_16);

    /* Load 4-byte data using atomics*/
    tmp_res_32 = atomic_load_signed_global_32(&g_tmp_data_32);
    et_printf("atomic_load_signed_global_32 data: %d\n", tmp_res_32);

    /* Load 8-byte data using atomics*/
    tmp_res_64 = atomic_load_signed_global_64(&g_tmp_data_64);
    et_printf("atomic_load_signed_global_64 data: %d\n", tmp_res_64);

    /* Store one byte data using atomics*/
    atomic_store_local_8(&test_data_8, 100);

    /* Store 2-byte data using atomics*/
    atomic_store_local_16(&tmp_data_16, 200);

    /* Store 4-byte data using atomics*/
    atomic_store_local_32(&tmp_data_32, 300);

    /* Store 8-byte data using atomics*/
    atomic_store_local_64(&tmp_data_64, 400);

    /* Store one byte data using atomics*/
    atomic_store_global_8(&g_tmp_data_8);

    /* Store 2-byte data using atomics*/
    atomic_store_global_16(&g_tmp_data_16);

    /* Store 4-byte data using atomics*/
    atomic_store_global_32(&g_tmp_data_32);

    /* Store 8-byte data using atomics*/
    atomic_store_global_64(&g_tmp_data_64);

    /* Store one byte data using atomics*/
    atomic_store_signed_local_8(&test_data_8);

    /* Store 2-byte data using atomics*/
    atomic_store_signed_local_16(&tmp_data_16);

    /* Store one 4-byte data using atomics*/
    atomic_store_signed_local_32(&tmp_data_32);

    /* Store 8-byte data using atomics*/
    atomic_store_signed_local_64(&tmp_data_64);

    /* Store one byte data using atomics*/
    atomic_store_signed_global_8(&g_tmp_data_8);

    /* Store 2-byte data using atomics*/
    atomic_store_signed_global_16(&g_tmp_data_16);

    /* Store 4-byte data using atomics*/
    atomic_store_signed_global_32(&g_tmp_data_32);

    /* Store 8-byte data using atomics*/
    atomic_store_signed_global_64(&g_tmp_data_64);

    /* Exchange 4-byte data using atomics*/
    tmp_res_32 = atomic_exchange_local_32(&tmp_data_32, 500);
    et_printf("atomic_exchange_local_32 data: %d\n", tmp_res_32);

    /* Exchange 8-byte data using atomics*/
    tmp_res_64 = atomic_exchange_local_64(&tmp_data_64, 1234);
    et_printf("atomic_exchange_local_64 data: %d\n", tmp_res_64);

    /* Exchange 4-byte data using atomics*/
    tmp_res_32 = atomic_exchange_global_32(&g_tmp_data_32, 500);
    et_printf("atomic_exchange_global_32 data: %d\n", tmp_res_32);

    /* Exchange 8-byte data using atomics*/
    tmp_res_64 = atomic_exchange_global_32(&g_tmp_data_64, 1234);
    et_printf("atomic_exchange_global_32 data: %d\n", tmp_res_64);

    /* Add 4-byte data using atomics*/
    tmp_res_32 = atomic_add_local_32(&tmp_data_32, 5);
    et_printf("atomic_add_local_32 data: %d\n", tmp_res_32);

    /* Add 8-byte data using atomics*/
    tmp_res_64 = atomic_add_local_64(&tmp_data_64, 1);
    et_printf("atomic_add_local_64 data: %d\n", tmp_res_64);

    /* Add 4-byte data using atomics*/
    tmp_res_32 = atomic_add_global_32(&g_tmp_data_32, 1);
    et_printf("atomic_add_global_32 data: %d\n", tmp_res_32);

    /* Add 8-byte data using atomics*/
    tmp_res_64 = atomic_add_global_64(&g_tmp_data_64, 1);
    et_printf("atomic_add_global_64 data: %d\n", tmp_res_64);

    /* Add 4-byte data using atomics*/
    tmp_res_32 = atomic_add_signed_local_32(&tmp_data_32, 1);
    et_printf("atomic_add_signed_local_32 data: %d\n", tmp_res_32);

    /* Add 8-byte data using atomics*/
    tmp_res_64 = atomic_add_signed_local_64(&tmp_data_64, 1);
    et_printf("atomic_add_signed_local_64 data: %d\n", tmp_res_64);

    /* AND 4-byte data using atomics*/
    tmp_res_32 = atomic_and_local_32(&tmp_data_32, 1);
    et_printf("atomic_and_local_32 data: %d\n", tmp_res_32);

    /* AND 8-byte data using atomics*/
    tmp_res_64 = atomic_and_local_64(&tmp_data_64, 1);
    et_printf("atomic_and_local_64 data: %d\n", tmp_res_64);

    /* AND 4-byte data using atomics*/
    tmp_res_32 = atomic_and_global_32(&g_tmp_data_32, 1);
    et_printf("atomic_and_global_32 data: %d\n", tmp_res_32);

    /* AND 8-byte data using atomics*/
    tmp_res_64 = atomic_and_global_64(&g_tmp_data_64, 1);
    et_printf("atomic_and_global_64 data: %d\n", tmp_res_64);

    /* OR 4-byte data using atomics*/
    tmp_res_32 = atomic_or_local_32(&tmp_data_32, 1);
    et_printf("atomic_or_local_32 data: %d\n", tmp_res_32);

    /* OR 8-byte data using atomics*/
    tmp_res_64 = atomic_or_local_64(&tmp_data_64, 1);
    et_printf("atomic_or_local_64 data: %d\n", tmp_res_64);

    /* OR 4-byte data using atomics*/
    tmp_res_32 = atomic_or_global_32(&g_tmp_data_32, 1);
    et_printf("atomic_or_global_32 data: %d\n", tmp_res_32);

    /* OR 8-byte data using atomics*/
    tmp_res_64 = atomic_or_global_64(&g_tmp_data_64, 1);
    et_printf("atomic_or_global_64 data: %d\n", tmp_res_64);

    /* Compare and exchange 4-byte data using atomics*/
    tmp_res_32 = atomic_compare_and_exchange_local_32(&tmp_data_32, 1, 0);
    et_printf("atomic_compare_and_exchange_local_32 data: %d\n", tmp_res_32);

    /* Compare and exchange 8-byte data using atomics*/
    tmp_res_64 = atomic_compare_and_exchange_local_64(&tmp_data_64, 1, 0);
    et_printf("atomic_compare_and_exchange_local_64 data: %d\n", tmp_res_64);

    /* Compare and exchange 4-byte data using atomics*/
    tmp_res_32 = atomic_compare_and_exchange_global_32(&g_tmp_data_32, 1, 0);
    et_printf("atomic_compare_and_exchange_global_32 data: %d\n", tmp_res_32);

    /* Compare and exchange 8-byte data using atomics*/
    tmp_res_64 = atomic_compare_and_exchange_global_64(&g_tmp_data_64, 1, 0);
    et_printf("atomic_compare_and_exchange_global_64 data: %d\n", tmp_res_64);
    return 0;
}