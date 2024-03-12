#pragma once
//  ---------------------------------------------
//  Macotec machine data descriptor
//  ---------------------------------------------
//  #include "macotec_machine_data.hpp" // macotec::MachineData
//  ---------------------------------------------
#include <stdexcept>
#include <cstdint> // std::uint8_t
#include <cmath> // std::fabs, ...
#include <string>
#include <string_view>
#include <array>
#include <format>

#include "options_set.hpp" // MG::options_set
#include "string_utilities.hpp" // str::tolower
#include "ascii_simple_lexer.hpp" // ascii::simple_lexer

using namespace std::literals; // "..."sv



//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace macotec //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
class MachineFamily final
{
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

    static constexpr std::array<std::string_view, std::to_underlying(family::size)>
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

    static constexpr std::array<std::string_view, std::to_underlying(family::size)>
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

 private:
    family m_family = family::undefined;

 public:
    [[nodiscard]] constexpr bool operator==(const MachineFamily other) const noexcept { return m_family==other.m_family; }

    void assign(const std::string_view sv)
       {
        if( sv.ends_with("hp") )
           {
            m_family = family::acthp;
           }
        else if( sv.ends_with("wr") )
           {
            m_family = family::actwr;
           }
        else if( sv.ends_with("w") )
           {
            m_family = family::actw;
           }
        else if( sv.contains("act") and (sv.ends_with("frs") or sv.ends_with("fr")) )
           {
            m_family = family::actfr;
           }
        else if( (sv.contains("act") and (sv.ends_with('f') or sv.ends_with('e'))) or
                 (sv.contains("strato") and sv.ends_with('s')) )
           {
            m_family = family::actf;
           }
        else if( sv.ends_with("frv") )
           {
            m_family = family::msfrv;
           }
        else if( sv.ends_with("fr") and sv.contains('m') )
           {
            m_family = family::msfr;
           }
        else if( sv.contains("starcut") or sv.contains("stc") )
           {
            m_family = family::scut;
           }
        else
           {
            throw std::runtime_error( std::format("Unrecognized machine: {}", sv) );
           }
       }

    [[nodiscard]] constexpr bool is_defined() const noexcept { return m_family!=family::undefined; }

    [[nodiscard]] constexpr bool is_float() const noexcept { return m_family>family::undefined and m_family<family::actf; }
    [[nodiscard]] constexpr bool is_strato() const noexcept { return m_family>=family::actf; }

    [[nodiscard]] constexpr bool is_strato_hp()  const noexcept { return m_family == family::acthp; }
    [[nodiscard]] constexpr bool is_strato_wr()  const noexcept { return m_family == family::actwr; }
    [[nodiscard]] constexpr bool is_strato_w()   const noexcept { return m_family == family::actw; }
    [[nodiscard]] constexpr bool is_strato_fr()  const noexcept { return m_family == family::actfr; }
    [[nodiscard]] constexpr bool is_strato_f()   const noexcept { return m_family == family::actf; }
    [[nodiscard]] constexpr bool is_float_frv()  const noexcept { return m_family == family::msfrv; }
    [[nodiscard]] constexpr bool is_float_fr()   const noexcept { return m_family == family::msfr; }
    [[nodiscard]] constexpr bool is_float_scut() const noexcept { return m_family == family::scut; }

    constexpr void set_as_strato_hp()  noexcept { m_family = family::acthp; }
    constexpr void set_as_strato_wr()  noexcept { m_family = family::actwr; }
    constexpr void set_as_strato_w()   noexcept { m_family = family::actw; }
    constexpr void set_as_strato_fr()  noexcept { m_family = family::actfr; }
    constexpr void set_as_strato_s()   noexcept { m_family = family::actf; }
    constexpr void set_as_float_frv()  noexcept { m_family = family::msfrv; }
    constexpr void set_as_float_fr()   noexcept { m_family = family::msfr; }
    constexpr void set_as_float_scut() noexcept { m_family = family::scut; }

    [[nodiscard]] constexpr std::string_view id_string() const noexcept { return mach_ids[std::to_underlying(m_family)]; }
    [[nodiscard]] constexpr std::string_view name() const noexcept { return mach_names[std::to_underlying(m_family)]; }
};



/////////////////////////////////////////////////////////////////////////////
// Descrittore possibili misure ponte di taglio
class CutBridgeDim final
{
    enum class cutbridgedim : std::uint8_t
       {// Strato machines cut bridge recognized sizes
        undefined=0,
        c37, // ⎫ F
        c46, // ⎭
        c40, // ⎫
        c49, // ⎬ W/WR/HP
        c60  // ⎭
       };

    static constexpr std::array cutbridgedim_ids =
       {
        ""sv,    // undefined
        "3.7"sv, // c37
        "4.6"sv, // c46
        "4.0"sv, // c40
        "4.9"sv, // c49
        "6.0"sv  // c60
       };

 private:
    cutbridgedim m_dim_category = cutbridgedim::undefined;

 public:
    [[nodiscard]] constexpr bool operator==(const CutBridgeDim other) const noexcept { return m_dim_category==other.m_dim_category; }

    void assign(const double dim, const MachineFamily mach_family)
       {
        // Le misure riconosciute sono: 4.0, 4.9, 6.0 (ActiveW)
        //                              3.7, 4.6 (ActiveF)
        auto matches = [dim](const double other_dim) noexcept -> bool
           {
            return std::fabs(dim-other_dim) < 0.2;
           };

        if( mach_family.is_strato_f() )
           {
                 if( matches(3.7) ) m_dim_category = cutbridgedim::c37;
            else if( matches(4.6) ) m_dim_category = cutbridgedim::c46;
            else throw std::runtime_error( std::format("Unrecognized {} cut bridge size: {}", mach_family.name(), dim) );
           }
        else
           {
                 if( matches(4.0) ) m_dim_category = cutbridgedim::c40;
            else if( matches(4.9) ) m_dim_category = cutbridgedim::c49;
            else if( matches(6.0) ) m_dim_category = cutbridgedim::c60;
            else throw std::runtime_error( std::format("Unrecognized {} cut bridge size: {}", mach_family.name(), dim) );
           }
       }

    [[nodiscard]] constexpr bool is_defined() const noexcept { return m_dim_category!=cutbridgedim::undefined; }
    [[nodiscard]] constexpr bool is_37() const noexcept { return m_dim_category == cutbridgedim::c37; }
    [[nodiscard]] constexpr bool is_46() const noexcept { return m_dim_category == cutbridgedim::c46; }
    [[nodiscard]] constexpr bool is_40() const noexcept { return m_dim_category == cutbridgedim::c40; }
    [[nodiscard]] constexpr bool is_49() const noexcept { return m_dim_category == cutbridgedim::c49; }
    [[nodiscard]] constexpr bool is_60() const noexcept { return m_dim_category == cutbridgedim::c60; }

    [[nodiscard]] constexpr std::string_view string() const noexcept { return cutbridgedim_ids[std::to_underlying(m_dim_category)]; }
};



/////////////////////////////////////////////////////////////////////////////
// Descrittore possibili misure riscontro
class AlignSpanDim final
{
    enum class aligndim : std::uint8_t
       {// Strato machines align recognized sizes
        undefined=0,
        a32, // F/W/WR/HP
        a46  // W/WR/HP
       };

    static constexpr std::array aligndim_ids =
       {
        ""sv,    // undefined
        "3.2"sv, // a32
        "4.6"sv  // a46
       };

 private:
    aligndim m_dim_category = aligndim::undefined;

 public:
    [[nodiscard]] constexpr bool operator==(const AlignSpanDim other) const noexcept { return m_dim_category==other.m_dim_category; }

    void assign(const double dim)
       {
        // Le misure riconosciute sono: 3.2, 4.6 (ActiveW)
        //                              3.2 (ActiveF)
        auto matches = [dim](const double other_dim) noexcept -> bool
           {
            return std::fabs(dim-other_dim) < 0.2;
           };

             if( matches(3.2) ) m_dim_category = aligndim::a32;
        else if( matches(4.6) ) m_dim_category = aligndim::a46;
        else throw std::runtime_error( std::format("Unrecognized align size: {}",dim) );
       }

    [[nodiscard]] constexpr bool is_defined() const noexcept { return m_dim_category!=aligndim::undefined; }
    [[nodiscard]] constexpr bool is_32() const noexcept { return m_dim_category == aligndim::a32; }
    [[nodiscard]] constexpr bool is_46() const noexcept { return m_dim_category == aligndim::a46; }

    [[nodiscard]] constexpr std::string_view string() const noexcept { return aligndim_ids[std::to_underlying(m_dim_category)]; }
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

 private:
    std::underlying_type_t<bits> m_bitmask = 0;
    MG::options_set m_other; // Other options

 public:
    [[nodiscard]] constexpr bool operator==(MachineOptions const& other) const noexcept { return m_bitmask==other.m_bitmask and m_other==other.m_other; }

    [[nodiscard]] constexpr bool contains(MachineOptions const& other) const noexcept
       {
        return (m_bitmask | other.m_bitmask) == m_bitmask // Contains known options
               and m_other.contains(other.m_other); // Contains also unknown ones
       }

    [[nodiscard]] constexpr bool is_empty() const noexcept { return m_bitmask==0 and m_other.is_empty(); }
    //void reset() noexcept { m_bitmask=0; }

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

    [[nodiscard]] constexpr bool has(const std::string_view opt) const noexcept
       {
        MachineOptions other;
        other.add(opt);
        return contains(other);
       }

    constexpr void add(const std::string_view opt) noexcept
       {
        // Try to recognize option...
             if( opt=="opp" )    set_opp(true);
        else if( opt=="lowe" )   set_lowe(true);
        else if( opt=="rot" )    set_rot(true);
        else if( opt=="buf" )    set_buf(true);
        else if( opt=="no-buf" ) set_nobuf(true);
        //...Otherwise just collect it
        else m_other.insert( opt );
       }

    [[nodiscard]] std::string string(const char sep =',') const
       {
        std::string s;
        if( is_opp() )    { s+="opp";    s+=sep; }
        if( has_lowe() )  { s+="lowe";   s+=sep; }
        if( has_rot() )   { s+="rot";    s+=sep; }
        if( has_buf() )   { s+="buf";    s+=sep; }
        if( has_nobuf() ) { s+="no-buf"; s+=sep; }
        // Also the possible other ones
        for( const std::string& opt : m_other )
           {
            s+=opt; s+=sep;
           }
        if(s.size()>0) s.resize(s.size()-1);
        return s;
       }

 private:
    [[nodiscard]] constexpr bool has_bit(const bits bit) const noexcept { return m_bitmask & std::to_underlying(bit); }
    constexpr void set_bit(const bits bit, const bool b) noexcept { if(b)m_bitmask|=std::to_underlying(bit); else m_bitmask&=~std::to_underlying(bit); }
};



/////////////////////////////////////////////////////////////////////////////
class MachineData final
{
 private:
    MachineFamily m_family;
    CutBridgeDim m_cutbridgedim;
    AlignSpanDim m_algndim;
    MachineOptions m_options;

 public:

    [[nodiscard]] constexpr bool operator==(MachineData const& other) const noexcept
       {
        return m_family == other.m_family
           and m_cutbridgedim == other.m_cutbridgedim
           and m_algndim == other.m_algndim
           and m_options == other.m_options;
       }

    MachineData() noexcept =default;
    explicit MachineData(const std::string_view sv) { assign(sv); }
    void assign(const std::string_view sv) { *this = recognize_machine(sv); }

    [[nodiscard]] explicit operator bool() const noexcept { return m_family.is_defined(); }
    [[nodiscard]] bool is_incomplete() const noexcept
       {
        return !m_family.is_defined() or (m_family.is_strato() and !has_cutbridge_dim());
       }

    [[nodiscard]] constexpr MachineFamily family() const noexcept { return m_family; }
    [[nodiscard]] constexpr MachineFamily& mutable_family() noexcept { return m_family; }

    [[nodiscard]] constexpr bool has_cutbridge_dim() const noexcept { return m_cutbridgedim.is_defined(); }
    [[nodiscard]] constexpr CutBridgeDim cutbridge_dim() const noexcept { return m_cutbridgedim; }
    [[nodiscard]] constexpr CutBridgeDim& mutable_cutbridge_dim() noexcept { return m_cutbridgedim; }

    [[nodiscard]] constexpr bool has_align_dim() const noexcept { return m_algndim.is_defined(); }
    [[nodiscard]] constexpr AlignSpanDim align_dim() const noexcept { return m_algndim; }
    [[nodiscard]] constexpr AlignSpanDim& mutable_align_dim() noexcept { return m_algndim; }

    [[nodiscard]] constexpr const MachineOptions& options() const noexcept { return m_options; }
    [[nodiscard]] constexpr MachineOptions& mutable_options() noexcept { return m_options; }

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
        if( not options().is_empty() )
           {
            s += std::format("-({})", options().string());
           }
        return s;
       }

 private:
    //------------------------------------------------------------------------
    [[nodiscard]] static MachineData recognize_machine(const std::string_view sv)
       {// From strings like "ActiveWR-4.9/4.6-(opp,no-buf)"
        //                    family🠉    🠉dims    🠉options
        MachineData mach;

        // Need lowercase string
        const std::string s = str::to_lower( std::string{sv} );
        ascii::simple_lexer<char> lexer(s);

        // Machine family
        lexer.skip_nonalphas();
        if( not lexer.got_alpha() )
           {
            throw std::runtime_error( std::format("Invalid machine name: {}",sv) );
           }
        mach.m_family.assign( lexer.get_alphas() );

        // Possible given first dimension (cut bridge in case of strato machines)
        lexer.skip_nonalnums();
        if( lexer.got_digit() )
           {
            auto get_dim = [&lexer]() mutable -> double
               {
                double val = ascii::value_of_digit( lexer.current_char() );
                while( lexer.get_next() and lexer.got_digit() )
                   {
                    val = (10.0 * val) + ascii::value_of_digit( lexer.current_char() );
                   }
                if( lexer.got('.') )
                   {
                    lexer.get_next();
                    if( lexer.got_digit() )
                       {
                        double k = 0.1;
                        do {
                            val += k * ascii::value_of_digit( lexer.current_char() );
                            k *= 0.1;
                           }
                        while( lexer.get_next() and lexer.got_digit() );
                       }
                   }
                return val;
               };
            const double dim1 = get_dim();
            if( mach.family().is_strato() )
               {
                mach.m_cutbridgedim.assign( dim1, mach.m_family );
               }

            // Possible given second dimension (align max in case of strato machines)
            lexer.skip_nonalnums();
            if( lexer.got_digit() )
               {
                const double dim2 = get_dim();
                if( mach.family().is_strato() )
                   {
                    mach.m_algndim.assign( dim2 );
                   }
               }
           }

        // Get possible options
        while( lexer.got_data() )
           {
            lexer.skip_nonalphas();
            if( not lexer.got_alpha() ) break;
            const std::string_view opt = lexer.get_while(ascii::is_alpha_or_any_of<'-'>);
            mach.m_options.add(opt);
           }

        // If here, all ok
        return mach;
       }
};

}//:::::::::::::::::::::::::::::::: macotec :::::::::::::::::::::::::::::::::





/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"macotec_machine_data"> macotec_machine_data_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("macotec::MachineFamily") = []
   {
    macotec::MachineFamily fam;
    fam.assign("ahp"sv);
    ut::expect( fam.is_strato_hp() );
    fam.assign("bwr"sv);
    ut::expect( fam.is_strato_wr() );
    fam.assign("cw"sv);
    ut::expect( fam.is_strato_w() );
    fam.assign("activefr"sv);
    ut::expect( fam.is_strato_fr() );
    fam.assign("actf"sv);
    ut::expect( fam.is_strato_f() );
    fam.assign("mfrv"sv);
    ut::expect( fam.is_float_frv() );
    fam.assign("mfr"sv);
    ut::expect( fam.is_float_fr()  );
    fam.assign("starcut"sv);
    ut::expect( fam.is_float_scut() );
    ut::expect( ut::throws([]{ macotec::MachineFamily fam; fam.assign("boh"sv); }) ) << "bad family should throw\n";
   };

ut::test("macotec::CutBridgeDim") = []
   {
    macotec::MachineFamily fam; fam.set_as_strato_w();
    macotec::CutBridgeDim dim;
    ut::expect( not dim.is_defined() );
    ut::expect( ut::throws([]{ macotec::CutBridgeDim dim; dim.assign(2.1,{}); }) ) << "bad dim should throw\n";

    dim.assign(3.9, fam);
    ut::expect( dim.is_defined() );
    ut::expect( dim.is_40() );
    ut::expect( ut::that % dim.string()=="4.0"sv );
    dim.assign(4.9, fam);
    ut::expect( dim.is_49() );
    ut::expect( ut::that % dim.string()=="4.9"sv );
    dim.assign(6.1, fam);
    ut::expect( dim.is_60() );
    ut::expect( ut::that % dim.string()=="6.0"sv );
   };

ut::test("macotec::AlignSpanDim") = []
   {
    macotec::AlignSpanDim dim;
    ut::expect( not dim.is_defined() );
    ut::expect( ut::throws([]{ macotec::AlignSpanDim dim; dim.assign(4.1); }) ) << "bad dim should throw\n";
    dim.assign(4.7);
    ut::expect( dim.is_defined() );
    ut::expect( dim.is_46() );
    ut::expect( ut::that % dim.string()=="4.6"sv );
    dim.assign(3.2);
    ut::expect( dim.is_32() );
    ut::expect( ut::that % dim.string()=="3.2"sv );
   };

ut::test("macotec::MachineOptions") = []
   {
    ut::test("basic") = []
       {
        macotec::MachineOptions o1;
        ut::expect( o1.is_empty() );
        o1.add("lowe"sv);
        o1.add("rot"sv);
        o1.add("x-ray"sv);

        ut::expect( not o1.is_opp() );
        ut::expect( o1.has_lowe() );
        ut::expect( o1.has_rot() );
        ut::expect( not o1.has_buf() );
        ut::expect( not o1.has_nobuf() );
        ut::expect( o1.has("x-ray") );


        macotec::MachineOptions o2;
        o2.set_lowe(true);
        o2.set_buf(true);

        ut::expect( not o2.is_opp() );
        ut::expect( o2.has_lowe() );
        ut::expect( not o2.has_rot() );
        ut::expect( o2.has_buf() );
        ut::expect( not o2.has_nobuf() );
        ut::expect( not o2.has("x-ray") );

        ut::expect( not o1.contains(o2) );
        ut::expect( o1!=o2 );

        o1.add("buf"sv);
        ut::expect( o1.contains(o2) );

        ut::expect( ut::that % o1.string()=="lowe,rot,buf,x-ray"sv );
        ut::expect( ut::that % o2.string()=="lowe,buf"sv );
       };

    ut::test("set/reset single known") = []
       {
        macotec::MachineOptions o;
        ut::expect( not o.is_opp() );
        ut::expect( not o.has_lowe() );
        ut::expect( not o.has_rot() );
        ut::expect( not o.has_buf() );
        ut::expect( not o.has_nobuf() );

        o.set_opp(true);
        ut::expect( o.is_opp() );
        ut::expect( not o.has_lowe() );
        ut::expect( not o.has_rot() );
        ut::expect( not o.has_buf() );
        ut::expect( not o.has_nobuf() );

        o.set_opp(false);
        ut::expect( not o.is_opp() );
        ut::expect( not o.has_lowe() );
        ut::expect( not o.has_rot() );
        ut::expect( not o.has_buf() );
        ut::expect( not o.has_nobuf() );

        o.set_lowe(true);
        ut::expect( not o.is_opp() );
        ut::expect( o.has_lowe() );
        ut::expect( not o.has_rot() );
        ut::expect( not o.has_buf() );
        ut::expect( not o.has_nobuf() );

        o.set_lowe(false);
        ut::expect( not o.is_opp() );
        ut::expect( not o.has_lowe() );
        ut::expect( not o.has_rot() );
        ut::expect( not o.has_buf() );
        ut::expect( not o.has_nobuf() );

        o.set_rot(true);
        ut::expect( not o.is_opp() );
        ut::expect( not o.has_lowe() );
        ut::expect( o.has_rot() );
        ut::expect( not o.has_buf() );
        ut::expect( not o.has_nobuf() );

        o.set_rot(false);
        ut::expect( not o.is_opp() );
        ut::expect( not o.has_lowe() );
        ut::expect( not o.has_rot() );
        ut::expect( not o.has_buf() );
        ut::expect( not o.has_nobuf() );

        o.set_buf(true);
        ut::expect( not o.is_opp() );
        ut::expect( not o.has_lowe() );
        ut::expect( not o.has_rot() );
        ut::expect( o.has_buf() );
        ut::expect( not o.has_nobuf() );

        o.set_buf(false);
        ut::expect( not o.is_opp() );
        ut::expect( not o.has_lowe() );
        ut::expect( not o.has_rot() );
        ut::expect( not o.has_buf() );
        ut::expect( not o.has_nobuf() );

        o.set_nobuf(true);
        ut::expect( not o.is_opp() );
        ut::expect( not o.has_lowe() );
        ut::expect( not o.has_rot() );
        ut::expect( not o.has_buf() );
        ut::expect( o.has_nobuf() );

        o.set_nobuf(false);
        ut::expect( not o.is_opp() );
        ut::expect( not o.has_lowe() );
        ut::expect( not o.has_rot() );
        ut::expect( not o.has_buf() );
        ut::expect( not o.has_nobuf() );
       };
   };

ut::test("macotec::MachineData") = []
   {
    ut::should("recognize strato wr") = []
       {
        const macotec::MachineData m{ "somethingWR-4.9/4.6-(opp,no-buf,other)"sv };
        ut::expect( m.family().is_strato_wr() );
        ut::expect( m.family().is_strato() and not m.family().is_float() );
        ut::expect( m.cutbridge_dim().is_49() );
        ut::expect( m.align_dim().is_46() );
        ut::expect( m.options().is_opp() );
        ut::expect( not m.options().has_lowe() );
        ut::expect( not m.options().has_rot() );
        ut::expect( not m.options().has_buf() );
        ut::expect( m.options().has_nobuf() );
        ut::expect( m.options().has("other") );

        const macotec::MachineData m2{ m.string() };
        ut::expect( m == m2 );
       };

    ut::should("recognize float") = []
       {
        const macotec::MachineData m{ "STC6.0-(lowe)"sv };
        ut::expect( m.family().is_float_scut() );
        ut::expect( m.family().is_float() and not m.family().is_strato() );
        ut::expect( not m.cutbridge_dim().is_defined() );
        ut::expect( not m.align_dim().is_defined() );
        ut::expect( not m.options().is_opp() );
        ut::expect( m.options().has_lowe() );
        ut::expect( not m.options().has_rot() );
        ut::expect( not m.options().has_buf() );
        ut::expect( not m.options().has_nobuf() );
        ut::expect( not m.options().has("other") );

        const macotec::MachineData m2{ m.string() };
        ut::expect( m == m2 );
       };

    ut::should("recognize float frv") = []
       {
        const macotec::MachineData m{ "MsFRV"sv };
        ut::expect( m.family().is_float_frv() );
        ut::expect( m.family().is_float() and not m.family().is_strato() );
        ut::expect( not m.cutbridge_dim().is_defined() );
        ut::expect( not m.align_dim().is_defined() );
        ut::expect( m.options().is_empty() );
       };

    ut::should("bad name") = []
       {
        ut::expect( ut::throws([]{ [[maybe_unused]] macotec::MachineData m{"abc-4.9/4.6-(lowe)"sv}; }) ) << "unknown machine should throw\n";
       };

    ut::should("bad cut bridge") = []
       {
        ut::expect( ut::throws([]{ [[maybe_unused]] macotec::MachineData m{"ActiveW-1.9/4.6-(lowe)"sv}; }) ) << "unknown cut bridge should throw\n";
       };

    ut::should("bad align dim") = []
       {
        ut::expect( ut::throws([]{ [[maybe_unused]] macotec::MachineData m{"ActiveW-6.0/3.9-(lowe)"sv}; }) ) << "unknown cut bridge should throw\n";
       };
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
