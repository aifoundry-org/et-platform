
#ifndef _VPU_POWER_VIRUS_KERNEL_
#define _VPU_POWER_VIRUS_KERNEL_ 1

void vpu_pv_set_m0_m1(uint64_t m0_m1);
void vpu_pv_load_f0(volatile uint32_t *mem);
void vpu_pv_load_f1(volatile uint32_t *mem);
void vpu_pv_load_f2(volatile uint32_t *mem);
void vpu_pv_load_f3(volatile uint32_t *mem);
void vpu_pv_load_f28(volatile uint32_t *mem);
void vpu_pv_load_f29(volatile uint32_t *mem);
void vpu_pv_load_f30(volatile uint32_t *mem);
void vpu_pv_load_f31(volatile uint32_t *mem);
void vpu_pv_setup0(void);
void vpu_pv_setup1(void);
uint64_t vpu_power_virus(uint64_t loop_size);

void vpu_pv_set_m0_m1(uint64_t m0_m1) {
   __asm__ __volatile__ (
      "mova.m.x %[m0_m1]"
      :
      : [m0_m1] "r" (m0_m1)
      :
   );
}

void vpu_pv_load_f0(volatile uint32_t *mem) {
   __asm__ __volatile__ (
      "flq2 f0, 0(%[ptr1])\n"
      :
      : [ptr1]  "r" (mem)
      :
   );
}

void vpu_pv_load_f1(volatile uint32_t *mem) {
   __asm__ __volatile__ (
      "flq2 f1, 0(%[ptr1])\n"
      :
      : [ptr1]  "r" (mem)
      :
   );
}

void vpu_pv_load_f2(volatile uint32_t *mem) {
   __asm__ __volatile__ (
      "flq2 f2, 0(%[ptr1])\n"
      :
      : [ptr1]  "r" (mem)
      :
   );
}

void vpu_pv_load_f3(volatile uint32_t *mem) {
   __asm__ __volatile__ (
      "flq2 f3, 0(%[ptr1])\n"
      :
      : [ptr1]  "r" (mem)
      :
   );
}

void vpu_pv_load_f28(volatile uint32_t *mem) {
   __asm__ __volatile__ (
      "flq2 f28, 0(%[ptr1])\n"
      :
      : [ptr1]  "r" (mem)
      :
   );
}

void vpu_pv_load_f29(volatile uint32_t *mem) {
   __asm__ __volatile__ (
      "flq2 f29, 0(%[ptr1])\n"
      :
      : [ptr1]  "r" (mem)
      :
   );
}

void vpu_pv_load_f30(volatile uint32_t *mem) {
   __asm__ __volatile__ (
      "flq2 f30, 0(%[ptr1])\n"
      :
      : [ptr1]  "r" (mem)
      :
   );
}

void vpu_pv_load_f31(volatile uint32_t *mem) {
   __asm__ __volatile__ (
      "flq2 f31, 0(%[ptr1])\n"
      :
      : [ptr1]  "r" (mem)
      :
   );
}

void vpu_pv_setup0() {
   volatile uint32_t fp0[8] = {0x40cfdec3, 0x410fe49d, 0x405f4e4c, 0x3ed132d3, 0x4117d1b8, 0x40f4cb93, 0x3fa20bd3, 0x40a14d56};
   volatile uint32_t fp1[8] = {0x41244a09, 0x40e45af4, 0x40bf0f57, 0x411f4990, 0x4056f289, 0x40091346, 0x408e2e84, 0x40c56b2a};
   volatile uint32_t fp2[8] = {0x40ccf1ce, 0x40745d3c, 0x40f93613, 0x401f6c18, 0x410ba60c, 0x411456a3, 0x4030af38, 0x402223dd};
   vpu_pv_load_f1(fp0);
   vpu_pv_load_f2(fp1);
   vpu_pv_load_f3(fp2);

   volatile uint32_t gold_ref_0[8] = {0x429235ef, 0x4287fda5, 0x41e4f651, 0x40d1e0ba, 0x422262a8, 0x41cd3eb1, 0x41062bcf, 0x42068647};
   vpu_pv_load_f0(gold_ref_0);

   /****************************************************************************************************************************************************************************/

   volatile uint32_t fp3[8] = {0x40fd87f5, 0x40fe7503, 0x404f9d7f, 0x3face846, 0x4103dcef, 0x412ea37d, 0x40b9568e, 0x4121bdb1};
   volatile uint32_t fp4[8] = {0x4122af59, 0x4123d522, 0x411433bc, 0x40d216e0, 0x40caff35, 0x3fbe4dda, 0x41013746, 0x3f67eda3};
   volatile uint32_t fp5[8] = {0x411238b6, 0x3f345ed4, 0x40c1f1d5, 0x404e668d, 0x3f65eb4d, 0x40b63b29, 0x40662350, 0x3e50551c};
   vpu_pv_load_f28(fp3);
   vpu_pv_load_f29(fp4);
   vpu_pv_load_f30(fp5);

   volatile uint32_t gold_ref_1[8] = {0x42b364c8, 0x42a44118, 0x42106f3d, 0x41417fa0, 0x4254b73f, 0x41af613d, 0x42497b79, 0x4115c9a3};
   vpu_pv_load_f31(gold_ref_1);
}

void vpu_pv_setup1() {

   volatile uint32_t fp0[8] = {0x4095bbd6, 0x411e38cf, 0x412980c6, 0x40972723, 0x4102c487, 0x40f2aa23, 0x41246c7b, 0x4128019d};
   volatile uint32_t fp1[8] = {0x410ee5f3, 0x3ff7d9ee, 0x40ef7179, 0x3fec5c94, 0x410901a4, 0x403cbc4e, 0x401cba34, 0x407674f9};
   volatile uint32_t fp2[8] = {0x40a2229f, 0x40fa685c, 0x40311e78, 0x409af716, 0x40db6a5d, 0x3ff18f31, 0x3f8f6349, 0x41217f77};
   vpu_pv_load_f1(fp0);
   vpu_pv_load_f2(fp1);
   vpu_pv_load_f3(fp2);

   volatile uint32_t gold_ref_0[8] = {0x423b6db4, 0x41d7c998, 0x42a4134f, 0x41590a49, 0x4299aeab, 0x41c20064, 0x41d2499b, 0x424a1e2f};
   vpu_pv_load_f0(gold_ref_0);

   /****************************************************************************************************************************************************************************/

   volatile uint32_t fp3[8] = {0x408e8153, 0x41239df7, 0x41175c98, 0x40f450e9, 0x3fd7d4e0, 0x410a2166, 0x411509a9, 0x411de35f};
   volatile uint32_t fp4[8] = {0x40e2e8ef, 0x40c23ac1, 0x407da4f9, 0x411ce5c8, 0x402f3bff, 0x411fa93a, 0x40b3edab, 0x40149d95};
   volatile uint32_t fp5[8] = {0x40faf3f6, 0x4042c4b4, 0x4125ef8f, 0x3e24489b, 0x412e2c13, 0x40fba4fe, 0x4093f9f1, 0x3f1185b1};
   vpu_pv_load_f28(fp3);
   vpu_pv_load_f29(fp4);
   vpu_pv_load_f30(fp5);

   volatile uint32_t gold_ref_1[8] = {0x421dae54, 0x42823968, 0x423f73f1, 0x42960ebe, 0x41780aa0, 0x42bc066b, 0x4263ff7b, 0x41bbdd74};
   vpu_pv_load_f31(gold_ref_1);
}

uint64_t vpu_power_virus(uint64_t loop_size) {
   uint64_t mask_gold_ref = 0xffffffffff;
   vpu_pv_set_m0_m1(mask_gold_ref);

   uint64_t tid = get_thread_id();
   if (tid == 0) {
      vpu_pv_setup0();
   }
   else {
      vpu_pv_setup1();
   }

   for (uint64_t i = 0; i < loop_size; i++) {

      for (int j = 0; j < 20; j++) {
         __asm__ __volatile__ (
            "fmadd.ps   f4, f1, f2, f3\n"
            "fmadd.ps   f5, f28, f29, f30\n"
            "fmadd.ps   f6, f1, f2, f3\n"
            "fmadd.ps   f7, f28, f29, f30\n"
            "fmadd.ps   f8, f1, f2, f3\n"
            "fmadd.ps   f9, f28, f29, f30\n"
            "fmadd.ps  f10, f1, f2, f3\n"
            "fmadd.ps  f11, f28, f29, f30\n"
            "fmadd.ps  f12, f1, f2, f3\n"
            "fmadd.ps  f13, f28, f29, f30\n"
            "fmadd.ps  f14, f1, f2, f3\n"
            "fmadd.ps  f15, f28, f29, f30\n"
            "fmadd.ps  f16, f1, f2, f3\n"
            "fmadd.ps  f17, f28, f29, f30\n"
            "fmadd.ps  f18, f1, f2, f3\n"
            "fmadd.ps  f19, f28, f29, f30\n"
            "fmadd.ps  f20, f1, f2, f3\n"
            "fmadd.ps  f21, f28, f29, f30\n"
            "fmadd.ps  f22, f1, f2, f3\n"
            "fmadd.ps  f23, f28, f29, f30\n"
            "fmadd.ps  f24, f1, f2, f3\n"
            "fmadd.ps  f25, f28, f29, f30\n"
            "fmadd.ps  f26, f1, f2, f3\n"
            "fmadd.ps  f27, f28, f29, f30\n"
         );
      }

      __asm__ __volatile__ (
         "feqm.ps m2, f0, f4\n"
         "maskand m1, m1, m2\n"
         
         "feqm.ps m4, f31, f5\n"
         "maskand m3, m3, m4\n"
         
         "feqm.ps m2, f0, f6\n"
         "maskand m1, m1, m2\n"
         
         "feqm.ps m4, f31, f7\n"
         "maskand m3, m3, m4\n"
         
         "feqm.ps m2, f0, f8\n"
         "maskand m1, m1, m2\n"
         
         "feqm.ps m4, f31, f9\n"
         "maskand m3, m3, m4\n"
         
         "feqm.ps m2, f0, f10\n"
         "maskand m1, m1, m2\n"
         
         "feqm.ps m4, f31, f11\n"
         "maskand m3, m3, m4\n"
         
         "feqm.ps m2, f0, f12\n"
         "maskand m1, m1, m2\n"
         
         "feqm.ps m4, f31, f13\n"
         "maskand m3, m3, m4\n"
         
         "feqm.ps m2, f0, f14\n"
         "maskand m1, m1, m2\n"
         
         "feqm.ps m4, f31, f15\n"
         "maskand m3, m3, m4\n"
         
         "feqm.ps m2, f0, f16\n"
         "maskand m1, m1, m2\n"
         
         "feqm.ps m4, f31, f17\n"
         "maskand m3, m3, m4\n"
         
         "feqm.ps m2, f0, f18\n"
         "maskand m1, m1, m2\n"
         
         "feqm.ps m4, f31, f19\n"
         "maskand m3, m3, m4\n"
         
         "feqm.ps m2, f0, f20\n"
         "maskand m1, m1, m2\n"
         
         "feqm.ps m4, f31, f21\n"
         "maskand m3, m3, m4\n"
         
         "feqm.ps m2, f0, f22\n"
         "maskand m1, m1, m2\n"
         
         "feqm.ps m4, f31, f23\n"
         "maskand m3, m3, m4\n"
         
         "feqm.ps m2, f0, f24\n"
         "maskand m1, m1, m2\n"
         
         "feqm.ps m4, f31, f25\n"
         "maskand m3, m3, m4\n"
         
         "feqm.ps m2, f0, f26\n"
         "maskand m1, m1, m2\n"
         
         "feqm.ps m4, f31, f27\n"
         "maskand m3, m3, m4\n"
      );

      uint64_t mask_regs;
      __asm__ __volatile__ (
         "mova.x.m %[mask_regs]"
         : [mask_regs] "=r" (mask_regs)
         :
         :
      );

      if (mask_regs != mask_gold_ref) {
         return 0x1;
      }
   }
   return 0x0;
}
#endif
