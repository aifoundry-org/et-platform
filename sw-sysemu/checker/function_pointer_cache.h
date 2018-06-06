#ifndef _FUNCTION_POINTER_CACHE
#define _FUNCTION_POINTER_CACHE

// Local
#include "checker_defines.h"
#include "testLog.h"

// STD
#include <unordered_map>
#include <string>

// This class is a cache that returns a pointer to the emulation functions of
// RiscV. The class loads a shared object that contains the emu functions and
// has a cache that in case of miss looks for the function name in the shared
// object. Otherwise returns the pointer straight from the cache
class function_pointer_cache
{
    public:
        // Constructor and destructor
        function_pointer_cache(std::string shared_obj_file_name);
        ~function_pointer_cache();

        func_ptr get_function_ptr(std::string func, bool * error = NULL, std::string * error_msg = NULL);
    private:
        // Typedef for the hash of functions
        typedef std::unordered_map<std::string, func_ptr>           emu_ptr_hash_t;
        typedef std::unordered_map<std::string, func_ptr>::iterator emu_ptr_hash_it_t;

        emu_ptr_hash_t               pointer_cache; // Pointer to the hash of functions
        void                       * handle;        // Pointer to the shared object
        testLog                      log;

        func_ptr find_function_ptr(std::string func, bool * error, std::string * error_msg); // Fill for the cache
};

#endif // _FUNCTION_POINTER_CACHE

