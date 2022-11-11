#ifndef GUARD_clipboard_hpp
#define GUARD_clipboard_hpp
//  ---------------------------------------------
//  Clipboard manipulation
//  ---------------------------------------------
#include <stdexcept> // std::runtime_error
#include <string>
#include <string_view>
#include <cstring> // std::memcpy

#include "os-detect.hpp" // MS_WINDOWS, POSIX


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sys //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

  #if defined(MS_WINDOWS)
    /////////////////////////////////////////////////////////////////////////
    // Wrapper to GlobalLock/GlobalUnlock
    class WinGlobalMemLock final
    {
     public:
        explicit WinGlobalMemLock(HGLOBAL h) noexcept
           : m_hmem(h)
           , m_ptr(::GlobalLock(m_hmem)) {}

        ~WinGlobalMemLock() noexcept
           {
            ::GlobalUnlock(m_hmem);
           }

        [[nodiscard]] HGLOBAL handle() const noexcept { return m_hmem; }
        [[nodiscard]] LPVOID data() const noexcept { return m_ptr; }

     private:
        HGLOBAL m_hmem;
        LPVOID m_ptr;
    };
  #endif



/////////////////////////////////////////////////////////////////////////////
// Wrapper to system clipboard
class Clipboard final
{
 public:
    Clipboard()
       {
      #if defined(MS_WINDOWS)
        if( !::OpenClipboard(NULL) )
           {
            throw std::runtime_error("Cannot access clipboard");
           }
      #elif defined(POSIX)
        //<implement>
      #endif
       }

    ~Clipboard() noexcept
       {
      #if defined(MS_WINDOWS)
        ::CloseClipboard();
      #elif defined(POSIX)
        //<implement>
      #endif
       }

    void set([[maybe_unused]] const std::string_view sv) const noexcept
       {
      #if defined(MS_WINDOWS)
        ::EmptyClipboard();
        WinGlobalMemLock mem{ ::GlobalAlloc(GMEM_MOVEABLE, sv.size()+1) }; // | GMEM_DDESHARE
        if( mem.data() )
           {
            char* const txt_arr = static_cast<char*>(mem.data());
            std::memcpy(txt_arr, sv.data(), sv.size());
            txt_arr[sv.size()] = '\0';
            ::SetClipboardData(CF_TEXT, mem.handle());
           }
      #elif defined(POSIX)
        //<implement>
      #endif
       }

    [[nodiscard]] std::string get() const noexcept
       {
        std::string text;
      #if defined(MS_WINDOWS)
        // Get handle of clipboard object for ANSI text
        if( HANDLE hData = ::GetClipboardData(CF_TEXT) )
           {
            WinGlobalMemLock mem(hData);
            if( mem.data() )
               {
                const char* const txt_arr = static_cast<char*>(mem.data());
                text = std::string(txt_arr);
               }
           }
      #elif defined(POSIX)
        //<implement>
      #endif
        return text;
       }
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


//---- end unit -------------------------------------------------------------
#endif
