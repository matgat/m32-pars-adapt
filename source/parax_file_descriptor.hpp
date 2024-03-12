#pragma once
//  ---------------------------------------------
//  Sipro par2kax.txt file descriptor
//  A list of name=value fields for each axis
//  ---------------------------------------------
//  #include "parax_file_descriptor.hpp" // parax::File
//  ---------------------------------------------
#include <map>
#include <format>

#include "string_utilities.hpp" // str::unquoted
#include "sipro_txt_parser.hpp" // sipro::TxtParser
#include "sipro_txt_file_descriptor.hpp" // sipro::TxtFile


namespace parax //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
class File final : public sipro::TxtFile
{
    using field_t = sipro::TxtField;
    using fields_t = std::map<std::string_view, field_t>;
    using blocks_t = std::map<std::string_view, fields_t>;

 private:
    blocks_t m_axblocks; // Axes fields

 public:
    explicit File(const std::string& pth, fnotify_t const& notify_issue)
      : sipro::TxtFile{pth}
       {
        parse( notify_issue );
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] fields_t* get_fields_of_axis(const std::string_view axid) noexcept
       {
        if( auto it=m_axblocks.find(axid); it!=m_axblocks.end() )
           {
            return &(it->second);
           }
        return nullptr;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] static const field_t* get_field_by_varname(const fields_t& fields, const std::string_view key) noexcept
       {
        if( auto it=fields.find(key); it!=fields.end() )
           {
            return &(it->second);
           }
        return nullptr;
       }
    [[nodiscard]] static field_t* get_field_by_varname(fields_t& fields, const std::string_view key) noexcept
       {
        if( auto it=fields.find(key); it!=fields.end() )
           {
            return &(it->second);
           }
        return nullptr;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::size_t modified_values_count() const noexcept
       {
        std::size_t count = 0;
        for( auto& [axid, axblock] : m_axblocks )
           {
            for( auto& [var_name, field] : axblock )
               {
                if( field.is_value_modified() ) ++count;
               }
           }
        return count;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::string info_string() const
       {
        return std::format("{} lines, {} axes", m_lines.size(), m_axblocks.size());
       }


 private:
    //-----------------------------------------------------------------------
    void parse(fnotify_t const& notify_issue)
       {
        sipro::TxtParser parser( buf() );
        parser.set_file_path( path() );
        parser.set_on_notify_issue(notify_issue);

        // A context for collecting axis fields
        class curr_ax_block_t final
        {
         private:
            std::string_view m_tagname;
            std::size_t m_line_idx = 0;
            fields_t m_collected_fields;
            bool m_inside = false;

         public:
            void start(const std::string_view nam, const std::size_t lin)
               {
                m_tagname = nam;
                m_line_idx = lin;
                m_collected_fields.clear();
                m_inside = true;
               }

            void end()
               {
                m_inside = false;
               }

            [[nodiscard]] explicit operator bool() const noexcept { return m_inside; }
            [[nodiscard]] std::string_view tag_name() const noexcept { return m_tagname; }
            [[nodiscard]] std::size_t line_idx() const noexcept { return m_line_idx; }
            [[nodiscard]] auto& collected_fields() noexcept { return m_collected_fields; }
        } curr_ax_block;

        while( const auto& line = parser.next_line() )
           {
            field_t* line_associated_field = nullptr;
            if( curr_ax_block )
               {// Collecting the fields of an axis
                if( parser.is_inside_note_block() )
                   {// Ignoring notes
                   }
                else if( line.is_end_tag() )
                   {// Detect Ax definition block end [End###Ax]
                    if( line.name()==curr_ax_block.tag_name() )
                       {
                        curr_ax_block.end();
                        if( curr_ax_block.collected_fields().empty() )
                           {
                            notify_issue( std::format("[{}:{}] No fields collected in axis block"sv, path(), curr_ax_block.line_idx()) );
                           }
                        // I need the axis name to store the collected fields
                        else if( const field_t* const ax_name_field = get_field_by_varname(curr_ax_block.collected_fields(),"Name") )
                           {
                            const std::string_view ax_name = str::unquoted(ax_name_field->value());
                            if( m_axblocks.contains(ax_name) )
                               {
                                notify_issue( std::format("[{}:{}] Duplicate axis name `{}`"sv, path(), curr_ax_block.line_idx(), ax_name) );
                               }
                            else
                               {
                                const auto [it, inserted] = m_axblocks.insert( {ax_name, std::move(curr_ax_block.collected_fields())} );
                                assert(curr_ax_block.collected_fields().empty()); // After move should be empty
                                if( not inserted )
                                   {
                                    notify_issue( std::format("[{}:{}] Axis block `{}` was not inserted"sv, path(), curr_ax_block.line_idx(), ax_name) );
                                   }
                               }
                           }
                        else
                           {
                            notify_issue( std::format("[{}:{}] `Name` not found in axis block"sv, path(), curr_ax_block.line_idx()) );
                           }
                       }
                    else if( line.name()!="Note"sv )
                       {
                        notify_issue( std::format("[{}:{}] Unexpected end tag `{}` inside axis block"sv, path(), m_lines.size()+1, line.name()) );
                       }
                   }
                else if( line.is_start_tag() )
                   {
                    notify_issue( std::format("[{}:{}] Unexpected start tag `{}` inside axis block"sv, path(), m_lines.size()+1, line.name()) );
                   }
                else if( line.is_assignment() )
                   {// Collect axis fields
                    if( curr_ax_block.collected_fields().contains(line.name()) )
                       {
                        notify_issue( std::format("[{}:{}] Duplicate field `{}`"sv, path(), m_lines.size()+1, line.name()) );
                       }
                    else
                       {
                        // Populate the map of fields
                        const auto [it, inserted] = curr_ax_block.collected_fields().try_emplace( line.name(), // key
                                                                                                  line.name(),
                                                                                                  line.value(),
                                                                                                  line.comment(),
                                                                                                  ""sv,
                                                                                                  m_lines.size() );
                        line_associated_field = &(it->second);
                        if( not inserted )
                           {
                            notify_issue( std::format("[{}:{}] Field `{}` was not inserted"sv, path(), m_lines.size()+1, line.name()) );
                           }
                       }
                   }
               }
            else
               {// Searching for a [Start###Ax] tag
                if( parser.is_inside_note_block() )
                   {// Ignoring notes
                   }
                else if( line.is_start_tag() and line.name().ends_with("Ax") )
                   {// Detect entering Ax definition block [Start###Ax]
                    curr_ax_block.start(line.name(), m_lines.size()+1);
                   }
                else if( line.is_assignment() )
                   {
                    notify_issue( std::format("[{}:{}] Unexpected field `{}` outside axis block"sv, path(), m_lines.size()+1, line.name()) );
                   }
               }

            // All lines are collected to reproduce the original file
            m_lines.emplace_back( line.content(), line_associated_field );
           }

        parser.check_unclosed_note_block();
       }
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::





/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include "issues_collector.hpp" // MG::issues
#include "string_write.hpp" // MG::string_write
/////////////////////////////////////////////////////////////////////////////
void check_field(parax::File& par, const std::string_view ax_name, const std::string_view var_name, const std::string_view value)
   {
    const auto* const ax_fields = par.get_fields_of_axis(ax_name);
    ut::expect( ut::fatal(ax_fields!=nullptr) ) << "Axis " << ax_name << " not found\n";
    const auto* const fld = par.get_field_by_varname(*ax_fields, var_name);
    ut::expect( ut::fatal(fld!=nullptr) ) << "Field " << var_name << " not found in axis " << ax_name << '\n';
    ut::expect( ut::that % fld->var_name()==var_name );
    ut::expect( ut::that % fld->value()==value );
    ut::expect( ut::that % fld->comment()==""sv );
    ut::expect( ut::that % fld->label()==""sv );
   }
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"parax_file_descriptor"> parax_file_descriptor_tests = []
{////////////////////////////////////////////////////////////////////////////

struct issues_t final { int num=0; void operator()(std::string&& msg) noexcept {++num; ut::log << msg << '\n';}; };

ut::test("basic") = []
   {
    test::TemporaryFile f_in("~test-parax.txt",
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
        "      Riscontri (Xr, AX_XR)\n"
        "    [EndNote]\n"
        "\n"
        "    AxId = 1\n"
        "    Name = \"Xr\"\n"
        "    TimeAcc = 0.3\n"
        "    TimeDec = 0.3\n"
        "    MinPos = -60\n"
        "    MaxPos = 4060\n"
        "  [EndEthercatAx]\n"
        "\n"
        "  [StartEthercatAx]\n"
        "    [StartNote]\n"
        "      Stacco (Xs, AX_XS)\n"
        "    [EndNote]\n"
        "\n"
        "    AxId = 2\n"
        "    Name = \"Xs\"\n"
        "    TimeAcc = 0.4\n"
        "    TimeDec = 0.3\n"
        "    MinPos = -95\n"
        "    MaxPos = 70\n"
        "  [EndEthercatAx]\n"
        "\n"
        "  [StartEthercatAx]\n"
        "    [StartNote]\n"
        "      Carrello (Y, AX_Y)\n"
        "    [EndNote]\n"
        "\n"
        "    AxId = 3\n"
        "    Name = \"Y\"\n"
        "    TimeAcc = 0.43\n"
        "    TimeDec = 0.3\n"
        "    MinPos = -300\n"
        "    MaxPos = 5186\n"
        "  [EndEthercatAx]\n"
        "\n"
        "\n"
        "  [StartEthercatAx]\n"
        "    [StartNote]\n"
        "      Orientazione Pinza (Zg, AX_ZG)\n"
        "    [EndNote]\n"
        "\n"
        "    AxId = 7\n"
        "    Name = \"Zg\"\n"
        "    TimeAcc = 0.4\n"
        "    TimeDec = 0.5\n"
        "    MinPos = 0\n"
        "    MaxPos = 0\n"
        "  [EndEthercatAx]\n"
        "\n"
        "[EndAxes]\n"sv);

    issues_t issues;
    parax::File par_file(f_in.path().string(), std::ref(issues));
    ut::expect( ut::that % issues.num==0 ) << "no issues expected\n";
    ut::expect( ut::that % par_file.info_string()=="63 lines, 4 axes"sv );

    check_field(par_file, "Xr"sv, "TimeAcc"sv, "0.3"sv);
    check_field(par_file, "Xr"sv, "TimeDec"sv, "0.3"sv);
    check_field(par_file, "Xr"sv, "MinPos"sv, "-60"sv);
    check_field(par_file, "Xr"sv, "MaxPos"sv, "4060"sv);
    check_field(par_file, "Xs"sv, "TimeAcc"sv, "0.4"sv);
    check_field(par_file, "Xs"sv, "TimeDec"sv, "0.3"sv);
    check_field(par_file, "Xs"sv, "MinPos"sv, "-95"sv);
    check_field(par_file, "Xs"sv, "MaxPos"sv, "70"sv);
    check_field(par_file, "Y"sv, "TimeAcc"sv, "0.43"sv);
    check_field(par_file, "Y"sv, "TimeDec"sv, "0.3"sv);
    check_field(par_file, "Y"sv, "MinPos"sv, "-300"sv);
    check_field(par_file, "Y"sv, "MaxPos"sv, "5186"sv);
    check_field(par_file, "Zg"sv, "TimeAcc"sv, "0.4"sv);
    check_field(par_file, "Zg"sv, "TimeDec"sv, "0.5"sv);
    check_field(par_file, "Zg"sv, "MinPos"sv, "0"sv);
    check_field(par_file, "Zg"sv, "MaxPos"sv, "0"sv);

    // Modify a field
    auto* const Xs_fields = par_file.get_fields_of_axis("Xs"sv);
    ut::expect( ut::fatal(Xs_fields!=nullptr) ) << "Xs axis not found\n";
    auto* const Xs_MinPos = par_file.get_field_by_varname(*Xs_fields, "MinPos");
    ut::expect( ut::fatal(Xs_MinPos!=nullptr) ) << "Field MinPos not found in axis Xs\n";

    const std::string_view new_val = "-123.456"sv;
    Xs_MinPos->modify_value(new_val);
    ut::expect( ut::that % par_file.modified_values_count()==1u );

    // Rewrite file
    test::TemporaryFile f_out("~test-parax-out.txt");
    par_file.write_to(f_out.path().string(), {});

    // Reparse written file
    parax::File par_file2(f_out.path().string(), std::ref(issues));
    ut::expect( ut::that % issues.num==0 ) << "no issues expected\n";
    ut::expect( ut::that % par_file2.info_string()=="64 lines, 4 axes"sv );

    check_field(par_file2, "Xr"sv, "TimeAcc"sv, "0.3"sv);
    check_field(par_file2, "Xr"sv, "TimeDec"sv, "0.3"sv);
    check_field(par_file2, "Xr"sv, "MinPos"sv, "-60"sv);
    check_field(par_file2, "Xr"sv, "MaxPos"sv, "4060"sv);
    check_field(par_file2, "Xs"sv, "TimeAcc"sv, "0.4"sv);
    check_field(par_file2, "Xs"sv, "TimeDec"sv, "0.3"sv);
    check_field(par_file2, "Xs"sv, "MinPos"sv, new_val);
    check_field(par_file2, "Xs"sv, "MaxPos"sv, "70"sv);
    check_field(par_file2, "Y"sv, "TimeAcc"sv, "0.43"sv);
    check_field(par_file2, "Y"sv, "TimeDec"sv, "0.3"sv);
    check_field(par_file2, "Y"sv, "MinPos"sv, "-300"sv);
    check_field(par_file2, "Y"sv, "MaxPos"sv, "5186"sv);
    check_field(par_file2, "Zg"sv, "TimeAcc"sv, "0.4"sv);
    check_field(par_file2, "Zg"sv, "TimeDec"sv, "0.5"sv);
    check_field(par_file2, "Zg"sv, "MinPos"sv, "0"sv);
    check_field(par_file2, "Zg"sv, "MaxPos"sv, "0"sv);
   };


ut::test("duplicate field name") = []
   {
    const std::string_view buf =
        "# Purposely inserted TimeAcc twice\n"
        "[StartTestAx]\n"
        "  Name = \"Ax1\"\n"
        "  TimeAcc = 0.3\n"
        "  TimeAcc = 0.1\n"
        "  MinPos = -60\n"
        "  MaxPos = 4060\n"
        "[EndTestAx]\n"sv;

    test::TemporaryFile f("~test-parax-dup-field.txt", buf);

    MG::issues issues;
    parax::File par_file(f.path().string(), std::ref(issues));
    ut::expect( ut::that % par_file.info_string()=="8 lines, 1 axes"sv );
    ut::expect( ut::that % issues.size()==1u ) << "one issue expected\n";
    ut::expect( issues.at(0).contains(":5] Duplicate field `TimeAcc`"sv) ) << issues.at(0);

    MG::string_write out;
    par_file.write_to(out, {}, ""sv);
    ut::expect( ut::that % out.str() == buf );
   };


ut::test("duplicate axis name") = []
   {
    const std::string_view buf =
        "[StartTestAx]\n"
        "  Name = \"Same\"\n"
        "  TimeAcc = 0.3\n"
        "  TimeDec = 0.3\n"
        "  MinPos = -60\n"
        "  MaxPos = 4060\n"
        "[EndTestAx]\n"
        "[StartTestAx]\n"
        "  Name = \"Same\"\n"
        "  TimeAcc = 0.3\n"
        "  TimeDec = 0.3\n"
        "  MinPos = -60\n"
        "  MaxPos = 4060\n"
        "[EndTestAx]\n"sv;

    test::TemporaryFile f("~test-parax-dup-axname.txt", buf);

    MG::issues issues;
    parax::File par_file(f.path().string(), std::ref(issues));
    ut::expect( ut::that % par_file.info_string()=="14 lines, 1 axes"sv );
    ut::expect( ut::that % issues.size()==1u ) << "one issue expected\n";
    ut::expect( issues.at(0).contains(":8] Duplicate axis name `Same`"sv) ) << issues.at(0);

    MG::string_write out;
    par_file.write_to(out, {}, ""sv);
    ut::expect( ut::that % out.str() == buf );
   };


ut::test("missing axis name") = []
   {
    const std::string_view buf =
        "# Purposely removed Name field\n"
        "[StartTestAx]\n"
        "  TimeAcc = 0.3\n"
        "  TimeDec = 0.3\n"
        "  MinPos = -60\n"
        "  MaxPos = 4060\n"
        "[EndTestAx]\n"sv;

    test::TemporaryFile f("~test-parax-no-axname.txt", buf);

    MG::issues issues;
    parax::File par_file(f.path().string(), std::ref(issues));
    ut::expect( ut::that % par_file.info_string()=="7 lines, 0 axes"sv );
    ut::expect( ut::that % issues.size()==1u ) << "one issue expected\n";
    ut::expect( issues.at(0).contains(":2] `Name` not found in axis block"sv) ) << issues.at(0);

    MG::string_write out;
    par_file.write_to(out, {}, ""sv);
    ut::expect( ut::that % out.str() == buf );
   };


ut::test("empty axis block") = []
   {
    const std::string_view buf =
        "[StartTestAx]\n"
        "  # Purposely empty\n"
        "[EndTestAx]\n"sv;
    test::TemporaryFile f("~test-parax-empty_block.txt", buf);

    MG::issues issues;
    parax::File par_file(f.path().string(), std::ref(issues));
    ut::expect( ut::that % par_file.info_string()=="3 lines, 0 axes"sv );
    ut::expect( ut::that % issues.size()==1u ) << "one issue expected\n";
    ut::expect( issues.at(0).contains(":1] No fields collected in axis block"sv) ) << issues.at(0);

    MG::string_write out;
    par_file.write_to(out, {}, ""sv);
    ut::expect( ut::that % out.str() == buf );
   };


ut::test("unexpected start tag") = []
   {
    const std::string_view buf =
        "[StartTestAx]\n"
        "  Name = \"Ax1\"\n"
        "  TimeAcc = 0.3\n"
        "  [StartSomething]\n"
        "  TimeDec = 0.3\n"
        "  MinPos = -60\n"
        "  MaxPos = 4060\n"
        "[EndTestAx]\n"sv;
    test::TemporaryFile f("~test-parax-unexp-start-tag.txt", buf);

    MG::issues issues;
    parax::File par_file(f.path().string(), std::ref(issues));
    ut::expect( ut::that % par_file.info_string()=="8 lines, 1 axes"sv );
    ut::expect( ut::that % issues.size()==1u ) << "one issue expected\n";
    ut::expect( issues.at(0).contains(":4] Unexpected start tag `Something` inside axis block"sv) ) << issues.at(0);

    MG::string_write out;
    par_file.write_to(out, {}, ""sv);
    ut::expect( ut::that % out.str() == buf );
   };


ut::test("unexpected end tag") = []
   {
    const std::string_view buf =
        "[StartTestAx]\n"
        "  Name = \"Ax1\"\n"
        "  TimeAcc = 0.3\n"
        "  [EndSomething]\n"
        "  TimeDec = 0.3\n"
        "  MinPos = -60\n"
        "  MaxPos = 4060\n"
        "[EndTestAx]\n"sv;
    test::TemporaryFile f("~test-parax-unexp-end-tag.txt", buf);

    MG::issues issues;
    parax::File par_file(f.path().string(), std::ref(issues));
    ut::expect( ut::that % par_file.info_string()=="8 lines, 1 axes"sv );
    ut::expect( ut::that % issues.size()==1u ) << "one issue expected\n";
    ut::expect( issues.at(0).contains(":4] Unexpected end tag `Something` inside axis block"sv) ) << issues.at(0);

    MG::string_write out;
    par_file.write_to(out, {}, ""sv);
    ut::expect( ut::that % out.str() == buf );
   };


ut::test("unexpected end tag") = []
   {
    const std::string_view buf =
        "[StartTestAx]\n"
        "  Name = \"Ax1\"\n"
        "  TimeAcc = 0.3\n"
        "  TimeDec = 0.3\n"
        "[EndTestAx]\n"
        "  Rogue = 123\n"
        "[StartTestAx]\n"
        "  Name = \"Ax2\"\n"
        "  TimeAcc = 0.3\n"
        "  TimeDec = 0.3\n"
        "[EndTestAx]\n"sv;
    test::TemporaryFile f("~test-parax-unexp-field.txt", buf);

    MG::issues issues;
    parax::File par_file(f.path().string(), std::ref(issues));
    ut::expect( ut::that % par_file.info_string()=="11 lines, 2 axes"sv );
    ut::expect( ut::that % issues.size()==1u ) << "one issue expected\n";
    ut::expect( issues.at(0).contains(":6] Unexpected field `Rogue` outside axis block"sv) ) << issues.at(0);

    MG::string_write out;
    par_file.write_to(out, {}, ""sv);
    ut::expect( ut::that % out.str() == buf );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
