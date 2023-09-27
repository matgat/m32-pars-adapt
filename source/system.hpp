#ifndef GUARD_system_hpp
#define GUARD_system_hpp
//  ---------------------------------------------
//  System utilities
//  ---------------------------------------------
#include <stdexcept>
#include <cassert> // assert
#include <concepts> // std::convertible_to
#include <string>
#include <string_view>
#include <cstdlib> // std::getenv
#include <filesystem> // std::filesystem
namespace fs = std::filesystem;
#include <regex> // std::regex*
#include <fmt/core.h> // fmt::format

#include "os-detect.hpp" // MS_WINDOWS, POSIX

#if defined(MS_WINDOWS)
  #include <cstdio> // fopen_s (Microsoft 'deprecated' std::fopen)
  #include <cctype> // std::tolower

  #include <Windows.h>
  //#include <unistd.h> // _stat
  #include <shellapi.h> // FindExecutableA
  #include <shlwapi.h> // AssocQueryString
#elif defined(POSIX)
  #include <cstdio> // std::fopen, ...

  #include <unistd.h> // unlink, exec*, fork, ...
  #include <fcntl.h> // open
  #include <sys/mman.h> // mmap, munmap
  #include <sys/stat.h> // fstat
  #include <time.h>   // nanosleep
  #include <sys/wait.h> // waitpid
#endif


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sys //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


//---------------------------------------------------------------------------
void sleep_ms( const int ms )
{
  #if defined(MS_WINDOWS)
    ::Sleep(ms);
  #elif defined(POSIX)
    timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
  #endif
}


//---------------------------------------------------------------------------
//std::string expand_env_variables( const std::string& s )
//{
//    const std::string::size_type i_start = s.find("${");
//    if( i_start==std::string::npos ) return s;
//    std::string_view pre( s.data(), i_start );
//    i_start += 2; // Skip "$("
//    const std::string::size_type i_end = s.find('}', i_start);
//    if( i_end==std::string::npos ) return s;
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


#if defined(MS_WINDOWS)
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
                         0, // MAKELANGID deprecated
                         buf,
                         buf_siz,
                         nullptr );

    return siz>0 ? fmt::format("[{}] {}", e, std::string_view(buf, siz))
                 : fmt::format("[{}] Unknown error", e);
}

//---------------------------------------------------------------------------
[[nodiscard]] std::string find_executable_by_file(const std::string& doc) noexcept
{
    char buf[MAX_PATH + 1] = {'\0'};
    ::FindExecutableA(doc.c_str(), NULL, buf);
    return std::string(buf);
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

//---------------------------------------------------------------------------
void shell_execute(const char* const pth, const char* const args =nullptr) noexcept
{
    SHELLEXECUTEINFOA ShExecInfo = {0};
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
    ShExecInfo.fMask = 0;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = "open";
    ShExecInfo.lpFile = pth;
    ShExecInfo.lpParameters = args;
    ShExecInfo.lpDirectory = NULL;
    ShExecInfo.nShow = SW_SHOW;
    ShExecInfo.hInstApp = NULL;
    ::ShellExecuteEx(&ShExecInfo);
}

//---------------------------------------------------------------------------
[[maybe_unused]] int shell_execute_wait(const char* const pth, const char* const args =nullptr)
{
    const bool show = true;
    const bool wait = true;

    SHELLEXECUTEINFO ShExecInfo = {0};
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = (wait ? SEE_MASK_NOCLOSEPROCESS : 0) // SEE_MASK_DEFAULT
                       // | SEE_MASK_FLAG_NO_UI // Do not show error dialog in case of exe not found
                       // | SEE_MASK_DOENVSUBST // Substitute environment vars
                       // cppcheck-suppress badBitmaskCheck
                       | (show ? 0 : SEE_MASK_FLAG_NO_UI);
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = NULL;
    ShExecInfo.lpFile = pth;
    ShExecInfo.lpParameters = args;
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
        do {
            const DWORD WaitResult = ::WaitForSingleObject(ShExecInfo.hProcess, 500);
            if( WaitResult==WAIT_TIMEOUT )
               {// Still executing...
                sys::sleep_ms(100); //Application->ProcessMessages();
               }
            else if( WaitResult==WAIT_OBJECT_0 )
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
}
#endif


//---------------------------------------------------------------------------
// Same effect as double click on a file
void launch_file(const std::string& pth) noexcept
{
  #if defined(MS_WINDOWS)
    shell_execute( pth.c_str() );
  #elif defined(POSIX)
    if( const auto pid=fork(); pid==0 ) // pid_t
       {
        execlp("xdg-open", "xdg-open", pth.c_str(), nullptr);
       }
  #endif
}


//---------------------------------------------------------------------------
template<std::convertible_to<std::string> ...Args>
void execute(const char* const exe, Args&&... args) noexcept
{
  #if defined(MS_WINDOWS)
    try{
        auto join_args = [... args = std::forward<Args>(args)]() -> std::string
           {
            std::string s;
            const std::size_t totsiz = sizeof...(args) + (std::size(args) + ...);
            s.reserve(totsiz);
            ((s+=' ', s+=args), ...);
            return s;
           };

        shell_execute( exe, join_args().c_str() );
       }
    catch(...){}
  #elif defined(POSIX)
    if( const auto pid=fork(); pid==0 ) // pid_t
       {
        struct loc final
           {// Extract char pointer for posix api exec*
            static const char* c_str(const char* const s) noexcept { return s; }
            static const char* c_str(const std::string& s) noexcept { return s.c_str(); }
           };
        execlp(exe, exe, loc::c_str(std::forward<Args>(args))..., nullptr);
       }
  #endif
}



//---------------------------------------------------------------------------
template<std::convertible_to<std::string> ...Args>
[[maybe_unused]] int execute_wait(const char* const exe, Args&&... args) noexcept
{
  #if defined(MS_WINDOWS)
    try{
        auto join_args = [... args = std::forward<Args>(args)]() -> std::string
           {
            std::string s;
            const std::size_t totsiz = sizeof...(args) + (std::size(args) + ...);
            s.reserve(totsiz);
            ((s+=' ', s+=args), ...);
            return s;
           };

        return shell_execute_wait( exe, join_args().c_str() );
       }
    catch(...){}
  #elif defined(POSIX)
    const auto pid_child = fork(); // pid_t
    if( pid_child==0 )
       {// Fork successful, inside child process
        struct loc final
           {// Extract char pointer for posix api exec*
            static const char* c_str(const char* const s) noexcept { return s; }
            static const char* c_str(const std::string& s) noexcept { return s.c_str(); }
           };
        execlp(exe, exe, loc::c_str(std::forward<Args>(args))..., nullptr);
       }
    else if( pid_child!=-1 )
       {// Fork successful, inside parent process: wait child
        pid_t pid;
        int status;
        do {
            pid = waitpid(pid_child, &status, WUNTRACED | WCONTINUED);
            if(pid==-1) return -2; // waitpid error
            else if( WIFEXITED(status) ) return WEXITSTATUS(status); // Exited, return result
            else if( WIFSIGNALED(status) ) return -1; // Killed by WTERMSIG(status)
            else if( WIFSTOPPED(status) ) return -1; // Stopped by WSTOPSIG(status)
            //else if( WIFCONTINUED(status) ) continue;
           }
        while( !WIFEXITED(status) && !WIFSIGNALED(status) );
        //if(pid != pid_child) // Failed: Child process vanished
       }
    //else // Fork failed
  #endif
    return -3; // Something failed
}



/////////////////////////////////////////////////////////////////////////////
class MemoryMappedFile final
{
 public:
    explicit MemoryMappedFile( std::string&& pth )
      : i_path(pth)
       {
      #if defined(MS_WINDOWS)
        hFile = ::CreateFileA(i_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, nullptr);
        if(hFile==INVALID_HANDLE_VALUE)
           {
            throw std::runtime_error( fmt::format("Couldn't open {} ({}))", i_path, get_lasterr_msg()));
           }
        i_bufsiz = ::GetFileSize(hFile, nullptr);

        hMapping = ::CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if(hMapping==nullptr)
           {
            ::CloseHandle(hFile);
            throw std::runtime_error( fmt::format("Couldn't map {} ({})", i_path, get_lasterr_msg()));
           }
        //
        i_buf = static_cast<const char*>( ::MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0) );
        if(i_buf==nullptr)
           {
            ::CloseHandle(hMapping);
            ::CloseHandle(hFile);
            throw std::runtime_error( fmt::format("Couldn't create view of {} ({})", i_path, get_lasterr_msg()) );
           }
      #elif defined(POSIX)
        const int fd = open(i_path.c_str(), O_RDONLY);
        if(fd==-1) throw std::runtime_error( fmt::format("Couldn't open {}", i_path) );

        // obtain file size
        struct stat sbuf {};
        if(fstat(fd, &sbuf)==-1) throw std::runtime_error("Cannot stat file size");
        i_bufsiz = static_cast<std::size_t>(sbuf.st_size);

        i_buf = static_cast<const char*>(mmap(nullptr, i_bufsiz, PROT_READ, MAP_PRIVATE, fd, 0U));
        if(i_buf==MAP_FAILED)
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
          #if defined(MS_WINDOWS)
            ::UnmapViewOfFile(i_buf);
            if(hMapping) ::CloseHandle(hMapping);
            if(hFile!=INVALID_HANDLE_VALUE) ::CloseHandle(hFile);
          #elif defined(POSIX)
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
    #if defined(MS_WINDOWS)
      , hFile(other.hFile)
      , hMapping(other.hMapping)
    #endif
       {
        other.i_bufsiz = 0;
        other.i_buf = nullptr;
      #if defined(MS_WINDOWS)
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
    //[[nodiscard]] char operator[](const std::size_t i) const { return i_buf[i]; }

    [[nodiscard]] const std::string& path() const noexcept { return i_path; }

 private:
    std::size_t i_bufsiz = 0;
    const char* i_buf = nullptr;
    std::string i_path;
  #if defined(MS_WINDOWS)
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
      : m_handle( open(pth.c_str(), "wb") ) // "a" for append
       {}

    ~file_write() noexcept
       {
        if( m_handle )
           {
            std::fclose(m_handle);
           }
       }

    file_write(const file_write&) = delete; // Prevent copy
    file_write& operator=(const file_write&) = delete;

    file_write(file_write&& other) noexcept
      : m_handle(other.m_handle)
       {
        other.m_handle = nullptr;
       }

    file_write& operator=(file_write&& other) noexcept
       {
        std::swap(m_handle, other.m_handle);
        return *this;
       }


    const file_write& operator<<(const char c) const noexcept
       {
        assert(m_handle!=nullptr);
        std::fputc(c, m_handle);
        return *this;
       }

    const file_write& operator<<(const std::string_view s) const noexcept
       {
        assert(m_handle!=nullptr);
        std::fwrite(s.data(), sizeof(std::string_view::value_type), s.length(), m_handle);
        return *this;
       }

    const file_write& operator<<(const std::string& s) const noexcept
       {
        assert(m_handle!=nullptr);
        std::fwrite(s.data(), sizeof(std::string::value_type), s.length(), m_handle);
        return *this;
       }

 private:
    std::FILE* m_handle;

    [[nodiscard]] static inline std::FILE* open( const char* const filename, const char* const mode )
       {
      #if defined(MS_WINDOWS)
        std::FILE* f = nullptr;
        const errno_t err = fopen_s(&f, filename, mode);
        if(err) f = nullptr;
      #elif defined(POSIX)
        std::FILE* const f = std::fopen(filename, mode);
      #endif
        // cppcheck-suppress syntaxError
        if(!f) throw std::runtime_error( fmt::format("file_write: Cannot open {} as '{}'",filename,mode) );
        return f;
       }
};



//---------------------------------------------------------------------------
//void delete_file(const std::string& pth) noexcept
//{
//    //std::filesystem::remove(pth);
//  #if defined(MS_WINDOWS)
//    ::DeleteFile( pth.c_str() );
//  #elif defined(POSIX)
//    unlink( pth.c_str() );
//  #endif



//---------------------------------------------------------------------------
// ex. const auto removed_count = remove_all_inside(fs::temp_directory_path(), std::regex{R"-(^.*\.(tmp)$)-"});
//[[maybe_unused]] std::size_t remove_all_inside(const std::filesystem::path& dir, std::regex&& reg)
//{
//    std::size_t removed_items_count { 0 };
//
//    if( !fs::is_directory(dir) )
//       {
//        throw std::invalid_argument("Not a directory: " + dir.string());
//       }
//
//    for( auto& elem : fs::directory_iterator(dir) )
//       {
//        if( std::regex_match(elem.path().filename().string(), reg) )
//           {
//            removed_items_count += fs::remove_all(elem.path());
//           }
//       }
//
//    return removed_items_count;
//}



//---------------------------------------------------------------------------
// ex. const auto removed_count = remove_files_inside(fs::temp_directory_path(), std::regex{R"-(^.*\.(tmp)$)-"});
[[maybe_unused]] std::size_t remove_files_inside(const std::filesystem::path& dir, std::regex&& reg)
{
    std::size_t removed_items_count { 0 };

    if( !fs::is_directory(dir) )
       {
        throw std::invalid_argument("Not a directory: " + dir.string());
       }

    for( auto& elem : fs::directory_iterator(dir) )
       {
        if( elem.is_regular_file() && std::regex_match(elem.path().filename().string(), reg) )
           {
            removed_items_count += fs::remove(elem.path());
           }
       }

    return removed_items_count;
}


//---------------------------------------------------------------------------
[[maybe_unused]] fs::path backup_file_same_dir(const fs::path src_pth)
{
    fs::path dst_pth{ src_pth.parent_path() / fmt::format("~{}.~1", src_pth.filename().string()) };
    int k = 1;
    while( fs::exists(dst_pth) )
       {
        dst_pth.replace_extension( fmt::format(".~{}", ++k) );
       }
    std::filesystem::copy_file(src_pth, dst_pth);
    return dst_pth;
}


//---------------------------------------------------------------------------
[[nodiscard]] bool are_paths_equivalent(const fs::path& pth1, const fs::path& pth2)
{
    // Unfortunately fs::equivalent() needs existing files
    if( fs::exists(pth1) && fs::exists(pth2) )
       {
        return fs::equivalent(pth1,pth2);
       }
    // The following is not perfect:
    //   .'fs::absolute' implementation may need the file existence
    //   .'fs::weakly_canonical' may need to be called multiple times
  #if defined(MS_WINDOWS)
    // Windows filesystem is case insensitive
    // For case insensitive comparison could use std::memicmp (<cstring>)
    const auto tolower = [](std::string&& s) noexcept -> std::string
       {
        for(char& c : s) c = static_cast<char>(std::tolower(c));
        return s;
       };
    return tolower(fs::weakly_canonical(fs::absolute(pth1)).string()) ==
           tolower(fs::weakly_canonical(fs::absolute(pth2)).string());
  #else
    return fs::weakly_canonical(fs::absolute(pth1)) ==
           fs::weakly_canonical(fs::absolute(pth2));
  #endif
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
