#ifndef GUARD_debug_hpp
#define GUARD_debug_hpp
//  ---------------------------------------------
//  Debug facilities
//  ---------------------------------------------
#include <cassert> // assert
//#include <format> // c++20 formatting library
#include <fmt/core.h> // fmt::*
#include <fmt/color.h> // fmt::color::*


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace dbg
{
  #ifdef NDEBUG
    #define D1LOG(...)
  #else
    // Occhio, fmt::print lancia std::runtime_error("invalid utf8") quindi meglio assicurarci di convertire
    #define D1LOG(s,...) fmt::print(fg(fmt::color::salmon) | fmt::emphasis::bold, s, __VA_ARGS__);
  #endif
  
  #ifdef NDEBUG
    #define D2LOG(...)
  #else
    #define D2LOG(s,...) fmt::print(fg(fmt::color::sky_blue) | fmt::emphasis::italic, s, __VA_ARGS__);
  #endif


  #ifdef MS_WINDOWS
    #ifdef NDEBUG
      #define EVTLOG(...)
    #else
      #define EVTLOG(s, ...) ::OutputDebugString(fmt::format(s,__VA_ARGS__).c_str());
    #endif
  #endif

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
