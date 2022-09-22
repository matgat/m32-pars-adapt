#ifndef GUARD_text_files_tools_hpp
#define GUARD_text_files_tools_hpp
//  ---------------------------------------------
//  Launch external program winmerge
//  ---------------------------------------------
#include "system.hpp" // sys::*
#ifdef MS_WINDOWS
  #include "winmerge.hpp" // winmerge::*
#else
#endif

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sys //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


//---------------------------------------------------------------------------
void edit_text_file(const std::string& pth, [[maybe_unused]] const std::size_t offset) noexcept
{
  #ifdef MS_WINDOWS
    const std::string exe_pth = find_executable_by_file(pth);
    //if( exe_pth.contains("notepad++") ) args = fmt::format(" -n{} -c{} -multiInst -nosession \"{}\"", line, column, pth);
    //else if( exe_pth.contains("subl") ) args = fmt::format(" \"{}\":{}:{}", pth, line, column);
    //else if( exe_pth.contains("scite") ) args = fmt::format("-open:\"{}\" goto:{},{}", pth, line, column);
    //else if( exe_pth.contains("uedit") ) args = fmt::format(" \"{}\" -l{} -c{}", pth, line, column);
    //else args =  fmt::format("\"{}\"", pth);
    launch(exe_pth, fmt::format("-nosession -p{} \"{}\"", offset, pth) ); // npp
  #else
    launch(pth);
  #endif
}


//---------------------------------------------------------------------------
// Compare files/folders
void compare(const std::string& pth1, const std::string& pth2, const std::string& pth3 ="")
{
  #ifdef MS_WINDOWS
    winmerge::compare(pth1, pth2, pth3);
  #else
    sys::launch("meld", fmt::format("{} {} {}", pth1, pth2, pth3));
  #endif
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


//---- end unit -------------------------------------------------------------
#endif
