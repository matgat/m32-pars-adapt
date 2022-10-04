#ifndef GUARD_time_stamp_hpp
#define GUARD_time_stamp_hpp
//  ---------------------------------------------
//  Facilities related to time and timestamps
//  ---------------------------------------------
#include <chrono> // std::chrono::*
#include <fmt/core.h> // fmt::*
#include <fmt/chrono.h> // formatters for std::chrono
//using namespace std::chrono_literals; // 1s, 2h, ...



//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sys //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


//---------------------------------------------------------------------------
// Formatted time stamp
[[nodiscard]] std::string get_formatted_time_stamp()
{
    return fmt::format("{}", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
}


//---------------------------------------------------------------------------
// The Unix epoch (or Unix time or POSIX time or Unix timestamp) is the
// number of seconds that have elapsed since 1970-01-01T00:00:00Z
//[[nodiscard]] auto epoch_time_stamp()
//{
//    return epoch_time_stamp( std::chrono::system_clock::now() );
//}
//[[nodiscard]] auto epoch_time_stamp(const std::chrono::system_clock::time_point tp)
//{
//    return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
//}


// Old style:
// #include <ctime> // std::time_t, std::strftime, std::localtime

//---------------------------------------------------------------------------
//[[nodiscard]] std::string get_formatted_time_stamp(const std::time_t t)
//{
//    //return fmt::format("{:%Y-%m-%d}", std::localtime(&t));
//    char buf[64];
//    const std::size_t len = std::strftime(buf, sizeof(buf), "%F %T", std::localtime(&t));
//        // %F  equivalent to "%Y-%m-%d" (the ISO 8601 date format)
//        // %T  equivalent to "%H:%M:%S" (the ISO 8601 time format)
//    return std::string(buf, len);
//}

//---------------------------------------------------------------------------
//[[nodiscard]] std::string get_formatted_time_stamp()
//{
//    const std::time_t now = std::time(nullptr);
//    return get_formatted_time_stamp(now);
//}


//---------------------------------------------------------------------------
//[[nodiscard]] std::string file_time_stamp(const std::filesystem::file_time_type ftime)
//{
//    std::time_t cftime = std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(ftime));
//    return std::asctime( std::localtime(&cftime) );
//}


//---------------------------------------------------------------------------
// auto [ctime, mtime] = get_file_dates("/path/to/file");
//[[nodiscard]] std::tuple<std::time_t, std::time_t> get_file_dates(const std::string& spth) noexcept
//{
//    //const std::filesystem::path pth(spth);
//    // Note: in c++20 std::filesystem::file_time_type is guaranteed to be epoch
//    //return {std::filesystem::last_creation_time(pth), std::filesystem::last_write_time(pth)};
//
//  #ifdef MS_WINDOWS
//    HANDLE h = ::CreateFile(spth.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
//    if( h!=INVALID_HANDLE_VALUE )
//       {
//        //BY_HANDLE_FILE_INFORMATION fd{0};
//        //::GetFileInformationByHandle(h, &fd);
//        FILETIME ftCreationTime{0}, ftLastAccessTime{0}, ftLastWriteTime{0};
//        ::GetFileTime(h, &ftCreationTime, &ftLastAccessTime, &ftLastWriteTime);
//        ::CloseHandle(h);
//
//        FILETIME lftCreationTime{0}, lftLastWriteTime{0}; // Local file times
//        ::FileTimeToLocalFileTime(&ftCreationTime, &lftCreationTime);
//        ::FileTimeToLocalFileTime(&ftLastWriteTime, &lftLastWriteTime);
//
//        auto to_time_t = [](const FILETIME& ft) -> std::time_t
//           {
//            // FILETIME is is the number of 100 ns increments since January 1 1601
//            ULARGE_INTEGER wt = { ft.dwLowDateTime, ft.dwHighDateTime };
//            //const ULONGLONG TICKS_PER_SECOND = 10'000'000ULL;
//            //const ULONGLONG EPOCH_DIFFERENCE = 11644473600ULL;
//            return wt.QuadPart / 10000000ULL - 11644473600ULL;
//           };
//
//        return std::make_tuple(to_time_t(lftCreationTime), to_time_t(lftLastWriteTime));
//       }
//  #else
//    struct stat result;
//    if( stat(spth.c_str(), &result )==0 )
//       {
//        return {result.st_ctime, result.st_mtime};
//       }
//  #endif
//  return {0,0};
//}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
