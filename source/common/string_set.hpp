#pragma once
//  ---------------------------------------------
//  A set implemented with vector
//  optimized to use strings as keys
//  (uses string_views for queries)
//  ---------------------------------------------
//  #include "string_set.hpp" // MG::string_set
//  ---------------------------------------------
#include <vector>
#include <string>
#include <string_view>
#include <concepts> // std::same_as<>



namespace MG //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

template<typename T>
concept StdBasicString = std::same_as<T, std::basic_string<typename T::value_type, typename T::traits_type, typename T::allocator_type>>;


/////////////////////////////////////////////////////////////////////////////
template<StdBasicString T> class string_set final
{
 public:
    using elem_type = T;
    using elem_view_type = decltype( std::basic_string_view{std::declval<const elem_type&>()} );
    using container_type = std::vector<elem_type>;
    using iterator = container_type::iterator;
    using const_iterator = container_type::const_iterator;

 private:
    container_type m_vect;

 public:
    //constexpr string_set(std::initializer_list<elem_view_type> values)
    //  : m_vect(values)
    //   {}

    [[nodiscard]] constexpr bool operator==(string_set<T> const& other) const noexcept
       {
        if( size()!=other.size() )
           {
            return false;
           }
        for( const elem_type& other_elem : other.m_vect )
           {
            if( not contains(other_elem) )
               {
                return false;
               }
           }
        return true;
       }


    [[nodiscard]] constexpr auto size() const noexcept { return m_vect.size(); }
    [[nodiscard]] constexpr bool is_empty() const noexcept { return m_vect.empty(); }
    constexpr void clear() { m_vect.clear(); }

    constexpr void insert(elem_type&& new_elem)
       {
        if( not contains(new_elem) )
           {
            m_vect.push_back(std::forward<T>(new_elem));
           }
       }

    constexpr void insert(const elem_view_type new_elem)
       {
        if( not contains(new_elem) )
           {
            m_vect.emplace_back(new_elem);
           }
       }

    constexpr void insert(const typename elem_view_type::value_type* const new_elem )
       {
        insert( elem_view_type{new_elem} );
       }

    [[maybe_unused]] constexpr bool erase(const elem_view_type elem)
       {
        iterator it = m_vect.begin();
        while( it!=m_vect.end() )
           {
            if( *it==elem )
               {
                it = m_vect.erase(it);
                return true;
               }
            else ++it;
           }
        return false;
       }

    [[nodiscard]] constexpr bool contains(const elem_view_type elem) const noexcept
       {
        return find(elem)!=end();
       }

    [[nodiscard]] constexpr bool contains(const string_set& other) const noexcept
       {
        //return std::ranges::includes(m_vect, other.m_vect); // Needs sorted
        for( const elem_type& other_elem : other.m_vect )
           {// cppcheck-suppress useStlAlgorithm
            if( not contains(other_elem) ) return false;
           }
        return true;
       }

    constexpr void merge_with(const string_set& other)
       {
        for( const elem_type& other_elem : other.m_vect )
           {
            insert( elem_type{other_elem} );
           }
       }

    [[nodiscard]] constexpr const_iterator begin() const noexcept { return m_vect.cbegin(); }
    [[nodiscard]] constexpr const_iterator end() const noexcept { return m_vect.cend(); }
    [[nodiscard]] constexpr iterator begin() noexcept { return m_vect.begin(); }
    [[nodiscard]] constexpr iterator end() noexcept { return m_vect.end(); }

 private:
    [[nodiscard]] constexpr const_iterator find(const elem_view_type elem) const noexcept
       {
        //return std::ranges::find(m_vect, elem)!=m_vect.end(); // Wants elem_type
        const_iterator it = begin();
        while( it!=end() )
           {// cppcheck-suppress useStlAlgorithm
            if(*it==elem) break;
            ++it;
           }
        return it;
       }
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------
[[nodiscard]] constexpr std::string to_string( MG::string_set<std::string> const& strset )
   {
    std::string s;
    auto it = strset.begin();
    if( it!=strset.end() )
       {
        s += *it;
        while( ++it!=strset.end() )
           {
            s += ',';
            s += *it;
           }
       }
    return s;
   }
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"string_set"> string_set_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("basic operations") = []
   {
    MG::string_set<std::string> o;
    ut::expect( o.is_empty() );
    ut::expect( ut::that % o.size()==0u );

    o.insert("key1"s);
    ut::expect( ut::that % o.size()==1u );
    ut::expect( o.contains("key1"sv) and not o.contains("key2"sv) and not o.contains("key3"sv) );
    ut::expect( ut::that % to_string(o)=="key1"sv );

    o.insert("key2"sv);
    ut::expect( ut::that % o.size()==2u );
    ut::expect( o.contains("key1"sv) and o.contains("key2"sv) and not o.contains("key3"sv) );
    ut::expect( ut::that % to_string(o)=="key1,key2"sv );

    o.insert("key3");
    ut::expect( ut::that % o.size()==3u );
    ut::expect( o.contains("key1"sv) and o.contains("key2"sv) and o.contains("key3"sv) );
    ut::expect( ut::that % to_string(o)=="key1,key2,key3"sv );

    o.insert("key2");
    ut::expect( ut::that % o.size()==3u );
    ut::expect( o.contains("key1"sv) and o.contains("key2"sv) and o.contains("key3"sv) );
    ut::expect( ut::that % to_string(o)=="key1,key2,key3"sv ) << "shouldn't modify\n";

    // Erasing
    o.erase("x");
    ut::expect( ut::that % o.size()==3u );
    ut::expect( o.contains("key1"sv) and o.contains("key2"sv) and o.contains("key3"sv) );
    ut::expect( ut::that % to_string(o)=="key1,key2,key3"sv );
    o.erase("key1");
    ut::expect( ut::that % o.size()==2u );
    ut::expect( not o.contains("key1"sv) and o.contains("key2"sv) and o.contains("key3"sv) );
    ut::expect( ut::that % to_string(o)=="key2,key3"sv );
    o.clear();
    ut::expect( o.is_empty() );
    ut::expect( ut::that % o.size()==0u );
    ut::expect( not o.contains("key1"sv) and not o.contains("key2"sv) and not o.contains("key3"sv) );
    ut::expect( ut::that % to_string(o)==""sv );
   };


ut::test("set operations") = []
   {
    MG::string_set<std::string> o1;
    o1.insert("blue");
    o1.insert("yellow");
    o1.insert("orange");
    o1.insert("red");
    o1.insert("purple");
    ut::expect( ut::that % to_string(o1)=="blue,yellow,orange,red,purple"sv );

    MG::string_set<std::string> o2;
    o2.insert("red");
    o2.insert("blue");
    o2.insert("green");
    ut::expect( ut::that % to_string(o2)=="red,blue,green"sv );

    ut::expect( not o1.contains(o2) );

    o1.merge_with(o2);
    ut::expect( ut::that % to_string(o1)=="blue,yellow,orange,red,purple,green"sv );
    ut::expect( o1.contains(o2) );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
