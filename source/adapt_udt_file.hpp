#pragma once
//  ---------------------------------------------
//  Adapt Sipro UDT file for a certain Macotec
//  machine type given parameters overlay
//  Or update an existing UDT given a newer one
//  ---------------------------------------------
//  #include "adapt_udt_file.hpp" // app::adapt_udt()
//  ---------------------------------------------
#include <string>
#include <format>

#include "string_utilities.hpp" // str::unquoted, str::quoted
#include "options_set.hpp" // MG::options_set
#include "macotec_parameters_database.hpp" // macotec::ParamsDB
#include "udt_file_descriptor.hpp" // udt::File


namespace app //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

inline constexpr std::string_view machname_field_label = "vaMachName"sv;


//---------------------------------------------------------------------------
template<typename FPRINT>
void adapt_udt( const std::string& target_file,
                const std::string& db_file,
                const std::string& out_path,
                macotec::MachineData mach_data,
                const MG::options_set& options,
                FPRINT const& verbose_print,
                fnotify_t const& notify_issue )
{
    // [The UDT file to adapt]
    udt::File udt_file(target_file, std::ref(notify_issue));

    if( mach_data )
       {// I have the machine data
        if( const auto vaMachName = udt_file.get_field_by_label(machname_field_label) )
           {
            // Better check if the target file has already superimposed options
            macotec::MachineData udt_mach_data;
            try{
                udt_mach_data.assign( str::unquoted(vaMachName->value()) );
               }
            catch( std::exception& e )
               {
                notify_issue( std::format("{} has an invalid vaMachName `{}`: {}", target_file, vaMachName->value(), e.what()) );
               }
            if( not udt_mach_data.options().is_empty() )
               {
                throw std::runtime_error( std::format("{} has already options: {}", target_file, udt_mach_data.options().string()) );
               }
            // Overwrite the specified machine type in the file
            vaMachName->modify_value( str::quoted(mach_data.string()) );
           }
        else
           {
            notify_issue( std::format("{} hasn't field vaMachName", target_file) );
           }
       }
    else
       {// Machine type not yet known, extract from udt file (not so useful)
        if( const auto vaMachName = udt_file.get_field_by_label(machname_field_label) )
           {
            mach_data.assign( str::unquoted(vaMachName->value()) );
           }
        else
           {
            throw std::runtime_error( std::format("Can't infer machine from: {}", target_file) );
           }
       }

    // [Parameters DB]
    const macotec::ParamsDB db{ db_file, notify_issue };
    const auto mach_udt_db = db.extract_udt_db_for(mach_data, notify_issue);

    verbose_print("  udt file: {}\n"
                  "  DB: {}\n",
                  udt_file.info_string(),
                  db.info_string());

    // Overwrite values from database
    for( const auto group_ref : mach_udt_db )
       {
        for( const auto& [nam, db_field] : group_ref.get().childs() )
           {
            if( not db_field.has_value() )
               {// All nodes at this level should be value fields
                notify_issue( std::format("Node {} hasn't a value in {}", nam, db_file) );
               }
            else if( const auto udt_field = udt_file.get_field_by_label(nam) )
               {
                udt_field->modify_value( db_field.value() );
               }
            else
               {
                udt_file.add_mod_issue( std::format("Not found: {}={}", nam, db_field.value()) );
               }
           }
       }

    verbose_print("  Modified {} values, {} issues\n", udt_file.modified_values_count(), udt_file.mod_issues().size());

    udt_file.write_to( out_path, options );
}


//---------------------------------------------------------------------------
[[nodiscard]] macotec::MachineData extract_mach_data_from(const udt::File& udt) noexcept
{
    macotec::MachineData mdat;
    if( const auto vaMachName = udt.get_field_by_label(machname_field_label) )
       {
        try{ mdat.assign( str::unquoted(vaMachName->value()) ); } catch(...){}
       }
    return mdat;
}

//---------------------------------------------------------------------------
[[nodiscard]] bool refer_to_same_machine(const udt::File& udt1, const udt::File& udt2) noexcept
{
    return extract_mach_data_from(udt1).family() ==
           extract_mach_data_from(udt2).family();
}


//---------------------------------------------------------------------------
template<typename FPRINT>
bool update_udt( const std::string& template_file,
                 const std::string& old_file,
                 const std::string& out_path,
                 const MG::options_set& options,
                 FPRINT const& verbose_print,
                 fnotify_t const& notify_issue )
{
    // [Machine type]
    // The machine type shouldn't be explicitly given

    // [The template UDT file (newest)]
    udt::File new_udt_file(template_file, notify_issue);

    // [The original UDT file to upgrade (oldest)]
    const udt::File old_udt_file(old_file, notify_issue);

    // Are both referring to the same machine type?
    const bool same_mach = refer_to_same_machine(old_udt_file, new_udt_file);

    verbose_print("  Old udt: {}\n"
                  "  New udt: {}\n",
                  old_udt_file.info_string(),
                  new_udt_file.info_string());

    // Overwrite values in newest file using the old as database
    new_udt_file.overwrite_values_from( old_udt_file );

    verbose_print("  Modified {} values, {} issues\n", new_udt_file.modified_values_count(), new_udt_file.mod_issues().size());

    new_udt_file.write_to( out_path, options );

    return same_mach;
}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::





/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"adapt_udt_file"> adapt_udt_file_tests = []
{////////////////////////////////////////////////////////////////////////////

struct issues_t final { int num=0; void operator()(std::string&& msg) noexcept {++num; ut::log << msg << '\n';}; };

ut::test("app::update_udt()") = []
   {
    //test::Directory tmp_dir("D:\\HD\\desktop\\~test-adapt_udt"); tmp_dir.create();
    test::TemporaryDirectory tmp_dir;

    const auto udt_old = tmp_dir.create_file("old.udt",
        "[StartNote]\n"
        "[EndNote]\n"
        "va0 = \"W\" # Mandatory field 'vaMachName'\n"
        "vn123 = old-type # Kept 'vnType'\n"
        "vn124 = old-type2 # Will be removed 'vnType2'\n"
        "\n"
        "vq1001 = old-opt # Will change varname 'vqOpt'\n"
        "vq1002 = old-opt2 # Kept 'vqOpt2'\n"
        "\n"
        "vq2500 = old-cut # Changed label 'vqCut'\n"
        "vq2501 = algn # Changed label and varname 'vqAlgn'\n"
        "\n"sv);

    const auto udt_new = tmp_dir.create_file("new.udt",
        "[StartNote]\n"
        "[EndNote]\n"
        "va0 = \"F\" # Mandatory field 'vaMachName'\n"
        "vn123 = new-type # Kept 'vnType'\n"
        "\n"
        "vq100x = new-opt # Changed varname 'vqOpt'\n"
        "vq1002 = new-opt2 # Kept 'vqOpt2'\n"
        "vq1003 = new-opt3 # Added 'vqOpt3'\n"
        "\n"
        "vq2500 = new-cut # Changed label and comment 'vqCutNew'\n"
        "vq2502 = algn # Changed label, varname and comment 'vqAlgnNew'\n"
        "\n"sv);

    const auto out = tmp_dir.decl_file("updated.udt");

    issues_t update_issues;
    const bool same_mach = app::update_udt( udt_new.path().string(), udt_old.path().string(), out.path().string(), {}, [](const std::string_view, const auto&...){}, std::ref(update_issues) );
    ut::expect( not same_mach );
    ut::expect( ut::that % update_issues.num==0 ) << "no issues expected\n";
    ut::expect( ut::fatal(fs::exists(out.path())) );
    
    // Expecting the following mod_issues:
    //    "! Not found: vnType2=old-type2 (removed or renamed)"
    //    "! Renamed: vqAlgn=algn => vqAlgnNew=algn (verify)"
    //    "! Renamed: vqCut=old-cut => vqCutNew=new-cut (verify)"

    issues_t reparse_issues;
    udt::File updated_udt(out.path().string(), std::ref(reparse_issues));
    ut::expect( ut::that % reparse_issues.num==0 ) << "no issues expected\n";
    ut::expect( ut::that % updated_udt.info_string()=="16 lines, 7 fields"sv );

    check_field(updated_udt, "va0"sv, "\"W\""sv, "Mandatory field"sv, "vaMachName"sv);
    check_field(updated_udt, "vn123"sv, "old-type"sv, "Kept"sv, "vnType"sv);
    check_field(updated_udt, "vq100x"sv, "old-opt"sv, "Changed varname"sv, "vqOpt"sv);
    check_field(updated_udt, "vq1002"sv, "old-opt2"sv, "Kept"sv, "vqOpt2"sv);
    check_field(updated_udt, "vq1003"sv, "new-opt3"sv, "Added"sv, "vqOpt3"sv);
    check_field(updated_udt, "vq2500"sv, "old-cut"sv, "Changed label and comment"sv, "vqCutNew"sv);
    check_field(updated_udt, "vq2502"sv, "algn"sv, "Changed label, varname and comment"sv, "vqAlgnNew"sv);
   };


ut::test("app::adapt_udt()") = []
   {
    //test::Directory tmp_dir("D:\\HD\\desktop\\~test-adapt_udt"); tmp_dir.create();
    test::TemporaryDirectory tmp_dir;

    const auto udt = tmp_dir.create_file("test.udt",
        "\xEF\xBB\xBF"
        "va0 = \"W\" # Mandatory field 'vaMachName'\n"
        "vn123 = 0 # Should be overwritten 'vnType'\n"
        "vn124 = 0 # Should remain unchanged 'vnType2'\n"
        "\n"
        "vq1000 = 0 # Should be overwritten 'vqOpt'\n"
        "vq1001 = 0 # Should remain unchanged 'vqOpt2'\n"
        "\n"
        "vq2500 = none # Should be overwritten 'vqCut'\n"
        "vq2501 = none # Should be overwritten 'vqAlgn'\n"
        "\n"sv);

    const auto db = tmp_dir.create_file("overlays.txt",
        "W,WR,HP: {\n"
        "    \"+buf-rot\": { vqOpt: 1 }\n"
        "    \"+fast\": { vqOpt: 2 }\n"
        "    \"+lowe\": { vqOpt: 3 }\n"
        "   }\n"
        "\n"
        "HP:{\n"
        "    \"common\": { vnType: 11 }\n"
        "    \"cut-bridge\" :\n"
        "       {\n"
        "        \"4.0\": { vqCut: hp-4.0 }\n"
        "        \"4.9\": { vqCut: hp-4.9 }\n"
        "        \"6.0\": { vqCut: hp-6.0 }\n"
        "       }\n"
        "    \"algn-span\" :\n"
        "       {\n"
        "        \"3.2\": { vqAlgn: hp-3.2 }\n"
        "        \"4.6\": { vqAlgn: hp-4.6 }\n"
        "       }\n"
        "   }\n"
        "WR:{\n"
        "    \"common\": { vnType: 10wr }\n"
        "    \"cut-bridge\" :\n"
        "       {\n"
        "        \"4.0\": { vqCut: wr-4.0 }\n"
        "        \"4.9\": { vqCut: wr-4.9 }\n"
        "        \"6.0\": { vqCut: wr-6.0 }\n"
        "       }\n"
        "    \"algn-span\" :\n"
        "       {\n"
        "        \"3.2\": { vqAlgn: wr-3.2 }\n"
        "        \"4.6\": { vqAlgn: wr-4.6 }\n"
        "       }\n"
        "   }\n"
        "W:{\n"
        "    \"common\": { vnType: 10 }\n"
        "    \"cut-bridge\" :\n"
        "       {\n"
        "        \"4.0\": { vqCut: w-4.0 }\n"
        "        \"4.9\": { vqCut: w-4.9 }\n"
        "        \"6.0\": { vqCut: w-6.0 }\n"
        "       }\n"
        "    \"algn-span\" :\n"
        "       {\n"
        "        \"3.2\": { vqAlgn: w-3.2 }\n"
        "        \"4.6\": { vqAlgn: w-4.6 }\n"
        "       }\n"
        "   }\n"sv);

    const auto out = tmp_dir.decl_file("adapted.udt");

    macotec::MachineData mach_data{"HP/6.0/4.6/(buf-rot,fast,other)"sv};

    issues_t adapt_issues;
    app::adapt_udt( udt.path().string(), db.path().string(), out.path().string(), mach_data, {}, [](const std::string_view, const auto&...){}, std::ref(adapt_issues) );
    ut::expect( ut::that % adapt_issues.num==0 ) << "no issues expected\n";
    ut::expect( ut::fatal(fs::exists(out.path())) );

    issues_t reparse_issues;
    udt::File adapted_udt(out.path().string(), std::ref(reparse_issues));
    ut::expect( ut::that % reparse_issues.num==0 ) << "no issues expected\n";
    ut::expect( ut::that % adapted_udt.info_string()=="10 lines, 7 fields"sv );

    check_field(adapted_udt, "va0"sv, "\"ActiveHP-6.0/4.6-(buf-rot,fast,other)\""sv, "Mandatory field"sv, "vaMachName"sv);
    check_field(adapted_udt, "vn123"sv, "11"sv, "Should be overwritten"sv, "vnType"sv);
    check_field(adapted_udt, "vn124"sv, "0"sv, "Should remain unchanged"sv, "vnType2"sv);
    check_field(adapted_udt, "vq1000"sv, "2"sv, "Should be overwritten"sv, "vqOpt"sv);
    check_field(adapted_udt, "vq1001"sv, "0"sv, "Should remain unchanged"sv, "vqOpt2"sv);
    check_field(adapted_udt, "vq2500"sv, "hp-6.0"sv, "Should be overwritten"sv, "vqCut"sv);
    check_field(adapted_udt, "vq2501"sv, "hp-4.6"sv, "Should be overwritten"sv, "vqAlgn"sv);
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
