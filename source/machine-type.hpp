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

  #if !defined(__cpp_lib_to_underlying)
    template<typename E> constexpr auto to_underlying(const E e) noexcept
       {
        return static_cast<std::underlying_type_t<E>>(e);
       }
  #else
    using std::to_underlying;
  #endif

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace macotec //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

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

    static constexpr std::array<std::string_view, to_underlying(type::size)>
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

    static constexpr std::array<std::string_view, to_underlying(type::size)>
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
    enum class cutbridgedim : std::uint8_t
       {
        undefined=0,
        c37, // ⎫ StratoS
        c46, // ⎭
        c40, // ⎫
        c49, // ⎬ StratoW
        c60, // ⎭
        size
       };

    static constexpr std::array<std::string_view, to_underlying(cutbridgedim::size)>
    cutbridgedim_ids =
       {
        ""sv,    // undefined
        "3.7"sv, // c37
        "4.6"sv, // c46
        "4.0"sv, // c40
        "4.9"sv, // c49
        "6.0"sv  // c60
       };

    // Strato machines align recognized sizes
    enum class aligndim : std::uint8_t
       {
        undefined=0,
        a32, // StratoS/W
        a46, // StratoW
        size
       };

    static constexpr std::array<std::string_view, to_underlying(aligndim::size)>
    aligndim_ids =
       {
        ""sv,    // undefined
        "3.2"sv, // a32
        "4.6"sv  // a46
       };

 public:
    MachineType() noexcept = default;
    explicit MachineType(const std::string_view s) { *this = recognize_machine(s); }

    void assign(const std::string_view s) { *this = recognize_machine(s); }

    [[nodiscard]] operator bool() const noexcept { return i_type != type::undefined; }
    //[[nodiscard]] constexpr bool is_undefined() const noexcept { return i_type == type::undefined; }
    [[nodiscard]] constexpr bool is_float() const noexcept { return i_type>type::undefined && i_type<type::s; }
    [[nodiscard]] constexpr bool is_strato() const noexcept { return i_type>type::undefined && i_type>=type::s; }
    [[nodiscard]] constexpr bool is_strato_s() const noexcept { return i_type==type::s; }
    [[nodiscard]] constexpr std::string_view mach_id() const noexcept { return mach_ids[to_underlying(i_type)]; }
    [[nodiscard]] constexpr std::string_view mach_name() const noexcept { return mach_names[to_underlying(i_type)]; }

    [[nodiscard]] constexpr bool has_cutbridge_dim() const noexcept { return i_cutbridgedim!=cutbridgedim::undefined; }
    [[nodiscard]] constexpr bool has_align_dim() const noexcept { return i_algndim!=aligndim::undefined; }

    [[nodiscard]] constexpr std::string_view cutbridge_dim() const noexcept { return cutbridgedim_ids[to_underlying(i_cutbridgedim)]; }
    [[nodiscard]] constexpr std::string_view align_dim() const noexcept { return aligndim_ids[to_underlying(i_algndim)]; }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::string string() const
       {
        std::string s{ mach_name() };
        if( is_strato() )
           {
            if( has_cutbridge_dim() )
               {
                s += '-';
                s += cutbridge_dim();
                if( has_align_dim() )
                   {
                    s += '/';
                    s += align_dim();
                   }
               }
           }
        return s;
       }

 private:
    type i_type = type::undefined;
    cutbridgedim i_cutbridgedim = cutbridgedim::undefined;
    aligndim i_algndim = aligndim::undefined;

    static MachineType recognize_machine(const std::string_view s)
       {// From strings like "StratoWR-4.9/4.6"
        // La stringa identificativa è composta da un prefisso
        // che determinano la tipologia di macchina, seguito da
        // una o più dimensioni.
        MachineType mach;

        // Machine type
        std::size_t i = 0;
        while( i<s.size() && !std::isalpha(s[i]) ) ++i;
        std::size_t i_start = i;
        while( i<s.size() && !std::ispunct(s[i]) ) ++i;
        mach.i_type = recognize_machine_type( std::string_view(s.data()+i_start, i-i_start) );

        // Possible given first dimension (cut bridge in case of strato machines)
        while( i<s.size() && !std::isdigit(s[i]) ) ++i;
        if( i<s.size() )
           {
            const double dim = extract_num(s, i);
            if( mach.is_strato() )
               {
                mach.i_cutbridgedim = recognize_strato_cutbridge_dim( mach, dim );
               }
           }

        // Possible given second dimension (align max in case of strato machines)
        while( i<s.size() && !std::isdigit(s[i]) ) ++i;
        if( i<s.size() )
           {
            const double dim = extract_num(s, i);
            if( mach.is_strato() )
               {
                mach.i_algndim = recognize_strato_align_dim( dim );
               }
           }

        // If here, all ok
        return mach;
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] static type recognize_machine_type(const std::string_view s)
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
    [[nodiscard]] static cutbridgedim recognize_strato_cutbridge_dim(const MachineType& mach, const double dim)
       {
        // Le misure riconosciute sono: 4.0, 4.9, 6.0 (W)
        //                              3.7, 4.6 (S)
        auto matches = [dim](const double other_dim) noexcept -> bool
           {
            return std::fabs(dim-other_dim) < 0.2;
           };

        if( mach.is_strato_s() )
           {// Macchine piccole
            if( matches(3.7) ) return cutbridgedim::c37;
            else if( matches(4.6) ) return cutbridgedim::c46;
           }
        else
           {// Tutte le altre
            if( matches(4.0) ) return cutbridgedim::c40;
            else if( matches(4.9) ) return cutbridgedim::c49;
            else if( matches(6.0) ) return cutbridgedim::c60;
           }
        throw std::runtime_error( fmt::format("Unrecognized {} cut bridge size: {}", mach.mach_name(), dim) );
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] static aligndim recognize_strato_align_dim(const double dim)
       {
        // Le misure riconosciute sono: 3.2, 4.6 (W)
        //                              3.2 (S)
        auto matches = [dim](const double other_dim) noexcept -> bool
           {
            return std::fabs(dim-other_dim) < 0.2;
           };

        if( matches(3.2) ) return aligndim::a32;
        else if( matches(4.6) ) return aligndim::a46;
        throw std::runtime_error( fmt::format("Unrecognized align size: {}",dim) );
       }

    //-----------------------------------------------------------------------
    // Extract a simple (base10, no sign, no exponent) floating point number
    [[nodiscard]] static double extract_num(const std::string_view s, std::size_t& i) noexcept
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


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
