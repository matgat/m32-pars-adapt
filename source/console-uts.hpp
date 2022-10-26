#ifndef GUARD_console_uts_hpp
#define GUARD_console_uts_hpp
//  ---------------------------------------------
//  Utilities for console I/O
//  ---------------------------------------------
#include <string>
#include <string_view>

#include "os-detect.hpp" // MS_WINDOWS, POSIX

#if defined(MS_WINDOWS)
  #include <Windows.h> // Beep (Utilapiset.h)
  #include <conio.h> // _getch, _putch, _cputs
#elif defined(POSIX)
  #include <cstdio> // std::putchar, std::puts
  #include <unistd.h> // STDIN_FILENO
  #include <termios.h> // struct termios
#endif



//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sys //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


//---------------------------------------------------------------------------
void print(const char ch) noexcept
{
  #if defined(MS_WINDOWS)
    _putch(ch);
  #elif defined(POSIX)
    std::putchar(ch);
  #endif
}

//---------------------------------------------------------------------------
void print(const char* const msg) noexcept
{
  #if defined(MS_WINDOWS)
    _cputs(msg);
  #elif defined(POSIX)
    std::puts(msg);
  #endif
}
void println(const char* const msg) noexcept
{
    print(msg);
    print('\n');
}

//---------------------------------------------------------------------------
void print(const std::string_view msg) noexcept
{
    for(const char ch : msg) print(ch);
}
void println(const std::string_view msg) noexcept
{
    print(msg);
    print('\n');
}

//---------------------------------------------------------------------------
void print(const std::string& msg) noexcept
{
    print(msg.c_str());
}
void println(const std::string& msg) noexcept
{
    print(msg);
    print('\n');
}

//---------------------------------------------------------------------------
[[nodiscard]] int get_char() noexcept
{
  #if defined(MS_WINDOWS)
    return _getch();
  #elif defined(POSIX)
    struct termios attr;
    tcgetattr( STDIN_FILENO, &attr );
    const tcflag_t orig_c_lflag = attr.c_lflag;
    const tcflag_t c_lflag_mask = ICANON | ISIG | ECHO; // Remove echo to print char
    // attr.c_cc[VMIN] = 1;
    // attr.c_cc[VTIME] = 0;
    attr.c_lflag &= ~c_lflag_mask;
    tcsetattr( STDIN_FILENO, TCSANOW, &attr );
    const int ch = std::getchar();
    attr.c_lflag = orig_c_lflag;
    tcsetattr( STDIN_FILENO, TCSANOW, &attr );
    return ch;
  #endif
}

//---------------------------------------------------------------------------
[[nodiscard]] std::string input_string(const char* const msg)
{
    std::string s;

    print(msg);
    do {
        const char ch = static_cast<char>(get_char());

        if( static_cast<unsigned char>(ch)>=' ' )
           {// A printable char
            print(ch); // Show pressed character
            s += ch;
           }
        else if( ch=='\r' || ch=='\n' )
           {// Pressed enter
            print('\n');
            break;
           }
        else
           {// A control character (ESC?)
            s.clear();
            break;
           }

       }
    while(true);

    return s;
}


//---------------------------------------------------------------------------
[[nodiscard]] char choice(const char* const msg, const char* const allowed_chars) noexcept
{
    const auto is_one_of = [](const char ch, const char* chars) noexcept -> bool
       {
        while( *chars )
           {
            if(ch==*chars) return true;
            else ++chars;
           }
        return false;
       };

    print(msg);
    char ch;
    do {
        ch = static_cast<char>(get_char());

        if( is_one_of(ch, allowed_chars) )
           {
            break;
           }

      #if defined(MS_WINDOWS)
        Beep(0x25, 200); // freq [0x25÷0x7FFF Hz], duration [ms]
      #elif defined(POSIX)
        print('\a');
      #endif
       }
    while(true);
    print(ch); // Show pressed character
    print('\n');
    return ch;
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



/////////////////////////////////////////////////////////////////////////////
#ifdef TEST /////////////////////////////////////////////////////////////////
#include <fmt/core.h> // fmt::*
#include <fmt/color.h> // fmt::color::*
/////////////////////////////////////////////////////////////////////////////
class test_console_uts
{
 public:
    void expect(const bool cond, const std::string_view msg)
       {
        if( cond )
           {
            fmt::print(fg(fmt::color::green), "[passed] {}\n",msg);
           }
        else
           {
            fmt::print(fmt::emphasis::bold | fg(fmt::color::red), "[error] {}\n",msg);
            ++m_errors_count;
           }
       };

    [[nodiscard]] int errors_count() const noexcept { return m_errors_count; }

 private:
    int m_errors_count = 0;
};
//---------------------------------------------------------------------------
int main()
{
    test_console_uts test;

    sys::println("Testing " __FILE__);

    test.expect( sys::choice("Press c","c")=='c', "Pressing key 'c'" );

    char ch = sys::choice("Press one of abc","abc");
    test.expect( ch == 'a' || ch == 'b' || ch == 'c', fmt::format("Pressed key {}",ch) );

    std::string s = sys::input_string("Press <enter>");
    test.expect( s.empty(), fmt::format("Entered \"{}\" (expected empty)",s) );

    s = sys::input_string("Press 123<enter>");
    test.expect( s=="123", fmt::format("Entered \"{}\" (expected \"123\")",s) );

    return test.errors_count();
}
#endif //////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


//---- end unit -------------------------------------------------------------
#endif
