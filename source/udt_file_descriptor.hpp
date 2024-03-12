#pragma once
//  ---------------------------------------------
//  Sipro udt file descriptor
//  A list of name=value fields
//  ---------------------------------------------
//  #include "udt_file_descriptor.hpp" // udt::File
//  ---------------------------------------------
#include <map>
#include <format>

#include "string_similarity.hpp" // str::are_similar
#include "string_utilities.hpp" // str::trim_right()
#include "sipro_txt_parser.hpp" // sipro::TxtParser
#include "sipro_txt_file_descriptor.hpp" // sipro::TxtFile
#include "sipro.hpp" // sipro::Register


namespace udt //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
class File final : public sipro::TxtFile
{
    using field_t = sipro::TxtField;
    using fields_t = std::map<std::string_view, field_t>;

 private:
    fields_t m_fields;

 public:
    explicit File(const std::string& pth, fnotify_t const& notify_issue)
      : sipro::TxtFile{pth}
       {
        parse( notify_issue );
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] const field_t* get_field_by_label(const std::string_view varlbl) const noexcept
       {
        if( const auto it=m_fields.find(varlbl); it!=m_fields.end() )
           {
            return &(it->second);
           }
        return nullptr;
       }
    [[nodiscard]] field_t* get_field_by_label(const std::string_view varlbl) noexcept
       {
        if( const auto it=m_fields.find(varlbl); it!=m_fields.end() )
           {
            return &(it->second);
           }
        return nullptr;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::size_t modified_values_count() const noexcept
       {
        std::size_t count = 0;
        for( const auto& [varlbl, field] : m_fields )
           {
            if( field.is_value_modified() ) ++count;
           }
        return count;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::string info_string() const
       {
        return std::format("{} lines, {} fields", m_lines.size(), m_fields.size());
       }


    //-----------------------------------------------------------------------
    void overwrite_values_from(File const& other_file) noexcept
       {
        for( const auto& [his_varlbl, his_field] : other_file.m_fields )
           {
            if( his_varlbl == "vqMachSettingsVer"sv )
               {// Skipping: I'll keep my own value
               }
            else if( auto my_field = get_field_by_label(his_varlbl);
                     my_field!=nullptr )
               {// I have its field, update my value
                my_field->modify_value( his_field.value() );
               }
            // Field not found, detect possible renames
            else if( const auto [renmd_varlbl, renmd_field] = detect_rename_of(his_field);
                     renmd_field!=nullptr )
               {
                add_mod_issue( std::format("Renamed: {}={} => {}={} (verify)", his_varlbl, his_field.value(), renmd_varlbl, renmd_field->value()) );
                renmd_field->modify_value( his_field.value() );
               }
            else
               {
                add_mod_issue( std::format("Not found: {}={} (removed or renamed)", his_varlbl, his_field.value()) );
               }
           }
       }


 private:
    //-----------------------------------------------------------------------
    void parse(fnotify_t const& notify_issue)
       {
        std::map<std::string_view, std::string_view> var_names;
        sipro::TxtParser parser( buf() );
        parser.set_file_path( path() );
        parser.set_on_notify_issue(notify_issue);

        while( const auto& line = parser.next_line() )
           {
            field_t* line_associated_field = nullptr;

            if( line.is_assignment() )
               {
                const auto [cmt, lbl] = extract_comment_label( line.comment() );

                if( var_names.contains(line.name()) )
                   {
                    notify_issue( std::format("[{}:{}] Duplicate variable name `{}`"sv, path(), m_lines.size()+1, line.name()) );
                   }
                else
                   {
                    var_names.try_emplace(line.name(), lbl);
                   }

                if( lbl.empty() )
                   {
                    notify_issue( std::format("[{}:{}] Unlabeled variable `{}`"sv, path(), m_lines.size()+1, line.name()) );
                   }
                else if( m_fields.contains(lbl) )
                   {
                    notify_issue( std::format("[{}:{}] Duplicate variable label `{}`"sv, path(), m_lines.size()+1, lbl) );
                   }
                else
                   {
                    const auto [it, inserted] = m_fields.try_emplace( lbl, // key
                                                                      line.name(),
                                                                      line.value(),
                                                                      cmt,
                                                                      lbl,
                                                                      m_lines.size() );
                    line_associated_field = &(it->second);
                    if( not inserted )
                       {
                        notify_issue( std::format("[{}:{}] Field `{}` was not inserted"sv, path(), m_lines.size()+1, lbl) );
                       }
                   }
               }

            // All lines are collected to reproduce the original file
            m_lines.emplace_back( line.content(), line_associated_field );
           }

        parser.check_unclosed_note_block();
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::pair<std::string_view,field_t*> detect_rename_of(field_t const& his_field) noexcept
       {
        try{
            const sipro::Register his_reg(his_field.var_name());

            for( auto& [my_varlbl, my_field] : m_fields )
               {
                if( my_field.var_name() == his_field.var_name() ) // Stesso registro...
                   {
                    if( str::have_same_prefix(my_field.comment(), his_field.comment(), 3) and //...Stesso inizio commento (unità di misura)...
                        str::are_similar(my_field.comment(), his_field.comment(), 0.7) ) //...Commento piuttosto simile
                       {//...È una ridenominazione
                        return {my_varlbl, &my_field};
                       }
                   }
                else if( his_reg.is_valid() ) //...Il suo è un registro Sipro...
                   {
                    if( const sipro::Register my_reg(my_field.var_name());
                        my_reg.is_valid() ) //...Anche il mio è un registro Sipro...
                       {
                        const auto sim_threshold = [](const double delta) constexpr -> double
                           {// Parto da 0.7 e tendo verso 1.0 allontanandomi
                            return 1.0 - ( (1.0-0.7) / (1.0 + 0.05*(delta-1.0)) );
                           };
                        using idx_t = decltype(my_reg.index());
                        const auto calc_delta_idx = [](const idx_t idx1, const idx_t idx2) constexpr -> double
                           {
                            double diff = static_cast<double>(idx1) - static_cast<double>(idx2);
                            if(diff<0.0) diff = -diff;
                            return diff;
                           };
                        const double delta_idx = calc_delta_idx(my_reg.index(), his_reg.index());
                        if( are_same_type(my_reg,his_reg) and //...Registri dello stesso tipo...
                            delta_idx<20 and // ...L'indirizzo non è troppo lontano...
                            my_field.value() == his_field.value() and // ...Stesso letterale del valore...
                            str::have_same_prefix(my_field.comment(), his_field.comment(), 3) and //...Stesso inizio commento (unità di misura)...
                            str::are_similar(my_field.comment(), his_field.comment(), sim_threshold(delta_idx)) ) //...Commento simile in base a distanza registri...
                           {//...È una ridenominazione
                            return {my_varlbl, &my_field};
                           }
                       }
                   }
               }
           }
        catch(...){}
        return {{},nullptr};
       }
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::





/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include "issues_collector.hpp" // MG::issues
/////////////////////////////////////////////////////////////////////////////
void check_field(const udt::File& udt, const std::string_view var_name, const std::string_view value, const std::string_view comment, const std::string_view label)
   {
    const auto* const fld = udt.get_field_by_label(label);
    ut::expect( ut::fatal(fld!=nullptr) ) << "Field " << label << " not found\n";
    ut::expect( ut::that % fld->var_name()==var_name );
    ut::expect( ut::that % fld->value()==value );
    ut::expect( ut::that % fld->comment()==comment );
    ut::expect( ut::that % fld->label()==label );
   }
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"udt_file_descriptor"> udt_file_descriptor_tests = []
{////////////////////////////////////////////////////////////////////////////

struct issues_t final { int num=0; void operator()(std::string&& msg) noexcept {++num; ut::log << msg << '\n';}; };

ut::test("basic") = []
   {
    test::TemporaryFile f_in("~test.udt",
        "[StartUdt]\n"
        "\n"
        "  [StartVars]\n"
        "    [StartNote]\n"
        "    Parametri macchina\n"
        "    [EndNote]\n"
        "\n"
        "    # [Release]\n"
        "    vq91 = 34.9 # Versione file (2024-03) 'vqMachSettingsVer'\n"
        "    va30 = \"ActiveWR-4.0/4.6\" # Nome macchina 'vaMachName'\n"
        "    va31 = \"Base\" # Note rilascio 'vaRelNotes'\n"
        "\n"
        "\n"
        "    # [Opzioni]\n"
        "    vn1500 = 10 # Tipologia macchina (0:Dummy 1:F 2:FR 10:W/WR 11:HP) 'vnMach_Type'\n"
        "    vq1500 = -0.001 # Funzionalità abilitate 'vqEnabledSettings'\n"
        "    vn1600 = 7 # Modalità lavorazione abilitate 'vnEnabledModes'\n"
        "    vn1601 = 28 # Ricette abilitate 'vnEnabledRecipes'\n"
        "    vq2500 = 206.000 # Minimum needed firmware version 'vqMinFwVer'\n"
        "\n"
        "    # -Misure posizione fotocellule-\n"
        "    #    +---------------------+-------------++--+ +-------------+\n"
        "    #    |                     |             ||  |=| ___________ |\n"
        "    #    |FeedBegin     FeedEnd|      DtchBegin  DtchLast_______ |\n"
        "    #    |°    FeedNearEnd°   °|°BufBegin    |°  °=| ____°___°__°| OutZoneEnd\n"
        "    #    +---------------------+-------------+|--| +-------------+\n"
        "    vq1700 = 1 # [mm] Larghezza fotocellule per correzione lettura in rilascio 'vqDXph_Size'\n"
        "    vq1701 = -10575 # [mm] Ascissa fotocellula inizio carico 'vqXph_FeedBegin'\n"
        "\n"
        "\n"
        "    # [Finally]\n"
        "    vq1499 = 123456.789 # Controllo corruzione registri 'vqGuardMachPars'\n"
        "    vb30 = 1 # Settings read 'vbMachSettings'\n"
        "  [EndVars]\n"
        "\n"
        "[EndUdt]\n"sv);

    issues_t issues;
    udt::File udt_file(f_in.path().string(), std::ref(issues));
    ut::expect( ut::that % issues.num==0 ) << "no issues expected\n";
    ut::expect( ut::that % udt_file.info_string()=="36 lines, 12 fields"sv );

    check_field(udt_file, "vq91"sv, "34.9"sv, "Versione file (2024-03)"sv, "vqMachSettingsVer"sv);
    check_field(udt_file, "va30"sv, "\"ActiveWR-4.0/4.6\""sv, "Nome macchina"sv, "vaMachName"sv);
    check_field(udt_file, "va31"sv, "\"Base\""sv, "Note rilascio"sv, "vaRelNotes"sv);
    check_field(udt_file, "vn1500"sv, "10"sv, "Tipologia macchina (0:Dummy 1:F 2:FR 10:W/WR 11:HP)"sv, "vnMach_Type"sv);
    check_field(udt_file, "vq1500"sv, "-0.001"sv, "Funzionalità abilitate"sv, "vqEnabledSettings"sv);
    check_field(udt_file, "vn1600"sv, "7"sv, "Modalità lavorazione abilitate"sv, "vnEnabledModes"sv);
    check_field(udt_file, "vn1601"sv, "28"sv, "Ricette abilitate"sv, "vnEnabledRecipes"sv);
    check_field(udt_file, "vq2500"sv, "206.000"sv, "Minimum needed firmware version"sv, "vqMinFwVer"sv);
    check_field(udt_file, "vq1700"sv, "1"sv, "[mm] Larghezza fotocellule per correzione lettura in rilascio"sv, "vqDXph_Size"sv);
    check_field(udt_file, "vq1701"sv, "-10575"sv, "[mm] Ascissa fotocellula inizio carico"sv, "vqXph_FeedBegin"sv);
    check_field(udt_file, "vq1499"sv, "123456.789"sv, "Controllo corruzione registri"sv, "vqGuardMachPars"sv);
    check_field(udt_file, "vb30"sv, "1"sv, "Settings read"sv, "vbMachSettings"sv);

    // Modify a field
    auto* const vqEnabledSettings = udt_file.get_field_by_label("vqEnabledSettings"sv);
    ut::expect( ut::fatal(vqEnabledSettings!=nullptr) ) << "vqEnabledSettings not found\n";
    const std::string_view new_val = "1234.567"sv;
    vqEnabledSettings->modify_value(new_val);
    ut::expect( ut::that % udt_file.modified_values_count()==1u );

    // Rewrite file
    test::TemporaryFile f_out("~test-out.udt");
    udt_file.write_to(f_out.path().string(), {});

    // Reparse written file
    udt::File udt_file2(f_out.path().string(), std::ref(issues));
    ut::expect( ut::that % issues.num==0 ) << "no issues expected\n";
    ut::expect( ut::that % udt_file2.info_string()=="37 lines, 12 fields"sv );

    check_field(udt_file2, "vq91"sv, "34.9"sv, "Versione file (2024-03)"sv, "vqMachSettingsVer"sv);
    check_field(udt_file2, "va30"sv, "\"ActiveWR-4.0/4.6\""sv, "Nome macchina"sv, "vaMachName"sv);
    check_field(udt_file2, "va31"sv, "\"Base\""sv, "Note rilascio"sv, "vaRelNotes"sv);
    check_field(udt_file2, "vn1500"sv, "10"sv, "Tipologia macchina (0:Dummy 1:F 2:FR 10:W/WR 11:HP)"sv, "vnMach_Type"sv);
    check_field(udt_file2, "vq1500"sv, new_val, "Funzionalità abilitate"sv, "vqEnabledSettings"sv);
    check_field(udt_file2, "vn1600"sv, "7"sv, "Modalità lavorazione abilitate"sv, "vnEnabledModes"sv);
    check_field(udt_file2, "vn1601"sv, "28"sv, "Ricette abilitate"sv, "vnEnabledRecipes"sv);
    check_field(udt_file2, "vq2500"sv, "206.000"sv, "Minimum needed firmware version"sv, "vqMinFwVer"sv);
    check_field(udt_file2, "vq1700"sv, "1"sv, "[mm] Larghezza fotocellule per correzione lettura in rilascio"sv, "vqDXph_Size"sv);
    check_field(udt_file2, "vq1701"sv, "-10575"sv, "[mm] Ascissa fotocellula inizio carico"sv, "vqXph_FeedBegin"sv);
    check_field(udt_file2, "vq1499"sv, "123456.789"sv, "Controllo corruzione registri"sv, "vqGuardMachPars"sv);
    check_field(udt_file2, "vb30"sv, "1"sv, "Settings read"sv, "vbMachSettings"sv);
   };


ut::test("detect banal rename") = []
   {
    test::TemporaryFile f_old("~test-rename-old.udt", "vn100 = oldval # Comment 'Previous'\n"sv);
    test::TemporaryFile f_new("~test-rename-new.udt", "vn100 = newval # Comment mod 'Renamed'\n"sv);

    issues_t issues;
    udt::File udt_old(f_old.path().string(), std::ref(issues));
    udt::File udt_new(f_new.path().string(), std::ref(issues));
    ut::expect( ut::that % issues.num==0 ) << "no issues expected\n";
    ut::expect( ut::that % udt_old.info_string()=="1 lines, 1 fields"sv );
    ut::expect( ut::that % udt_new.info_string()=="1 lines, 1 fields"sv );

    const auto* const fld = udt_new.get_field_by_label("Renamed"sv);
    ut::expect( ut::fatal(fld!=nullptr) ) << "Field not found\n";
    ut::expect( ut::that % fld->value()=="newval"sv );

    udt_new.overwrite_values_from(udt_old);

    ut::expect( ut::that % udt_new.modified_values_count()==1u );
    ut::expect( ut::that % fld->value()=="oldval"sv );

    ut::expect( ut::fatal(udt_new.mod_issues().size()==1u) );
    ut::expect( ut::that % udt_new.mod_issues().back()=="Renamed: Previous=oldval => Renamed=newval (verify)"sv );
   };


ut::test("detect tricky rename") = []
   {
    test::TemporaryFile f_old("~test-tricky-old.udt", "vq1255 = val # [mm] Comment 'Previous'\n"sv);
    test::TemporaryFile f_new("~test-tricky-new.udt", "vq1262 = val # [mm] Comment2 'Renamed'\n"sv);

    issues_t issues;
    udt::File udt_old(f_old.path().string(), std::ref(issues));
    udt::File udt_new(f_new.path().string(), std::ref(issues));
    ut::expect( ut::that % issues.num==0 ) << "no issues expected\n";
    ut::expect( ut::that % udt_old.info_string()=="1 lines, 1 fields"sv );
    ut::expect( ut::that % udt_new.info_string()=="1 lines, 1 fields"sv );

    udt_new.overwrite_values_from(udt_old);

    ut::expect( ut::that % udt_new.modified_values_count()==1u );

    ut::expect( ut::fatal(udt_new.mod_issues().size()==1u) );
    ut::expect( ut::that % udt_new.mod_issues().back()=="Renamed: Previous=val => Renamed=val (verify)"sv );
   };


ut::test("unlabeled variable") = []
   {
    test::TemporaryFile f("~test-unlabeled.udt",
        "var1 = val1 # Comment 1 'Label1'\n"
        "var2 = val2 # Comment 2\n"
        "var3 = val3 # Comment 3 'Label3'\n"sv);

    MG::issues issues;
    udt::File udt_file(f.path().string(), std::ref(issues));
    ut::expect( ut::that % issues.size()==1u ) << "one issue expected\n";
    ut::expect( issues.at(0).contains(":2] Unlabeled variable `var2`"sv) ) << issues.at(0);
   };


ut::test("duplicate variable name") = []
   {
    test::TemporaryFile f("~test-duplicate.udt",
        "same = val1 # Comment 1 'Label1'\n"
        "var2 = val2 # Comment 2 'Label2'\n"
        "same = val3 # Comment 3 'Label3'\n"
        "var4 = val4 # Comment 4 'Label4'\n"sv);

    MG::issues issues;
    udt::File udt_file(f.path().string(), std::ref(issues));
    ut::expect( ut::that % issues.size()==1u ) << "one issue expected\n";
    ut::expect( issues.at(0).contains(":3] Duplicate variable name `same`"sv) ) << issues.at(0);
   };


ut::test("duplicate variable label") = []
   {
    test::TemporaryFile f("~test-duplicate.udt",
        "var1 = val1 # Comment 1 'Same'\n"
        "var2 = val2 # Comment 2 'Label2'\n"
        "var3 = val3 # Comment 3 'Same'\n"
        "var4 = val4 # Comment 4 'Label4'\n"sv);

    MG::issues issues;
    udt::File udt_file(f.path().string(), std::ref(issues));
    ut::expect( ut::that % issues.size()==1u ) << "one issue expected\n";
    ut::expect( issues.at(0).contains(":3] Duplicate variable label `Same`"sv) ) << issues.at(0);
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
