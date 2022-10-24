#ifndef GUARD_console_uts_hpp
#define GUARD_console_uts_hpp
//  ---------------------------------------------
//  Utilities for console I/O
//  ---------------------------------------------
#if defined(_WIN32) || defined(_WIN64)
  #define MS_WINDOWS 1
  #undef POSIX
#elif defined(__unix__) || defined(__linux__)
  #undef MS_WINDOWS
  #define POSIX 1
#else
  #undef MS_WINDOWS
  #undef POSIX
#endif

#if defined(MS_WINDOWS)
  namespace win
   {
    #include <Windows.h> // Beep (Utilapiset.h)
    #include <conio.h> // _getch, _putch, _cputs
   }
#elif defined(POSIX)
    //#include <cstdio> // std::puts
  namespace psx
   {
    #include <unistd.h> // read(), ...
   }
#endif
#include <cctype> // std::iscntrl
#include <string>
#include <string_view>


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sys //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

  #if defined(MS_WINDOWS)
    using namespace win;
  #elif defined(POSIX)
    using namespace psx;
  #endif


//---------------------------------------------------------------------------
void print(const char ch) noexcept
{
    _putch(ch);
}

//---------------------------------------------------------------------------
void print(const char* const msg) noexcept
{
    _cputs(msg);
}
void println(const char* const msg) noexcept
{
    _cputs(msg);
    print('\n');
}

//---------------------------------------------------------------------------
void print(const std::string_view msg) noexcept
{
    for(const char ch : msg) _putch(ch);
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
[[nodiscard]] std::string input_string(const char* const msg)
{
    std::string s;

    _cputs(msg);
    do {
        const char ch = static_cast<char>(_getch()); // Blocks until key pressed
        if( std::iscntrl(static_cast<unsigned char>(ch)) )
           {// Pressed enter or...
            _putch('\n');
            break;
           }
        else
           {
            _putch(ch); // Show pressed character
            s += ch;
           }
       }
    while(true);

    return s;
}


//---------------------------------------------------------------------------
[[nodiscard]] char choice(const char* const msg, const char* const allowed_chars) noexcept
{
  #if defined(MS_WINDOWS)
    char ch;
    const auto is_one_of = [](const char ch, const char* chars) noexcept -> bool
       {
        while( *chars )
           {
            if(ch==*chars) return true;
            else ++chars;
           }
        return false;
       };

    _cputs(msg);
    do {
        ch = static_cast<char>(_getch()); // Blocks until key pressed
        if( is_one_of(ch, allowed_chars) )
           {
            break;
           }
        Beep(0x25, 200); // freq [0x25÷0x7FFF Hz], duration [ms]
       }
    while(true);
    _putch(ch); // Show pressed character
    _putch('\n');
    return ch;
  #elif defined(POSIX)
    return '\0';
  #endif
}


///**
// Linux (POSIX) implementation of _kbhit().
// Morgan McGuire, morgan@cs.brown.edu
// */
//#include <stdio.h>
//#include <sys/select.h>
//#include <termios.h>
//#include <stropts.h>
//
//int _kbhit() {
//    static const int STDIN = 0;
//    static bool initialized = false;
//
//    if (! initialized) {
//        // Use termios to turn off line buffering
//        termios term;
//        tcgetattr(STDIN, &term);
//        term.c_lflag &= ~ICANON;
//        tcsetattr(STDIN, TCSANOW, &term);
//        setbuf(stdin, NULL);
//        initialized = true;
//    }
//
//    int bytesWaiting;
//    ioctl(STDIN, FIONREAD, &bytesWaiting);
//    return bytesWaiting;
//}
//
////////////////////////////////////////////////
////    Simple demo of _kbhit()
//
//#include <unistd.h>
//
//int main(int argc, char** argv) {
//    printf("Press any key");
//    while (! _kbhit()) {
//        printf(".");
//        fflush(stdout);
//        usleep(1000);
//    }
//    printf("\nDone.\n");
//
//    return 0;
//}


//#include <unistd.h> // read()
//#include <termios.h>
//class keyboard{
//    public:
//        keyboard(){
//            tcgetattr(0,&initial_settings);
//            new_settings = initial_settings;
//            new_settings.c_lflag &= ~ICANON;
//            new_settings.c_lflag &= ~ECHO;
//            new_settings.c_lflag &= ~ISIG;
//            new_settings.c_cc[VMIN] = 1;
//            new_settings.c_cc[VTIME] = 0;
//            tcsetattr(0, TCSANOW, &new_settings);
//            peek_character=-1;
//        }
//
//        ~keyboard(){
//            tcsetattr(0, TCSANOW, &initial_settings);
//        }
//
//        int kbhit(){
//            unsigned char ch;
//            int nread;
//            if(peek_character != -1) return 1;
//            new_settings.c_cc[VMIN]=0;
//            tcsetattr(0, TCSANOW, &new_settings);
//            nread = read(0,&ch,1);
//            new_settings.c_cc[VMIN]=1;
//            tcsetattr(0, TCSANOW, &new_settings);
//
//            if (nread == 1){
//                peek_character = ch;
//                return 1;
//            }
//            return 0;
//        }
//
//        int getch(){
//            char ch;
//
//            if (peek_character != -1){
//                ch = peek_character;
//                peek_character = -1;
//            }
//            else read(0,&ch,1);
//            return ch;
//        }
//
//    private:
//        struct termios initial_settings, new_settings;
//        int peek_character;
//};



//#include <ncurses.h>
//int main()
//{
//    initscr();
//    cbreak();
//    noecho();
//    scrollok(stdscr, TRUE);
//    nodelay(stdscr, TRUE);
//    while (true) {
//        if (getch() == 'g') {
//            printw("You pressed G\n");
//        }
//        napms(500);
//        printw("Running\n");
//    }
//}


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
