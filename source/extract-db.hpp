#ifndef GUARD_extract_db_hpp
#define GUARD_extract_db_hpp
//  ---------------------------------------------
//  Get DB data pertinent to a certain machine type
//  ---------------------------------------------
#include "json-node.hpp" // json::Node
#include "machine-type.hpp" // macotec::MachineType


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace macotec //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


//----------------------------------------------------------------------------
// From a structure like this:  root┐
//                                  ├mach1┐
//                                  │     ├"common"-{nam=val,...}
//                                  │     ├"cut-bridge"
//                                  │     │  ├"dim1"-{nam=val,...}
//                                  │     │  ├"dim2"-{nam=val,...}
//                                  │     │  └···
//                                  │     └"algn-span"
//                                  │       ├"dim1"-{nam=val,...}
//                                  │       ├"dim2"-{nam=val,...}
//                                  │       └···
//                                  ├mach2┐
//                                  │     ├"common"─{nam=val,...}
//                                  │     ├"cut-bridge"
//                                  │     │  ├"dim1"-{nam=val,...}
//                                  │     │  ├"dim2"-{nam=val,...}
//                                  │     │  └···
//                                  │     └"algn-span"
//                                  │       ├"dim1"-{nam=val,...}
//                                  │       ├"dim2"-{nam=val,...}
//                                  │       └···
//                                  └···
// Will be extracted a list like this:   ┌{nam=val,...}
//                                       ├{nam=val,...}
//                                       └{nam=val,...}
[[nodiscard]] json::NodeSpan extract_db(const json::Node& db, const macotec::MachineType& mach, std::vector<std::string>& issues)
{
    json::NodeSpan mach_db;
    if( const json::Node* const mach_db_root = db.get_child(mach.mach_id()) )
       {// Good, got an entry for this machine
        // Here expecting a structure like this:
        // root┐
        //     ├"common"-{nam=val,...}
        //     ├"cut-bridge"
        //     │  ├"dim1"-{nam=val,...}           ┌{nam=val,...}
        //     │  ├"dim2"-{nam=val,...}     =>    ├{nam=val,...}
        //     │  └···                            └{nam=val,...}
        //     └"algn-span"
        //       ├"dim1"-{nam=val,...}
        //       ├"dim2"-{nam=val,...}
        //       └···
        for( const auto& [child_id, child] : mach_db_root->childs() )
           {
            if( child.is_leaf() )
               {
                issues.push_back( fmt::format("DB: Ignoring orphan assignment {}:{} in {}", child_id, child.value(), mach.mach_id()) );
               }
            else if( child_id=="common" )
               {
                mach_db.emplace_back( child );
               }
            else if( mach.has_cutbridge_dim() && child_id=="cut-bridge" )
               {
                if( const json::Node* const cutbridge_db = child.get_child(mach.cutbridge_dim()) )
                   {
                    mach_db.emplace_back( *cutbridge_db );
                   }
                else
                   {
                    issues.push_back( fmt::format("DB: Cut bridge dimension {} not found in {}.{}", mach.cutbridge_dim(), mach.mach_id(), child_id) );
                   }
               }
            else if( mach.has_align_dim() && child_id=="algn-span" )
               {
                if( const json::Node* const alignspan_db = child.get_child(mach.align_dim()) )
                   {
                    mach_db.emplace_back( *alignspan_db );
                   }
                else
                   {
                    issues.push_back( fmt::format("DB: Align dimension {} not found in {}.{}", mach.align_dim(), mach.mach_id(), child_id) );
                   }
               }
            else
               {
                issues.push_back( fmt::format("DB: Ignoring unrecognized block {}.{}", mach.mach_id(), child_id) );
               }
           }
       }
    else
       {
        issues.push_back( fmt::format("DB: Machine id {} not found in the {} entries", mach.mach_id(), db.childs_count()) );
       }
    return mach_db;
}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


//---- end unit -------------------------------------------------------------
#endif
