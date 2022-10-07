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
//                                  │     ├"algn-span"
//                                  │     │ ├"dim1"-{nam=val,...}
//                                  │     │ ├"dim2"-{nam=val,...}
//                                  │     │ └···
//                                  │     ├"+option"-{nam=val,...}
//                                  │     └···
//                                  ├mach2┐
//                                  │     ├"common"─{nam=val,...}
//                                  │     ├"cut-bridge"
//                                  │     │  ├"dim1"-{nam=val,...}
//                                  │     │  ├"dim2"-{nam=val,...}
//                                  │     │  └···
//                                  │     ├"algn-span"
//                                  │     │ ├"dim1"-{nam=val,...}
//                                  │     │ ├"dim2"-{nam=val,...}
//                                  │     │ └···
//                                  │     ├"+option"-{nam=val,...}
//                                  │     └···
//                                  └···
// Will be extracted a list like this:   ┌{nam=val,...}
//                                       ├{nam=val,...}
//                                       └{nam=val,...}
[[nodiscard]] json::NodeSpan extract_db(const json::Node& db, const macotec::MachineType& mach, std::vector<std::string>& issues)
{
    json::NodeSpan mach_db;
    mach_db.reserve(5); // I expect typically 3 nodes plus possible options

    if( const json::Node* const mach_db_root = db.get_child(mach.family().id_string()) )
       {// Good, got an entry for this machine
        // Here expecting a structure like this:
        // root┐
        //     ├"common"-{nam=val,...}
        //     ├"cut-bridge"
        //     │  ├"dim1"-{nam=val,...}           ┌{nam=val,...}
        //     │  ├"dim2"-{nam=val,...}     =>    ├{nam=val,...}
        //     │  └···                            └{nam=val,...}
        //     ├"algn-span"
        //     │ ├"dim1"-{nam=val,...}
        //     │ ├"dim2"-{nam=val,...}
        //     │ └···
        //     ├"+option"-{nam=val,...}
        //     └···
        json::NodeSpan options_db; // Possible options are collected in a separate container

        for( const auto& [child_id, child] : mach_db_root->childs() )
           {
            if( child.is_leaf() )
               {
                issues.push_back( fmt::format("DB: Ignoring orphan assignment {}:{} in {}", child_id, child.value(), mach.family().id_string()) );
               }
            else if( child_id=="common" )
               {
                mach_db.emplace_back( child );
               }
            else if( child_id=="cut-bridge" && mach.has_cutbridge_dim() )
               {
                if( const json::Node* const cutbridge_db = child.get_child(mach.cutbridge_dim().id_string()) )
                   {
                    mach_db.emplace_back( *cutbridge_db );
                   }
                else
                   {
                    issues.push_back( fmt::format("DB: Cut bridge dimension {} not found in {}:{{{}}}", mach.cutbridge_dim().id_string(), mach.family().id_string(), child_id) );
                   }
               }
            else if( child_id=="algn-span" && mach.has_align_dim() )
               {
                if( const json::Node* const alignspan_db = child.get_child(mach.align_dim().id_string()) )
                   {
                    mach_db.emplace_back( *alignspan_db );
                   }
                else
                   {
                    issues.push_back( fmt::format("DB: Align dimension {} not found in {}:{{{}}}", mach.align_dim().id_string(), mach.family().id_string(), child_id) );
                   }
               }
            else if( child_id.starts_with('+') )
               {
                const std::string_view opt(child_id.begin()+1, child_id.end());
                if( mach.options().has_option(opt) )
                   {
                    options_db.emplace_back( child );
                   }
                //else issues.push_back( fmt::format("DB: Ignoring option \"{}\" for {}", opt, mach.string()) );
               }
            else
               {
                issues.push_back( fmt::format("DB: Ignoring unrecognized block {}:{{{}}}", mach.family().id_string(), child_id) );
               }
           }
        
        // Ensuring option nodes in the back in order to overwrite the values in other groups
        if( options_db.size()>0 )
           {
            mach_db.append(options_db);
           }
       }
    else
       {
        issues.push_back( fmt::format("DB: Machine id {} not found in the {} DB entries", mach.family().id_string(), db.childs_count()) );
       }
    return mach_db;
}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


//---- end unit -------------------------------------------------------------
#endif
