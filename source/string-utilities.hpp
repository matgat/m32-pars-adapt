#ifndef GUARD_string_utilities_hpp
#define GUARD_string_utilities_hpp
//  ---------------------------------------------
//  String utilities
//  ---------------------------------------------
#include <cassert> // assert
//#include <algorithm> // std::min
#include <cctype> // std::isdigit, std::tolower, ...
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
// Change string to lowercase
inline std::string tolower(std::string s)
{
    for(char& c : s) c = static_cast<char>(std::tolower(c));
    // With c++20 ranges:
    //s |= action::transform([](unsigned char c){ return std::tolower(c); });
    return s;
}


//---------------------------------------------------------------------------
// Put a string between double quotes
inline std::string quoted(const std::string& s)
{
    if( s.contains('\"') )
       {// TODO: Should escape internal double quotes
        throw std::runtime_error( fmt::format("Won't quote {}: contains double quotes"sv,s) );
       }
    return fmt::format("\"{}\""sv,s);
}


//---------------------------------------------------------------------------
// Remove double quotes
inline std::string_view unquoted(const std::string_view s) noexcept
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
// See also std::filesystem::path::replace_extension
//std::string replace_extension( const std::string& pth, std::string_view newext ) noexcept
//{
//    const std::string::size_type i_extpos = pth.rfind('.'); // std::string::npos
//    return pth.substr(0,i_extpos).append(newext);
//}



//---------------------------------------------------------------------------
//std::string repeat(const std::string& input, size_t num)
//{
//    std::ostringstream os;
//    std::fill_n(std::ostream_iterator<std::string>(os), num, input);
//    return os.str();
//}


//---------------------------------------------------------------------------
// Convert a character to string, escaping it if necessary
std::string escape(const char c) noexcept
   {
    std::string s(1,c);
    switch( c )
       {
        case '\n': s = "\\n"; break;
        case '\r': s = "\\r"; break;
        case '\t': s = "\\t"; break;
        //case '\f': s = "\\f"; break;
        //case '\v': s = "\\v"; break;
        //case '\0': s = "\\0"; break;
       }
    return s;
   }


//---------------------------------------------------------------------------
// Show string special characters
std::string escape(std::string&& s) noexcept
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
std::string escape(const std::string_view sv) noexcept
   {
    return escape( std::string(sv) );
   }


//---------------------------------------------------------------------------
// Replace all occurrences in a string
void replace_all(std::string& s, const std::string& from, const std::string& to)
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
template<typename T> T to_num(const std::string_view s)
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
template<typename T> std::optional<T> as_num(const std::string_view s) noexcept
   {
    T result;
    const auto i_end = s.data() + s.size();
    const auto [i, ec] = std::from_chars(s.data(), i_end, result);
    if( ec!=std::errc() || i!=i_end ) return std::nullopt;
    return result;
   }


//---------------------------------------------------------------------------
// Generate an hash for a string
std::size_t hash(const std::string_view s)
{
    std::size_t val = 0;
    //for(const char c : s) crc += static_cast<std::size_t>(c);
    for(std::size_t i=0; i<s.length(); ++i) val += (s.length() - i) * static_cast<std::size_t>(s[i]);
    return val;
}



//---------------------------------------------------------------------------
bool contains_wildcards(const std::string& s) noexcept
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
bool glob_match(const char* text, const char* glob, const char dont_match ='/')
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
std::string iso_latin1_to_utf8(const std::string_view ansi)
{
    std::string utf8;
    utf8.reserve( (3 * ansi.size()) / 2 );
    for( std::size_t i=0; i<ansi.size(); ++i )
       {
        if( ansi[i] < 128 )
           {
            utf8 += ansi[i];
           }
        else
           {
            utf8 += 0xC0 | (ansi[i] >> 6);
            utf8 += 0x80 | (ansi[i] & 0x3f);
           }
       }
    return utf8;
}


//---------------------------------------------------------------------------
// String similarity [0÷1] using The levenshtein distance (edit distance)
//constexpr double calc_similarity1(const std::string_view s1, const std::string_view s2)
//{
//    int getEditDistance(std::string first, std::string second)
//    {
//        int m = s1.length();
//        int n = s2.length();
//
//        int T[m + 1][n + 1];
//        for (int i = 1; i <= m; i++)
//        {
//            T[i][0] = i;
//        }
//
//        for (int j = 1; j <= n; j++)
//        {
//            T[0][j] = j;
//        }
//
//        for (int i = 1; i <= m; i++)
//        {
//            for (int j = 1; j <= n; j++)
//            {
//                int weight = s1[i - 1] == s2[j - 1] ? 0: 1;
//                T[i][j] = std::min(std::min(T[i-1][j] + 1, T[i][j-1] + 1), T[i-1][j-1] + weight);
//            }
//        }
//
//        return T[m][n];
//    }
//
//
//
//    // Normalize
//    const double max_len = std::max(s1.length(), s2.length());
//    if(max_len)
//    if (max_length > 0) {
//        return (max_length - getEditDistance(first, second)) / max_length;
//    }
//    return 1.0;
//}



// Similarity based in Sørensen–Dice index.
//double dice_match(const char *string1, const char *string2) {
//
//    //check fast cases
//    if (((string1 != NULL) && (string1[0] == '\0')) ||
//        ((string2 != NULL) && (string2[0] == '\0'))) {
//        return 0;
//    }
//    if (string1 == string2) {
//        return 1;
//    }
//
//    size_t strlen1 = strlen(string1);
//    size_t strlen2 = strlen(string2);
//    if (strlen1 < 2 || strlen2 < 2) {
//        return 0;
//    }
//
//    size_t length1 = strlen1 - 1;
//    size_t length2 = strlen2 - 1;
//
//    double matches = 0;
//    int i = 0, j = 0;
//
//    //get bigrams and compare
//    while (i < length1 && j < length2) {
//        char a[3] = {string1[i], string1[i + 1], '\0'};
//        char b[3] = {string2[j], string2[j + 1], '\0'};
//        int cmp = strcmpi(a, b);
//        if (cmp == 0) {
//            matches += 2;
//        }
//        i++;
//        j++;
//    }
//
//    return matches / (length1 + length2);
//}

//double similarity_sorensen_dice(const std::string& _str1, const std::string& _str2) {
//    // Base case: if some string is empty.
//    if (_str1.empty() || _str2.empty()) {
//        return 1.0;
//    }
//
//    auto str1 = upper_string(_str1);
//    auto str2 = upper_string(_str2);
//
//    // Base case: if the strings are equals.
//    if (str1 == str2) {
//        return 0.0;
//    }
//
//    // Base case: if some string does not have bigrams.
//    if (str1.size() < 2 || str2.size() < 2) {
//        return 1.0;
//    }
//
//    // Extract bigrams from str1
//    auto num_pairs1 = str1.size() - 1;
//    std::unordered_set<std::string> str1_bigrams;
//    str1_bigrams.reserve(num_pairs1);
//    for (unsigned i = 0; i < num_pairs1; ++i) {
//        str1_bigrams.insert(str1.substr(i, 2));
//    }
//
//    // Extract bigrams from str2
//    auto num_pairs2 = str2.size() - 1;
//    std::unordered_set<std::string> str2_bigrams;
//    str2_bigrams.reserve(num_pairs2);
//    for (unsigned int i = 0; i < num_pairs2; ++i) {
//        str2_bigrams.insert(str2.substr(i, 2));
//    }
//
//    // Find the intersection between the two sets.
//    int intersection = 0;
//    if (str1_bigrams.size() < str2_bigrams.size()) {
//        const auto it_e = str2_bigrams.end();
//        for (const auto& bigram : str1_bigrams) {
//            intersection += str2_bigrams.find(bigram) != it_e;
//        }
//    } else {
//        const auto it_e = str1_bigrams.end();
//        for (const auto& bigram : str2_bigrams) {
//            intersection += str1_bigrams.find(bigram) != it_e;
//        }
//    }
//
//     //Returns similarity coefficient.
//    return (2.0 * intersection) / (num_pairs1 + num_pairs2);
//}



//---------------------------------------------------------------------------
// Split to lines
//std::vector<std::string_view> split_lines(const std::string_view buf)
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
