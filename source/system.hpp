#ifndef GUARD_system_hpp
#define GUARD_system_hpp
//  ---------------------------------------------
//  System utilities
//  ---------------------------------------------
#include <string>
#include <string_view>
#include <tuple>
#include <stdexcept>
#include <cstdio> // std::fopen, ...
#include <cstdlib> // std::getenv
//#include <fstream>
//#include <chrono> // std::chrono::system_clock
//using namespace std::chrono_literals; // 1s, 2h, ...
#include <ctime> // std::time_t, std::strftime
#include <filesystem> // std::filesystem
namespace fs = std::filesystem;
#include <regex> // std::regex*
#include <fmt/core.h> // fmt::format

#if defined(_WIN32) || defined(_WIN64)
  #define MS_WINDOWS 1
#else
  #undef MS_WINDOWS
#endif

#ifdef MS_WINDOWS
  #include <Windows.h>
  //#include <unistd.h> // _stat
  #include <shellapi.h> // FindExecutableA
  #include <shlwapi.h> // AssocQueryString
#else
  #include <fcntl.h> // open
  #include <sys/mman.h> // mmap, munmap
  #include <sys/stat.h> // fstat
  #include <unistd.h> // unlink
#endif


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sys //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


//---------------------------------------------------------------------------
void sleep_ms( const DWORD ms )
{
  #ifdef MS_WINDOWS
    ::Sleep(ms);
  #else
  #endif
}


//---------------------------------------------------------------------------
//std::string expand_env_variables( const std::string& s )
//{
//    const std::string::size_type i_start = s.find("${");
//    if( i_start == std::string::npos ) return s;
//    std::string_view pre( s.data(), i_start );
//    i_start += 2; // Skip "$("
//    const std::string::size_type i_end = s.find('}', i_start);
//    if( i_end == std::string::npos ) return s;
//    std::string_view post = ( s.data()+i_end+1, s.length()-(i_end+1) );
//    std::string_view variable( s.data()+i_start, i_end-i_start );
//    std::string value { std::getenv(variable.c_str()) };
//    return expand_env_variables( fmt::format("{}{}{}",pre,value,post) );
//}


//---------------------------------------------------------------------------
[[nodiscard]] std::string expand_env_variables( std::string s )
{
    static const std::regex env_re{R"--(\$\{([\w_]+)\}|%([\w_]+)%)--"};
    std::smatch match;
    while( std::regex_search(s, match, env_re) )
       {
        const std::string capture = match[1].matched ? match[1].str() : match[2].str();
        s.replace(match[0].first, match[0].second, std::getenv(capture.c_str()));
       }
    return s;
}


#ifdef MS_WINDOWS
//---------------------------------------------------------------------------
// Format system error message
[[nodiscard]] std::string get_lasterr_msg(DWORD e =0) noexcept
{
    //#include <system_error>
    //std::string message = std::system_category().message(e);

    if(e==0) e = ::GetLastError(); // ::WSAGetLastError()
    const DWORD buf_siz = 1024;
    TCHAR buf[buf_siz];
    const DWORD siz =
        ::FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM |
                         FORMAT_MESSAGE_IGNORE_INSERTS|
                         FORMAT_MESSAGE_MAX_WIDTH_MASK,
                         nullptr,
                         e,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                         buf,
                         buf_siz,
                         nullptr );

    return siz>0 ? fmt::format("[{}] {}", e, std::string(buf, siz))
                 : fmt::format("[{}] Unknown error", e);
}


//---------------------------------------------------------------------------
[[nodiscard]] std::string find_executable_by_file(const std::string& doc) noexcept
{
    char buf[MAX_PATH + 1] = {'\0'};
    ::FindExecutableA(doc.c_str(), NULL, buf);
    std::string exe_path(buf);
    return exe_path;
}

//---------------------------------------------------------------------------
// Find the application that opens an extension
[[nodiscard]] std::string find_executable_by_ext(const char* ext_with_dot)
{
    DWORD buf_len = MAX_PATH;
    TCHAR buf[MAX_PATH+1];
    HRESULT hr = ::AssocQueryString(ASSOCF_NOTRUNCATE, ASSOCSTR_EXECUTABLE, ext_with_dot, "open", buf, &buf_len);
    if(hr==E_POINTER)
       {
        throw std::runtime_error("AssocQueryString: buffer too small to hold the entire path");
       }
    else if( hr<0 )
       {
        throw std::runtime_error( fmt::format("AssocQueryString: {}", sys::get_lasterr_msg(HRESULT_CODE(hr))) );
       }
    return std::string(buf,buf_len);
}
#endif


//---------------------------------------------------------------------------
void launch(const std::string& pth, const std::string& args ="") noexcept
{
  #ifdef MS_WINDOWS
    SHELLEXECUTEINFOA ShExecInfo = {0};
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
    ShExecInfo.fMask = 0;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = "open";
    ShExecInfo.lpFile = pth.c_str();
    ShExecInfo.lpParameters = args.c_str();
    ShExecInfo.lpDirectory = NULL;
    ShExecInfo.nShow = SW_SHOW;
    ShExecInfo.hInstApp = NULL;
    ::ShellExecuteEx(&ShExecInfo);
  #else
    //g_spawn_command_line_sync ?
    //#include <unistd.h> // 'exec*'
    // v: take an array parameter to specify the argv[] array of the new program. The end of the arguments is indicated by an array element containing NULL.
    // l: take the arguments of the new program as a variable-length argument list to the function itself. The end of the arguments is indicated by a (char *)NULL argument. You should always include the type cast, because NULL is allowed to be an integer constant, and default argument conversions when calling a variadic function won't convert that to a pointer.
    // e: take an extra argument (or arguments in the l case) to provide the environment of the new program; otherwise, the program inherits the current process's environment. This is provided in the same way as the argv array: an array for execve(), separate arguments for execle().
    // p: search the PATH environment variable to find the program if it doesn't have a directory in it (i.e. it doesn't contain a / character). Otherwise, the program name is always treated as a path to the executable.
    //int execvp (const char *file, char *const argv[]);
    //int execlp(const char *file, const char *arg,.../* (char  *) NULL */);

    //pid = fork();
    //if( pid == 0 )
    //   {
    //    execlp("/usr/bin/xdg-open", "xdg-open", pth.c_str(), nullptr);
    //   }
  #endif
}


//---------------------------------------------------------------------------
[[maybe_unused]] int launch_wait(const std::string& pth, const std::string& args ="")
{
  #ifdef MS_WINDOWS
    const bool show = true;
    const bool wait = true;

    SHELLEXECUTEINFO ShExecInfo = {0};
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask =   (wait ? SEE_MASK_NOCLOSEPROCESS : 0) // SEE_MASK_DEFAULT
                       // | SEE_MASK_FLAG_NO_UI // Do not show error dialog in case of exe not found
                       // | SEE_MASK_DOENVSUBST // Substitute environment vars
                       | (show ? 0 : SEE_MASK_FLAG_NO_UI);
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = NULL;
    ShExecInfo.lpFile = pth.c_str();
    ShExecInfo.lpParameters = args.empty() ? NULL : args.c_str();
    ShExecInfo.lpDirectory = NULL; // base_dir.empty() ? NULL : base_dir.c_str();
    ShExecInfo.nShow = show ? SW_SHOW : SW_HIDE;
    ShExecInfo.hInstApp = NULL;
    if( !::ShellExecuteEx(&ShExecInfo) )
       {
        throw std::runtime_error( fmt::format("Cannot run {}: {}", pth, sys::get_lasterr_msg()) );
       }

    DWORD ret = 0xFFFFFFFF; // A default value to be interpreted as "no exit code"

    if( wait )
       {
        DWORD WaitResult;
        do {
            WaitResult = ::WaitForSingleObject(ShExecInfo.hProcess, 500);
            if( WaitResult == WAIT_TIMEOUT )
               {// Still executing...
                sys::sleep_ms(100); //Application->ProcessMessages();
               }
            else if( WaitResult == WAIT_OBJECT_0 )
               {
                break;
               }
            else
               {
                ::CloseHandle(ShExecInfo.hProcess); // SEE_MASK_NOCLOSEPROCESS
                throw std::runtime_error( fmt::format("ShellExecuteEx: spawn error of {}",pth) );
               }
           }
        while(true);

        // If here process ended
        ::GetExitCodeProcess(ShExecInfo.hProcess, &ret);
        ::CloseHandle(ShExecInfo.hProcess); // SEE_MASK_NOCLOSEPROCESS
       }

    return ret;
  #endif
}


//#include <unistd.h>
//#include <sys/types.h>
//int foo(char *adr[])
//{
//        pid_t pid;
//
//        pid=fork();
//        if (pid==0)
//        {
//                if (execv("/usr/bin/mozilla",adr)<0)
//                        return -1;
//                else
//                        return 1;
//        }
//        else if(pid>0)
//                return 2;
//        else
//                return 0;
//}

// sh_cmd() - executes a command in the background
// returns TRUE is command was executed (not the result of the command though..)
//static gint sh_cmd (gchar * path, gchar * cmd, gchar * args)
//{
//  gchar     cmd_line[256];
//  gchar   **argv;
//  gint      argp;
//  gint      rc = 0;
//
//  if (cmd == NULL)
//    return FALSE;
//
//  if (cmd[0] == '\0')
//    return FALSE;
//
//  if (path != NULL)
//    chdir (path);
//
//  snprintf (cmd_line, sizeof (cmd_line), "%s %s", cmd, args);
//
//  rc = g_shell_parse_argv (cmd_line, &argp, &argv, NULL);
//  if (!rc)
//  {
//    g_strfreev (argv);
//    return rc;
//  }
//
//  rc = g_spawn_async (path, argv, NULL,
//          G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_SEARCH_PATH,
//          NULL, NULL, NULL, NULL);
//
//  g_strfreev (argv);
//
//  return rc;
//}

// static gint get_new_ptable (P_Fah_monitor fm)
// {
//  gint   i_retcode = 0, i_exitcode = 0;
//  gchar cv_filename[384];
//
// #ifdef DLOGIC
//   g_message (CONFIG_NAME":> Entered get_new_ptable(%d)...\n",fm->cb_id);
// #endif
//
//   if ( fm->i_stanford_points )   // TRUE if point table IS out of date
//   {
//     chdir ( fm->path_string );
//
//     i_retcode = g_spawn_command_line_sync (
//                      g_strdup_printf ("wget -N %s", STANDFORD_FAHPOINTS_URL),
//                                          NULL, NULL, &i_exitcode, NULL);
//
//     if( i_retcode )
//     {
//      ... good if retcode = 0
//     }

//#include <stdlib.h>
//#include <unistd.h>
//#include <sys/types.h>
//#include <sys/wait.h>
//#include <errno.h>
// Launch preferred application (in parallel) to open the specified file.
// The function returns errno for (apparent) success,
// and nonzero error code otherwise.
// Note that error cases are visually reported by xdg-open to the user,
// so that there is no need to provide error messages to user.
//int open_preferred(const char *const filename)
//{
//    const char *const args[3] = { "xdg-open", filename, NULL };
//    pid_t child, p;
//    int status;
//
//    if (!filename || !*filename)
//        return errno = EINVAL; // Invalid file name
//
//    // Fork a child process.
//    child = fork();
//    if (child == (pid_t)-1)
//        return errno = EAGAIN; // Out of resources, or similar
//
//    if (!child) {
//        // Child process. Execute.
//        execvp(args[0], (char **)args);
//        // Failed. Return 3, "a required too could not be found".
//        exit(3);
//    }
//
//    // Parent process. Wait for child to exit.
//    do {
//        p = waitpid(child, &status, 0);
//    } while (p == (pid_t)-1 && errno == EINTR);
//    if (p != child)
//        return errno = ECHILD; // Failed; child process vanished
//
//    // Did the child process exit normally?
//    if (!WIFEXITED(status))
//        return errno = ECHILD; // Child process was aborted
//
//    switch (WEXITSTATUS(status)) {
//    case 0:  return errno = 0;       // Success
//    case 1:  return errno = EINVAL;  // Error in command line syntax
//    case 2:  return errno = ENOENT;  // File not found
//    case 3:  return errno = ENODEV;  // Application not found
//    default: return errno = EAGAIN;  // Failed for other reasons.
//    }
//}


//---------------------------------------------------------------------------
void edit_text_file(const std::string& pth, const std::size_t offset) noexcept
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
  #endif
}



/////////////////////////////////////////////////////////////////////////////
class MemoryMappedFile final
{
 public:
    explicit MemoryMappedFile( std::string&& pth )
      : i_path(pth)
       {
      #ifdef MS_WINDOWS
        hFile = ::CreateFileA(i_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, nullptr);
        if(hFile == INVALID_HANDLE_VALUE)
           {
            throw std::runtime_error( fmt::format("Couldn't open {} ({}))", i_path, get_lasterr_msg()));
           }
        i_bufsiz = ::GetFileSize(hFile, nullptr);

        hMapping = ::CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if(hMapping == nullptr)
           {
            ::CloseHandle(hFile);
            throw std::runtime_error( fmt::format("Couldn't map {} ({})", i_path, get_lasterr_msg()));
           }
        //
        i_buf = static_cast<const char*>( ::MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0) );
        if(i_buf == nullptr)
           {
            ::CloseHandle(hMapping);
            ::CloseHandle(hFile);
            throw std::runtime_error( fmt::format("Couldn't create view of {} ({})", i_path, get_lasterr_msg()) );
           }
      #else
        const int fd = open(i_path.c_str(), O_RDONLY);
        if(fd == -1) throw std::runtime_error( fmt::format("Couldn't open {}", i_path) );

        // obtain file size
        struct stat sbuf {};
        if(fstat(fd, &sbuf) == -1) throw std::runtime_error("Cannot stat file size");
        i_bufsiz = static_cast<std::size_t>(sbuf.st_size);

        i_buf = static_cast<const char*>(mmap(nullptr, i_bufsiz, PROT_READ, MAP_PRIVATE, fd, 0U));
        if(i_buf == MAP_FAILED)
           {
            i_buf = nullptr;
            throw std::runtime_error("Cannot map file");
           }
      #endif
       }

    ~MemoryMappedFile() noexcept
       {
        if(i_buf)
           {
          #ifdef MS_WINDOWS
            ::UnmapViewOfFile(i_buf);
            if(hMapping) ::CloseHandle(hMapping);
            if(hFile!=INVALID_HANDLE_VALUE) ::CloseHandle(hFile);
          #else
            /* const int ret = */ munmap(static_cast<void*>(const_cast<char*>(i_buf)), i_bufsiz);
            //if(ret==-1) std::cerr << "munmap() failed\n";
          #endif
           }
       }

    // Prevent copy
    MemoryMappedFile(const MemoryMappedFile& other) = delete;
    MemoryMappedFile& operator=(const MemoryMappedFile& other) = delete;

    // Move
    MemoryMappedFile(MemoryMappedFile&& other) noexcept
      : i_bufsiz(other.i_bufsiz)
      , i_buf(other.i_buf)
      , i_path(std::move(other.i_path))
    #ifdef MS_WINDOWS
      , hFile(other.hFile)
      , hMapping(other.hMapping)
    #endif
       {
        other.i_bufsiz = 0;
        other.i_buf = nullptr;
      #ifdef MS_WINDOWS
        other.hFile = INVALID_HANDLE_VALUE;
        other.hMapping = nullptr;
      #endif
       }
    // Prevent move assignment
    MemoryMappedFile& operator=(MemoryMappedFile&& other) = delete;

    [[nodiscard]] std::size_t size() const noexcept { return i_bufsiz; }
    [[nodiscard]] const char* begin() const noexcept { return i_buf; }
    [[nodiscard]] const char* end() const noexcept { return i_buf + i_bufsiz; }
    [[nodiscard]] std::string_view as_string_view() const noexcept { return std::string_view{i_buf, i_bufsiz}; }

    [[nodiscard]] const std::string& path() const noexcept { return i_path; }

 private:
    std::size_t i_bufsiz = 0;
    const char* i_buf = nullptr;
    std::string i_path;
  #ifdef MS_WINDOWS
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMapping = nullptr;
  #endif
};



/////////////////////////////////////////////////////////////////////////////
// (Over)Write a file
class file_write final
{
 public:
    explicit file_write(const std::string& pth)
       {
      #ifdef MS_WINDOWS
        const errno_t err = fopen_s(&i_File, pth.c_str(), "wb"); // "a" for append
        if(err) throw std::runtime_error( fmt::format("Cannot write to: {}",pth) );
      #else
        i_File = fopen(pth.c_str(), "wb"); // "a" for append
        if(!i_File) throw std::runtime_error( fmt::format("Cannot write to: {}",pth) );
      #endif
       }

    ~file_write() noexcept
       {
        fclose(i_File);
       }

    file_write(const file_write&) = delete;
    file_write(file_write&&) = delete;
    file_write& operator=(const file_write&) = delete;
    file_write& operator=(file_write&&) = delete;

    const file_write& operator<<(const char c) const noexcept
       {
        fputc(c, i_File);
        return *this;
       }

    const file_write& operator<<(const std::string_view s) const noexcept
       {
        fwrite(s.data(), sizeof(std::string_view::value_type), s.length(), i_File);
        return *this;
       }

    const file_write& operator<<(const std::string& s) const noexcept
       {
        fwrite(s.data(), sizeof(std::string::value_type), s.length(), i_File);
        return *this;
       }

 private:
    FILE* i_File;
};



//---------------------------------------------------------------------------
void delete_file(const std::string& pth) noexcept
{
    //std::filesystem::remove(pth);
  #ifdef MS_WINDOWS
    ::DeleteFile( pth.c_str() );
  #else
    unlink( pth.c_str() );
  #endif
}


//---------------------------------------------------------------------------
void backup_file_same_dir(const fs::path src_pth)
{
    fs::path dst_pth{ src_pth.parent_path() / fmt::format("~{}.~1", src_pth.filename().string()) };
    int k = 1;
    while( fs::exists(dst_pth) )
       {
        dst_pth.replace_extension( fmt::format(".~{}", ++k) );
       }
    std::filesystem::copy_file(src_pth, dst_pth);
}


//---------------------------------------------------------------------------
// Append string to existing file using c++ streams
//void append_to_file(const std::string_view path, const std::string_view txt)
//{
//    std::ofstream os;
//    //os.exceptions(os.exceptions() | std::ios::failbit); // Some gcc has a bug bug that raises a useless 'ios_base::failure'
//    os.open(path, std::ios::out | std::ios::app);
//    if(os.fail()) throw std::ios_base::failure(std::strerror(errno));
//    os.exceptions(os.exceptions() | std::ios::failbit | std::ifstream::badbit);
//    os << txt;
//}



//---------------------------------------------------------------------------
// Buffer the content of a file using c++ streams
//[[nodiscard]] std::string read(const std::string_view path)
//{
//    std::ifstream is(path, std::ios::in | std::ios::binary);
//        // Read file size
//        is.seekg(0, std::ios::end);
//        const std::size_t buf_siz = is.tellg();
//        is.seekg(0, std::ios::beg);
//    std::string buf;
//    buf.reserve(buf_siz+1);
//    is.read(buf.data(), buf_siz);
//    buf.set_length(buf_siz);
//    buf[buf_siz] = '\0';
//    return buf;
//}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
