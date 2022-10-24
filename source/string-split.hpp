#ifndef GUARD_string_split_hpp
#define GUARD_string_split_hpp
//  ---------------------------------------------
//  Get comma separated elements in a string
//  ---------------------------------------------
#include <cassert> // assert
#include <cctype> // std::isspace
#include <string>
#include <string_view>


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace str //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
class Splitter
{
 public:
    Splitter(const std::string_view sv, const char sep) noexcept
      : buf(sv)
      , delim(sep)
      {}

    [[nodiscard]] bool has_data() const noexcept { return i<buf.size(); }
    [[nodiscard]] std::string_view buffer() const noexcept { return buf; }

    //-----------------------------------------------------------------------
    // Trimming spaces
    [[nodiscard]] std::string_view get_next() noexcept
       {
        while( i<buf.size() && std::isspace(buf[i]) ) ++i; // Trim initial spaces

        // Intercept the edge case already at buffer end:
        if( i>=buf.size() )
           {
            return std::string_view(buf.data() + (buf.size()>0 ? buf.size()-1 : 0), 0);
           }
        // Intercept the edge case of a sudden delimiter (empty string):
        else if( buf[i]==delim )
           {
            ++i; // Skip delimiter
            return std::string_view(buf.data()+i-1, 0);
           }

        const std::size_t i_start = i;
        std::size_t i_last_not_blank = i;
        do {
            if( buf[i]==delim )
               {
                ++i; // Skip delimiter
                break;
               }
            else if( !std::isspace(buf[i]) )
               {
                i_last_not_blank = i; // To trim final spaces
               }
            ++i;
           }
        while( i<buf.size() );

        return std::string_view(buf.data()+i_start, i_last_not_blank-i_start+1);
       }

 private:
    const std::string_view buf;
    const char delim;
    std::size_t i = 0; // Current position
};



} //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



/////////////////////////////////////////////////////////////////////////////
#ifdef TEST /////////////////////////////////////////////////////////////////
#include <vector>
#include <fmt/core.h> // fmt::*
#include <fmt/color.h> // fmt::color::*
#include <fmt/ranges.h>
/////////////////////////////////////////////////////////////////////////////
class test_Splitter
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

    void parse(const std::string_view sv, std::vector<std::string_view>&& v_expected)
       {
        str::Splitter splitter(sv,',');
        std::vector<std::string_view> v;
        while( splitter.has_data() ) v.push_back( splitter.get_next() );
        expect(v==v_expected, fmt::format("Parsing of {}: {}",splitter.buffer(),v));
       };

    [[nodiscard]] int errors_count() const noexcept { return m_errors_count; }

 private:
    int m_errors_count = 0;
};
//---------------------------------------------------------------------------
int main()
{
    test_Splitter test;

    test.parse("", {});
    test.parse(",", {""});
    test.parse(", ", {"",""});
    test.parse("aaa, bb b, ccc", {"aaa","bb b","ccc"});
    test.parse("  aaa  ,  bb b ,  ccc  ", {"aaa","bb b","ccc"});
    test.parse(", bb b, ccc ,", {"","bb b","ccc"});
    test.parse(", bb b, ccc , ", {"","bb b","ccc",""});

    return test.errors_count();
}
#endif //////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////



//---- end unit --------------------------------------------------------------
#endif
