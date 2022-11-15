#ifndef GUARD_string_utilities_hpp
#define GUARD_string_utilities_hpp
//  ---------------------------------------------
//  String utilities
//  ---------------------------------------------
#include <cassert> // assert
#include <concepts> // std::convertible_to
//#include <algorithm> // std::find_if
#include <cctype> // std::isdigit, std::tolower, ...
//#include <limits> // std::numeric_limits
#include <string>
#include <string_view>
#include <charconv> // std::from_chars
#include <optional> // std::optional
#include <fmt/core.h> // fmt::format

using namespace std::literals; // "..."sv



//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace str //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
[[nodiscard]] constexpr std::string tolower(std::string s) noexcept
{
    for(char& c : s) c = static_cast<char>(std::tolower(c));
    //s |= action::transform([](unsigned char c){ return std::tolower(c); }); // With c++20 ranges
    return s;
}

//---------------------------------------------------------------------------
[[nodiscard]] constexpr std::string tolower(const std::string_view sv)
{
    std::string s{sv};
    for(char& c : s) c = static_cast<char>(std::tolower(c));
    return s;
}

//---------------------------------------------------------------------------
// trim from start (in place)
//constexpr void ltrim(std::string &s)
//{
//    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](const unsigned char ch){return !std::isspace(ch);}));
//}

//---------------------------------------------------------------------------
// trim from end (in place)
//constexpr void rtrim(std::string &s)
//{
//    s.erase(std::find_if(s.rbegin(), s.rend(), [](const unsigned char ch){return !std::isspace(ch);}).base(), s.end());
//    //while( !s.empty() && std::isspace(static_cast<unsigned char>(s.back())) ) s.pop_back();
//}

//---------------------------------------------------------------------------
//[[nodiscard]] constexpr std::string lowercase_trimmed_right(const std::string_view sv)
//{
//    std::string s;
//    if( sv.size()>0 )
//       {
//        std::size_t i = sv.size()-1;
//        // Trim right
//        while( std::isspace(static_cast<unsigned char>(sv[i])) )
//           {
//            if(i>0) [[likely]] --i;
//            else [[unlikely]] return s;
//           }
//        const auto i_last_not_space = i;
//        // Lowercase
//        s = sv.substr(0, i_last_not_space+1);
//        for(char& c : s) c = static_cast<char>(std::tolower(c));
//       }
//    return s;
//}


//---------------------------------------------------------------------------
// Put a string between double quotes
[[nodiscard]] /*constexpr*/ std::string quoted(const std::string& s)
{
    if( s.contains('\"') )
       {// TODO: Should escape internal double quotes
        throw std::runtime_error( fmt::format("Won't quote {}: contains double quotes"sv,s) );
       }
    return fmt::format("\"{}\""sv,s);
}


//---------------------------------------------------------------------------
// Remove double quotes
[[nodiscard]] constexpr std::string_view unquoted(const std::string_view s) noexcept
{
    auto i_first = s.cbegin();
    while(i_first!=s.cend() && std::isspace(*i_first)) ++i_first;
    if( i_first!=s.cend() && *i_first=='\"' )
       {
        auto i_last = s.cend()-1;
        while(i_last>i_first && std::isspace(*i_last)) --i_last;
        if( i_last>i_first && *i_last=='\"' )
           {
            return std::string_view(++i_first,i_last);
           }
       }
    return s;
}


//---------------------------------------------------------------------------
//void print_strings(std::convertible_to<std::string_view> auto&& ...s)
//{
//    for(const auto sv : std::initializer_list<std::string_view>{ s... }) std::cout << sv << '\n';
//}


//---------------------------------------------------------------------------
template<std::convertible_to<std::string_view>... Args>
[[nodiscard]] constexpr std::string concat(Args&&... args, const char delim)
{
    //const auto args_array = std::array<std::common_type_t<std::decay_t<Args>...>, sizeof...(Args)>{{ std::forward<Args>(args)... }};
    //return fmt::format("{}", fmt::join(args_array,delim));
    std::string s;
    const std::size_t totsiz = sizeof...(args) + (std::size(args) + ...);
    s.reserve(totsiz);
    ((s+=args, s+=delim), ...);
    if(!s.empty()) s.resize(s.size()-1);
    return s;
}


//---------------------------------------------------------------------------
// See also std::filesystem::path::replace_extension
//[[nodiscard]] constexpr std::string replace_extension( const std::string& pth, std::string_view newext ) noexcept
//{
//    const std::string::size_type i_extpos = pth.rfind('.'); // std::string::npos
//    return pth.substr(0,i_extpos).append(newext);
//}



//---------------------------------------------------------------------------
//[[nodiscard]] constexpr std::string repeat(const std::string& input, size_t num)
//{
//    std::ostringstream os;
//    std::fill_n(std::ostream_iterator<std::string>(os), num, input);
//    return os.str();
//}


//---------------------------------------------------------------------------
// Convert a character to string, escaping it if necessary
[[nodiscard]] constexpr std::string escape(const char c) noexcept
   {
    std::string s(1,c);
    switch( c )
       {
        case '\n': s = "\\n"; break;
        case '\r': s = "\\r"; break;
        case '\t': s = "\\t"; break;
        //case '\f': s = "\\f"; break;
        //case '\v': s = "\\v"; break;
        case '\0': s = "NULL"; break;
       }
    return s;
   }


//---------------------------------------------------------------------------
// Show string special characters
[[nodiscard]] constexpr std::string escape(std::string&& s) noexcept
   {
    std::string::size_type i = 0;
    while( i<s.size() )
       {
        switch( s[i] )
           {
            case '\n': s.replace(i, 2, "\\n"); i+=2; break;
            case '\r': s.replace(i, 2, "\\r"); i+=2; break;
            case '\t': s.replace(i, 2, "\\t"); i+=2; break;
            //case '\f': s.replace(i, 2, "\\f"); i+=2; break;
            //case '\v': s.replace(i, 2, "\\v"); i+=2; break;
            //case '\0': s.replace(i, 2, "\\0"); i+=2; break;
            default : ++i;
           }
       }
    return s;
   }


//---------------------------------------------------------------------------
// Show string special characters
[[nodiscard]] constexpr std::string escape(const std::string_view sv) noexcept
   {
    return escape( std::string(sv) );
   }


//---------------------------------------------------------------------------
// Replace all occurrences in a string
constexpr void replace_all(std::string& s, const std::string& from, const std::string& to)
{
    //std::string::size_type i = 0;
    //while( (i = s.find(from, i)) != std::string::npos )
    //   {
    //    s.replace(i, from.length(), to);
    //    i += to.length();
    //   }

    std::string sout;
    sout.reserve(s.length());
    std::string::size_type i, i_start=0;
    while( (i = s.find(from, i_start)) != std::string::npos )
       {
        sout.append(s, i_start, i-i_start);
        sout.append(to);
        i_start = i + from.length();
       }
    sout.append(s, i_start); //, std::string::npos);
    s.swap(sout);
}


//---------------------------------------------------------------------------
// Convert a string_view to number
template<typename T> [[nodiscard]] constexpr T to_num(const std::string_view s)
{
    T result;
    const auto i_end = s.data() + s.size();
    const auto [i, ec] = std::from_chars(s.data(), i_end, result);
    //if(ec == std::errc::runtime_error || ec == std::errc::result_out_of_range)
    if( ec!=std::errc() || i!=i_end ) throw std::runtime_error(fmt::format("\"{}\" is not a valid number",s));
    return result;
}

//---------------------------------------------------------------------------
// Try to convert a string_view to number
template<typename T> [[nodiscard]] constexpr std::optional<T> as_num(const std::string_view s) noexcept
   {
    T result;
    const auto i_end = s.data() + s.size();
    const auto [i, ec] = std::from_chars(s.data(), i_end, result);
    if( ec!=std::errc() || i!=i_end ) return std::nullopt;
    return result;
   }


//---------------------------------------------------------------------------
// Generate an hash for a string
[[nodiscard]] constexpr std::size_t hash(const std::string_view s)
{
    std::size_t val = 0;
    //for(const char c : s) crc += static_cast<std::size_t>(c);
    for(std::size_t i=0; i<s.length(); ++i) val += (s.length() - i) * static_cast<std::size_t>(s[i]);
    return val;
}



//---------------------------------------------------------------------------
[[nodiscard]] constexpr bool contains_wildcards(const std::string& s) noexcept
{
    //return std::regex_search(s, std::regex("*?")); // Seems slow
    //return s.rfind('*') != std::string::npos || s.rfind('?') != std::string::npos;
    // Two loops? I'd rather loop once
    auto i = s.length();
    while( i>0 )
       {
        const char c = s[--i];
        if( c=='*' || c=='?' ) return true;
       }
    return false;
}


//-----------------------------------------------------------------------
// Returns true if text matches glob-like pattern with wildcards (*, ?)
[[nodiscard]] constexpr bool glob_match(const char* text, const char* glob, const char dont_match ='/')
{
    // 'dont_match': character not matched by any wildcards
    const char *text_backup = nullptr;
    const char *glob_backup = nullptr;
    while( *text!='\0' )
       {
        if( *glob=='*' )
           {// new '*'-loop: backup positions in pattern and text
            text_backup = text;
            glob_backup = ++glob;
           }
        else if( *glob==*text || (*glob=='?' && *text!=dont_match) )
           {// Character matched
            ++text;
            ++glob;
           }
        else if( !glob_backup || (text_backup && *text_backup==dont_match) )
           {// No match
            return false;
           }
        else
           {// '*'-loop: backtrack after the last '*'
            if(text_backup) text = ++text_backup;
            assert(glob_backup!=nullptr);
            glob = glob_backup;
           }
       }
    // Ignore trailing stars
    while(*glob=='*') ++glob;
    // At end of text means success if nothing else is left to match
    return *glob=='\0';
}


//---------------------------------------------------------------------------
[[nodiscard]] constexpr std::string iso_latin1_to_utf8(const std::string_view ansi)
{
    std::string utf8;
    utf8.reserve( (3 * ansi.size()) / 2 );
    for( std::size_t i=0; i<ansi.size(); ++i )
       {
        if( !(ansi[i] & '\x80') ) // ansi[i] < 128
           {
            utf8 += ansi[i];
           }
        else
           {
            utf8 += '\xC0' | (ansi[i] >> 6);
            utf8 += '\x80' | (ansi[i] & '\x3f');
           }
       }
    return utf8;
}


//---------------------------------------------------------------------------
// Split to lines
//[[nodiscard]] constexpr std::vector<std::string_view> split_lines(const std::string_view buf)
//{
//    std::vector<std::string_view> lines;
//    lines.reserve(buf.size() / 40);
//    std::size_t i=0, i_start=0;
//    while( i<buf.size() )
//       {
//        if( buf[i] == '\n' )
//           {
//            lines.emplace_back(buf.data()+i_start, i-i_start);
//            i_start = ++i;
//           }
//        else ++i;
//       }
//    if(i>i_start) lines.emplace_back(buf.data()+i_start, i-i_start);
//    return lines;
//}


/////////////////////////////////////////////////////////////////////////////
// Natural sorting compare functor
// std::map<std::string,std::string,NatSortLess> map;
// std::sort(vector, NatSortLess cmp);
//class NatSortLess final
//{
// public:
//    bool operator()(const char* const a, const char* const b) const { return natcmp(a, b) < 0; }
//    bool operator()(const std::string& a, const std::string& b) const { return natcmp(a.c_str(), b.c_str()) < 0; }
//    //bool operator()(const std::string_view a, const std::string_view b) const { return natcmp(a, b) < 0; }
//
// private:
//    //-----------------------------------------------------------------------
//    // Natural compare function
//    static int natcmp(const char* const a, const char* const b, const bool no_case =true)
//       {
//        int ia=0, ib=0;
//        while(true)
//           {
//            // Skip leading spaces
//            while(std::isspace(a[ia])) ++ia;
//            while(std::isspace(b[ib])) ++ib;
//
//            char ca = a[ia];
//            char cb = b[ib];
//
//            if( std::isdigit(ca) && std::isdigit(cb) )
//               {// Compare numbers
//                int result = (ca=='0' || cb=='0') ? cmp_left(a+ia, b+ib) : cmp_right(a+ia, b+ib);
//                if(result!=0) return result;
//               }
//
//            // If both terminated, have the same order
//            if(!ca && !cb) return 0; // could call 'std::strcmp' for further check
//
//            if(no_case)
//               {
//                ca = static_cast<char>(std::toupper(ca));
//                cb = static_cast<char>(std::toupper(cb));
//               }
//
//            // Check sorting
//            if(ca<cb) return -1;
//            else if(ca>cb) return +1;
//
//            // Next chars
//            if(ca) ++ia;
//            if(cb) ++ib;
//           }
//       }
//
//    //-----------------------------------------------------------------------
//    // The longest run of digits wins.
//    // That aside, the greatest value wins, but we can't know that it will
//    // until we've scanned both numbers to know that they have the
//    // same magnitude, so we remember it in BIAS.
//    static inline int cmp_right(const char* a, const char* b)
//       {
//        int bias = 0;
//        while(true)
//           {
//            if(std::isdigit(*a)) { if(!std::isdigit(*b)) return +1; }
//            else return std::isdigit(*b) ? -1 : bias;
//
//            // If here the strings are both digits, so not yet terminated
//            if(*a<*b) { if(!bias) bias = -1; }
//            else if(*a>*b) { if(!bias) bias = +1; }
//            //else if(!*a && !*b) return bias; // this should never be true
//            ++a, ++b;
//           }
//       }
//
//    //-----------------------------------------------------------------------
//    // Compare two left-aligned numbers: the first to have a different value wins
//    static inline int cmp_left(const char* a, const char* b)
//       {
//        while(true)
//           {
//            if(std::isdigit(*a)) { if(!std::isdigit(*b)) return +1; }
//            else return std::isdigit(*b) ? -1 : 0;
//
//            // If here the strings are both digits, so not yet terminated
//            if(*a<*b) return -1;
//            else if(*a>*b) return +1;
//            ++a, ++b;
//           }
//       }
//};





/////////////////////////////////////////////////////////////////////////////
// A compile time string
//class static_string final
//{
// public:
//    template<std::size_t N> consteval static_string(const char(&a)[N]) : ptr(a), len(N-1) {}
//    //consteval char operator[](const std::size_t n)
//    //   {
//    //    return n < len ? ptr[n] :
//    //    throw std::out_of_range("");
//    //   }
//    //consteval std::size_t size() { return len; }
//    consteval std::string_view view() const noexcept { return std::string_view(ptr,len); }
//
// private:
//    constinit char* const ptr;
//    constinit std::size_t len;
//};

//template<class Int> constexpr typename std::enable_if<std::is_unsigned<Int>::value, Int>::type make_mask(const unsigned char pattern)
//   {
//    return ((std::numeric_limits<Int>::max() / std::numeric_limits<unsigned char>::max()) * pattern);
//   }

//---------------------------------------------------------------------------
//consteval std::string indent(const std::size_t n)
//{
//    return std::string(n, "\t");
//}



}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
