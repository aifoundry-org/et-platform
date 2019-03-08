#include <stdio.h>
#include <assert.h>
#include <memory>
#include <unordered_map>
#include <mutex>
#include "etrt.h"
#include "registry.h"
#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"
#include "demangle.h"
#include "utils.h"

static constexpr size_t UPPER_BOUND_FOR_KERNELS_ELF_SIZE = 1024*1024*1024; // 1GB

#define NVIDIA_FATBIN_DATA_MAGIC 0xBA55ED50
#define ELF_MAGIC                0x464C457F

////////////////////////////////////////////////////////////////////////////////////
// from: /usr/local/cuda-9.1/targets/x86_64-linux/include/fatBinaryCtl.h
////////////////////////////////////////////////////////////////////////////////////

/*
 * These defines are for the fatbin.c runtime wrapper
 */
#define FATBINC_MAGIC   0x466243B1
#define FATBINC_VERSION 1
#define FATBINC_LINK_VERSION 2

typedef struct {
  int magic;
  int version;
  const unsigned long long* data;
  void *filename_or_fatbins;  /* version 1: offline filename,
                               * version 2: array of prelinked fatbins */
} __fatBinC_Wrapper_t;

////////////////////////////////////////////////////////////////////////////////////

/*
 * Note: __etrtRegisterFunction is called during DSO linking (before our global variables constructors),
 *       so in __etrtRegisterFunction we may only store data to statically initialized data types (POD).
 */
static constexpr size_t kRegisteredFatbinWrappersMax = 1024;
static const __fatBinC_Wrapper_t *gRegisteredFatbinWrappers[kRegisteredFatbinWrappersMax];
static size_t gRegisteredFatbinWrappersNum = 0;

static constexpr size_t kRegisteredFunctionsMax = 1024;
static const void *gRegisteredFunctionsPtr[kRegisteredFunctionsMax];
static const char *gRegisteredFunctionsName[kRegisteredFunctionsMax];
static const __fatBinC_Wrapper_t *gRegisteredFunctionsFatbinWrapper[kRegisteredFunctionsMax];
static size_t gRegisteredFunctionsNum = 0;


EXAPI void** __etrtRegisterFatBinary(void *fatCubin)
{
    // FYI: fatbin wrapper located in .nvFatBinSegment section
    __fatBinC_Wrapper_t *fatbin_wrapper_p = (__fatBinC_Wrapper_t *)fatCubin;
    assert(fatbin_wrapper_p->magic == FATBINC_MAGIC);
    assert(fatbin_wrapper_p->version == FATBINC_VERSION);

    // FYI: fatbin data located in .nv_fatbin section
    const unsigned long long *fatbin_data_p = fatbin_wrapper_p->data;

    if ((uint32_t)*fatbin_data_p == NVIDIA_FATBIN_DATA_MAGIC) {
//        fprintf(stderr, "This is original NVIDIA fatbin data\n");
    } else if ((uint32_t)*fatbin_data_p == ELF_MAGIC) {
//        fprintf(stderr, "This is ELF fatbin data (Esperanto RISCV shared object)\n");
//        parse_elf(fatbin_data_p);
    } else {
        fprintf(stderr, "fatbin_data_p = %p, *fatbin_data_p = 0x%016llx\n", fatbin_data_p, *fatbin_data_p);
        THROW("Unknown fatbin data\n");
    }

    THROW_IF(gRegisteredFatbinWrappersNum >= kRegisteredFatbinWrappersMax, "RegisteredFatbinWrappers exhausted");
    const __fatBinC_Wrapper_t **fatbin_handle = &gRegisteredFatbinWrappers[gRegisteredFatbinWrappersNum];
    gRegisteredFatbinWrappers[gRegisteredFatbinWrappersNum] = fatbin_wrapper_p;
    gRegisteredFatbinWrappersNum++;

    return (void **)fatbin_handle;
}

EXAPI void __etrtUnregisterFatBinary(void **fatCubinHandle)
{
    // do nothing
    // TODO: ...?
}

EXAPI void __etrtRegisterFunction(
        void   **fatCubinHandle,
  const char    *hostFun,
        char    *deviceFun,
  const char    *deviceName,
        int      thread_limit,
        uint3   *tid,
        uint3   *bid,
        dim3    *bDim,
        dim3    *gDim,
        int     *wSize)
{
    // handle is the pointer inside gRegisteredFatbinWrappers array
    const __fatBinC_Wrapper_t **fatbin_handle = (const __fatBinC_Wrapper_t **)fatCubinHandle;
    assert(fatbin_handle >= &gRegisteredFatbinWrappers[0]);
    assert(fatbin_handle < &gRegisteredFatbinWrappers[gRegisteredFatbinWrappersNum]);

    assert(deviceFun == deviceName);

    THROW_IF(gRegisteredFunctionsNum >= kRegisteredFunctionsMax, "RegisteredFunctions exhausted");
    gRegisteredFunctionsPtr[gRegisteredFunctionsNum] = hostFun;
    gRegisteredFunctionsName[gRegisteredFunctionsNum] = deviceName;
    gRegisteredFunctionsFatbinWrapper[gRegisteredFunctionsNum] = *fatbin_handle;
    gRegisteredFunctionsNum++;
}

EXAPI void __etrtRegisterVar(
        void **fatCubinHandle,
        char  *hostVar,
        char  *deviceAddress,
  const char  *deviceName,
        int    ext,
        size_t size,
        int    constant,
        int    global)
{
    assert(deviceAddress == deviceName);
    // do nothing
    // TODO: ...?
}

////////////////////////////////////////////////////////////////////////////////////


struct EtKernelsElfDescr
{
    const void *elf_p = nullptr;
    size_t elf_size = 0;
    std::unordered_map<std::string, size_t> kernel_offset; // kernel name -> kernel entry point offset
};

struct EtKernelHostFunDescr
{
    const EtKernelsElfDescr *elf_descr_p;
    std::string name;
};

/*
 * These data are supposed to be modified only once in lazy_init_once().
 * Then they are stay unmodified, so after lazy_init_once() we access to them without synchronization.
 */
static std::vector<std::unique_ptr<EtKernelsElfDescr>> gElfDescrStorage;
static std::unordered_map<const void *, EtKernelHostFunDescr> gRegisteredFunctions;

static void lazy_init_once()
{
    std::unordered_map<const __fatBinC_Wrapper_t *, const EtKernelsElfDescr *> wrapper_2_elf_descr;

    for (size_t i = 0; i < gRegisteredFatbinWrappersNum; i++)
    {
        const __fatBinC_Wrapper_t *fatbin_wrapper_p = gRegisteredFatbinWrappers[i];
        assert(fatbin_wrapper_p->magic == FATBINC_MAGIC);
        assert(fatbin_wrapper_p->version == FATBINC_VERSION);

        if ((uint32_t)*fatbin_wrapper_p->data == ELF_MAGIC) {
            std::unique_ptr<EtKernelsElfDescr> elf_descr(new EtKernelsElfDescr);
            elf_descr->elf_p = fatbin_wrapper_p->data;
            parse_elf(elf_descr->elf_p, &elf_descr->elf_size, &elf_descr->kernel_offset, nullptr);

            wrapper_2_elf_descr[fatbin_wrapper_p] = elf_descr.get();
            gElfDescrStorage.push_back(std::move(elf_descr));
        }
    }

    for (size_t i = 0; i < gRegisteredFunctionsNum; i++)
    {
        const void *hostFun = gRegisteredFunctionsPtr[i];
        const char *name = gRegisteredFunctionsName[i];
        const __fatBinC_Wrapper_t *fatbin_wrapper_p = gRegisteredFunctionsFatbinWrapper[i];

        EtKernelHostFunDescr &fun_descr = gRegisteredFunctions[hostFun];
        fun_descr.elf_descr_p = wrapper_2_elf_descr[fatbin_wrapper_p];
        fun_descr.name = name;
    }
}

EtKernelInfo etrtGetKernelInfoByHostFun(const void *hostFun)
{
    static std::once_flag flag;
    std::call_once(flag, lazy_init_once);

    const EtKernelHostFunDescr &fun_descr = gRegisteredFunctions.at(hostFun);
    const std::string &name = fun_descr.name;
    const EtKernelsElfDescr *elf_descr_p = fun_descr.elf_descr_p;

    EtKernelInfo kernel_info(name);
    if (elf_descr_p) {
        kernel_info.elf_p = elf_descr_p->elf_p;
        kernel_info.elf_size = elf_descr_p->elf_size;
        kernel_info.offset = elf_descr_p->kernel_offset.at(name);
    }
    return kernel_info;
}


////////////////////////////////////////////////////////////////////////////////////

static std::string cut_suffix(const std::string &str, const char *suffix)
{
    size_t suffix_len = strlen(suffix);
    if (str.size() <= suffix_len) {
        return "";
    }
    size_t pos = str.size() - suffix_len;
    if (strcmp(str.c_str() + pos, suffix) != 0) {
        return "";
    }
    return str.substr(0, pos);
}

void parse_elf(const void *elf_p,
               size_t *elf_size_p,
               std::unordered_map<std::string, size_t> *kernel_offset_p,
               std::unordered_map<std::string, size_t> *raw_kernel_offset_p)
{
    struct readmembuf : std::streambuf
    {
        readmembuf(char *base, size_t n) {
            this->setg(base, base, base + n);
        }

        virtual pos_type seekoff(off_type __off, std::ios_base::seekdir __way, std::ios_base::openmode) {
            if (__way == std::ios_base::beg) {
                setg(eback(), eback() + __off, egptr());
            } else if (__way == std::ios_base::cur) {
                setg(eback(), gptr() + __off, egptr());
            } else {
                setg(eback(), egptr() + __off, egptr());
            }
            return gptr() - eback();
        }

        virtual pos_type seekpos(pos_type __pos, std::ios_base::openmode __mode) {
            return seekoff(__pos, std::ios_base::beg, __mode);
        }
    };

    readmembuf sbuf((char*)elf_p, UPPER_BOUND_FOR_KERNELS_ELF_SIZE);
    std::istream in(&sbuf);


    ELFIO::elfio reader;

    if ( !reader.load(in) ) {
        THROW("Could not load an ELF file.");
    }

    if (false) {
        ELFIO::dump::header         ( std::cerr, reader );
        ELFIO::dump::section_headers( std::cerr, reader );
        ELFIO::dump::segment_headers( std::cerr, reader );
        ELFIO::dump::symbol_tables  ( std::cerr, reader );
        ELFIO::dump::notes          ( std::cerr, reader );
        ELFIO::dump::dynamic_tags   ( std::cerr, reader );
        ELFIO::dump::section_datas  ( std::cerr, reader );
        ELFIO::dump::segment_datas  ( std::cerr, reader );
    }

    // Sanity checks.
    THROW_IF(reader.get_machine() != EM_RISCV, "Kernels ELF machine is not EM_RISCV.");
    THROW_IF(reader.get_class() != ELFCLASS64, "Kernels ELF class is not ELFCLASS64.");

    /*
     * Compute Elf File Size by formula: e_shoff + ( e_shentsize * e_shnum )
     * This assumes that the section header table (SHT) is the last part of the ELF.
     * This is usually the case but it could also be that the last section is the last part of the ELF
     */
    size_t file_size = reader.get_sections_offset() + reader.get_section_entry_size() * reader.sections.size();
    fprintf(stderr, "Esperanto kernels ELF file size = %lu\n", file_size);
    if (elf_size_p) {
        *elf_size_p = file_size;
    }

    /*
     * Check out all kernel entry functions in .dynsym section.
     */
    using ELFIO::Elf_Half;
    using ELFIO::Elf_Xword;
    using ELFIO::Elf64_Addr;
    Elf_Half n = reader.sections.size();
    for (Elf_Half i = 0; i < n; ++i)
    {
        ELFIO::section *sec = reader.sections[i];
        if (SHT_DYNSYM == sec->get_type() || SHT_SYMTAB == sec->get_type() )
        {
            ELFIO::symbol_section_accessor symbols(reader, sec);

            Elf_Xword sym_num = symbols.get_symbols_num();
            for (Elf_Xword sym_idx = 0; sym_idx < sym_num; ++sym_idx)
            {
                std::string name;
                Elf64_Addr value = 0;
                Elf_Xword size = 0;
                unsigned char bind = 0;
                unsigned char type = 0;
                Elf_Half section = 0;
                unsigned char other = 0;
                symbols.get_symbol(sym_idx, name, value, size, bind, type, section, other);
                if (type == STT_FUNC && bind == STB_GLOBAL)
                {
                    std::string kernel_name = cut_suffix(name, "_ETKERNEL_entry_point");
                    if (!kernel_name.empty())
                    {
                        fprintf(stderr, "Esperanto kernel: offset = 0x%lx, sym name = %s, kernel name = %s [%s]\n",
                               (size_t)value, name.c_str(), kernel_name.c_str(), demangle(kernel_name).c_str());
                        if (kernel_offset_p) {
                            assert(value);
                            (*kernel_offset_p)[kernel_name] = value;
                        }
                    }
                    kernel_name = cut_suffix(name, "_RAWKERNEL_entry_point");
                    if (!kernel_name.empty())
                    {
                        fprintf(stderr, "Esperanto kernel: offset = 0x%lx, sym name = %s, kernel name = %s [%s]\n",
                               (size_t)value, name.c_str(), kernel_name.c_str(), demangle(kernel_name).c_str());
                        if (raw_kernel_offset_p) {
                            assert(value);
                            (*raw_kernel_offset_p)[kernel_name] = value;
                        }
                    }
                }
            }
        }
    }
}
