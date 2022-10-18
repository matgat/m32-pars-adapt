#ifndef GUARD_string_similarity_hpp
#define GUARD_string_similarity_hpp
//  ---------------------------------------------
//  String utilities
//  ---------------------------------------------
#include <cassert> // assert
#include <type_traits> // std::is_same_v
//#include <algorithm> // std::min, std::max
#include <cctype> // std::tolower, std::isspace
#include <string_view>
#include <vector>

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace str //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
// String similarity [0÷1] based on Sørensen–Dice index
[[nodiscard]] double calc_similarity_sorensen( const std::string_view s1, const std::string_view s2 )
{
    // Check banal cases
    if( s1.empty() || s2.empty() )
       {// Empty string is never similar to another
        return 0.0;
       }
    else if( s1==s2 )
       {// Perfectly equal
        return 1.0;
       }
    else if( s1.length()==1 || s2.length()==1 )
       {// Single (not equal) characters have zero similarity (no bi-grams)
        return 0.0;
       }

    /////////////////////////////////////////////////////////////////////////
    // Represents a pair of adjacent characters
    class charpair_t final
    {
     public:
         charpair_t(const char a, const char b) noexcept : c1(a), c2(b) {}
         [[nodiscard]] bool operator==(const charpair_t& other) const noexcept { return c1==other.c1 && c2==other.c2; }
     private:
        char c1, c2;
    };

    /////////////////////////////////////////////////////////////////////////
    // Collects and access a sequence of adjacent characters (skipping spaces)
    class charpairs_t final
    {
     public:
         explicit charpairs_t(const std::string_view s)
            {
             assert( !s.empty() ); // Sould be already intercepted
             const std::size_t i_last = s.size()-1;
             std::size_t i = 0;
             chpairs.reserve(i_last);
             while( i<i_last )
               {
                // Skipping single-character words (as in the original article http://www.catalysoft.com/articles/StrikeAMatch.html)
                if( std::isspace(s[i]) ) ; // Skip
                else if( std::isspace(s[i+1]) ) ++i; // Skip also next
                else chpairs.emplace_back( static_cast<char>(std::tolower(s[i])),
                                           static_cast<char>(std::tolower(s[i+1])) );

                // Accepting also single-character words (the second is whatever)
                //if( !std::isspace(s[i]) ) chpairs.emplace_back( std::tolower(s[i]), std::tolower(s[i+1]) );
                ++i;
               }
            }

         [[nodiscard]] auto size() const noexcept { return chpairs.size(); }
         [[nodiscard]] auto cbegin() const noexcept { return chpairs.cbegin(); }
         [[nodiscard]] auto cend() const noexcept { return chpairs.cend(); }
         auto erase(std::vector<charpair_t>::const_iterator i) { return chpairs.erase(i); }

     private:
        std::vector<charpair_t> chpairs;
    };

    charpairs_t chpairs1{s1},
                chpairs2{s2};
    const double orig_avg_bigrams_count = 0.5 * static_cast<double>(chpairs1.size() + chpairs2.size());
    std::size_t matching_bigrams_count = 0;
    for( auto ib1=chpairs1.cbegin(); ib1!=chpairs1.cend(); ++ib1 )
       {
        for( auto ib2=chpairs2.cbegin(); ib2!=chpairs2.cend(); )
           {
            if( *ib1==*ib2 )
               {
                ++matching_bigrams_count;
                ib2 = chpairs2.erase(ib2); // Avoid to match the same character pair multiple times
                break;
               }
            else ++ib2;
           }
       }
    return static_cast<double>(matching_bigrams_count) / orig_avg_bigrams_count;
}


//---------------------------------------------------------------------------
// String similarity [0÷1] using The levenshtein distance (edit distance)
//constexpr double calc_similarity_lev(const std::string_view s1, const std::string_view s2)
//{
//    auto get_edit_distance = [](const std::string_view s1, const std::string_view s2) noexcept -> std::size_t
//       {
//        assert(s1.size()<=s2.size());
//        std::vector<std::size_t> d(s1.size() + 1);
//        for(std::size_t i=0; i<=s1.size(); ++i) d[i] = i;
//        for(std::size_t j=0; j<s2.size(); ++j)
//           {
//            std::size_t prev_diagonal = d[0];
//            ++d[0];
//            for(std::size_t i=0; i<s1.size(); ++i)
//               {
//                const std::size_t new_diagonal = std::tolower(s1[i])==std::tolower(s2[j])
//                                                 ? prev_diagonal
//                                                 : std::min(std::min(d[i], d[i+1]), prev_diagonal) + 1;
//                prev_diagonal = d[i+1];
//                d[i+1] = new_diagonal;
//               }
//           }
//        return d[s1.size()];
//       };
//
//    // Normalize
//    const auto max_len = std::max(s1.length(), s2.length());
//    if( max_len > 0 )
//       {
//        const auto dist = s1.size()<s2.size() ? get_edit_distance(s1,s2)
//                                              : get_edit_distance(s2,s1);
//        return (static_cast<double>(max_len) - static_cast<double>(dist)) / static_cast<double>(max_len);
//       }
//    return 1.0;
//}

//---------------------------------------------------------------------------
[[nodiscard]] bool are_similar( const std::string_view s1, const std::string_view s2, const double threshold )
{
    const auto similarity = calc_similarity_sorensen(s1, s2);

    static_assert(std::is_same_v<double, std::decay_t<decltype(similarity)>>); // similarity type
    assert( similarity>=0.0 && similarity<=1.0); // similarity range
    assert( threshold>=0.0 && threshold<=1.0); // threshold range

    if( s2.starts_with("[s] Tempo salita/discesa utensile intestatura") )
    {xxx
        int i = 1;
    }

    return similarity > threshold;
}

} //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

//---- end unit --------------------------------------------------------------
#endif
