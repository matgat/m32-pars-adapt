﻿#pragma once
//  ---------------------------------------------
//  A file buffered in memory
//  ---------------------------------------------
//  #include "memory_mapped_file.hpp" // sys::memory_mapped_file
//  ---------------------------------------------
#include <string_view>
#include <stdexcept> // std::runtime_error
#include <format>

#include "os-detect.hpp" // MS_WINDOWS, POSIX
#include "system_base.hpp" // sys::get_lasterr_msg()

#if defined(POSIX)
  #include <fcntl.h> // open
  #include <sys/stat.h> // fstat
  #include <sys/mman.h> // mmap, munmap
#endif


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sys
{

/////////////////////////////////////////////////////////////////////////////
class memory_mapped_file final
{
 private:
    const char* m_buf = nullptr;
    std::size_t m_bufsiz = 0;
  #if defined(MS_WINDOWS)
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMapping = nullptr;
  #endif

 public:
    explicit memory_mapped_file( const char* const pth_cstr )
       {
      #if defined(MS_WINDOWS)
        // cppcheck-suppress useInitializationList
        hFile = ::CreateFileA(pth_cstr, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, nullptr);
        if(hFile==INVALID_HANDLE_VALUE)
           {
            throw std::runtime_error{ std::format("Couldn't open {} ({}))", pth_cstr, get_lasterr_msg()) };
           }
        m_bufsiz = ::GetFileSize(hFile, nullptr);

        hMapping = ::CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if(hMapping==nullptr)
           {
            ::CloseHandle(hFile);
            throw std::runtime_error{ std::format("Couldn't map {} ({})", pth_cstr, get_lasterr_msg()) };
           }
        //
        m_buf = static_cast<const char*>( ::MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0) );
        if( m_buf==nullptr )
           {
            ::CloseHandle(hMapping);
            ::CloseHandle(hFile);
            throw std::runtime_error{ std::format("Couldn't create view of {} ({})", pth_cstr, get_lasterr_msg()) };
           }
      #elif defined(POSIX)
        const int fd = open(pth_cstr, O_RDONLY);
        if(fd==-1) throw std::runtime_error{ std::format("Couldn't open {}", pth_cstr) };

        // obtain file size
        struct stat sbuf {};
        if( fstat(fd, &sbuf)==-1 )
           {
            throw std::runtime_error{"Cannot fstat file size"};
           }
        m_bufsiz = static_cast<std::size_t>(sbuf.st_size);

        m_buf = static_cast<const char*>(mmap(nullptr, m_bufsiz, PROT_READ, MAP_PRIVATE, fd, 0U));
        if( m_buf==MAP_FAILED )
           {
            m_buf = nullptr;
            throw std::runtime_error{"Cannot map file"};
           }
      #endif
       }

    ~memory_mapped_file() noexcept
       {
        if( m_buf )
           {
          #if defined(MS_WINDOWS)
            ::UnmapViewOfFile(m_buf);
            if(hMapping) ::CloseHandle(hMapping);
            if(hFile!=INVALID_HANDLE_VALUE) ::CloseHandle(hFile);
          #elif defined(POSIX)
            /* const int ret = */ munmap(static_cast<void*>(const_cast<char*>(m_buf)), m_bufsiz);
            //if(ret==-1) std::cerr << "munmap() failed\n";
          #endif
           }
       }

    // Prevent copy
    memory_mapped_file(const memory_mapped_file& other) = delete;
    memory_mapped_file& operator=(const memory_mapped_file& other) = delete;

    // Move
    memory_mapped_file(memory_mapped_file&& other) noexcept
      : m_buf{other.m_buf}
      , m_bufsiz{other.m_bufsiz}
    #if defined(MS_WINDOWS)
      , hFile{other.hFile}
      , hMapping{other.hMapping}
    #endif
       {
        other.m_buf = nullptr;
        other.m_bufsiz = 0;
      #if defined(MS_WINDOWS)
        other.hFile = INVALID_HANDLE_VALUE;
        other.hMapping = nullptr;
      #endif
       }
    // Prevent move assignment
    memory_mapped_file& operator=(memory_mapped_file&& other) = delete;

    [[nodiscard]] std::string_view as_string_view() const noexcept { return std::string_view{m_buf, m_bufsiz}; }
};

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"sys::memory_mapped_file"> memory_mapped_file_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("file content match") = []
   {
    const std::string_view file_content = "123456"sv;
    test::TemporaryFile file("~file.tmp", file_content);
    const sys::memory_mapped_file mapped_file{ file.path().string().c_str() };

    ut::expect( ut::that % mapped_file.as_string_view() == file_content );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
