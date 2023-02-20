#ifndef GUARD_basic_parser_hpp
#define GUARD_basic_parser_hpp
//  ---------------------------------------------
//  Common parsing facilities
//  UTF-8 files (yeah, drop other encodings)
//  Unix line end '\n' (are you using a typewriter?)
//  ---------------------------------------------
#include <cctype> // std::isdigit, std::isblank, ...
#include <vector>
#include <string_view>
#include <stdexcept> // std::exception, std::runtime_error, ...
#include <fmt/core.h> // fmt::format

#include "string-utilities.hpp" // str::escape
#include "debug.hpp" // DLOG#

using namespace std::literals; // "..."sv



/////////////////////////////////////////////////////////////////////////////
class parse_error final : public std::exception
{
 public:
    explicit parse_error(const std::string_view msg,
                         const std::string_view pth,
                         const std::size_t lin,
                         const std::size_t off) noexcept
       : m_msg(fmt::format("{} ({}:{})",msg,pth,lin))
       , m_filepath(pth)
       , m_line(lin)
       , m_pos(off) {}

    const char* what() const noexcept override { return m_msg.c_str(); } // Could rise a '-Wweak-vtables'

    const std::string& file_path() const noexcept { return m_filepath; }
    std::size_t line() const noexcept { return m_line; }
    std::size_t pos() const noexcept { return m_pos; }

 private:
    std::string m_msg;
    std::string m_filepath;
    std::size_t m_line; // Line
    std::size_t m_pos; // Character offset
};



/////////////////////////////////////////////////////////////////////////////
class BasicParser
{
 protected:
    const std::string file_path;
    const char* const buf;
    const std::size_t siz; // buffer size
    const std::size_t i_last; // index of the last character
    std::size_t line; // Current line number
    std::size_t i; // Current character
    std::vector<std::string>& issues; // Problems found
    const bool fussy;

 public:
    BasicParser(const std::string& pth,
                const std::string_view dat,
                std::vector<std::string>& lst,
                const bool fus)
      : file_path(pth)
      , buf(dat.data())
      , siz(dat.size())
      , i_last(siz-1u) // siz>0
      , line(1)
      , i(0)
      , issues(lst)
      , fussy(fus)
       {
        check_and_skip_bom();
        if( i>=siz )
           {
            throw std::runtime_error("No data to parse (empty file?)");
           }

        // Doesn't play well with windows EOL "\r\n", since uses string_view extensively, cannot eat '\r'
        //if(buf.find('\r') != buf.npos) throw std::runtime_error("Use unix EOL, remove CR (\\r) characters");
       }

    BasicParser(const BasicParser&) = delete; // Prevent copy
    BasicParser(BasicParser&&) = delete; // Prevent move
    BasicParser& operator=(const BasicParser&) = delete; // Prevent copy assignment
    BasicParser& operator=(BasicParser&&) = delete; // Prevent move assignment
    ~BasicParser() = default; // Yes, destructor is not virtual


    //-----------------------------------------------------------------------
    [[nodiscard]] bool has_data() const noexcept { return i<siz; }
    [[nodiscard]] std::size_t curr_line() const noexcept { return line; }
    [[nodiscard]] std::size_t curr_pos() const noexcept { return i; }


    //-----------------------------------------------------------------------
    parse_error create_parse_error(const std::string_view msg) const noexcept
       {
        return parse_error(msg, file_path, line, i<=i_last ? i : i_last);
       }

    //-----------------------------------------------------------------------
    parse_error create_parse_error(const std::string_view msg, const std::size_t lin, const std::size_t off) const noexcept
       {
        return parse_error(msg, file_path, lin, off<=i_last ? off : i_last);
       }

    //-----------------------------------------------------------------------
    //template<typename ...Args> void notify_error(const std::string_view msg, Args&&... args) const
    //   {
    //    // Unfortunately this generates error: "call to immediate function is not a constant expression"
    //    //if(fussy) throw create_parse_error( fmt::format(msg, args...) );
    //    //else issues.push_back( fmt::format("{} (line {}, offset {})"sv, fmt::format(msg, args...), line, i) );
    //    if(fussy) throw create_parse_error( fmt::format(fmt::runtime(msg), args...) );
    //    else issues.push_back( fmt::format("{} (line {}, offset {})", fmt::format(fmt::runtime(msg), args...), line, i) );
    //   }
    // Need to pass a compile-time string literal. A technique:
    //template<std::size_t N> struct fixed_string_t
    //{
    //    char str[N];
    //    constexpr fixed_string_t(const char (&str_)[N]) noexcept { std::ranges::copy(str_, str); }
    //};
    //template<fixed_string_t fmt_str, typename... Args> void my_format(Args&&... args)
    //{
    //    puts( std::format(fmt_str.str, args...).c_str() );
    //}
    //my_format<"This is a {}">("test");


    // consteval friendly:
    #define notify_error(...) \
       {\
        if(fussy) throw create_parse_error( fmt::format(__VA_ARGS__) );\
        else issues.push_back( fmt::format("{} (line {}, offset {})"sv, fmt::format(__VA_ARGS__), line, i) );\
       }


 protected:

    //-----------------------------------------------------------------------
    [[nodiscard]] static bool is_identifier(const char ch) noexcept
       {
        return std::isalnum(ch) || ch=='_';
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] static bool is_numeric_literal(const char ch) noexcept
       {
        return std::isdigit(ch) || ch=='+' || ch=='-' || ch=='.' || ch=='E';
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] static bool is_blank(const char ch) noexcept
       {
        return std::isspace(ch) && ch!='\n';
       }

    //-----------------------------------------------------------------------
    // Skip space chars except new line
    void skip_blanks() noexcept
       {
        while( i<siz && is_blank(buf[i]) ) ++i;
       }

    //-----------------------------------------------------------------------
    // Tell if skipped space chars except new line
    //[[nodiscard]] bool eat_blanks() noexcept
    //   {
    //    assert(i<siz);
    //    if( is_blank(buf[i]) )
    //       {
    //        do{ ++i; } while( i<siz && is_blank(buf[i]) );
    //        return true;
    //       }
    //    return false;
    //   }

    //-----------------------------------------------------------------------
    // Get blank chars
    //[[nodiscard]] std::string_view eat_blanks() noexcept
    //   {
    //    // Intercept the edge case already at buffer end:
    //    if(i>i_last) return std::string_view(buf+i_last, 0);
    //
    //    const std::size_t i_start = i;
    //    while( i<siz && is_blank(buf[i]) ) ++i;
    //    return std::string_view(buf+i_start, i-i_start);
    //   }

    //-----------------------------------------------------------------------
    // Skip any space, including new line
    void skip_any_space() noexcept
       {
        while( i<siz && std::isspace(buf[i]) )
           {
            if( buf[i]=='\n' ) ++line;
            ++i;
           }
       }

    //-----------------------------------------------------------------------
    // Tell if skipped any space, including new line
    //[[nodiscard]] bool eat_any_space() noexcept
    //   {
    //    assert(i<siz);
    //    if( std::isspace(buf[i]) )
    //       {
    //        do {
    //            if( buf[i]=='\n' ) ++line;
    //            ++i;
    //           }
    //        while( i<siz && std::isspace(buf[i]) );
    //        return true;
    //       }
    //    return false;
    //   }

    //-----------------------------------------------------------------------
    [[maybe_unused]] bool eat_line_end() noexcept
       {
        assert(i<siz);
        if( buf[i]=='\n' )
           {
            ++i;
            ++line;
            return true;
           }
        return false;
       }


    //-----------------------------------------------------------------------
    // Skip empty lines
    void skip_empty_lines() noexcept
       {
        do{ skip_blanks(); }
        while( eat_line_end() );
       }


    //-----------------------------------------------------------------------
    void check_if_line_ended_after(const std::string_view fmt_str, const std::string_view fmt_arg)
       {
        skip_blanks();
        if( !eat_line_end() )
           {
            notify_error("Unexpected content after {}: {}", fmt::format(fmt::runtime(fmt_str), fmt_arg), str::escape(skip_line()));
           }
       }


    //-----------------------------------------------------------------------
    [[maybe_unused]] std::string_view skip_line() noexcept
       {
        // Intercept the edge case already at buffer end:
        if(i>i_last) return std::string_view(buf+i_last, 0);

        const std::size_t i_start = i;
        while( i<siz && !eat_line_end() ) ++i;
        return std::string_view(buf+i_start, i-i_start);
        // Note: If '\n' not found returns what remains in buf and i==siz
       }


    //-----------------------------------------------------------------------
    //void skip_line_check_no_further_content()
    //   {
    //    while( i<siz && !eat_line_end() )
    //       {
    //        if( !std::isspace(buf[i]) )
    //           {
    //            notify_error("Unexpected content: {}", str::escape(skip_line()));
    //           }
    //        ++i;
    //       }
    //   }


    //-----------------------------------------------------------------------
    [[nodiscard]] bool eat(const std::string_view s) noexcept
       {
        const std::size_t i_end = i+s.length();
        if( i_end<=siz && s==std::string_view(buf+i,s.length()) )
           {
            i = i_end;
            return true;
           }
        return false;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] bool eat_token(const std::string_view s) noexcept
       {
        const std::size_t i_end = i+s.length();
        if( ((i_end<siz && !std::isalnum(buf[i_end])) || i_end==siz) && s==std::string_view(buf+i,s.length()) )
           {
            i = i_end;
            return true;
           }
        return false;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::string_view collect_token() noexcept
       {
        assert(i<siz);
        const std::size_t i_start = i;
        while( i<siz && !std::isspace(buf[i]) ) ++i;
        return std::string_view(buf+i_start, i-i_start);
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::string_view collect_identifier() noexcept
       {
        assert(i<siz);
        const std::size_t i_start = i;
        while( i<siz && is_identifier(buf[i]) ) ++i;
        return std::string_view(buf+i_start, i-i_start);
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::string_view collect_numeric_value() noexcept
       {
        assert(i<siz);
        const std::size_t i_start = i;
        while( i<siz && is_numeric_literal(buf[i]) ) ++i;
        return std::string_view(buf+i_start, i-i_start);
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::string_view collect_digits() noexcept
       {
        assert(i<siz);
        const std::size_t i_start = i;
        while( i<siz && std::isdigit(buf[i]) ) ++i;
        return std::string_view(buf+i_start, i-i_start);
       }


    //-----------------------------------------------------------------------
    // Read a (base10) positive integer literal
    [[nodiscard]] std::size_t extract_index()
       {
        if( i>=siz )
           {
            throw create_parse_error("Index not found");
           }

        if( buf[i]=='+' )
           {
            ++i;
            if( i>=siz )
               {
                throw create_parse_error("Invalid index \'+\'");
               }
           }
        else if( buf[i]=='-' )
           {
            throw create_parse_error("Index can\'t be negative");
           }

        if( !std::isdigit(buf[i]) )
           {
            throw create_parse_error(fmt::format("Invalid char \'{}\' in index", buf[i]));
           }

        std::size_t result = (buf[i]-'0');
        const std::size_t base = 10u;
        while( ++i<siz && std::isdigit(buf[i]) )
           {
            result = (base*result) + (buf[i]-'0');
           }
        return result;
       }


    //-----------------------------------------------------------------------
    // Read a (base10) integer literal
    [[nodiscard]] int extract_integer()
       {
        if( i>=siz )
           {
            throw create_parse_error("No integer found");
           }
        int sign = 1;
        if( buf[i]=='+' )
           {
            //sign = 1;
            ++i;
            if( i>=siz )
               {
                throw create_parse_error("Invalid integer \'+\'");
               }
           }
        else if( buf[i]=='-' )
           {
            sign = -1;
            ++i;
            if( i>=siz )
               {
                throw create_parse_error("Invalid integer \'-\'");
               }
           }
        if( !std::isdigit(buf[i]) )
           {
            throw create_parse_error(fmt::format("Invalid char \'{}\' in integer", buf[i]));
           }
        int result = (buf[i]-'0');
        const int base = 10;
        while( ++i<siz && std::isdigit(buf[i]) )
           {
            result = (base*result) + (buf[i]-'0');
           }
        return sign * result;
       }


    //-----------------------------------------------------------------------
    //[[nodiscard]] std::string_view collect_until_char(const char ch)
    //   {
    //    const std::size_t i_start = i;
    //    while( i<siz )
    //       {
    //        if( buf[i]==ch )
    //           {
    //            //DLOG1("    [*] Collected \"{}\" at line {}\n", std::string_view(buf+i_start, i-i_start), line)
    //            return std::string_view(buf+i_start, i-i_start);
    //           }
    //        else if( buf[i]=='\n' ) ++line;
    //        ++i;
    //       }
    //    throw create_parse_error(fmt::format("Unclosed content (\'{}\' expected)", str::escape(ch)));
    //   }


    //-----------------------------------------------------------------------
    //[[nodiscard]] std::string_view collect_until_char_same_line(const char ch)
    //   {
    //    const std::size_t i_start = i;
    //    while( i<siz )
    //       {
    //        if( buf[i]==ch )
    //           {
    //            //DLOG1("    [*] Collected \"{}\" at line {}\n", std::string_view(buf+i_start, i-i_start), line)
    //            return std::string_view(buf+i_start, i-i_start);
    //           }
    //        else if( buf[i]=='\n' ) break;
    //        ++i;
    //       }
    //    throw create_parse_error(fmt::format("Unclosed content (\'{}\' expected)", str::escape(ch)));
    //   }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::string_view collect_until_char_trimmed(const char ch)
       {
        const std::size_t line_start = line; // Store current line...
        const std::size_t i_start = i;       // ...and position
        std::size_t i_end = i_start; // Index past last char not blank
        while( i<siz )
           {
            if( buf[i]==ch )
               {
                //++i; // Nah, do not eat ch
                return std::string_view(buf+i_start, i_end-i_start);
               }
            else if( buf[i]=='\n' )
               {
                ++line;
                ++i;
               }
            else
               {
                if( !is_blank(buf[i]) ) i_end = ++i;
                else ++i;
               }
           }
        throw create_parse_error(fmt::format("Unclosed content (\'{}\' expected)", str::escape(ch)), line_start, i_start);
       }


    //-----------------------------------------------------------------------
    //[[nodiscard]] std::string_view collect_until_token(const std::string_view tok)
    //   {
    //    const std::size_t i_start = i;
    //    const std::size_t max_siz = siz-tok.length();
    //    while( i<max_siz )
    //       {
    //        if( buf[i]==tok[0] && eat_token(tok) )
    //           {
    //            return std::string_view(buf+i_start, i-i_start-tok.length());
    //           }
    //        else if( buf[i]=='\n' ) ++line;
    //        ++i;
    //       }
    //    throw create_parse_error(fmt::format("Unclosed content (\"{}\" expected)",tok), line_start, i_start);
    //   }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::string_view collect_until_newline_token(const std::string_view tok)
       {
        const std::size_t line_start = line;
        const std::size_t i_start = i;
        while( i<siz )
           {
            if( buf[i]=='\n' )
               {
                ++i;
                ++line;
                skip_blanks();
                if( eat_token(tok) )
                   {
                    return std::string_view(buf+i_start, i-i_start-tok.length());
                   }
               }
            else ++i;
           }
        throw create_parse_error(fmt::format("Unclosed content (\"{}\" expected)",tok), line_start, i_start);
       }


    //-----------------------------------------------------------------------
    //[[nodiscard]] bool eat_character(const char ch) noexcept
    //   {
    //    if( i<siz && buf[i]==ch )
    //       {
    //        ++i;
    //        return true;
    //       }
    //    return false;
    //   }


    //-----------------------------------------------------------------------
    //[[nodiscard]] bool eat_characters(const char c1, const char c2) noexcept
    //   {
    //    if( i<(siz-1) && buf[i]==c1 && buf[i+1]==c2 )
    //       {
    //        i += 2; // Skip them
    //        return true;
    //       }
    //    return false;
    //   }


    //-----------------------------------------------------------------------
    //void skip_until_characters(const char c1, const char c2)
    //   {
    //    const std::size_t line_start = line; // Store current line
    //    const std::size_t i_start = i; // Store current position
    //    while( i<i_last )
    //       {
    //        if( buf[i]==c1 && buf[i+1]==c2 )
    //           {
    //            i += 2; // Skip them
    //            return;
    //           }
    //        else if( buf[i]=='\n' )
    //           {
    //            ++line;
    //           }
    //        ++i;
    //       }
    //    throw create_parse_error(fmt::format("Unclosed block (\"{}{}\" expected)",c1,c2), line_start, i_start);
    //   }

 private:

    //-----------------------------------------------------------------------
    void check_and_skip_bom()
       {//      +--------------+-------------+-------+
        //      |  Encoding    |   Bytes     | Chars |
        //      |--------------|-------------|-------|
        //      | UTF-8        | EF BB BF    | ï»¿   |
        //      | UTF-16 (BE)  | FE FF       | þÿ    |
        //      | UTF-16 (LE)  | FF FE       | ÿþ    |
        //      | UTF-32 (BE)  | 00 00 FE FF | ..þÿ  |
        //      | UTF-32 (LE)  | FF FE 00 00 | ÿþ..  |
        //      +--------------+-------------+-------+
        // I'll accept just UTF-8 or any other 8-bit encodings
        if( siz>=4 && ( (buf[0]=='\xFF' && buf[1]=='\xFE' && buf[2]=='\x00' && buf[3]=='\x00') ||
                        (buf[0]=='\x00' && buf[1]=='\x00' && buf[2]=='\xFE' && buf[3]=='\xFF') ) )
           {
            i = 4;
            throw std::runtime_error("UTF-32 not supported, convert to UTF-8");
           }
        else if( siz>=2 && ( (buf[0]=='\xFF' && buf[1]=='\xFE') ||
                             (buf[0]=='\xFE' && buf[1]=='\xFF') ) )
           {
            i = 2;
            throw std::runtime_error("UTF-16 not supported, convert to UTF-8");
           }
        else if( siz>=3 && buf[0]=='\xEF' && buf[1]=='\xBB' && buf[2]=='\xBF' )
           {
            i = 3;
           }
       }
};

//#undef notify_error


//---- end unit -------------------------------------------------------------
#endif
