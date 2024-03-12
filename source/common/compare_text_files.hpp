#pragma once
//  ---------------------------------------------
//  Launch external utility to compare text files
//  ---------------------------------------------
//  #include "compare_text_files.hpp" // sys::compare_files_wait()
//  ---------------------------------------------
#include "system_process.hpp" // sys::*
#ifdef MS_WINDOWS
  #include "winmerge.hpp" // winmerge::*
#endif

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sys //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


//---------------------------------------------------------------------------
// Launch files/folders compare program
template<typename ... Args> void compare_files(Args&& ... args)
{
  #ifdef MS_WINDOWS
    winmerge::compare(std::forward<Args>(args) ...);
  #else
    sys::execute("meld", std::forward<Args>(args) ...);
  #endif
}


//---------------------------------------------------------------------------
// Launch files/folders compare program waiting for its termination
template<typename ... Args> [[maybe_unused]] int compare_files_wait(Args&& ... args)
{
  #ifdef MS_WINDOWS
    return winmerge::compare_wait(std::forward<Args>(args) ...);
  #else
    return sys::execute_wait("meld", std::forward<Args>(args) ...);
  #endif
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
