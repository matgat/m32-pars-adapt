#ifndef GUARD_sipro_register_hpp
#define GUARD_sipro_register_hpp
//  ---------------------------------------------
//  Sipro register descriptor
//  ---------------------------------------------
#include <cstdint> // std::uint8_t, std::uint16_t
//#include <limits> // std::numeric_limits
#include <cctype> // std::isdigit, ...
#include <array>
#include <string_view>
//#include <charconv> // std::from_chars



//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sipro
{

/////////////////////////////////////////////////////////////////////////////
class Register final
{
 private:
    enum class type : std::uint8_t
       {
        none=0, // !Consecutive indexes!
        vb,
        vn,
        vq,
        vd,
        va,
        size
       };

    static constexpr std::array<std::string_view, std::to_underlying(type::size)>
    reg_iec_types =
       {
        ""sv,      // none
        "BOOL"sv,  // vb
        "INT"sv,   // vn
        "DINT"sv,  // vq
        "LREAL"sv, // vd
        "STRING"sv // va
       };

    static constexpr std::array<char, std::to_underlying(type::size)>
    plc_var_type =
       {
        '\0', // none
        'B',  // vb
        'W',  // vn
        'D',  // vq
        'L',  // vd
        'B'   // va
       };

    static constexpr std::array<std::uint16_t, std::to_underlying(type::size)>
    plc_var_address =
       {
        0,   // none
        300, // vb
        400, // vn
        500, // vq
        600, // vd
        700  // va
       };

    //static constexpr std::array<std::string_view, std::to_underlying(type::size)>
    //plc_mem_map =
    //   {
    //    ""sv,      // none
    //    "MB300"sv, // vb
    //    "MW400"sv, // vn
    //    "MD500"sv, // vq
    //    "ML600"sv, // vd
    //    "MB700"sv  // va
    //   };

 public:
    explicit constexpr Register(std::string_view sv) noexcept
       {// From strings like "vq123"
        // Prefix
        if( sv.length()>=3 && (sv[0]=='v' || sv[0]=='V') )
           {
            // Type
                 if(sv[1]=='b' || sv[1]=='B') i_type = type::vb;
            else if(sv[1]=='n' || sv[1]=='N') i_type = type::vn;
            else if(sv[1]=='q' || sv[1]=='Q') i_type = type::vq;
            else if(sv[1]=='d' || sv[1]=='D') i_type = type::vd;
            else if(sv[1]=='a' || sv[1]=='A') i_type = type::va;

            // Index
            if( i_type!=type::none )
               {
                // Too permissive:
                //const auto i_end = s.data() + s.length();
                //const auto [i, ec] = std::from_chars(s.data()+2, i_end, i_index);
                //if( ec!=std::errc() || i!=i_end ) i_type = type::none; // Not an index, invalidate
                sv.remove_prefix(2);
                if( !is_valid_index(sv, i_index) )
                   {
                    i_type = type::none; // Not an index, invalidate
                   }
               }
           }
       }

    [[nodiscard]] constexpr std::uint16_t index() const noexcept { return i_index; }

    [[nodiscard]] friend constexpr bool are_same_type(const Register& lhs, const Register& rhs) noexcept { return lhs.i_type==rhs.i_type; }
    [[nodiscard]] constexpr bool is_valid() const noexcept { return i_type!=type::none; }
    [[nodiscard]] constexpr bool is_va() const noexcept { return i_type==type::va; }

    [[nodiscard]] constexpr std::uint16_t get_va_length() const noexcept { return 80; }

    [[nodiscard]] constexpr std::string_view iec_type() const noexcept { return reg_iec_types[std::to_underlying(i_type)]; }
    [[nodiscard]] constexpr char iec_address_type() const noexcept { return 'M'; }
    [[nodiscard]] constexpr char iec_address_vartype() const noexcept { return plc_var_type[std::to_underlying(i_type)]; }
    [[nodiscard]] constexpr std::uint16_t iec_address_index() const noexcept { return plc_var_address[std::to_underlying(i_type)]; }

 private:
    std::uint16_t i_index = 0;
    type i_type = type::none;

    //-----------------------------------------------------------------------
    // Try to recognize a base10 unsigned 16-bit natural number
    [[nodiscard]] bool is_valid_index(const std::string_view buf, std::uint16_t& returned_index) noexcept
       {
        const std::uint32_t max_index = 9999u;
        //static_assert(max_index<std::numeric_limits<uint16_t>::max);
        const std::uint32_t base = 10u;
        std::uint32_t parsed_index = 0u;
        std::size_t i = 0;
        if( i<buf.size() && std::isdigit(buf[i]) )
           {
            do {
                parsed_index = (base*parsed_index) + (buf[i] - '0');
                ++i;
               }
            while( i<buf.size() && std::isdigit(buf[i]) );

            if( i==buf.size() && parsed_index<=max_index )
               {
                returned_index = static_cast<std::uint16_t>(parsed_index);
                return true;
               }
           }
        return false;
       }
};

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
