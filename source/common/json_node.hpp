#pragma once
//  ---------------------------------------------
//  json node descriptor
//  ---------------------------------------------
//  #include "json_node.hpp" // json::Node
//  ---------------------------------------------
#include <string>
#include <string_view>
#include <map>
#include <vector>
#include <functional> // std::less, std::reference_wrapper, std::cref()
#include <concepts> // std::convertible_to
#include <format>


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace json //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
// A node of the tree (could be just a string or a map of other nodes)
class Node final
{
    using value_type = std::string;
    using key_type = std::string;
    using childs_type = std::map<key_type,Node,std::less<>>; // transparent comparator for heterogeneous lookup with string_view

 private:
    childs_type m_childs;
    value_type m_value;

 public:
    //-----------------------------------------------------------------------
    void set_value(const std::string_view newval)
       {
        if( has_childs() )
           {
            throw std::runtime_error( std::format("Cannot assign value \"{}\" to a parent node", newval) );
           }
        if( has_value() )
           {
            throw std::runtime_error( std::format("Cannot overwrite already existing value \"{}\" with \"{}\"", m_value, newval) );
           }
        m_value = newval;
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] const value_type& value() const noexcept
       {
        return m_value;
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] bool has_value() const noexcept
       {
        return not m_value.empty();
       }

    //-----------------------------------------------------------------------
    // Is a key:value assignment node, so no childs and has a value
    [[nodiscard]] bool is_leaf() const noexcept
       {
        return m_childs.empty() and not m_value.empty();
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] bool has_childs() const noexcept
       {
        return not m_childs.empty();
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] const auto& childs() const noexcept
       {
        return m_childs;
       }

    //-----------------------------------------------------------------------
    [[maybe_unused]] Node& ensure_child(const std::string_view key)
       {
        const auto [it, success] = m_childs.insert({key_type(key), Node()});
        //if( not success )
        //   {
        //    throw std::runtime_error( std::format("Cannot insert child \"{}\"", key) );
        //   }
        return it->second; // Returns the newly inserted or the already existing one
       }

    //-----------------------------------------------------------------------
    // Query child/descendant presence, returns nullptr if not found
    template<std::convertible_to<std::string_view> T, std::convertible_to<std::string_view>... Args>
    [[nodiscard]] const Node* get_child(T&& childname, Args&&... subchilds) const noexcept
       {
        if( const auto it_child = m_childs.find(childname); it_child!=m_childs.end() )
           {
            if constexpr( sizeof...(subchilds) > 0 )
               {
                return it_child->second.get_child(std::forward<Args>(subchilds)...);
               }
            else
               {
                return &(it_child->second);
               }
           }
        return nullptr;
       }

    //-----------------------------------------------------------------------
    void insert_childs_of(const Node& other)
       {
        if( has_value() )
           {
            throw std::runtime_error("Won't merge a value node");
           }

        // Check for clashes
        for( const auto& other_pair : other.m_childs )
           {
            if( auto it_mychild = m_childs.find(other_pair.first); it_mychild!=m_childs.end() )
               {// I already have this child!
                if( it_mychild->second.is_leaf() and other_pair.second.is_leaf() )
                   {// I have a value with the same key of the other's
                    if( it_mychild->second.value() != other_pair.second.value() )
                       {// Different values
                        throw std::runtime_error( std::format("Value conflict \"{}\": \"{}\" != \"{}\"", it_mychild->first, it_mychild->second.value(), other_pair.second.value()) );
                       }
                    else
                       {// Same value
                        throw std::runtime_error( std::format("Value \"{}:{}\" specified twice", other_pair.first, other_pair.second.value()) );
                       }
                   }
                else if( it_mychild->second.is_leaf() )
                   {// I have a value with the same key of the other group
                    throw std::runtime_error( std::format("Can't merge group \"{}\" with value \"{}\"", other_pair.first, it_mychild->second.value()) );
                   }
                else if( other_pair.second.is_leaf() )
                   {// I have a group with the same key of the other value
                    throw std::runtime_error( std::format("Can't merge a value \"{}:{}\" with a group", other_pair.first, other_pair.second.value()) );
                   }
                else
                   {// Two groups, merge them
                    it_mychild->second.insert_childs_of( other_pair.second );
                   }
               }
            else
               {// I don't already have this child
                m_childs.insert(other_pair);
               }
           }
       }


    //-----------------------------------------------------------------------
    //[[nodiscard]] std::size_t total_descendants_count() const noexcept
    //   {
    //    std::size_t n = 0u;
    //    for( const auto& pair : m_childs )
    //       {
    //        n += 1u + pair.second.total_descendants_count();
    //       }
    //    return n;
    //   }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::size_t total_values_count() const noexcept
       {
        if( is_leaf() ) return 1;
        std::size_t n = 0;
        for( const auto& pair : m_childs )
           {
            // cppcheck-suppress useStlAlgorithm
            n += pair.second.total_values_count();
           }
        return n;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::string string() const
       {
        std::string s;
        if( has_value() )
           {
            s += m_value;
           }
        if( has_childs() )
           {
            s += '{';
            auto ip = m_childs.begin();
            //if( ip!=m_childs.end() ) // Already true if here
            s += ip->first;
            s += ':';
            s += ip->second.string();
            while( ++ip!=m_childs.end() )
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
};


/////////////////////////////////////////////////////////////////////////////
// A list of references to existing nodes
class RefNodeList final
{
    using node_cref_t = std::reference_wrapper<const json::Node>;
 private:
    std::vector<node_cref_t> m_v;

 public:
    [[nodiscard]] auto size() const noexcept { return m_v.size(); }
    [[nodiscard]] auto begin() const noexcept { return m_v.cbegin(); }
    [[nodiscard]] auto end() const noexcept { return m_v.cend(); }

    void append_ref_of(const json::Node& node)
       {
        m_v.push_back(std::cref(node));
       }

    void append_refs(const RefNodeList& other)
       {
        m_v.insert(m_v.end(), other.m_v.begin(), other.m_v.end());
       }
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include <iterator> // std::prev
#include <print>
/////////////////////////////////////////////////////////////////////////////
void print(json::Node const& n, std::string&& indent ="")
   {
    if( n.has_value() )
       {
        std::print(":{}", n.value());
       }
    if( n.has_childs() )
       {
        const auto it_last = std::prev(n.childs().end());
        auto it = n.childs().begin();
        //if( indent.empty() ) std::print("\n┐");
        while( it!=it_last )
           {
            std::print("\n{}├{}", indent, it->first);
            print(it->second, indent+"│ ");
            ++it;
           }
        // Last element
        std::print("\n{}┕{}", indent, it->first);
        print(it->second, indent+"  ");
        if( indent.empty() ) std::print("\n");
       }
   }
/////////////////////////////////////////////////////////////////////////////
json::Node create_json_tree_aei()
   {
    json::Node root;                   // root┐
    auto& a = root.ensure_child("a");  //     ├a┐
        auto& b = a.ensure_child("b"); //     │ ├b┐
            b.ensure_child("c");       //     │ │ └c
        a.ensure_child("d");           //     │ └d
    auto& e = root.ensure_child("e");  //     ├e┐
        auto& f = e.ensure_child("f"); //     │ ├f┐
            f.ensure_child("g");       //     │ │ └g
        e.ensure_child("h");           //     │ └h
    root.ensure_child("i");            //     └i
    return root;
   }
/////////////////////////////////////////////////////////////////////////////
json::Node create_json_tree_a0()      //                        ┌b1=B1
   {                                  //                     ┌b0┴b2=B2
    json::Node a0;                    //                   a0┤     ┌c11=C11
    auto& b0 = a0.ensure_child("b0"); //                     └c0─c1┴c12=C12
    auto& b1 = b0.ensure_child("b1"); b1.set_value("B1");
    auto& b2 = b0.ensure_child("b2"); b2.set_value("B2");
    auto& c0 = a0.ensure_child("c0");
    auto& c1 = c0.ensure_child("c1");
    auto& c11 = c1.ensure_child("c11"); c11.set_value("C11");
    auto& c12 = c1.ensure_child("c12"); c12.set_value("C12");
    return a0;
   }
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"json_node"> json_node_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("json::Node") = []
   {
    json::Node a0 = create_json_tree_a0();

    ut::expect( ut::that % a0.string()=="{b0:{b1:B1,b2:B2},c0:{c1:{c11:C11,c12:C12}}}"sv );

    ut::expect( ut::that % a0.childs().size() == 2u );
    ut::expect( ut::that % a0.total_values_count() == 4u );
    ut::expect( ut::that % a0.get_child("X") == nullptr );
    ut::expect( ut::that % a0.get_child("b0", "X") == nullptr );

    const json::Node* const b0 = a0.get_child("b0");
    const json::Node* const b1 = a0.get_child("b0", "b1");
    const json::Node* const b2 = a0.get_child("b0", "b2");

    ut::expect( ut::fatal(b0!=nullptr) );
    ut::expect( b0->has_childs() );
    ut::expect( not b0->is_leaf() );
    ut::expect( not b0->has_value() );
    ut::expect( b0->value().empty() );
    ut::expect( ut::that % b0->get_child("b1") == b1 );
    ut::expect( ut::that % b0->get_child("b2") == b2 );

    ut::expect( ut::fatal(b1!=nullptr) );
    ut::expect( not b1->has_childs() );
    ut::expect( b1->is_leaf() );
    ut::expect( b1->has_value() );
    ut::expect( ut::that % b1->value() == "B1"sv );

    ut::expect( ut::fatal(b2!=nullptr) );
    ut::expect( not b2->has_childs() );
    ut::expect( b2->is_leaf() );
    ut::expect( b2->has_value() );
    ut::expect( ut::that % b2->value() == "B2"sv );

    const json::Node* const c0 = a0.get_child("c0");
    const json::Node* const c1 = a0.get_child("c0", "c1");
    const json::Node* const c11 = a0.get_child("c0", "c1", "c11");
    const json::Node* const c12 = a0.get_child("c0", "c1", "c12");

    ut::expect( ut::fatal(c0!=nullptr) );
    ut::expect( c0->has_childs() );
    ut::expect( not c0->is_leaf() );
    ut::expect( not c0->has_value() );
    ut::expect( c0->value().empty() );
    ut::expect( ut::that % c0->childs().size() == 1u );
    ut::expect( ut::that % c0->get_child("X") == nullptr );
    ut::expect( ut::that % c0->get_child("c1", "c11") == c11 );

    ut::expect( ut::fatal(c1!=nullptr) );
    ut::expect( c1->has_childs() );
    ut::expect( not c1->is_leaf() );
    ut::expect( not c1->has_value() );
    ut::expect( c1->value().empty() );
    ut::expect( ut::that % c1->childs().size() == 2u );
    ut::expect( ut::that % c1->get_child("X") == nullptr );
    ut::expect( ut::that % c1->get_child("c11") == c11 );

    ut::expect( ut::fatal(c11!=nullptr) );
    ut::expect( not c11->has_childs() );
    ut::expect( c11->is_leaf() );
    ut::expect( c11->has_value() );
    ut::expect( ut::that % c11->value() == "C11"sv );

    ut::expect( ut::fatal(c12!=nullptr) );
    ut::expect( not c12->has_childs() );
    ut::expect( c12->is_leaf() );
    ut::expect( c12->has_value() );
    ut::expect( ut::that % c12->value() == "C12"sv );

    json::Node root = create_json_tree_aei();
    ut::expect( ut::that % root.string()=="{a:{b:{c:},d:},e:{f:{g:},h:},i:}"sv );

    root.insert_childs_of(a0);
    // root┐
    //     ├a┐
    //     │ ├b┐
    //     │ │ ┕c
    //     │ ┕d
    //     ├b0
    //     │ ├b1:B1
    //     │ ┕b2:B2
    //     ├c0
    //     │ ┕c1
    //     │   ├c11:C11
    //     │   ┕c12:C12
    //     ├e┐
    //     │ ├f┐
    //     │ │ ┕g
    //     │ ┕h
    //     ┕i
    ut::expect( ut::that % root.string()=="{a:{b:{c:},d:},b0:{b1:B1,b2:B2},c0:{c1:{c11:C11,c12:C12}},e:{f:{g:},h:},i:}"sv );
   };


ut::test("merging trees") = []
   {
    ut::should("value conflict") = []
       {
        json::Node r1; {
                        r1.ensure_child("a").set_value("aval"); //   ┌a=aval
                        r1.ensure_child("b").set_value("bval"); // r1┼b=bval
                        r1.ensure_child("c").set_value("cval"); //   └c=cval
                       }

        json::Node r2; {
                        r2.ensure_child("a").set_value("aval"); //   ┌a=aval
                        r2.ensure_child("d").set_value("dval"); // r2┼d=dval
                        r2.ensure_child("e").set_value("eval"); //   └e=eval
                       }

        ut::expect( ut::throws([&r1,&r2]{ r1.insert_childs_of(r2); }) ) << "should throw\n";
       };

    ut::should("cant merge group with a value") = []
       {
        json::Node r1; {
                        auto& a = r1.ensure_child("a");
                        a.ensure_child("a1").set_value("a1val"); //     ┌a1=a1val
                        a.ensure_child("a2").set_value("a2val"); //   ┌a┴a2=a2val
                        r1.ensure_child("b").set_value("bval");  // r1┼b=bval
                        r1.ensure_child("c").set_value("cval");  //   └c=cval
                       }

        json::Node r2; {
                        r2.ensure_child("a").set_value("aval"); //   ┌a=aval
                        r2.ensure_child("d").set_value("dval"); // r2┼d=dval
                        r2.ensure_child("e").set_value("eval"); //   └e=eval
                       }

        ut::expect( ut::throws([&r1,&r2]{ r1.insert_childs_of(r2); }) ) << "should throw\n";
       };
   };


ut::test("json::RefNodeList") = []
   {                                                      // root┐
    json::Node root = create_json_tree_aei();             //     ├a┐
    const json::Node* const b = root.get_child("a", "b"); //     │ ├b┐
    const json::Node* const e = root.get_child("e");      //     │ │ └c
    ut::expect( ut::fatal(b!=nullptr and e!=nullptr) );   //     │ └d
                                                          //     ├e┐
    json::RefNodeList nodes;                              //     │ ├f┐
    nodes.append_ref_of(*b);                              //     │ │ └g
    nodes.append_ref_of(*e);                              //     │ └h
    ut::expect( ut::that % nodes.size()==2u );            //     └i
    std::size_t idx = 0;
    for( const auto noderef : nodes )
       {
        switch(++idx)
           {
            case 1: ut::expect( ut::that % noderef.get().string()=="{c:}"sv ) << " node " << idx << '\n'; break;
            case 2: ut::expect( ut::that % noderef.get().string()=="{f:{g:},h:}"sv ) << " node " << idx << '\n'; break;
            default: ut::expect(false) << "shouldn't be here (" << idx << ")\n";
           }
       }
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
