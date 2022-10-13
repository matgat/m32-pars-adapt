#ifndef GUARD_extract_mach_db_hpp
#define GUARD_extract_mach_db_hpp
//  ---------------------------------------------
//  Assuming a certain DB structure, extract
//  data pertinent to a certain machine type
//  ---------------------------------------------
#include <map>
#include "json-node.hpp" // json::Node
#include "machine-type.hpp" // macotec::MachineType


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace macotec //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


//----------------------------------------------------------------------------
[[nodiscard]] auto extract_mach_udt_db( const json::Node& db,
                                        const macotec::MachineType& mach,
                                        std::vector<std::string>& issues )
// Extracts a list of node groups :      ┌{nam=val,...}
//                                       ├{nam=val,...}
// from this DB structure:    root┐      └{nam=val,...}
//                                ├mach┐
//                                │    ├"common"-{nam=val,...}
//                                │    ├"cut-bridge"
//                                │    │  ├"dim"-{nam=val,...}
//                                │    │  └···
//                                │    ├"algn-span"
//                                │    │ ├"dim"-{nam=val,...}
//                                │    │ └···
//                                │    ├"+option"-{nam=val,...}
//                                │    └···
//                                ├mach┐
//                                │    └···
//                                └···
{
    json::NodeSpan mach_db;
    mach_db.reserve(5); // I expect typically 3 nodes plus possible options

    if( const json::Node* const mach_db_root = db.get_child(mach.family().id_string()) )
       {// Good, got an entry for this machine:
        //  ┌"common"-{nam=val,...}
        //  ├"cut-bridge"
        //  │  ├"dim"-{nam=val,...}
        //  │  └···                              ┌{nam=val,...}
        //  ├"algn-span"                   =>    ├{nam=val,...}
        //  │ ├"dim"-{nam=val,...}               └{nam=val,...}
        //  │ └···
        //  ├"+option"-{nam=val,...}
        //  └···
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
            else if( child_id=="cut-bridge" )
               {
                if( mach.has_cutbridge_dim() )
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
               }
            else if( child_id=="algn-span" )
               {
                if( mach.has_align_dim() )
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

        // Appending options last in order to overwrite the existing values
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


//----------------------------------------------------------------------------
[[nodiscard]] auto extract_mach_parax_db( const json::Node& db,
                                          const macotec::MachineType& mach,
                                          std::vector<std::string>& issues )
// Extracts a map of node groups :       ┌"ax"-{nam=val,...},{nam=val,...},...
//                                       ├"ax"-{nam=val,...},{nam=val,...},...
// from this DB structure:   root┐       └"ax"-{nam=val,...},{nam=val,...},...
//                               ├mach┐
//                               │    ├"common"┐
//                               │    │        ├"ax"-{nam=val,...}
//                               │    │        └···
//                               │    ├"cut-bridge"┐
//                               │    │            ├"dim"┐
//                               │    │            │     ├"ax"-{nam=val,...}
//                               │    │            │     └···
//                               │    │            └···
//                               │    ├"algn-span"┐
//                               │    │           ├"dim"┐
//                               │    │           │     ├"ax"-{nam=val,...}
//                               │    │           │     └···
//                               │    │           └···
//                               │    ├"+option"┐
//                               │    │         ├"ax"-{nam=val,...}
//                               │    │         └···
//                               │    └···
//                               └···
{
    std::map<std::string_view,json::NodeSpan> mach_db;
    //const auto ensure_ax = [&mach_db](const std::string_view ax_id) -> json::NodeSpan&
    //   {
    //    const auto [it, ins] = mach_db.try_emplace(ax_id);
    //    if( ins )
    //       {// Axis entry just inserted
    //        it->second.reserve(5); // I expect for each axis typically 3 nodes plus possible options
    //       }
    //    return it->second;
    //   };
    const auto add_axfields_of = [&mach_db](const json::Node& node) -> void
       {
        for( const auto& [ax_id, ax_fields] : node.childs() )
           {
            // Ensure axis entry in map...
            const auto [it, ins] = mach_db.try_emplace(ax_id);
            if( ins )
               {// Axis entry just inserted
                it->second.reserve(5); // I expect for each axis typically 3 nodes plus possible options
               }
            // ...Then add this group
            it->second.emplace_back( ax_fields );
           }
       };

    if( const json::Node* const mach_db_root = db.get_child(mach.family().id_string()) )
       {// Good, got an entry for this machine
        json::NodeSpan options_db; // Possible options are collected in a separate container

        for( const auto& [child_id, child] : mach_db_root->childs() )
           {
            if( child.is_leaf() )
               {
                issues.push_back( fmt::format("DB: Ignoring orphan assignment {}:{} in {}", child_id, child.value(), mach.family().id_string()) );
               }
            else if( child_id=="common" )
               {
                add_axfields_of(child);
               }
            else if( child_id=="cut-bridge" )
               {
                if( mach.has_cutbridge_dim() )
                   {
                    if( const json::Node* const cutbridge_db = child.get_child(mach.cutbridge_dim().id_string()) )
                       {
                        add_axfields_of(*cutbridge_db);
                       }
                    else
                       {
                        issues.push_back( fmt::format("DB: Cut bridge dimension {} not found in {}:{{{}}}", mach.cutbridge_dim().id_string(), mach.family().id_string(), child_id) );
                       }
                   }
               }
            else if( child_id=="algn-span" )
               {
                if( mach.has_align_dim() )
                   {
                    if( const json::Node* const alignspan_db = child.get_child(mach.align_dim().id_string()) )
                       {
                        add_axfields_of(*alignspan_db);
                       }
                    else
                       {
                        issues.push_back( fmt::format("DB: Align dimension {} not found in {}:{{{}}}", mach.align_dim().id_string(), mach.family().id_string(), child_id) );
                       }
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

        // Appending options last in order to overwrite the existing values
        for( const auto group_ref : options_db )
           {
            add_axfields_of(group_ref.get());
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
