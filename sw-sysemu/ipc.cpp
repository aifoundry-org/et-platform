#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "ipc.h"

extern void gprintf(const char* format, ...);
extern void gsprintf(char* str, const char* format, ...);
extern void gfprintf(FILE *stream, const char* format, ...);

#define MAX(a,b) ((a) >= (b) ? (a) : (b))

typedef enum
{
        FPU,
        DCACHE,
        TBOX,
        ALU,
        PRODNONE,
        MAXPRODUCERS
} producers;

typedef struct
{
        uint64 cycle;
        producers producer;
} ready_t;

// Array that indicates each opcode latency
int32 latency[MAXOPCODE];
ready_t fready[MAXFREG];
ready_t xready[MAXXREG];
ready_t mready[MAXMREG];
uint64  cycle = 0;
uint64  last_texsnd = 0;
uint64  numins = 0;
uint32 debug_ipc = 0;
uint32 exit_on_first_ps = 0;
int    lat_sampler;
uint64 histogram[MAXPRODUCERS] = {0,0,0,0,0};
const char   *pnames[MAXPRODUCERS] = {"FPU", "DCACHE", "TBOX", "ALU", "NONE"};

FILE *ipcoutFile;


typedef struct cache_tag_t
{
        uint64       tag;
        uint64       available;
        bool         valid;
} cache_tag_t;

#define DWAYS (4)

#define LINESIZE (64)
#define LOGLINE  (6)  // should be log2(LINESIZE)

#define DSETS (32)
#define LOGDSETS (5) // should be log2(DSETS)
#define DSETMSK (0x1F)


cache_tag_t dcache[DSETS][DWAYS];
uint32         lru[DSETS][DWAYS];
uint32 dinit = 0;

uint64 dcache_num_lookups = 0;
uint64 dcache_num_rdhit   = 0;
uint64 dcache_num_wrhit   = 0;
uint64 dcache_num_rddelayhit   = 0;
uint64 dcache_num_wrdelayhit   = 0;
uint64 dcache_num_rdmiss  = 0;
uint64 dcache_num_wrmiss  = 0;

void dcache_init()
{
        //if ( dinit == 1 ) return;
        //dinit = 1;

        if ( debug_ipc ) gfprintf(ipcoutFile,"dcache_init: sets %d ways %d line size %d total capacity %d\n",DSETS,DWAYS,LINESIZE,DSETS*DWAYS*LINESIZE);

        // Clean the cache
        for (uint32 s = 0; s < DSETS; s++ )
        {
                for (uint32 w = 0; w < DWAYS; w++ )
                {
                        dcache[s][w].valid = false;
                        dcache[s][w].tag   = 0;
                        dcache[s][w].available = 0;
                        lru[s][w] = w;
                }
        }

        dcache_num_lookups = 0;
        dcache_num_rdhit   = 0;
        dcache_num_wrhit   = 0;
        dcache_num_rddelayhit   = 0;
        dcache_num_wrdelayhit   = 0;
        dcache_num_rdmiss  = 0;
        dcache_num_wrmiss  = 0;
}

void make_mru(uint32 set, uint32 way)
{
        uint32 newlru[DWAYS];
        uint32 idx = 1;

        // The given way is the most MRU, so we put in the '0' position
        newlru[0] = way;

        // now we copy the other ways in their current order, skipping the given 'way'
        for (uint32 w = 0; w < DWAYS; w++ )
        {
                if ( lru[set][w] != way )
                {
                        newlru[idx++] = lru[set][w];
                }
        }
        assert(idx == DWAYS);

        // copy back into lru array
        for (uint32 w = 0; w < DWAYS; w++ )
        {
                lru[set][w] = newlru[w];
        }
}


uint32 get_victim(uint32 set)
{
        // The LRU way is always at the back of the LRU list
        return lru[set][DWAYS-1];
}

typedef enum
{
        READ,
        WRITE
} access_t;

uint32 dcache_lookup(uint64 addr, access_t acc)
{
        const uint32 hitlat  = 2;
        const uint32 misslat = 30;
        const char *type = acc == READ ? "READ" : "WRITE";
        uint32 set = (addr >> LOGLINE) & DSETMSK;
        assert(set < DSETS);

        uint64 tag = addr >> (LOGLINE+LOGDSETS);

        dcache_num_lookups++;

        for (uint32 w = 0; w < DWAYS; w++ )
        {
                if ( dcache[set][w].valid && dcache[set][w].tag == tag )
                {
                        make_mru(set,w);

                        if ( dcache[set][w].available <= cycle )
                        {
                                if  ( debug_ipc ) gfprintf(ipcoutFile,"dcache_lookup: %6s: cycle %llu available %llu hit on set %d way %d tag %llx addr %llx\n",
                                                                                     type,      cycle,dcache[set][w].available,set,w,tag,addr);
                                if      ( acc == READ ) dcache_num_rdhit++;
                                else if ( acc == WRITE) dcache_num_wrhit++;
                                return hitlat; // hit latency
                        }
                        else
                        {
                                uint64 pending = dcache[set][w].available - cycle;

                                if  ( debug_ipc ) gfprintf(ipcoutFile,"dcache_lookup: %6s: cycle %llu available %llu wait on hit on set %d way %d tag %llx addr %llx pending %llu \n",type,cycle,dcache[set][w].available,set,w,tag,addr,pending);
                                if      ( acc == READ ) dcache_num_rddelayhit++;
                                else if ( acc == WRITE) dcache_num_wrdelayhit++;
                                return (uint32)pending;
                        }
                }
        }

        uint32 victim = get_victim(set);
        dcache[set][victim].valid       = 1;
        dcache[set][victim].tag         = tag;
        dcache[set][victim].available   = cycle + misslat;
        if ( debug_ipc ) gfprintf(ipcoutFile,"dcache_lookup: %6s: cycle %llu miss on set %d tag %llx addr %llx --> victim %d\n",type,cycle,set,tag,addr,victim);
        make_mru(set,victim);

        if      ( acc == READ ) dcache_num_rdmiss++;
        else if ( acc == WRITE) dcache_num_wrmiss++;
        return misslat; // miss latency
}

uint32 dcache_read(uint64 addr)
{
        return dcache_lookup(addr, READ);
}

uint32 dcache_write(uint64 addr)
{
        return dcache_lookup(addr, WRITE);
}

void ipc_init(const char *type, int debug)
{
        int i;

        debug_ipc = debug;

        // We always open the ipc.txt file to make things easy, even if ipc
        // has not been compiled into the binary. It simplifies corner cases
        // and helps keeping the ipc.txt file consistent between runs. -- roger
        if ( ipcoutFile == NULL )
        {
                ipcoutFile = fopen("ipc.txt","w");
                if ( ipcoutFile == NULL )
                {
                        gfprintf(stderr,"Failed to open ipc.txt\n");
                        exit(-1);
                }
        }

        if ( debug_ipc ) gfprintf(ipcoutFile,"\nipc_init(%s)\n",type);

        cycle = 0;
        numins = 0;
        dcache_init();

        for (uint32 p = 0; p < MAXPRODUCERS; p++)
        {
                histogram[p] = 0;
        }

        for (i = 0; i < MAXFREG; i++ ) { fready[i].cycle = (uint64)0; fready[i].producer = PRODNONE; }
        for (i = 0; i < MAXXREG; i++ ) { xready[i].cycle = (uint64)0; xready[i].producer = PRODNONE; }
        for (i = 0; i < MAXMREG; i++ ) { mready[i].cycle = (uint64)0; mready[i].producer = PRODNONE; }
        for (i = 0; i < MAXOPCODE; i++ ) { latency[i] = -1; }

        // register X0 is always ready :-)
        xready[0].cycle = 0;
        xready[0].producer = ALU;

        int lat_dcache_to_fpu = 3;
        int lat_fma           = 4;
        int lat_fcmp          = 2;
        int lat_fint          = 2;
        int lat_fcvt          = 1;

        lat_sampler          = 100; // Careful! global variable used in texrcv!

        latency[FLW]         = lat_dcache_to_fpu;
        latency[FSW]         = 1;
        latency[FBC]         = lat_dcache_to_fpu;
        latency[FBCI]        = 1;
        latency[FGW]         = lat_dcache_to_fpu;
        latency[FGH]         = lat_dcache_to_fpu;
        latency[FGB]         = lat_dcache_to_fpu;
        latency[FSCW]        = 1;
        latency[FSCH]        = 1;
        latency[FSCB]        = 1;
        latency[FADD]        = lat_fma;
        latency[FSUB]        = lat_fma;
        latency[FMUL]        = lat_fma;
        latency[FDIV]        = 12;
        latency[FMIN]        = lat_fcmp;
        latency[FMAX]        = lat_fcmp;
        latency[FMADD]       = lat_fma;
        latency[FMSUB]       = lat_fma;
        latency[FNMADD]      = lat_fma;
        latency[FNMSUB]      = lat_fma;
        latency[FSQRT]       = 12;
        latency[FRSQ]        = 12;
        latency[FSIN]        = 28;
        //latency[FCOS]        = 28;
        latency[FEXP]        = 28;
        latency[FLOG]        = 12;
        latency[FRCP]        = 6;
        latency[FRCPFXP]     = 6;
        latency[FCVTPSPW]    = lat_fcvt;
        latency[FCVTPSPWU]   = lat_fcvt;
        latency[FFRC]        = lat_fcvt;
        latency[FROUND]      = lat_fcvt;
        latency[FSWIZZ]      = 1;
        latency[FCMOV]       = 1;
        latency[FCVTPWPS]    = lat_fcvt;
        latency[FCVTPWUPS]   = lat_fcvt;
        latency[FSGNJ]       = 1;
        latency[FSGNJN]      = 1;
        latency[FSGNJX]      = 1;
        latency[FMVZXPS]      = 1;
        latency[FMVSXPS]      = 1;
        latency[FEQ]         = lat_fcmp;
        latency[FLE]         = lat_fcmp;
        latency[FLT]         = lat_fcmp;
        //latency[FLTABS]      = lat_fcmp;
        latency[FCLASS]      = 1;
        latency[FCVTPSF16]   = lat_fcvt;
        latency[FCVTPSF11]   = lat_fcvt;
        latency[FCVTPSF10]   = lat_fcvt;
        latency[FCVTPSUN24]  = lat_fcvt;
        latency[FCVTPSUN16]  = lat_fcvt;
        latency[FCVTPSUN10]  = lat_fcvt;
        latency[FCVTPSUN8]   = lat_fcvt;
        latency[FCVTPSUN2]   = lat_fcvt;
        //latency[FCVTPSSN24]  = lat_fcvt;
        latency[FCVTPSSN16]  = lat_fcvt;
        //latency[FCVTPSSN10]  = lat_fcvt;
        latency[FCVTPSSN8]   = lat_fcvt;
        //latency[FCVTPSSN2]   = lat_fcvt;
        latency[FCVTF16PS]   = lat_fcvt;
        latency[FCVTF11PS]   = lat_fcvt;
        latency[FCVTF10PS]   = lat_fcvt;
        latency[FCVTUN24PS]  = lat_fcvt;
        latency[FCVTUN16PS]  = lat_fcvt;
        latency[FCVTUN10PS]  = lat_fcvt;
        latency[FCVTUN8PS]   = lat_fcvt;
        latency[FCVTUN2PS]   = lat_fcvt;
        //latency[FCVTSN24PS]  = lat_fcvt;
        latency[FCVTSN16PS]  = lat_fcvt;
        latency[FCVTSN8PS]   = lat_fcvt;
        latency[MAND]        = 1;
        latency[MOR]         = 1;
        latency[MXOR]        = 1;
        latency[MNOT]        = 1;
        latency[FSET]        = 1;
        latency[MOVAMX]      = 1;
        latency[MOVAXM]      = 1;
        latency[FADDPI]      = lat_fint;
        latency[FSUBPI]      = lat_fint;
        latency[FMULPI]      = 1;
        latency[FDIVPI]      = 13;
        latency[FDIVUPI]     = 13;
        latency[FREMUPI]     = 13;
        latency[FMINPI]      = lat_fint;
        latency[FMAXPI]      = lat_fint;
        latency[FMINUPI]     = lat_fint;
        latency[FMAXUPI]     = lat_fint;
        latency[FANDPI]      = lat_fint;
        latency[FORPI]       = 1;
        latency[FXORPI]      = 1;
        latency[FNOTPI]      = 1;
        latency[FSLLPI]      = 1;
        latency[FSRLPI]      = 1;
        latency[FSRAPI]      = 1;
        latency[FLTPI]       = 1;
        latency[FLTUPI]      = 1;
        latency[FLEPI]       = 1;
        latency[FEQPI]       = 1;
        //latency[FADDPQ]      = lat_fma;
        latency[FADDIPI]     = 1;
        latency[FANDIPI]     = 1;
        latency[FORIPI]      = 1;
        latency[FXORIPI]     = 1;
        latency[FSLLIPI]     = 1;
        latency[FSRLIPI]     = 1;
        latency[FSRAIPI]     = 1;
        //latency[FSLLOIPI]    = 1;
        latency[FCVTWS]      = lat_fcvt;
        latency[FCVTWUS]     = lat_fcvt;
        latency[FCVTSW]      = lat_fcvt;
        latency[FCVTSWU]     = lat_fcvt;

        latency[SIMPLE_INT]  = 1;
        latency[MUL_INT]     = 3;
        latency[DIV_INT]     = 7;
        latency[REM_INT]     = 7;
        latency[MASKOP]      = 1;
        latency[LD]          = 2;
        latency[STORE_INT]   = 1;

        latency[CUBEFACE]    = 1;
        latency[CUBEFACEIDX] = 1;
        latency[CUBESGNSC]   = 1;
        latency[CUBESGNTC]   = 1;

        for (i = 0; i < MAXOPCODE; i++ )
        {
               if ( latency[i] == -1 )
               {
                       gfprintf(stderr,"fatal: ipc_init(): found unset latency for opcode %d\n",i);
                       exit(-1);
               }
        }
}

void ipc_init_xreg(xreg dst)
{
        xready[dst].cycle = 0;
        xready[dst].producer = ALU;
}

void ipc_init_mreg(mreg dst)
{
        mready[dst].cycle = 0;
        mready[dst].producer = ALU;
}


ready_t earliest(ready_t start, xreg src)
{
        // if the src is ready in the future, then return its ready cycle and producer as the ealiest time we can execute
        if ( src != xnone && xready[src].cycle > start.cycle )
        {
                return xready[src];
        }
        // otherwise, return the start time we've been given
        else
        {
                return start;
        }
}

ready_t earliest(ready_t start, freg src)
{
        // if the src is ready in the future, then return its ready cycle and producer as the ealiest time we can execute
        if ( src != fnone && fready[src].cycle > start.cycle )
        {
                return fready[src];
        }
        // otherwise, return the start time we've been given
        else
        {
                return start;
        }
}

void set_dst_cycle(freg dst, uint64 newcycle, producers p)
{
        fready[dst].cycle = newcycle;
        fready[dst].producer = p;
}

void set_dst_cycle(xreg dst, uint64 newcycle, producers p)
{
        xready[dst].cycle = newcycle;
        xready[dst].producer = p;
}

void delay_stats(ready_t ready)
{
        // If we have been delayed (i.e., if ready.cycle > cycle+1, then keep a tally of the lost
        // cycles and count it against the unit that has caused the most delay
        if ( ready.cycle > (cycle+1) )
        {
                histogram[ready.producer] += ready.cycle - (cycle+1);
                if ( debug_ipc ) gfprintf(ipcoutFile,"histo[%d] += %llu (%llu - %llu)\n",ready.producer,(ready.cycle-(cycle+1)),ready.cycle,cycle+1);
        }
}

uint64 advance_time(ready_t ready)
{
        delay_stats(ready);
        cycle = ready.cycle;
        numins++;
        return cycle;
}


void debug_instruction(xreg xdst, freg fdst, xreg xsrc1, xreg xsrc2, freg fsrc1, freg fsrc2, freg fsrc3, uint64 lat, char *dis,bool miss)
{
        char latinfo[512];
        char srcinfo[512];
        char dstinfo[512];

        if ( debug_ipc )
        {
                gsprintf(latinfo,"cycle: %6llu: ins %6llu: ",cycle,numins);
                gsprintf(srcinfo,"");
                gsprintf(dstinfo,"");
                if ( xsrc1 != xnone ) gsprintf(srcinfo,"%s x%02d=%4llu",srcinfo,xsrc1,xready[xsrc1].cycle);
                if ( fsrc1 != fnone ) gsprintf(srcinfo,"%s f%02d=%4llu",srcinfo,fsrc1,fready[fsrc1].cycle);
                if ( xsrc2 != xnone ) gsprintf(srcinfo,"%s x%02d=%4llu",srcinfo,xsrc2,xready[xsrc2].cycle);
                if ( fsrc2 != fnone ) gsprintf(srcinfo,"%s f%02d=%4llu",srcinfo,fsrc2,fready[fsrc2].cycle);
                if ( fsrc3 != fnone ) gsprintf(srcinfo,"%s f%02d=%4llu",srcinfo,fsrc3,fready[fsrc3].cycle);
                if ( xdst  != xnone ) gsprintf(dstinfo,"--> x%02d=%4llu",xdst,cycle+lat);
                if ( fdst  != fnone ) gsprintf(dstinfo,"--> f%02d=%4llu",fdst,cycle+lat);
                //if ( miss ) gfprintf(ipcoutFile,"%c[%d;%dm",27,1,31);
                gfprintf(ipcoutFile, "%-20s: %-30s %-14s %4s %s\n",latinfo,srcinfo,dstinfo,miss ? "miss" : "",dis);
                //if ( miss ) gfprintf(ipcoutFile,"%c[%dm",27,0);
        }
}

void ipc_int(opcode opc, xreg dst, xreg src1, xreg src2,char *dis)
{
        uint32 lat = latency[opc];
        ready_t ready = { cycle+1, PRODNONE };

        ready = earliest(ready, src1);
        ready = earliest(ready, src2);
        cycle = advance_time(ready); // advance time to earliest point we can execute and compute delay stats
        set_dst_cycle(dst, cycle  + lat, ALU);
        debug_instruction(dst,fnone,src1,src2,fnone,fnone,fnone,lat,dis,false);
}

void ipc_f2x(opcode opc, xreg dst, freg src1, char *dis)
{
        uint32 lat = latency[opc];
        ready_t ready = { cycle+1, PRODNONE };

        ready = earliest(ready, src1);
        cycle = advance_time(ready); // advance time to earliest point we can execute and compute delay stats
        set_dst_cycle(dst, cycle  + lat, ALU);
        debug_instruction(dst,fnone,xnone,xnone,src1,fnone,fnone,lat,dis,false);
}

void ipc_f2x(opcode opc, xreg dst, freg src1, freg src2, char *dis)
{
        uint32 lat = latency[opc];
        ready_t ready = { cycle+1, PRODNONE };

        ready = earliest(ready, src1);
        ready = earliest(ready, src2);
        cycle = advance_time(ready); // advance time to earliest point we can execute and compute delay stats
        set_dst_cycle(dst, cycle  + lat, ALU);
        debug_instruction(dst,fnone,xnone,xnone,src1,fnone,fnone,lat,dis,false);
}

void ipc_ld(opcode opc, xreg dst, xreg src1, uint64 addr,char *dis)
{
        uint32 lat = dcache_read(addr);
        ready_t ready = { cycle+1, PRODNONE };

        ready = earliest(ready, src1);
        cycle = advance_time(ready); // advance time to earliest point we can execute and compute delay stats
        set_dst_cycle(dst, cycle  + lat, DCACHE);
        debug_instruction(dst,fnone,src1,xnone,fnone,fnone,fnone,lat,dis,lat>2);
}

void ipc_ld(opcode opc, int count, int size, freg dst, xreg src1, uint64 addr,char *dis)
{
        uint32 lat = dcache_read(addr);
        ready_t ready = { cycle+1, PRODNONE };

        ready = earliest(ready, src1);
        cycle = advance_time(ready); // advance time to earliest point we can execute and compute delay stats
        set_dst_cycle(dst, cycle  + lat, DCACHE);
        debug_instruction(xnone,dst,src1,xnone,fnone,fnone,fnone,lat,dis,lat>2);
}

void ipc_gt(opcode opc, int count, int size, freg dst, freg src1, xreg base, uint64 addr,char *dis, int idx)
{
        uint32 lat = dcache_read(addr);
        ready_t ready = { cycle+1, PRODNONE };

        if (idx == 0) {
            // input dependence by src1 take into account only for first active element, as src1 may be equal dst
            ready = earliest(ready, src1);
        }
        ready = earliest(ready, base);
        cycle = advance_time(ready); // advance time to earliest point we can execute and compute delay stats
        set_dst_cycle(dst, cycle  + lat, DCACHE);
        debug_instruction(xnone,dst,base,xnone,src1,fnone,fnone,lat,dis,lat>2);
}

void ipc_st(opcode opc, int count, int size, freg src1, xreg base, uint64 addr,char *dis)
{
        uint32 lat = dcache_write(addr);
        ready_t ready = { cycle+1, PRODNONE };

        ready = earliest(ready, src1);
        ready = earliest(ready, base);
        cycle = advance_time(ready); // advance time to earliest point we can execute and compute delay stats
        debug_instruction(xnone,fnone,base,xnone,src1,fnone,fnone,lat,dis,lat>2);
}

void ipc_st(opcode opc, int count, int size, xreg src1, xreg base, uint64 addr,char *dis)
{
        uint32 lat = dcache_write(addr);
        ready_t ready = { cycle+1, PRODNONE };

        ready = earliest(ready, src1);
        ready = earliest(ready, base);
        cycle = advance_time(ready); // advance time to earliest point we can execute and compute delay stats
        debug_instruction(xnone,fnone,src1,base,fnone,fnone,fnone,lat,dis,lat>2);
}

void ipc_sc(opcode opc, int count, int size, freg src1, freg src2, xreg base, uint64 addr,char *dis)
{
        uint32 lat = dcache_write(addr);
        ready_t ready = { cycle+1, PRODNONE };

        ready = earliest(ready, src1);
        ready = earliest(ready, src2);
        ready = earliest(ready, base);
        cycle = advance_time(ready); // advance time to earliest point we can execute and compute delay stats
        debug_instruction(xnone,fnone,base,xnone,src1,src2,fnone,lat,dis,lat>2);
}
void ipc_pi(opcode opc, int count, freg dst, freg src1, freg src2, freg src3,char *dis)
{
        ipc_ps(opc,count,dst,src1,src2,src3,dis);
}
void ipc_ps(opcode opc, int count, freg dst, freg src1, freg src2, freg src3,char *dis)
{
        uint32 lat = latency[opc];
        ready_t ready = { cycle+1, PRODNONE };

        ready = earliest(ready, src1);
        ready = earliest(ready, src2);
        ready = earliest(ready, src3);
        cycle = advance_time(ready); // advance time to earliest point we can execute and compute delay stats
        set_dst_cycle(dst, cycle  + lat, FPU);
        debug_instruction(xnone,dst,xnone,xnone,src1,src2,src3,lat,dis,false);
}

void ipc_msk(opcode opc, mreg dst, freg src1, freg src2,char *dis)
{
}

void ipc_msk(opcode opc, mreg dst, mreg src1, mreg src2,char *dis)
{
}

void ipc_texsnd(xreg src1, xreg src2, freg fsrc, char *dis)
{
        ready_t ready = { cycle+1, PRODNONE };

        ready = earliest(ready, src1);
        ready = earliest(ready, src2);
        ready = earliest(ready, fsrc);
        cycle = advance_time(ready); // advance time to earliest point we can execute and compute delay stats
        last_texsnd = cycle;
        debug_instruction(xnone,fnone,src1,src2,fsrc,fnone,fnone,1,dis,false);
}

void ipc_texrcv(freg dst, char *dis)
{
        ready_t ready;

        ready.cycle    = MAX(cycle+1,last_texsnd + lat_sampler);
        ready.producer = TBOX;
        cycle = advance_time(ready); // advance time to earliest point we can execute and compute delay stats
        set_dst_cycle(dst, cycle  + 1, TBOX);
        debug_instruction(xnone,dst,xnone,xnone,fnone,fnone,fnone,1,dis,false);
}

void ipc_print_stats(const char *type)
{
        uint64 sum = 0;
        if ( debug_ipc )
        {
                gfprintf(ipcoutFile,"\nipc_stats: type %s: cycle: %6llu: ins %6llu: ipc %.3f\n",type,cycle,numins,(double)numins/(double)cycle);
                for (uint32 p = 0; p < ALU; p++)
                {
                        gfprintf(ipcoutFile,"ipc_stats: cycle: %6llu: wait time for %s: %llu: pc %.3f\n",cycle,pnames[p],histogram[p],(double)histogram[p]/(double)cycle);
                        sum += histogram[p];
                }
                gfprintf(ipcoutFile, "ipc_stats: cycle: %6llu: wait sum %6llu: ipc %.3f\n",cycle,sum,(double)sum/(double)cycle);
                gfprintf(ipcoutFile, "ipc_stats: ins %6llu +  wait sum %6llu =  %6llu\n",numins,sum,sum+numins);
        }

        gfprintf(ipcoutFile, "ipc_summary oneline type %s cycles %llu ins %llu wfpc %llu wdpc %llu wtpc %llu dclookups %llu dcrdhit %llu dcrddelayhit %llu dcrdmiss %llu dcwrhit %llu dcwrdelayhit %llu dcwrmiss %llu\n",
                         type,
                         cycle,
                         numins,
                         histogram[FPU],
                         histogram[DCACHE],
                         histogram[TBOX],
                         dcache_num_lookups,
                         dcache_num_rdhit,
                         dcache_num_rddelayhit,
                         dcache_num_rdmiss,
                         dcache_num_wrhit,
                         dcache_num_wrdelayhit,
                         dcache_num_wrmiss
              );

        if ( exit_on_first_ps && type[0] == 'p' )
        {
                exit(0);
        }
}
