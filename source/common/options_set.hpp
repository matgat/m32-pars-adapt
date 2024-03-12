#pragma once
//  ---------------------------------------------
//  A set of string options
//  ---------------------------------------------
//  #include "options_set.hpp" // MG::options_set
//  ---------------------------------------------
#include "string_set.hpp" // MG::string_set
#include "ascii_simple_lexer.hpp" // ascii::simple_lexer


namespace MG //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
class options_set final
{
    using container_type = MG::string_set<std::string>;

 private:
    container_type m_set;

 public:
    options_set() noexcept = default;
    explicit options_set(const std::string_view input) { assign(input); }

    [[nodiscard]] constexpr bool operator==(options_set const& other) const noexcept
       {
        return m_set == other.m_set;
       }

    void assign(const std::string_view input)
       {// Get from strings like: "elem1,elem2,elem3"
        class lexer_t final : public ascii::simple_lexer<typename std::string_view::value_type>
        {
         private:
            std::string_view m_elem_token;

         public:
            explicit lexer_t(const std::string_view buf) noexcept
              : ascii::simple_lexer<char>(buf)
               {}

            [[nodiscard]] bool got_elem()
               {
                while( got_data() )
                   {
                    skip_any_space();
                    if( got(ascii::is_any_of<',',';'>) )
                       {
                        get_next();
                       }
                    else if( m_elem_token=get_until(ascii::is_space_or_any_of<',',';'>);
                             not m_elem_token.empty() )
                       {
                        return true;
                       }
                    else
                       {
                        get_next();
                       }
                   }
                return false;
               }

            [[nodiscard]] std::string_view elem() const noexcept { return m_elem_token; }
        } lexer{input};


        while( lexer.got_elem() )
           {
            m_set.insert( lexer.elem() );
           }
       }

    [[nodiscard]] bool is_empty() const noexcept { return m_set.is_empty(); }
    [[nodiscard]] bool contains(const std::string_view elem) const noexcept { return m_set.contains(elem); }
    [[nodiscard]] bool contains(const options_set& other) const noexcept { return m_set.contains(other.m_set); }
    void insert(const std::string_view new_elem) { m_set.insert(new_elem); }

    //-----------------------------------------------------------------------
    [[nodiscard]] container_type::const_iterator begin() const noexcept { return m_set.begin(); }
    [[nodiscard]] container_type::const_iterator end() const noexcept { return m_set.end(); }
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"options_set"> options_set_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("basic usage") = []
   {
    MG::options_set o;
    ut::expect( o.is_empty() );

    o.assign("elem1,  elem2  ,elem3"sv);
    ut::expect( not o.is_empty() );
    ut::expect( o.contains("elem1"sv) and o.contains("elem2"sv) and o.contains("elem3"sv) and not o.contains("elem4"sv) );

    o.insert("elem4"s);
    ut::expect( o.contains("elem1"sv) and o.contains("elem2"sv) and o.contains("elem3"sv) and o.contains("elem4"sv) );
    
    std::string s;
    for( const std::string& opt : o ) s += opt;
    ut::expect( ut::that % s == "elem1elem2elem3elem4"sv );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
