#ifndef GUARD_machine_type_hpp
#define GUARD_machine_type_hpp
//  ---------------------------------------------
//  Machine type descriptor
//  ---------------------------------------------
#include <string>
#include <string_view>
#include <array>
#include <cctype> // std::isdigit, std::tolower, ...
#include <cmath> // std::fabs, ...
#include <stdexcept>
#include <fmt/core.h> // fmt::format

using namespace std::literals; // "..."sv


/////////////////////////////////////////////////////////////////////////////
class MachineType final
{
 private:
    enum class type : std::uint8_t
       {
        undefined=0,
        scut,  // StarCut   ⎫
        msfr,  // MasterFR  ⎬ Float families
        msfrv, // MasterFRV ⎭
        s,  // ActiveE/F ⎫
        w,  // ActiveW   ⎬ Strato families
        wr, // AcriveWR  │
        hp, // ActiveHP  ⎭
        size
       };

    static constexpr std::array<std::string_view, std::to_underlying(type::size)>
    mach_ids =
       {
        ""sv,    // type::undefined
        "STC"sv, // type::starcut
        "FR"sv,  // type::msfr
        "FRV"sv, // type::msfrv
        "S"sv,   // type::s
        "W"sv,   // type::w
        "WR"sv,  // type::wr
        "HP"sv   // type::hp
       };

    static constexpr std::array<std::string_view, std::to_underlying(type::size)>
    mach_names =
       {
        ""sv,          // type::undefined
        "StarCut"sv,   // type::starcut
        "MasterFR"sv,  // type::msfr
        "MasterFRV"sv, // type::msfrv
        "ActiveF"sv,   // type::s
        "ActiveW"sv,   // type::w
        "ActiveWR"sv,  // type::wr
        "ActiveHP"sv   // type::hp
       };

    // Strato machines cut bridge recognized sizes
    enum class cutbridgesiz : std::uint8_t
       {
        undefined=0,
        c37, // ⎫ StratoS
        c46, // ⎭
        c40, // ⎫
        c49, // ⎬ StratoW
        c60, // ⎭
        size
       };

    static constexpr std::array<std::string_view, std::to_underlying(cutbridgesiz::size)>
    cutbridgesiz_ids =
       {
        ""sv,    // undefined
        "3.7"sv, // c37
        "4.6"sv, // c46
        "4.0"sv, // c40
        "4.9"sv, // c49
        "6.0"sv  // c60
       };

    // Strato machines align recognized sizes
    enum class alignsiz : std::uint8_t
       {
        undefined=0,
        a32, // StratoS/W
        a46, // StratoW
        size
       };

    static constexpr std::array<std::string_view, std::to_underlying(alignsiz::size)>
    alignsiz_ids =
       {
        ""sv,    // undefined
        "3.2"sv, // a32
        "4.6"sv  // a46
       };

 public:
    explicit MachineType(const std::string_view s) noexcept
       {// From strings like "StratoWR-4.9/4.6"
        // La stringa indentificativa è composta da un prefisso
        // che determinano la tipologia di macchina, seguito da
        // una o più dimensioni.

        // Machine type
        std::size_t i = 0;
        while( i<s.size() && !std::isalpha(s[i]) ) ++i;
        std::size_t i_start = i;
        while( i<s.size() && !std::ispunct(s[i]) ) ++i;
        i_type = recognize_machine_type( std::string_view(s.data()+i_start, i-i_start) );

        // Possible given first dimension (cut bridge in case of strato machines)
        while( i<s.size() && !std::isdigit(s[i]) ) ++i;
        if( i<s.size() )
           {
            const double dim = extract_num(s, i);
            if( is_strato() )
               {
                i_cutbridgesiz = recognize_strato_cutbridge_size( dim );
               }
           }

        // Possible given second dimension (align max in case of strato machines)
        while( i<s.size() && !std::isdigit(s[i]) ) ++i;
        if( i<s.size() )
           {
            const double dim = extract_num(s, i);
            if( is_strato() )
               {
                i_algnsiz = recognize_strato_align_size( dim );
               }
           }
       }

    [[nodiscard]] constexpr bool is_undefined() const noexcept { return i_type == type::undefined; }
    [[nodiscard]] constexpr bool is_float() const noexcept { return i_type>type::undefined && i_type<type::s; }
    [[nodiscard]] constexpr bool is_strato() const noexcept { return i_type>type::undefined && i_type>=type::s; }
    [[nodiscard]] constexpr bool is_strato_s() const noexcept { return i_type==type::s; }
    [[nodiscard]] constexpr std::string_view mach_id() const noexcept { return mach_ids[std::to_underlying(i_type)]; }
    [[nodiscard]] constexpr std::string_view mach_name() const noexcept { return mach_names[std::to_underlying(i_type)]; }

    [[nodiscard]] constexpr bool has_cutbridge_size() const noexcept { return i_cutbridgesiz!=cutbridgesiz::undefined; }
    [[nodiscard]] constexpr bool has_align_size() const noexcept { return i_algnsiz!=alignsiz::undefined; }

    [[nodiscard]] constexpr std::string_view cutbridge_size() const noexcept { return cutbridgesiz_ids[std::to_underlying(i_cutbridgesiz)]; }
    [[nodiscard]] constexpr std::string_view align_size() const noexcept { return alignsiz_ids[std::to_underlying(i_algnsiz)]; }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::string to_str() const
       {
        std::string s{ mach_name() };
        if( is_strato() )
           {
            if( has_cutbridge_size() )
               {
                s += '-';
                s += cutbridge_size();
                if( has_align_size() )
                   {
                    s += '/';
                    s += align_size();
                   }
               }
           }
        return s;
       }

 private:
    type i_type = type::undefined;
    cutbridgesiz i_cutbridgesiz = cutbridgesiz::undefined;
    alignsiz i_algnsiz = alignsiz::undefined;

    //-----------------------------------------------------------------------
    static type recognize_machine_type(const std::string_view s)
       {
             if( s.ends_with("HP") ) return type::hp;
        else if( s.ends_with("WR") ) return type::wr;
        else if( s.ends_with("atoS") || s.ends_with("iveF") || s.ends_with("iveE") ) return type::s;
        else if( s.ends_with("W") ) return type::w;
        else if( s.ends_with("FRV") ) return type::msfrv;
        else if( s.ends_with("FR") ) return type::msfr;
        else if( s.contains("StarCut") || s.contains("STC") ) return type::scut;

        throw std::runtime_error( fmt::format("Unrecognized machine: {}", s) );
       }

    //-----------------------------------------------------------------------
    cutbridgesiz recognize_strato_cutbridge_size(const double dim) const
       {
        // Le misure riconosciute sono: 4.0, 4.9, 6.0 (W)
        //                              3.7, 4.6 (S)
        auto matches = [dim](const double other_dim) noexcept -> bool
           {
            return std::fabs(dim-other_dim) < 0.2;
           };

        if( is_strato_s() )
           {// Macchine piccole
            if( matches(3.7) ) cutbridgesiz::c37;
            else if( matches(4.6) ) cutbridgesiz::c46;
           }
        else
           {// Tutte le altre
            if( matches(4.0) ) return cutbridgesiz::c40;
            else if( matches(4.9) ) return cutbridgesiz::c49;
            else if( matches(6.0) ) return cutbridgesiz::c60;
           }
        throw std::runtime_error( fmt::format("Unrecognized {} align size: {}", mach_name(), dim) );
       }

    //-----------------------------------------------------------------------
    static alignsiz recognize_strato_align_size(const double dim)
       {
        // Le misure riconosciute sono: 3.2, 4.6 (W)
        //                              3.2 (S)
        auto matches = [dim](const double other_dim) noexcept -> bool
           {
            return std::fabs(dim-other_dim) < 0.2;
           };

        if( matches(3.2) ) return alignsiz::a32;
        else if( matches(4.6) ) return alignsiz::a46;
        throw std::runtime_error( fmt::format("Unrecognized align size: {}",dim) );
       }

    //-----------------------------------------------------------------------
    // Extract a simple (base10, no sign, no exponent) floating point number
    static [[nodiscard]] double extract_num(const std::string_view s, std::size_t& i)
       {
        //assert( i<s.size() && std::isdigit(s[i]) );

        // integral part
        double mantissa = 0;
        do {
            mantissa = (10.0 * mantissa) + static_cast<double>(s[i] - '0');
            ++i;
           }
        while( i<s.size() && std::isdigit(s[i]) );

        // fractional part
        if( i<s.size() && s[i]=='.' )
           {
            ++i;
            double k = 0.1; // shift of decimal part
            while( i<s.size() && std::isdigit(s[i]) )
               {
                mantissa += k * static_cast<double>(s[i] - '0');
                k *= 0.1;
                ++i;
               }
           }
        return mantissa;
       }
};



//---- end unit -------------------------------------------------------------
#endif
