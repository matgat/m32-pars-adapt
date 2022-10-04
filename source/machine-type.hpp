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
        c37, // ⎫ S
        c46, // ⎭
        c40, // ⎫
        c49, // ⎬ W/WR/HP
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
        a32, // S/W/WR/HP
        a46, // W/WR/HP
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

    [[nodiscard]] constexpr bool is_opposite() const noexcept { return i_opposite; }


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
        if( is_opposite() )
           {
            s += "-opp";
           }
        return s;
       }

 private:
    type i_type = type::undefined;
    cutbridgedim i_cutbridgedim = cutbridgedim::undefined;
    aligndim i_algndim = aligndim::undefined;
    bool i_opposite = false;

    //------------------------------------------------------------------------
    static MachineType recognize_machine(const std::string_view sv)
       {// From strings like "StratoWR-4.9/4.6"
        // La stringa identificativa è composta da un prefisso
        // che determinano la tipologia di macchina, seguito da
        // una o più dimensioni.
        MachineType mach;

        class parser_t
           {
            public:
                parser_t(const std::string_view s) noexcept : buf(s) {}

                [[nodiscard]] bool has_data() noexcept { return i<buf.size(); }
                void skip_nondigits() noexcept { while( i<buf.size() && !std::isdigit(buf[i]) ) ++i; }

                //-----------------------------------------------------------------------
                // Extract first identifier
                [[nodiscard]] std::string_view extract_first_id() noexcept
                   {
                    while( i<buf.size() && !std::isalpha(buf[i]) ) ++i; // Skip possible initial garbage
                    const std::size_t i_start = i;
                    do { ++i; } while( i<buf.size() && std::isalpha(buf[i]) );
                    return std::string_view(buf.data()+i_start, i-i_start);
                   }

                //-----------------------------------------------------------------------
                // Extract a simple (base10, no sign, no exponent) floating point number
                [[nodiscard]] double extract_num() noexcept
                   {
                    //assert( i<buf.size() && std::isdigit(buf[i]) );

                    // integral part
                    double mantissa = 0;
                    do {
                        mantissa = (10.0 * mantissa) + static_cast<double>(buf[i] - '0');
                        ++i;
                       }
                    while( i<buf.size() && std::isdigit(buf[i]) );

                    // fractional part
                    if( i<buf.size() && buf[i]=='.' )
                       {
                        ++i;
                        double k = 0.1; // shift of decimal part
                        while( i<buf.size() && std::isdigit(buf[i]) )
                           {
                            mantissa += k * static_cast<double>(buf[i] - '0');
                            k *= 0.1;
                            ++i;
                           }
                       }
                    return mantissa;
                   }

            private:
                const std::string_view buf;
                std::size_t i = 0;
           };

        // To be more tolerant, should ignore case and trim right
        auto lowercase_trimmed_right = [](const std::string_view sv) constexpr
           {
            std::string s;
            if( sv.size()>0 )
               {
                std::size_t i = sv.size()-1;
                // Trim right
                while( std::isspace(static_cast<unsigned char>(sv[i])) )
                   {
                    if(i>0) [[likely]] --i;
                    else [[unlikely]] return s;
                   }
                const auto i_last_not_space = i;
                // Lowercase
                s = sv.substr(0, i_last_not_space+1);
                for(char& c : s) c = static_cast<char>(std::tolower(c));
               }
            return s;
           };
        const std::string s = lowercase_trimmed_right(sv);
        parser_t parser(s);

        // Machine type
        mach.i_type = recognize_machine_type( parser.extract_first_id() );

        // Possible given first dimension (cut bridge in case of strato machines)
        parser.skip_nondigits();
        if( parser.has_data() )
           {
            const double dim = parser.extract_num();
            if( mach.is_strato() )
               {
                mach.i_cutbridgedim = recognize_strato_cutbridge_dim( mach, dim );
               }
           }

        // Possible given second dimension (align max in case of strato machines)
        parser.skip_nondigits();
        if( parser.has_data() )
           {
            const double dim = parser.extract_num();
            if( mach.is_strato() )
               {
                mach.i_algndim = recognize_strato_align_dim( dim );
               }
           }

        // Determine if is opposite
        mach.i_opposite = s.ends_with("opp");

        // If here, all ok
        return mach;
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] static type recognize_machine_type(const std::string_view s)
       {
             if( s.ends_with("hp") ) return type::hp;
        else if( s.ends_with("wr") ) return type::wr;
        else if( s.ends_with("atos") || s.ends_with("ivef") || s.ends_with("ivee") ) return type::s;
        else if( s.ends_with("w") ) return type::w;
        else if( s.ends_with("frv") ) return type::msfrv;
        else if( s.ends_with("fr") ) return type::msfr;
        else if( s.contains("starcut") || s.contains("stc") ) return type::scut;

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
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
