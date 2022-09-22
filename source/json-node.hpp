#ifndef GUARD_json_node_hpp
#define GUARD_json_node_hpp
//  ---------------------------------------------
//  json node descriptor
//  ---------------------------------------------
#include <string>
#include <string_view>
#include <map>
#include <iterator> // std::prev
#include <concepts> // std::same_as
#include <fmt/core.h> // fmt::format, fmt::print


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace json //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
// A node of the tree (could be just a string or a map of other nodes)
class Node final
{
 public:
    using value_type = std::string;
    using key_type = std::string;
    using childs_type = std::map<key_type,Node>;


    //-----------------------------------------------------------------------
    void set_value(const std::string_view newval)
       {
        if( !i_childs.empty() )
           {
            throw std::runtime_error( fmt::format("Cannot assign value \"{}\" to a parent node", newval) );
           }
        if( !i_value.empty() )
           {
            throw std::runtime_error( fmt::format("Cannot overwrite value \"{}\" on already existing \"{}\"", newval, i_value) );
           }
        i_value = newval;
       }

    //-----------------------------------------------------------------------
    const value_type& value() const noexcept
       {
        return i_value;
       }

    //-----------------------------------------------------------------------
    bool has_value() const noexcept
       {
        return !i_value.empty();
       }


    //-----------------------------------------------------------------------
    bool is_leaf() const noexcept
       {
        return i_childs.empty();
       }

    //-----------------------------------------------------------------------
    [[maybe_unused]] Node& ensure_child(const std::string_view key)
       {
        const auto [it, success] = i_childs.insert({key_type(key), Node()});
        //if( !success )
        //   {
        //    throw std::runtime_error( fmt::format("Cannot insert child \"{}\"", key) );
        //   }
        return it->second; // Returns the newly inserted or the already existing one
       }

    //-----------------------------------------------------------------------
    // Access childs
    //[[nodiscard]] const Node& operator[](const key_type& key) const
    //   {
    //    const auto it = i_childs.find(key);
    //    if( it==i_childs.end() )
    //       {
    //        throw std::runtime_error( fmt::format("child \"{}\" not found", key) );
    //       }
    //    return it->second;
    //   }

    //-----------------------------------------------------------------------
    // Access childs
    //template<typename... Args> const Node& get_descendant(key_type&& childname, Args&&... subchilds) const noexcept
    template<std::same_as<key_type>... Args> const Node& get_descendant(key_type&& childname, Args&&... subchilds) const noexcept
       {
        if( const auto it_child = i_childs.find(childname); it_child!=i_childs.end() )
            {
            if constexpr( sizeof...(subchilds) > 0 )
               {
                return it_child->second.get_descendant(std::forward<Args>(subchilds)...);
               }
            else
               {
                return it_child->second;
               }
            }
        return empty_node;
       }

    //-----------------------------------------------------------------------
    void merge_childs(const Node& other)
       {
        if( has_value() )
           {
            throw std::runtime_error("Won't merge a value node");
           }
        //i_childs.merge(other.i_childs); // Won't update existing ones
        // Check for clashes
        for( const auto& other_pair : other.i_childs )
           {
            if( i_childs.contains(other_pair.first) )
               {
                throw std::runtime_error( fmt::format("Won't merge existing child \"{}\"", other_pair.first) );
               }
            i_childs.insert(other_pair);
           }
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::size_t direct_childs_count() const noexcept
       {
        return i_childs.size();
       }


    //-----------------------------------------------------------------------
    //[[nodiscard]] std::size_t overall_childs_count() const noexcept
    //   {
    //    std::size_t n = 0;
    //    for( const auto& pair : i_childs )
    //       {
    //        n += 1 + pair.second.overall_childs_count();
    //       }
    //    return n;
    //   }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::size_t overall_leafs_count() const noexcept
       {
        if( is_leaf() ) return 1;
        std::size_t n = 0;
        for( const auto& pair : i_childs )
           {
            n += pair.second.overall_leafs_count();
           }
        return n;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::string string() const
       {
        std::string s;
        if( is_leaf() )
           {
            s += i_value;
           }
        else
           {
            s += '{';
            auto ip = i_childs.begin();
            //if( ip!=i_childs.end() ) // Already true if here
            s += ip->first;
            s += ':';
            s += ip->second.string();
            while( ++ip!=i_childs.end() )
               {
                s += ',';
                s += ip->first;
                s += ':';
                s += ip->second.string();
               }
            s += '}';
           }
        return s;
       }

    //-----------------------------------------------------------------------
    void print(std::string&& indent ="") const
       {
        if( is_leaf() )
           {
            fmt::print(":{}", i_value);
           }
        else
           {
            const auto it_last = std::prev(i_childs.end());
            auto it = i_childs.begin();
            //if( indent.empty() ) fmt::print("\n┐");
            while( it != it_last )
               {
                fmt::print("\n{}├{}", indent, it->first);
                it->second.print(indent+"│ ");
                ++it;
               }
            // Last element
            fmt::print("\n{}┕{}", indent, it->first);
            it->second.print(indent+"  ");
            if( indent.empty() ) fmt::print("\n");
           }
       }

 private:
    childs_type i_childs;
    value_type i_value;
    static const Node empty_node;
};

inline const Node Node::empty_node;


#ifndef NDEBUG
//---------------------------------------------------------------------------
[[nodiscard]] json::Node create_test_tree()
{
    //      ┌b1=B1
    //    ┌b┴b2=B2
    //   a┤    ┌c11=C11
    //    └c─c1┴c12=C12
    json::Node a;
    json::Node& b = a.ensure_child("b");
    json::Node& b1 = b.ensure_child("b1");
    b1.set_value("B1");
    json::Node& b2 = b.ensure_child("b2");
    b2.set_value("B2");
    json::Node& c = a.ensure_child("c");
    json::Node& c1 = c.ensure_child("c1");
    json::Node& c11 = c1.ensure_child("c11");
    c11.set_value("C11");
    json::Node& c12 = c1.ensure_child("c12");
    c12.set_value("C12");

    return a;
}

//---------------------------------------------------------------------------
//[[nodiscard]] json::Node create_test_tree2()
//{
//    // root┐
//    //     ├a┐
//    //     │  ├b┐
//    //     │  │  └c
//    //     │  └d
//    //     ├e┐
//    //     │  ├f┐
//    //     │  │  └g
//    //     │  └h
//    //     └i
//    json::Node root;
//    json::Node& a = root.ensure_child("a");
//        json::Node& b = a.ensure_child("b");
//            json::Node& c = b.ensure_child("c");
//        json::Node& d = a.ensure_child("d");
//    json::Node& e = root.ensure_child("e");
//        json::Node& f = e.ensure_child("f");
//            json::Node& g = f.ensure_child("g");
//        json::Node& h = e.ensure_child("h");
//    json::Node& i = root.ensure_child("i");
//
//    return root;
//}
#endif



}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
