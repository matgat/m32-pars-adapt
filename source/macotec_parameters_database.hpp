#pragma once
//  ---------------------------------------------
//  Parse json containing parameters organized by
//  machine type and extract the data pertinent
//  to a machine type assuming database structure
//  ---------------------------------------------
//  #include "macotec_parameters_database.hpp" // macotec::ParamsDB
//  ---------------------------------------------
#include <string>
#include <string_view>
#include <format>

#include "json_node.hpp" // json::Node
#include "json_parser.hpp" // json::parse()
#include "vectmap.hpp" // MG::vectmap<>
#include "memory_mapped_file.hpp" // sys::memory_mapped_file
#include "macotec_machine_data.hpp" // macotec::MachineData


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace macotec //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//----------------------------------------------------------------------------
[[nodiscard]] auto extract_mach_udt_db( const json::Node& db,
                                        const macotec::MachineData& mach,
                                        fnotify_t const& notify_issue )
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
    json::RefNodeList mach_db;

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
        json::RefNodeList options_db; // Possible options are collected in a separate container

        for( const auto& [child_id, child] : mach_db_root->childs() )
           {
            if( child.is_leaf() )
               {
                notify_issue( std::format("DB: Ignoring orphan field `{}:{}` in {}", child_id, child.value(), mach.family().id_string()) );
               }
            else if( child_id=="common" )
               {
                mach_db.append_ref_of( child );
               }
            else if( child_id=="cut-bridge" )
               {
                if( mach.has_cutbridge_dim() )
                   {
                    if( const json::Node* const cutbridge_db = child.get_child(mach.cutbridge_dim().string()) )
                       {
                        mach_db.append_ref_of( *cutbridge_db );
                       }
                    else
                       {
                        notify_issue( std::format("DB: Cut bridge dimension `{}` not found in {}:{{{}}}", mach.cutbridge_dim().string(), mach.family().id_string(), child_id) );
                       }
                   }
               }
            else if( child_id=="algn-span" )
               {
                if( mach.has_align_dim() )
                   {
                    if( const json::Node* const alignspan_db = child.get_child(mach.align_dim().string()) )
                       {
                        mach_db.append_ref_of( *alignspan_db );
                       }
                    else
                       {
                        notify_issue( std::format("DB: Align dimension `{}` not found in {}:{{{}}}", mach.align_dim().string(), mach.family().id_string(), child_id) );
                       }
                   }
               }
            else if( child_id.starts_with('+') )
               {
                const std::string_view opt(child_id.begin()+1, child_id.end());
                if( mach.options().has(opt) )
                   {
                    options_db.append_ref_of( child );
                    // Support dimensions here?
                    //if( mach.has_cutbridge_dim() and const json::Node* const cutbridge_db = child.get_child(mach.cutbridge_dim().string()) )
                    //   {
                    //   }
                    //if( mach.has_align_dim() and const json::Node* const alignspan_db = child.get_child(mach.align_dim().string()) )
                    //   {
                    //   }
                   }
               }
            else
               {
                notify_issue( std::format("DB: Ignoring unrecognized block {}:{{{}}}", mach.family().id_string(), child_id) );
               }
           }

        // Appending options last in order to overwrite the existing values
        if( options_db.size()>0 )
           {
            mach_db.append_refs(options_db);
           }
       }
    else
       {
        notify_issue( std::format("DB: Machine id `{}` not found in the {} DB entries", mach.family().id_string(), db.childs().size()) );
       }
    return mach_db;
}


//----------------------------------------------------------------------------
[[nodiscard]] auto extract_mach_parax_db( const json::Node& db,
                                          const macotec::MachineData& mach,
                                          fnotify_t const& notify_issue )
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
    MG::vectmap<std::string_view,json::RefNodeList> mach_db;

    const auto add_axfields_of = [&mach_db](const json::Node& node) -> void
       {
        for( const auto& [ax_id, ax_fields] : node.childs() )
           {
            json::RefNodeList& ax_nodes = mach_db.insert_if_missing(ax_id);
            ax_nodes.append_ref_of( ax_fields );
           }
       };

    if( const json::Node* const mach_db_root = db.get_child(mach.family().id_string()) )
       {// Good, got an entry for this machine
        json::RefNodeList options_db; // Possible options are collected in a separate container

        for( const auto& [child_id, child] : mach_db_root->childs() )
           {
            if( child.is_leaf() )
               {
                notify_issue( std::format("DB: Ignoring orphan field `{}:{}` in {}", child_id, child.value(), mach.family().id_string()) );
               }
            else if( child_id=="common" )
               {
                add_axfields_of(child);
               }
            else if( child_id=="cut-bridge" )
               {
                if( mach.has_cutbridge_dim() )
                   {
                    if( const json::Node* const cutbridge_db = child.get_child(mach.cutbridge_dim().string()) )
                       {
                        add_axfields_of(*cutbridge_db);
                       }
                    else
                       {
                        notify_issue( std::format("DB: Cut bridge dimension `{}` not found in {}:{{{}}}", mach.cutbridge_dim().string(), mach.family().id_string(), child_id) );
                       }
                   }
               }
            else if( child_id=="algn-span" )
               {
                if( mach.has_align_dim() )
                   {
                    if( const json::Node* const alignspan_db = child.get_child(mach.align_dim().string()) )
                       {
                        add_axfields_of(*alignspan_db);
                       }
                    else
                       {
                        notify_issue( std::format("DB: Align dimension `{}` not found in {}:{{{}}}", mach.align_dim().string(), mach.family().id_string(), child_id) );
                       }
                   }
               }
            else if( child_id.starts_with('+') )
               {
                const std::string_view opt(child_id.begin()+1, child_id.end());
                if( mach.options().has(opt) )
                   {
                    options_db.append_ref_of( child );
                   }
                //else notify_issue( std::format("DB: Ignoring option `{}` for {}", opt, mach.string()) );
               }
            else
               {
                notify_issue( std::format("DB: Ignoring unrecognized block {}:{{{}}}", mach.family().id_string(), child_id) );
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
        notify_issue( std::format("DB: Machine id `{}` not found in the {} DB entries", mach.family().id_string(), db.childs().size()) );
       }

    return mach_db;
}


/////////////////////////////////////////////////////////////////////////////
class ParamsDB final
{
 private:
    json::Node m_root;

 public:
    explicit ParamsDB(const std::string& pth, fnotify_t const& notify_issue)
       {
        const sys::memory_mapped_file dbfile_buf(pth.c_str());
        json::parse(pth, dbfile_buf.as_string_view(), m_root, notify_issue);
       }

    [[nodiscard]] auto extract_udt_db_for(const macotec::MachineData& mach, fnotify_t const& notify_issue ) const { return extract_mach_udt_db(m_root, mach, notify_issue); }
    [[nodiscard]] auto extract_parax_db_for(const macotec::MachineData& mach, fnotify_t const& notify_issue ) const { return extract_mach_parax_db(m_root, mach, notify_issue); }

    [[nodiscard]] const json::Node& root() const noexcept { return m_root; }
    [[nodiscard]] std::string info_string() const { return std::format("{} first level nodes ({} values)", m_root.childs().size(), m_root.total_values_count()); }
};




}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include "issues_collector.hpp" // MG::issues
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"macotec_parameters_database"> macotec_parameters_database_tests = []
{////////////////////////////////////////////////////////////////////////////

struct issues_t final { int num=0; void operator()(std::string&& msg) noexcept {++num; ut::log << msg << '\n';}; };

ut::test("macotec::ParamsDB::extract_udt_db_for()") = []
   {
    test::TemporaryFile f("~test-udt-db.txt",
        "WR,HP :\n"
        "   {\n"
        "    \"common\" :\n"
        "       {\n"
        "        com: wrhp\n"
        "        com2: 123\n"
        "       }\n"
        "\n"
        "    \"cut-bridge\" :\n"
        "       {\n"
        "        \"4.0\" :\n"
        "           {\n"
        "            wrhp-cut: 40\n"
        "           }\n"
        "\n"
        "        \"4.9\" :\n"
        "           {\n"
        "            wrhp-cut: 49\n"
        "           }\n"
        "\n"
        "        \"6.0\" :\n"
        "           {\n"
        "            wrhp-cut: 60\n"
        "           }\n"
        "       }\n"
        "\n"
        "    \"algn-span\" :\n"
        "       {\n"
        "        \"3.2\" :\n"
        "           {\n"
        "            wrhp-algn: 32\n"
        "           }\n"
        "\n"
        "        \"4.6\" :\n"
        "           {\n"
        "            wrhp-algn: 46\n"
        "           }\n"
        "       }\n"
        "   }\n"
        "\n"
        "\n"
        "W :\n"
        "   {\n"
        "    \"common\" :\n"
        "       {\n"
        "        com: w\n"
        "       }\n"
        "\n"
        "    \"cut-bridge\" :\n"
        "       {\n"
        "        \"4.0\" :\n"
        "           {\n"
        "            w-cut: 40\n"
        "           }\n"
        "\n"
        "        \"4.9\" :\n"
        "           {\n"
        "            w-cut: 49\n"
        "           }\n"
        "\n"
        "        \"6.0\" :\n"
        "           {\n"
        "            w-cut: 60\n"
        "           }\n"
        "       }\n"
        "\n"
        "    \"algn-span\" :\n"
        "       {\n"
        "        \"3.2\" :\n"
        "           {\n"
        "            wrhp-algn: 32\n"
        "           }\n"
        "\n"
        "        \"4.6\" :\n"
        "           {\n"
        "            wrhp-algn: 46\n"
        "           }\n"
        "       }\n"
        "   }\n"
        "\n"
        "W,WR,HP :\n"
        "   {\n"
        "    \"+lowe\" :\n"
        "       {\n"
        "        vnGrLe_Type: 2\n"
        "       }\n"
        "   }\n"sv);

    issues_t issues;
    const macotec::ParamsDB db{ f.path().string(), std::ref(issues) };
    ut::expect( ut::that % issues.num==0 ) << "no issues expected\n";
    ut::expect( ut::that % db.info_string()=="3 first level nodes (23 values)"sv );

    const macotec::MachineData mach{ "WR-4.9/4.6-(opp,lowe)"sv };
    const auto mach_udt_db = db.extract_udt_db_for(mach, std::ref(issues));
    ut::expect( ut::that % issues.num==0 ) << "no issues expected\n";

    // Four nodes: WR:algn-span:4.6, WR:common, WR:cut-bridge:4.9, WR:+lowe
    ut::expect( ut::that % mach_udt_db.size()==4u );

    std::size_t idx = 0;
    for( const auto group_ref : mach_udt_db )
       {
        ut::expect( ut::that % group_ref.get().childs().size()>0u );
        for( const auto& [nam, db_field] : group_ref.get().childs() )
           {
            ut::expect( db_field.has_value() ) << std::format("field {} hasn't a value\n", nam);

            switch(++idx)
               {
                case 1:
                    ut::expect( ut::that % nam=="wrhp-algn"sv );
                    ut::expect( ut::that % db_field.value()=="46"sv );
                    break;

                case 2:
                    ut::expect( ut::that % nam=="com"sv );
                    ut::expect( ut::that % db_field.value()=="wrhp"sv );
                    break;

                case 3:
                    ut::expect( ut::that % nam=="com2"sv );
                    ut::expect( ut::that % db_field.value()=="123"sv );
                    break;

                case 4:
                    ut::expect( ut::that % nam=="wrhp-cut"sv );
                    ut::expect( ut::that % db_field.value()=="49"sv );
                    break;

                case 5:
                    ut::expect( ut::that % nam=="vnGrLe_Type"sv );
                    ut::expect( ut::that % db_field.value()=="2"sv );
                    break;

                default:
                    ut::expect(false) << "unexpected field " << nam << "=" << db_field.value() << " at " << idx << '\n';
               }
           }
       }
   };


ut::test("macotec::ParamsDB::extract_parax_db_for()") = []
   {
    test::TemporaryFile f("~test-parax-db.txt",
        "W,WR,HP :\n"
        "   {\n"
        "    \"+opp\" :\n"
        "       {\n"
        "        \"Xr\" :\n"
        "           {\n"
        "            InvDir = 1\n"
        "           }\n"
        "       }\n"
        "\n"
        "    \"+lowe\" :\n"
        "       {\n"
        "        \"Sle\" :\n"
        "           {\n"
        "            AxEnabled = 1\n"
        "           }\n"
        "       }\n"
        "   }\n"
        "\n"
        "HP :\n"
        "   {\n"
        "    \"common\" :\n"
        "       {\n"
        "        \"Ysup\", \"Yinf\" :\n"
        "           {\n"
        "            MinPos = -100\n"
        "           }\n"
        "       }\n"
        "\n"
        "    \"cut-bridge\" :\n"
        "       {\n"
        "        \"4.0\" :\n"
        "           {\n"
        "            \"Ysup\", \"Yinf\" :\n"
        "               {\n"
        "                MaxPos = 4100\n"
        "               }\n"
        "           }\n"
        "\n"
        "        \"4.9\" :\n"
        "           {\n"
        "            \"Ysup\", \"Yinf\" :\n"
        "               {\n"
        "                MaxPos = 5100\n"
        "               }\n"
        "           }\n"
        "\n"
        "        \"6.0\" :\n"
        "           {\n"
        "            \"Ysup\", \"Yinf\" :\n"
        "               {\n"
        "                MaxPos = 6100\n"
        "               }\n"
        "           }\n"
        "       }\n"
        "\n"
        "    \"algn-span\" :\n"
        "       {\n"
        "        \"3.2\" :\n"
        "           {\n"
        "            \"Xr\" :\n"
        "               {\n"
        "                MaxPos = 2100\n"
        "               }\n"
        "           }\n"
        "\n"
        "        \"4.6\" :\n"
        "           {\n"
        "            \"Xr\" :\n"
        "               {\n"
        "                MaxPos = 3100\n"
        "               }\n"
        "           }\n"
        "       }\n"
        "   }\n"
        "\n"
        "\n"
        "W,WR :\n"
        "   {\n"
        "    \"common\" :\n"
        "       {\n"
        "        \"Ysup\", \"Yinf\" :\n"
        "           {\n"
        "            MinPos = -200\n"
        "           }\n"
        "       }\n"
        "\n"
        "    \"cut-bridge\" :\n"
        "       {\n"
        "        \"4.0\" :\n"
        "           {\n"
        "            \"Ysup\", \"Yinf\" :\n"
        "               {\n"
        "                MaxPos = 4200\n"
        "               }\n"
        "           }\n"
        "\n"
        "        \"4.9\" :\n"
        "           {\n"
        "            \"Ysup\", \"Yinf\" :\n"
        "               {\n"
        "                MaxPos = 5200\n"
        "               }\n"
        "           }\n"
        "\n"
        "        \"6.0\" :\n"
        "           {\n"
        "            \"Ysup\", \"Yinf\" :\n"
        "               {\n"
        "                MaxPos = 6200\n"
        "               }\n"
        "           }\n"
        "       }\n"
        "\n"
        "    \"algn-span\" :\n"
        "       {\n"
        "        \"3.2\" :\n"
        "           {\n"
        "            \"Xr\" :\n"
        "               {\n"
        "                MaxPos = 2200\n"
        "               }\n"
        "           }\n"
        "\n"
        "        \"4.6\" :\n"
        "           {\n"
        "            \"Xr\" :\n"
        "               {\n"
        "                MaxPos = 3200\n"
        "               }\n"
        "           }\n"
        "       }\n"
        "   }\n"
        "\n"
        "WR,HP :\n"
        "   {\n"
        "    \"common\" :\n"
        "       {\n"
        "        \"Co\" :\n"
        "           {\n"
        "            AxEnabled = 1\n"
        "           }\n"
        "       }\n"
        "}\n"
        "\n"
        "W :\n"
        "   {\n"
        "    \"common\" :\n"
        "       {\n"
        "        \"Co\" :\n"
        "           {\n"
        "            AxEnabled = 0\n"
        "           }\n"
        "       }\n"
        "   }\n"sv);

    issues_t issues;
    const macotec::ParamsDB db{ f.path().string(), std::ref(issues) };
    ut::expect( ut::that % issues.num==0 ) << "no issues expected\n";
    ut::expect( ut::that % db.info_string()=="3 first level nodes (39 values)"sv );

    const macotec::MachineData mach{ "WR-4.9/4.6-(lowe)"sv };
    const auto mach_parax_db = db.extract_parax_db_for(mach, std::ref(issues));
    ut::expect( ut::that % issues.num==0 ) << "no issues expected\n";

    // Five nodes: WR:algn-span:4.6:Xr, WR:common:Co, WR:cut-bridge:Yinf, WR:cut-bridge:Ysup, WR:+lowe:Sle
    ut::expect( ut::that % mach_parax_db.size()==5u );

    std::size_t idx = 0;
    for( const auto& [axid, db_axfields] : mach_parax_db )
       {
        for( const auto group_ref : db_axfields )
           {
            ut::expect( ut::that % group_ref.get().childs().size()>0u );
            for( const auto& [nam, db_field] : group_ref.get().childs() )
               {
                ut::expect( db_field.has_value() ) << std::format("Axis field {}.{} hasn't a value\n", axid, nam);

                switch(++idx)
                   {
                    case 1:
                        ut::expect( ut::that % axid=="Xr"sv );
                        ut::expect( ut::that % nam=="MaxPos"sv );
                        ut::expect( ut::that % db_field.value()=="3200"sv );
                        break;

                    case 2:
                        ut::expect( ut::that % axid=="Co"sv );
                        ut::expect( ut::that % nam=="AxEnabled"sv );
                        ut::expect( ut::that % db_field.value()=="1"sv );
                        break;

                    case 3:
                        ut::expect( ut::that % axid=="Yinf"sv );
                        ut::expect( ut::that % nam=="MinPos"sv );
                        ut::expect( ut::that % db_field.value()=="-200"sv );
                        break;

                    case 4:
                        ut::expect( ut::that % axid=="Yinf"sv );
                        ut::expect( ut::that % nam=="MaxPos"sv );
                        ut::expect( ut::that % db_field.value()=="5200"sv );
                        break;

                    case 5:
                        ut::expect( ut::that % axid=="Ysup"sv );
                        ut::expect( ut::that % nam=="MinPos"sv );
                        ut::expect( ut::that % db_field.value()=="-200"sv );
                        break;

                    case 6:
                        ut::expect( ut::that % axid=="Ysup"sv );
                        ut::expect( ut::that % nam=="MaxPos"sv );
                        ut::expect( ut::that % db_field.value()=="5200"sv );
                        break;

                    case 7:
                        ut::expect( ut::that % axid=="Sle"sv );
                        ut::expect( ut::that % nam=="AxEnabled"sv );
                        ut::expect( ut::that % db_field.value()=="1"sv );
                        break;

                    default:
                        ut::expect(false) << "unexpected " << axid << '.' << nam << "=" << db_field.value() << " at " << idx << '\n';
                   }
               }
           }
       }
   };

                                                           // ├F
ut::test("test udt orphan field") = []                     // │ ├common
   {                                                       // │ │ ┕ok:fval
    test::TemporaryFile f("~test-udt-db-issue-orphan.txt", // │ ┕ignored-orphan3:3
        "ignored-orphan0:0\n"                              // ├W
        "W:{ orphan1:1, common:{ok:wval} }\n"              // │ ├common
        "ignored-orphan2:2\n"                              // │ │ ┕ok:wval
        "F:{ ignored-orphan3:3, common:{ok:fval} }\n"sv);  // │ ┕orphan1:1
                                                           // ├ignored-orphan0:0
    MG::issues issues;                                     // ┕ignored-orphan2:2
    const macotec::ParamsDB db{ f.path().string(), std::ref(issues) };
    ut::expect( ut::that % issues.size()==0u ) << "no issues expected\n";
    ut::expect( ut::that % db.info_string()=="4 first level nodes (6 values)"sv );

    const macotec::MachineData mach{ "W-4.9/4.6"sv };
    const auto mach_udt_db = db.extract_udt_db_for(mach, std::ref(issues));
    ut::expect( ut::that % issues.size()==1u ) << "one issue expected\n";
    ut::expect( ut::that % issues.at(0)=="DB: Ignoring orphan field `orphan1:1` in W"sv );
   };


ut::test("test udt no dims") = []
   {
    test::TemporaryFile f("~test-udt-db-no-dims.txt",
        "W:{\n"
        "   common:{}\n"
        "   cut-bridge:{ a:{}, b:{} }\n"
        "   algn-span:{ a:{}, b:{} }\n"
        "  }\n"
        "F:{ common:{}, cut-bridge:{} }\n"sv);

    MG::issues issues;
    const macotec::ParamsDB db{ f.path().string(), std::ref(issues) };
    ut::expect( ut::that % issues.size()==0u ) << "no issues expected\n";
    ut::expect( ut::that % db.info_string()=="2 first level nodes (0 values)"sv );

    const macotec::MachineData mach{ "W-4.9/4.6"sv };
    [[maybe_unused]] const auto mach_udt_db = db.extract_udt_db_for(mach, std::ref(issues));
    ut::expect( ut::that % issues.size()==2u ) << "two issues expected\n";
    ut::expect( ut::that % issues.at(0)=="DB: Align dimension `4.6` not found in W:{algn-span}"sv );
    ut::expect( ut::that % issues.at(1)=="DB: Cut bridge dimension `4.9` not found in W:{cut-bridge}"sv );
   };

ut::test("test udt no mach") = []
   {
    test::TemporaryFile f("~test-udt-db-no-mach.txt",
        "W:{ common:{}, cut-bridge:{} }\n"
        "F:{ common:{}, cut-bridge:{} }\n"sv);

    MG::issues issues;
    const macotec::ParamsDB db{ f.path().string(), std::ref(issues) };
    ut::expect( ut::that % issues.size()==0u ) << "no issues expected\n";
    ut::expect( ut::that % db.info_string()=="2 first level nodes (0 values)"sv );

    const macotec::MachineData mach{ "WR-4.9/4.6"sv };
    [[maybe_unused]] const auto mach_udt_db = db.extract_udt_db_for(mach, std::ref(issues));
    ut::expect( ut::that % issues.size()==1u ) << "one issue expected\n";
    ut::expect( ut::that % issues.at(0)=="DB: Machine id `WR` not found in the 2 DB entries"sv );
   };

ut::test("test udt unrec block") = []
   {
    test::TemporaryFile f("~test-udt-db-unrec-block.txt",
        "W:{ common:{}, random:{}, cut-bridge:{ \"4.9\":{} } }\n"
        "F:{ common:{}, random2:{} cut-bridge:{} }\n"sv);

    MG::issues issues;
    const macotec::ParamsDB db{ f.path().string(), std::ref(issues) };
    ut::expect( ut::that % issues.size()==0u ) << "no issues expected\n";
    ut::expect( ut::that % db.info_string()=="2 first level nodes (0 values)"sv );

    const macotec::MachineData mach{ "W-4.9/4.6"sv };
    [[maybe_unused]] const auto mach_udt_db = db.extract_udt_db_for(mach, std::ref(issues));
    ut::expect( ut::that % issues.size()==1u ) << "one issue expected\n";
    ut::expect( ut::that % issues.at(0)=="DB: Ignoring unrecognized block W:{random}"sv );
   };


ut::test("test parax orphan field") = []
   {
    test::TemporaryFile f("~test-parax-db-issue-orphan.txt",
        "ignored-orphan0:0\n"
        "W:{ orphan1:1, common:{ok:wval} }\n"
        "ignored-orphan2:2\n"
        "F:{ ignored-orphan3:3, common:{ok:fval} }\n"sv);

    MG::issues issues;
    const macotec::ParamsDB db{ f.path().string(), std::ref(issues) };
    ut::expect( ut::that % issues.size()==0u ) << "no issues expected\n";
    ut::expect( ut::that % db.info_string()=="4 first level nodes (6 values)"sv );

    const macotec::MachineData mach{ "W-4.9/4.6"sv };
    [[maybe_unused]] const auto mach_parax_db = db.extract_parax_db_for(mach, std::ref(issues));
    ut::expect( ut::that % issues.size()==1u ) << "one issue expected\n";
    ut::expect( ut::that % issues.at(0)=="DB: Ignoring orphan field `orphan1:1` in W"sv );
   };


ut::test("test parax no dims") = []
   {
    test::TemporaryFile f("~test-parax-db-no-dims.txt",
        "W:{\n"
        "   common:{}\n"
        "   cut-bridge:{ a:{}, b:{} }\n"
        "   algn-span:{ a:{}, b:{} }\n"
        "  }\n"
        "F:{ common:{}, cut-bridge:{} }\n"sv);

    MG::issues issues;
    const macotec::ParamsDB db{ f.path().string(), std::ref(issues) };
    ut::expect( ut::that % issues.size()==0u ) << "no issues expected\n";
    ut::expect( ut::that % db.info_string()=="2 first level nodes (0 values)"sv );

    const macotec::MachineData mach{ "W-4.9/4.6"sv };
    [[maybe_unused]] const auto mach_parax_db = db.extract_parax_db_for(mach, std::ref(issues));
    ut::expect( ut::that % issues.size()==2u ) << "two issues expected\n";
    ut::expect( ut::that % issues.at(0)=="DB: Align dimension `4.6` not found in W:{algn-span}"sv );
    ut::expect( ut::that % issues.at(1)=="DB: Cut bridge dimension `4.9` not found in W:{cut-bridge}"sv );
   };

ut::test("test parax no mach") = []
   {
    test::TemporaryFile f("~test-parax-db-no-mach.txt",
        "W:{ common:{}, cut-bridge:{} }\n"
        "F:{ common:{}, cut-bridge:{} }\n"sv);

    MG::issues issues;
    const macotec::ParamsDB db{ f.path().string(), std::ref(issues) };
    ut::expect( ut::that % issues.size()==0u ) << "no issues expected\n";
    ut::expect( ut::that % db.info_string()=="2 first level nodes (0 values)"sv );

    const macotec::MachineData mach{ "WR-4.9/4.6"sv };
    [[maybe_unused]] const auto mach_parax_db = db.extract_parax_db_for(mach, std::ref(issues));
    ut::expect( ut::that % issues.size()==1u ) << "one issue expected\n";
    ut::expect( ut::that % issues.at(0)=="DB: Machine id `WR` not found in the 2 DB entries"sv );
   };

ut::test("test parax unrec block") = []
   {
    test::TemporaryFile f("~test-parax-db-unrec-block.txt",
        "W:{ common:{}, random:{}, cut-bridge:{ \"4.9\":{} } }\n"
        "F:{ common:{}, random2:{} cut-bridge:{} }\n"sv);

    MG::issues issues;
    const macotec::ParamsDB db{ f.path().string(), std::ref(issues) };
    ut::expect( ut::that % issues.size()==0u ) << "no issues expected\n";
    ut::expect( ut::that % db.info_string()=="2 first level nodes (0 values)"sv );

    const macotec::MachineData mach{ "W-4.9/4.6"sv };
    [[maybe_unused]] const auto mach_parax_db = db.extract_parax_db_for(mach, std::ref(issues));
    ut::expect( ut::that % issues.size()==1u ) << "one issue expected\n";
    ut::expect( ut::that % issues.at(0)=="DB: Ignoring unrecognized block W:{random}"sv );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
