typedef struct {
   volatile uint32_t ts_irq_enable;          
   volatile uint32_t ts_irq_status;          
   volatile uint32_t ts_irq_clr;             
   volatile uint32_t ts_irq_test;            
   volatile uint32_t ts_sdif_rdata;          
   volatile uint32_t ts_sdif_done;           
   volatile uint32_t ts_sdif_data;           
   volatile uint32_t unimplemented_ts_0;     
   volatile uint32_t ts_alarma_cfg;          
   volatile uint32_t ts_alarmb_cfg;          
   volatile uint32_t ts_smpl_hilo;           
   volatile uint32_t ts_hilo_reset;         
   volatile uint32_t unimplemented_ts_1 [4];
} TS_regs, *PTR_TS;

typedef struct {
   volatile uint32_t pd_irq_enable;          
   volatile uint32_t pd_irq_status;         
   volatile uint32_t pd_irq_clr;             
   volatile uint32_t pd_irq_test;            
   volatile uint32_t pd_sdif_rdata;          
   volatile uint32_t pd_sdif_done;           
   volatile uint32_t pd_sdif_data;           
   volatile uint32_t unimplemented_pd_0;     
   volatile uint32_t pd_alarma_cfg;          
   volatile uint32_t pd_alarmb_cfg;          
   volatile uint32_t pd_smpl_hilo;           
   volatile uint32_t pd_hilo_reset;          
   volatile uint32_t unimplemented_pd_1 [4];
} PD_regs, *PTR_PD;

typedef struct {
   volatile uint32_t vm_irq_enable;        
   volatile uint32_t vm_irq_status;        
   volatile uint32_t vm_irq_clr;           
   volatile uint32_t vm_irq_test;          
   volatile uint32_t vm_irq_alarma_enable; 
   volatile uint32_t vm_irq_alarma_status; 
   volatile uint32_t vm_irq_alarma_clr;    
   volatile uint32_t vm_irq_alarma_test;   
   volatile uint32_t vm_irq_alarmb_enable; 
   volatile uint32_t vm_irq_alarmb_status; 
   volatile uint32_t vm_irq_alarmb_clr;    
   volatile uint32_t vm_irq_alarmb_test;   
   volatile uint32_t vm_sdif_rdata;
   volatile uint32_t vm_sdif_done;      
} VM_regs, *PTR_VM;

typedef struct {
   volatile uint32_t vm_ch_alarma_cfg;
   volatile uint32_t vm_ch_alarmb_cfg;
   volatile uint32_t vm_ch_smpl_hilo;
   volatile uint32_t vm_ch_hilo_reset;     
} VM_regs_per_ch, *PTR_VM_PER_CH;
