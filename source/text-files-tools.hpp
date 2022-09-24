#ifndef GUARD_text_files_tools_hpp
#define GUARD_text_files_tools_hpp
//  ---------------------------------------------
//  Launch external program winmerge
//  ---------------------------------------------
#include "system.hpp" // sys::*
#ifdef MS_WINDOWS
  #include "winmerge.hpp" // winmerge::*
#endif

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sys //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


//---------------------------------------------------------------------------
void edit_text_file(const std::string& pth, [[maybe_unused]] const std::size_t line, [[maybe_unused]] const std::size_t offset) noexcept
{
  #ifdef MS_WINDOWS
    const std::string exe_pth = find_executable_by_file(pth);
    //if( exe_pth.contains("scite") ) args = fmt::format("-open:\"{}\" goto:{},{}",pth,line,col);
    sys::shell_execute(exe_pth.c_str(), fmt::format("-nosession -p{} \"{}\"", offset, pth).c_str()); // npp
  #else
    //sys::launch_file(pth);
    //sys::execute("notepadqq", fmt::format("--line {}",line).c_str(), pth.c_str());
    //sys::execute("scite", fmt::format("-open:\"{}\"",pth).c_str(), fmt::format("goto:{},1",line,1).c_str());
    sys::execute("mousepad", fmt::format("--line={}",line).c_str(), pth.c_str());
  #endif
}


//---------------------------------------------------------------------------
// Compare files/folders
template<typename ... Args> void compare(Args&& ... args)
{
  #ifdef MS_WINDOWS
    winmerge::compare(std::forward<Args>(args) ...);
  #else
    sys::execute("meld", std::forward<Args>(args) ...);
  #endif
}



}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


//---- end unit -------------------------------------------------------------
#endif
