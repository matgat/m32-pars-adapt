#ifndef GUARD_machine_type_hpp
#define GUARD_machine_type_hpp
//  ---------------------------------------------
//  Machine type descriptor
//  ---------------------------------------------
#include <stdexcept>
#include <cstdint> // std::uint8_t
#include <cmath> // std::fabs, ...
#include <string>
#include <string_view>
#include <array>
#include <cctype> // std::isdigit, std::tolower, ...
#include <fmt/core.h> // fmt::format

#include "vectset.hpp" // mat::vectset
#include "string-utilities.hpp" // str::tolower
#include "console-uts.hpp" // sys::choice, sys::prompt_string
#include "string-split.hpp" // str::Splitter

using namespace std::literals; // "..."sv


  #if !defined(__cpp_lib_to_underlying)
    template<typename E> [[nodiscard]] constexpr auto to_underlying(const E e) noexcept
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
class MachineFamily final
{
 private:
    enum class family : std::uint8_t
       {
        undefined=0,
        scut,  // StarCut   ⎫
        msfr,  // MasterFR  ⎬ Float families
        msfrv, // MasterFRV ⎭
        actf,  // ActiveF/E    ⎫
        actfr, // ActiveFR/FRS │
        actw,  // ActiveW      ⎬ Strato families
        actwr, // AcriveWR     │
        acthp, // ActiveHP     ⎭
        size
       };

    static constexpr std::array<std::string_view, to_underlying(family::size)>
    mach_ids =
       {
        ""sv, // family::undefined
        "STC"sv,   // family::scut  ⎫
        "MSFR"sv,  // family::msfr  ⎬ Float families
        "MSFRV"sv, // family::msfrv ⎭
        "F"sv,  // family::actf  ⎫
        "FR"sv, // family::actfr │
        "W"sv,  // family::actw  ⎬ Strato families
        "WR"sv, // family::actwr │
        "HP"sv  // family::acthp ⎭
       };

    static constexpr std::array<std::string_view, to_underlying(family::size)>
    mach_names =
       {
        ""sv,          // family::undefined
        "StarCut"sv,   // family::starcut ⎫
        "MasterFR"sv,  // family::msfr    ⎬ Float families
        "MasterFRV"sv, // family::msfrv   ⎭
        "ActiveF"sv,   // family::actf  ⎫
        "ActiveFR"sv,  // family::actfr │
        "ActiveW"sv,   // family::actw  ⎬ Strato families
        "ActiveWR"sv,  // family::actwr │
        "ActiveHP"sv   // family::acthp ⎭
       };

    family i_family = family::undefined;

 public:
    //-----------------------------------------------------------------------
    void assign(const std::string_view sv)
       {
        if( sv.ends_with("hp") )
           {
            i_family = family::acthp;
           }
        else if( sv.ends_with("wr") )
           {
            i_family = family::actwr;
           }
        else if( sv.ends_with("w") )
           {
            i_family = family::actw;
           }
        else if( sv.ends_with("frs") || (sv.ends_with("fr") && !sv.contains('m')) )
           {
            i_family = family::actfr;
           }
        else if( (sv.contains("active") && (sv.ends_with('f') || sv.ends_with('e'))) ||
                 (sv.contains("strato") && sv.ends_with('s')) )
           {
            i_family = family::actf;
           }
        else if( sv.ends_with("frv") )
           {
            i_family = family::msfrv;
           }
        else if( sv.ends_with("fr") && sv.contains('m') )
           {
            i_family = family::msfr;
           }
        else if( sv.contains("starcut") || sv.contains("stc") )
           {
            i_family = family::scut;
           }
        else
           {
            throw std::runtime_error( fmt::format("Unrecognized machine: {}", sv) );
           }
       }

    [[nodiscard]] constexpr bool operator==(const MachineFamily other) const noexcept { return i_family==other.i_family; }

    [[nodiscard]] constexpr bool is_defined() const noexcept { return i_family!=family::undefined; }

    [[nodiscard]] constexpr bool is_float() const noexcept { return i_family>family::undefined && i_family<family::actf; }
    [[nodiscard]] constexpr bool is_strato() const noexcept { return i_family>=family::actf; }
    [[nodiscard]] constexpr bool is_activef() const noexcept { return i_family==family::actf; }
    [[nodiscard]] constexpr bool is_activefr() const noexcept { return i_family==family::actfr; }

    constexpr void set_as_strato_hp()  noexcept { i_family = family::acthp; }
    constexpr void set_as_strato_wr()  noexcept { i_family = family::actwr; }
    constexpr void set_as_strato_w()   noexcept { i_family = family::actw; }
    constexpr void set_as_strato_frs() noexcept { i_family = family::actfr; }
    constexpr void set_as_strato_s()   noexcept { i_family = family::actf; }
    constexpr void set_as_float_frv()  noexcept { i_family = family::msfrv; }
    constexpr void set_as_float_fr()   noexcept { i_family = family::msfr; }
    constexpr void set_as_float_scut() noexcept { i_family = family::scut; }

    [[nodiscard]] constexpr std::string_view id_string() const noexcept { return mach_ids[to_underlying(i_family)]; }
    [[nodiscard]] constexpr std::string_view name() const noexcept { return mach_names[to_underlying(i_family)]; }
};



/////////////////////////////////////////////////////////////////////////////
// Descrittore possibili misure ponte di taglio
class CutBridgeDim final
{
 private:

    enum class cutbridgedim : std::uint8_t
       {// Strato machines cut bridge recognized sizes
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

    cutbridgedim i_dimcat = cutbridgedim::undefined;

 public:
    //-----------------------------------------------------------------------
    void assign(const double dim, const MachineFamily mach_family)
       {
        // Le misure riconosciute sono: 4.0, 4.9, 6.0 (W)
        //                              3.7, 4.6 (S)
        auto matches = [dim](const double other_dim) noexcept -> bool
           {
            return std::fabs(dim-other_dim) < 0.2;
           };

        if( mach_family.is_activef() )
           {
                 if( matches(3.7) ) i_dimcat = cutbridgedim::c37;
            else if( matches(4.6) ) i_dimcat = cutbridgedim::c46;
            else throw std::runtime_error( fmt::format("Unrecognized {} cut bridge size: {}", mach_family.name(), dim) );
           }
        else if( mach_family.is_activefr() )
           {
                 if( matches(4.0) ) i_dimcat = cutbridgedim::c40;
            else if( matches(4.9) ) i_dimcat = cutbridgedim::c49;
            else if( matches(6.0) ) i_dimcat = cutbridgedim::c60;
            else throw std::runtime_error( fmt::format("Unrecognized {} cut bridge size: {}", mach_family.name(), dim) );
           }
        else
           {
                 if( matches(4.0) ) i_dimcat = cutbridgedim::c40;
            else if( matches(4.9) ) i_dimcat = cutbridgedim::c49;
            else if( matches(6.0) ) i_dimcat = cutbridgedim::c60;
            else throw std::runtime_error( fmt::format("Unrecognized {} cut bridge size: {}", mach_family.name(), dim) );
           }
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr bool operator==(const CutBridgeDim other) const noexcept { return i_dimcat==other.i_dimcat; }
    [[nodiscard]] constexpr bool is_defined() const noexcept { return i_dimcat!=cutbridgedim::undefined; }
    [[nodiscard]] constexpr std::string_view string() const noexcept { return cutbridgedim_ids[to_underlying(i_dimcat)]; }
};



/////////////////////////////////////////////////////////////////////////////
// Descrittore possibili misure riscontro
class AlignSpanDim final
{
 private:
    enum class aligndim : std::uint8_t
       {// Strato machines align recognized sizes
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

    aligndim i_dimcat = aligndim::undefined;

 public:
    //-----------------------------------------------------------------------
    void assign(const double dim)
       {
        // Le misure riconosciute sono: 3.2, 4.6 (W)
        //                              3.2 (S)
        auto matches = [dim](const double other_dim) noexcept -> bool
           {
            return std::fabs(dim-other_dim) < 0.2;
           };

             if( matches(3.2) ) i_dimcat = aligndim::a32;
        else if( matches(4.6) ) i_dimcat = aligndim::a46;
        else throw std::runtime_error( fmt::format("Unrecognized align size: {}",dim) );
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr bool operator==(const AlignSpanDim other) const noexcept { return i_dimcat==other.i_dimcat; }
    [[nodiscard]] constexpr bool is_defined() const noexcept { return i_dimcat!=aligndim::undefined; }
    [[nodiscard]] constexpr std::string_view string() const noexcept { return aligndim_ids[to_underlying(i_dimcat)]; }
};



/////////////////////////////////////////////////////////////////////////////
// Descrittore possibili opzioni macchina
class MachineOptions final
{
    enum class bits : std::uint8_t
       {
        opp   = 0x0001, // (bit0) Opposta
        lowe  = 0x0002, // (bit3) Presenza mola basso emissivo
        rot   = 0x0008, // (bit3) Presenza girapezzi
        buf   = 0x0010, // (bit4) Presenza buffer
        nobuf = 0x0020, // (bit5) Assenza buffer
       };

 public:
    [[nodiscard]] constexpr bool contains(const MachineOptions& other) const noexcept
       {
        return (bit_mask | other.bit_mask) == bit_mask // Contains known options
               && unrecognized.contains(other.unrecognized); // Contains also unknown ones
       }

    [[nodiscard]] constexpr bool is_empty() const noexcept { return bit_mask==0 && unrecognized.is_empty(); }
    //void reset() noexcept { bit_mask=0; }

    [[nodiscard]] constexpr bool is_opp() const noexcept { return has_bit(bits::opp); }
    constexpr void set_opp(const bool b) noexcept { set_bit(bits::opp,b); }

    [[nodiscard]] constexpr bool has_lowe() const noexcept { return has_bit(bits::lowe); }
    constexpr void set_lowe(const bool b) noexcept { set_bit(bits::lowe,b); }

    [[nodiscard]] constexpr bool has_rot() const noexcept { return has_bit(bits::rot); }
    constexpr void set_rot(const bool b) noexcept { set_bit(bits::rot,b); }

    [[nodiscard]] constexpr bool has_buf() const noexcept { return has_bit(bits::buf); }
    constexpr void set_buf(const bool b) noexcept { set_bit(bits::buf,b); }

    [[nodiscard]] constexpr bool has_nobuf() const noexcept { return has_bit(bits::nobuf); }
    constexpr void set_nobuf(const bool b) noexcept { set_bit(bits::nobuf,b); }


    [[nodiscard]] constexpr bool has_option(const std::string_view opt) const noexcept
       {
        MachineOptions other;
        other.add_option(opt);
        return contains(other);
       }

    constexpr void add_option(const std::string_view opt) noexcept
       {
        // Try to recognize option...
             if( opt=="opp" )    set_opp(true);
        else if( opt=="lowe" )   set_lowe(true);
        else if( opt=="rot" )    set_rot(true);
        else if( opt=="buf" )    set_buf(true);
        else if( opt=="no-buf" ) set_nobuf(true);
        //...Otherwise just collect it
        else unrecognized.insert( std::string(opt) );
       }

    [[nodiscard]] std::string string(const char sep =',') const
       {
        std::string s;
        if( is_opp() )    { s+="opp";    s+=sep; }
        if( has_lowe() )  { s+="lowe";   s+=sep; }
        if( has_rot() )   { s+="rot";    s+=sep; }
        if( has_buf() )   { s+="buf";    s+=sep; }
        if( has_nobuf() ) { s+="no-buf"; s+=sep; }
        // Also the unrecognized ones
        for( const std::string& opt : unrecognized )
           {
            s+=opt; s+=sep;
           }
        if(s.size()>0) s.resize(s.size()-1);
        return s;
       }

 private:
    std::underlying_type_t<bits> bit_mask = 0;
    mat::vectset<std::string> unrecognized; // Unknown options

    [[nodiscard]] constexpr bool has_bit(const bits bit) const noexcept { return bit_mask & to_underlying(bit); }
    constexpr void set_bit(const bits bit, const bool b) noexcept { if(b)bit_mask|=to_underlying(bit); else bit_mask&=~to_underlying(bit); }
};



/////////////////////////////////////////////////////////////////////////////
class MachineType final
{
 private:
    MachineFamily i_family;
    CutBridgeDim i_cutbridgedim;
    AlignSpanDim i_algndim;
    MachineOptions i_options;

 public:
    void assign(const std::string_view s) { *this = recognize_machine(s); }

    [[nodiscard]] operator bool() const noexcept { return i_family.is_defined(); }
    [[nodiscard]] bool is_incomplete() const noexcept
       {
        return !i_family.is_defined() || (i_family.is_strato() && !has_cutbridge_dim());
       }

    [[nodiscard]] constexpr MachineFamily family() const noexcept { return i_family; }
    [[nodiscard]] constexpr MachineFamily& mutable_family() noexcept { return i_family; }

    [[nodiscard]] constexpr bool has_cutbridge_dim() const noexcept { return i_cutbridgedim.is_defined(); }
    [[nodiscard]] constexpr CutBridgeDim cutbridge_dim() const noexcept { return i_cutbridgedim; }
    [[nodiscard]] constexpr CutBridgeDim& mutable_cutbridge_dim() noexcept { return i_cutbridgedim; }

    [[nodiscard]] constexpr bool has_align_dim() const noexcept { return i_algndim.is_defined(); }
    [[nodiscard]] constexpr AlignSpanDim align_dim() const noexcept { return i_algndim; }
    [[nodiscard]] constexpr AlignSpanDim& mutable_align_dim() noexcept { return i_algndim; }

    [[nodiscard]] constexpr const MachineOptions& options() const noexcept { return i_options; }
    [[nodiscard]] constexpr MachineOptions& mutable_options() noexcept { return i_options; }

    //-----------------------------------------------------------------------
    [[nodiscard]] std::string string() const
       {
        std::string s{ family().name() };
        if( family().is_strato() )
           {
            if( has_cutbridge_dim() )
               {
                s += '-';
                s += cutbridge_dim().string();
                if( has_align_dim() )
                   {
                    s += '/';
                    s += align_dim().string();
                   }
               }
           }
        if( !options().is_empty() )
           {
            s += '-';
            s += '(';
            s += options().string();
            s += ')';
           }
        return s;
       }

    //------------------------------------------------------------------------
    [[nodiscard]] static MachineType recognize_machine(const std::string_view sv)
       {// From strings like "StratoWR-4.9/4.6-(opp,no-buf)"
        //                      type🠉    🠉dims    🠉options
        MachineType mach;

        class parser_t
           {
            public:
                explicit parser_t(const std::string_view s) noexcept : buf(s) {}

                [[nodiscard]] bool has_data() const noexcept { return i<buf.size(); }
                [[nodiscard]] bool is_alpha() const noexcept { return i<buf.size() && std::isalpha(buf[i]); }
                [[nodiscard]] bool is_digit() const noexcept { return i<buf.size() && std::isdigit(buf[i]); }

                void skip_nonalnum() noexcept { while( i<buf.size() && !std::isalnum(buf[i]) ) ++i; }
                void skip_nonalpha() noexcept { while( i<buf.size() && !std::isalpha(buf[i]) ) ++i; }
                void skip_nondigits() noexcept { while( i<buf.size() && !std::isdigit(buf[i]) ) ++i; }

                //-----------------------------------------------------------------------
                // Extract alphabetic identifier
                [[nodiscard]] std::string_view extract_alpha() noexcept
                   {
                    assert( is_alpha() );
                    const std::size_t i_start = i;
                    do { ++i; } while( i<buf.size() && std::isalpha(buf[i]) );
                    return std::string_view(buf.data()+i_start, i-i_start);
                   }

                //-----------------------------------------------------------------------
                // Extract generic identifier (can include -)
                [[nodiscard]] std::string_view extract_option() noexcept
                   {
                    assert( is_alpha() );
                    const std::size_t i_start = i;
                    do { ++i; } while( i<buf.size() && (std::isalpha(buf[i]) || buf[i]=='-') );
                    return std::string_view(buf.data()+i_start, i-i_start);
                   }

                //-----------------------------------------------------------------------
                // Extract a simple (base10, no sign, no exponent) floating point number
                [[nodiscard]] double extract_num() noexcept
                   {
                    assert( is_digit() );

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

        // To be more tolerant, better ignore case
        const std::string s = str::tolower(sv);
        parser_t parser(s);

        // Machine family
        parser.skip_nonalpha();
        if( parser.is_alpha() )
           {
            mach.i_family.assign( parser.extract_alpha() );
           }
        else
           {
            throw std::runtime_error( fmt::format("Invalid machine name: {}",sv) );
           }

        // Possible given first dimension (cut bridge in case of strato machines)
        parser.skip_nonalnum();
        if( parser.is_digit() )
           {
            const double dim1 = parser.extract_num();
            if( mach.family().is_strato() )
               {
                mach.i_cutbridgedim.assign( dim1, mach.i_family );
               }

            // Possible given second dimension (align max in case of strato machines)
            parser.skip_nonalnum();
            if( parser.is_digit() )
               {
                const double dim2 = parser.extract_num();
                if( mach.family().is_strato() )
                   {
                    mach.i_algndim.assign( dim2 );
                   }
               }
           }

        // Get possible options
        while( parser.has_data() )
           {
            parser.skip_nonalpha();
            if( !parser.is_alpha() ) break;
            const std::string_view opt = parser.extract_option();
            mach.i_options.add_option(opt);
           }

        // If here, all ok
        return mach;
       }
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
