#ifndef GUARD_prompt_hpp
#define GUARD_prompt_hpp
//  ---------------------------------------------
//  Get data from user
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
    #include <conio.h> // _getch, _cputs
   }
#elif defined(POSIX)
    //#include <cstdio> // std::puts
  namespace psx
   {
    #include <unistd.h> // read(), ...
   }
#endif

#include <string_view>


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sys //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


//---------------------------------------------------------------------------
[[nodiscard]] char choice( const std::string_view msg ) noexcept
{
    //std::cout << msg << '\n';
  #if defined(MS_WINDOWS)
    win::_cputs( msg.data(), msg.size() );
    return static_cast<char>(win::_getch());
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



//---- end unit -------------------------------------------------------------
#endif
