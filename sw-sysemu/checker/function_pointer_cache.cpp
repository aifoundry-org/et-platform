// Local
#include "function_pointer_cache.h"

// Global
#include <dlfcn.h>

// STD
#include <iostream>

// Constructor
function_pointer_cache::function_pointer_cache(std::string shared_obj_file_name)
    : log("function_pointer_cache", LOG_DEBUG)
{
    handle = dlopen(shared_obj_file_name.c_str(), RTLD_LAZY);
    if(!handle)
        log << LOG_FTL << dlerror() << endm;
    // Clears any existing error
    dlerror();
}

// Destructor
function_pointer_cache::~function_pointer_cache()
{
    // Unloads the shared object
    dlclose(handle);
}

// Returns the pointer to a function based on name
func_ptr function_pointer_cache::get_function_ptr(std::string func, bool * error, std::string * error_msg)
{
    emu_ptr_hash_it_t el = pointer_cache.find(func);
    if(el != pointer_cache.end())
    {
        if(error != NULL)
            * error = false;
        return el->second;
    }
    return find_function_ptr(func, error, error_msg);
}

// Gets the pointer of a function name in a shared object
func_ptr function_pointer_cache::find_function_ptr(std::string func, bool * error, std::string * error_msg)
{
    void * ret;
    char * error_dl;
 
    ret = dlsym(handle, func.c_str());
    if((error_dl = dlerror()) != NULL)
    {
        // No way to report the error, simply 
        if(error == NULL)
            log << LOG_FTL << error_dl << endm;
        // Reports the error to source
        else
        {
            * error = true;
            * error_msg = error_dl;
        }
    }
    else
    {
        if(error != NULL)
            * error = false;
        pointer_cache[func] = (func_ptr) ret;
    }
    return (func_ptr) ret;
}

