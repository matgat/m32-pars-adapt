#pragma once
//  ---------------------------------------------
//  Adapt Sipro par2kax.txt file for a certain
//  Macotec machine type given parameters overlay
//  ---------------------------------------------
//  #include "adapt_parax_file.hpp" // app::adapt_parax()
//  ---------------------------------------------
#include <string>
#include <format>

#include "options_set.hpp" // MG::options_set
#include "macotec_parameters_database.hpp" // macotec::ParamsDB
#include "parax_file_descriptor.hpp" // parax::File



namespace app //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
template<typename FPRINT>
void adapt_parax( const std::string& target_file,
                  const std::string& db_file,
                  const std::string& out_path,
                  const macotec::MachineData& mach_data,
                  const MG::options_set& options,
                  FPRINT const& verbose_print,
                  fnotify_t const& notify_issue )
{
    // [The parax file to adapt]
    parax::File parax_file(target_file, notify_issue);

    // [Parameters DB]
    const macotec::ParamsDB db{ db_file, notify_issue };
    const auto mach_parax_db = db.extract_parax_db_for(mach_data, notify_issue);

    verbose_print( "  parax file: {}\n"
                   "  DB: {}\n",
                   parax_file.info_string(),
                   db.info_string() );

    // Overwrite values from database
    for( const auto& [axid, db_axfields] : mach_parax_db )
       {
        if( const auto par_ax_fields = parax_file.get_fields_of_axis(axid) )
           {
            for( const auto group_ref : db_axfields )
               {
                for( const auto& [nam, db_field] : group_ref.get().childs() )
                   {
                    if( not db_field.has_value() )
                       {// All nodes at this level should be value fields
                        notify_issue( std::format("Axis field {}.{} hasn't a value in {}", axid, nam, db_file) );
                       }
                    else if( const auto par_field = parax_file.get_field_by_varname(*par_ax_fields,nam) )
                       {
                        par_field->modify_value( db_field.value() );
                       }
                    else
                       {
                        parax_file.add_mod_issue( std::format("Axis parameter not found: {}={}", nam, db_field.value()) );
                       }
                   }
               }
           }
        else
           {
            parax_file.add_mod_issue( std::format("Axis not found here: {}", axid) );
           }
       }

    verbose_print("  Modified {} values, {} issues\n", parax_file.modified_values_count(), parax_file.mod_issues().size());

    parax_file.write_to( out_path, options, std::format("Machine: {}", mach_data.string()) );
}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::





/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
static ut::suite<"adapt_parax_file"> adapt_parax_file_tests = []
{////////////////////////////////////////////////////////////////////////////

struct issues_t final { int num=0; void operator()(std::string&& msg) noexcept {++num; ut::log << msg << '\n';}; };

ut::test("app::adapt_parax()") = []
   {
    //test::Directory tmp_dir("D:\\HD\\desktop\\~test-adapt_parax"); tmp_dir.create();
    test::TemporaryDirectory tmp_dir;

    const auto parax = tmp_dir.create_file("parax.txt",
        "[StartAxes]\n"
        "  #Version 1.0.6.11602\n"
        "  #CanAdd StdAx\n"
        "\n"
        "  [StartNote]\n"
        "    Parametri Assi\n"
        "  [EndNote]\n"
        "\n"
        "\n"
        "  [StartEthercatAx]\n"
        "    [StartNote]\n"
        "      Riscontri\n"
        "    [EndNote]\n"
        "\n"
        "    AxId = 1\n"
        "    Name = \"Xr\"\n"
        "    AxEnabled = 1\n"
        "    InvDir = 0\n"
        "    TimeAcc = 0.3\n"
        "    TimeDec = 0.3\n"
        "    MinPos = -60\n"
        "    MaxPos = 4000\n"
        "  [EndEthercatAx]\n"
        "\n"
        "  [StartEthercatAx]\n"
        "    [StartNote]\n"
        "      Stacco\n"
        "    [EndNote]\n"
        "\n"
        "    AxId = 2\n"
        "    Name = \"Xs\"\n"
        "    AxEnabled = 1\n"
        "    InvDir = 0\n"
        "    TimeAcc = 0.4\n"
        "    TimeDec = 0.4\n"
        "    MinPos = -95\n"
        "    MaxPos = 70\n"
        "  [EndEthercatAx]\n"
        "\n"
        "  [StartEthercatAx]\n"
        "    [StartNote]\n"
        "      Carrello superiore\n"
        "    [EndNote]\n"
        "\n"
        "    AxId = 3\n"
        "    Name = \"Ysup\"\n"
        "    AxEnabled = 1\n"
        "    InvDir = 0\n"
        "    TimeAcc = 0.43\n"
        "    TimeDec = 0.3\n"
        "    MinPos = -500\n"
        "    MaxPos = 5000\n"
        "  [EndEthercatAx]\n"
        "\n"
        "  [StartEthercatAx]\n"
        "    [StartNote]\n"
        "      Carrello inferiore\n"
        "    [EndNote]\n"
        "\n"
        "    AxId = 4\n"
        "    Name = \"Yinf\"\n"
        "    AxEnabled = 1\n"
        "    InvDir = 0\n"
        "    TimeAcc = 0.43\n"
        "    TimeDec = 0.3\n"
        "    MinPos = -500\n"
        "    MaxPos = 5000\n"
        "  [EndEthercatAx]\n"
        "\n"
        "  [StartEthercatAx]\n"
        "    [StartNote]\n"
        "      Cinghie fine linea\n"
        "    [EndNote]\n"
        "\n"
        "    AxId = 14\n"
        "    Name = \"Co\"\n"
        "    AxEnabled = 0\n"
        "    InvDir = 0\n"
        "    TimeAcc = 0.8\n"
        "    TimeDec = 0.8\n"
        "    MinPos = 0\n"
        "    MaxPos = 0\n"
        "  [EndEthercatAx]\n"
        "\n"
        "  [StartEthercatAx]\n"
        "    [StartNote]\n"
        "      Mandrino mola\n"
        "    [EndNote]\n"
        "\n"
        "    AxId = 21\n"
        "    Name = \"Sle\"\n"
        "    AxEnabled = 0\n"
        "    InvDir = 0\n"
        "    TimeAcc = 0.5\n"
        "    TimeDec = 0.5\n"
        "    MinPos = 0\n"
        "    MaxPos = 0\n"
        "  [EndEthercatAx]\n"
        "\n"
        "[EndAxes]\n"sv);

    const auto db = tmp_dir.create_file("overlays.txt",
        "W,WR,HP :\n"
        "   {\n"
        "    +opp:\n"
        "       {\n"
        "        Ysup, Yinf: { InvDir = 1 }\n"
        "       }\n"
        "    +lowe:\n"
        "       {\n"
        "        Sle: { AxEnabled = 1 }\n"
        "       }\n"
        "   }\n"
        "\n"
        "HP :\n"
        "   {\n"
        "    common:\n"
        "       {\n"
        "        Ysup, Yinf: { MinPos = -100 }\n"
        "       }\n"
        "    cut-bridge:\n"
        "       {\n"
        "        4.0:\n"
        "           {\n"
        "            Ysup, Yinf: { MaxPos = 4100 }\n"
        "           }\n"
        "        4.9:\n"
        "           {\n"
        "            Ysup, Yinf: { MaxPos = 5100 }\n"
        "           }\n"
        "        6.0:\n"
        "           {\n"
        "            Ysup, Yinf: { MaxPos = 6100 }\n"
        "           }\n"
        "       }\n"
        "\n"
        "    algn-span:\n"
        "       {\n"
        "        3.2:\n"
        "           {\n"
        "            Xr: { MaxPos = 2100 }\n"
        "           }\n"
        "        4.6:\n"
        "           {\n"
        "            Xr: { MaxPos = 3100 }\n"
        "           }\n"
        "       }\n"
        "   }\n"
        "\n"
        "W,WR:\n"
        "   {\n"
        "    common:\n"
        "       {\n"
        "        Ysup, Yinf: { MinPos = -200 }\n"
        "       }\n"
        "    cut-bridge:\n"
        "       {\n"
        "        4.0:\n"
        "           {\n"
        "            Ysup, Yinf: { MaxPos = 4200 }\n"
        "           }\n"
        "        4.9:\n"
        "           {\n"
        "            Ysup, Yinf: { MaxPos = 5200 }\n"
        "           }\n"
        "        6.0:\n"
        "           {\n"
        "            Ysup, Yinf: { MaxPos = 6200 }\n"
        "           }\n"
        "       }\n"
        "\n"
        "    algn-span:\n"
        "       {\n"
        "        3.2:\n"
        "           {\n"
        "            Xr: {  MaxPos = 2200 }\n"
        "           }\n"
        "        4.6:\n"
        "           {\n"
        "            Xr: { MaxPos = 3200 }\n"
        "           }\n"
        "       }\n"
        "   }\n"
        "\n"
        "WR,HP:\n"
        "   {\n"
        "    common:\n"
        "       {\n"
        "        Co: { AxEnabled = 1 }\n"
        "       }\n"
        "}\n"
        "\n"
        "W :\n"
        "   {\n"
        "    common:\n"
        "       {\n"
        "        Co: { AxEnabled = 0 }\n"
        "       }\n"
        "   }\n"sv);

    const auto out = tmp_dir.decl_file("adapted_parax.txt");

    macotec::MachineData mach_data{"HP*6.0*4.6*(opp,other)"sv};

    issues_t adapt_issues;
    app::adapt_parax( parax.path().string(), db.path().string(), out.path().string(), mach_data, {}, [](const std::string_view, const auto&...){}, std::ref(adapt_issues) );
    ut::expect( ut::that % adapt_issues.num==0 ) << "no issues expected\n";
    ut::expect( ut::fatal(fs::exists(out.path())) );

    issues_t reparse_issues;
    parax::File adapted_parax(out.path().string(), std::ref(reparse_issues));
    ut::expect( ut::that % reparse_issues.num==0 ) << "no issues expected\n";
    ut::expect( ut::that % adapted_parax.info_string()=="102 lines, 6 axes"sv );

    check_field(adapted_parax, "Xr"sv, "AxId"sv, "1"sv);
    check_field(adapted_parax, "Xr"sv, "Name"sv, "\"Xr\""sv);
    check_field(adapted_parax, "Xr"sv, "AxEnabled"sv, "1"sv);
    check_field(adapted_parax, "Xr"sv, "InvDir"sv, "0"sv);
    check_field(adapted_parax, "Xr"sv, "TimeAcc"sv, "0.3"sv);
    check_field(adapted_parax, "Xr"sv, "TimeDec"sv, "0.3"sv);
    check_field(adapted_parax, "Xr"sv, "MinPos"sv, "-60"sv);
    check_field(adapted_parax, "Xr"sv, "MaxPos"sv, "3100"sv);

    check_field(adapted_parax, "Xs"sv, "AxId"sv, "2"sv);
    check_field(adapted_parax, "Xs"sv, "Name"sv, "\"Xs\""sv);
    check_field(adapted_parax, "Xs"sv, "AxEnabled"sv, "1"sv);
    check_field(adapted_parax, "Xs"sv, "InvDir"sv, "0"sv);
    check_field(adapted_parax, "Xs"sv, "TimeAcc"sv, "0.4"sv);
    check_field(adapted_parax, "Xs"sv, "TimeDec"sv, "0.4"sv);
    check_field(adapted_parax, "Xs"sv, "MinPos"sv, "-95"sv);
    check_field(adapted_parax, "Xs"sv, "MaxPos"sv, "70"sv);

    check_field(adapted_parax, "Ysup"sv, "AxId"sv, "3"sv);
    check_field(adapted_parax, "Ysup"sv, "Name"sv, "\"Ysup\""sv);
    check_field(adapted_parax, "Ysup"sv, "AxEnabled"sv, "1"sv);
    check_field(adapted_parax, "Ysup"sv, "InvDir"sv, "1"sv);
    check_field(adapted_parax, "Ysup"sv, "TimeAcc"sv, "0.43"sv);
    check_field(adapted_parax, "Ysup"sv, "TimeDec"sv, "0.3"sv);
    check_field(adapted_parax, "Ysup"sv, "MinPos"sv, "-100"sv);
    check_field(adapted_parax, "Ysup"sv, "MaxPos"sv, "6100"sv);

    check_field(adapted_parax, "Yinf"sv, "AxId"sv, "4"sv);
    check_field(adapted_parax, "Yinf"sv, "Name"sv, "\"Yinf\""sv);
    check_field(adapted_parax, "Yinf"sv, "AxEnabled"sv, "1"sv);
    check_field(adapted_parax, "Yinf"sv, "InvDir"sv, "1"sv);
    check_field(adapted_parax, "Yinf"sv, "TimeAcc"sv, "0.43"sv);
    check_field(adapted_parax, "Yinf"sv, "TimeDec"sv, "0.3"sv);
    check_field(adapted_parax, "Yinf"sv, "MinPos"sv, "-100"sv);
    check_field(adapted_parax, "Yinf"sv, "MaxPos"sv, "6100"sv);

    check_field(adapted_parax, "Co"sv, "AxId"sv, "14"sv);
    check_field(adapted_parax, "Co"sv, "Name"sv, "\"Co\""sv);
    check_field(adapted_parax, "Co"sv, "AxEnabled"sv, "1"sv);
    check_field(adapted_parax, "Co"sv, "InvDir"sv, "0"sv);
    check_field(adapted_parax, "Co"sv, "TimeAcc"sv, "0.8"sv);
    check_field(adapted_parax, "Co"sv, "TimeDec"sv, "0.8"sv);
    check_field(adapted_parax, "Co"sv, "MinPos"sv, "0"sv);
    check_field(adapted_parax, "Co"sv, "MaxPos"sv, "0"sv);

    check_field(adapted_parax, "Sle"sv, "AxId"sv, "21"sv);
    check_field(adapted_parax, "Sle"sv, "Name"sv, "\"Sle\""sv);
    check_field(adapted_parax, "Sle"sv, "AxEnabled"sv, "0"sv);
    check_field(adapted_parax, "Sle"sv, "InvDir"sv, "0"sv);
    check_field(adapted_parax, "Sle"sv, "TimeAcc"sv, "0.5"sv);
    check_field(adapted_parax, "Sle"sv, "TimeDec"sv, "0.5"sv);
    check_field(adapted_parax, "Sle"sv, "MinPos"sv, "0"sv);
    check_field(adapted_parax, "Sle"sv, "MaxPos"sv, "0"sv);
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
