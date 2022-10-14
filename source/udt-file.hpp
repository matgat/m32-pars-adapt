#ifndef GUARD_udt_file_hpp
#define GUARD_udt_file_hpp
//  ---------------------------------------------
//  Sipro udt file descriptor
//  Consists in a list of name=value assignments
//  ---------------------------------------------
#include <stdexcept>
#include <string_view>
#include <map>
#include <fmt/core.h> // fmt::format

#include "system.hpp" // sys::MemoryMappedFile, sys::file_write
#include "string-similarity.hpp" // str::calc_similarity_sorensen
#include "time-stamp.hpp" // sys::get_formatted_time_stamp()
#include "sipro-parser.hpp" // sipro::Parser


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace udt //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


/////////////////////////////////////////////////////////////////////////////
class File final
{
    /////////////////////////////////////////////////////////////////////////
    class Assignment final
       {
        public:
            explicit Assignment(const sipro::Assignment a, const std::size_t l) : i_Assignment(a), i_LineIdx(l) {}

            [[nodiscard]] std::string_view var_name() const noexcept { return i_Assignment.var_name(); }
            [[nodiscard]] std::string_view value() const noexcept { return i_NewVal.empty() ? i_Assignment.value() : i_NewVal; }
            [[nodiscard]] std::string_view comment() const noexcept { return i_Assignment.comment(); }
            [[nodiscard]] std::string_view added_label() const noexcept { return i_Assignment.added_label(); }

            [[nodiscard]] bool is_value_modified() const noexcept { return !i_NewVal.empty(); }
            [[nodiscard]] std::string_view modified_value() const noexcept { return i_NewVal; }
            void modify_value(const std::string_view new_val) { i_NewVal = new_val; }

            [[nodiscard]] std::size_t line_index() const noexcept { return i_LineIdx; }

        private:
            sipro::Assignment i_Assignment;
            std::size_t i_LineIdx;
            std::string i_NewVal;
       };

    /////////////////////////////////////////////////////////////////////////
    class Line final
       {
        public:
            explicit Line(const std::string_view l, Assignment* const pa) : i_LineSpan(l), i_AssignmentPtr(pa) {}

            [[nodiscard]] std::string_view span() const noexcept { return i_LineSpan; }

            [[nodiscard]] const Assignment* assignment_ptr() const noexcept { return i_AssignmentPtr; }
            [[nodiscard]] Assignment* assignment_ptr() noexcept { return i_AssignmentPtr; }

        private:
            std::string_view i_LineSpan;
            Assignment* i_AssignmentPtr;
       };

 public:
    explicit File(const fs::path& pth, std::vector<std::string>& issues)
      : file_buf{pth.string()}
       {
        std::vector<std::string> parse_issues;
        sipro::Parser parser(file_buf.path(), file_buf.as_string_view(), parse_issues, true);

        while( parser.has_data() )
           {
            const sipro::Line line = parser.next_line();
            Assignment* stored_asgnm_ptr = nullptr; // Possible assignment associated to this line

            if( parser.is_inside_note_block() )
               {// Ignoring notes
               }
            else if( line.tag() )
               {// Ignoring tags
               }
            else if( line.assignment() )
               {// Collect assignment
                if( line.assignment().added_label().empty() )
                   {
                    parse_issues.push_back( fmt::format("Unlabeled variable {} at line {}"sv, line.assignment().var_name(), i_lines.size()+1) );
                   }
                else if( i_assignments.contains(line.assignment().added_label()) )
                   {
                    parse_issues.push_back( fmt::format("Duplicate variable {} at line {}"sv, line.assignment().added_label(), i_lines.size()+1) );
                   }
                else
                   {
                    // Populate the map of assignments
                    const auto [it, inserted] = i_assignments.try_emplace(line.assignment().added_label(), line.assignment(), i_lines.size());
                    stored_asgnm_ptr = &(it->second); // Pointer to the constructed assignment to be associated with the collected line
                    if( !inserted )
                       {
                        parse_issues.push_back( fmt::format("Assignment of {} at line {} was not inserted"sv, line.assignment().added_label(), i_lines.size()+1) );
                       }
                   }
               }

            // All lines are collected to reproduce the original file
            i_lines.emplace_back( line.span(), stored_asgnm_ptr );
           }

        // Append parsing issues to overall issues list
        if( !parse_issues.empty() )
           {
            const std::string prefix{ pth.filename().string() };
            for(const auto& issue_entry : parse_issues )
               {
                // cppcheck-suppress useStlAlgorithm
                issues.push_back( fmt::format("[{}]: {}", prefix , issue_entry) );
               }
           }
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] Assignment* get_field(const std::string_view varlbl) noexcept
       {
        if( const auto it=i_assignments.find(varlbl); it!=i_assignments.end() )
           {
            return &(it->second);
           }
        return nullptr;
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] Assignment* detect_rename_of(const Assignment& his_assgnm) noexcept
       {
        try{
            for( auto& [my_varlbl, my_assgnm] : i_assignments )
               {
                // Deve corrispondere l'indirizzo del registro
                if( my_assgnm.var_name() == his_assgnm.var_name() )
                   {
                    // Se il commento è abbastanza simile, è lui!
                    const auto comment_similarity = str::calc_similarity_sorensen(my_assgnm.comment(), his_assgnm.comment());
                    assert( comment_similarity>=0.0 && comment_similarity<=1.0 );
                    if( comment_similarity > 0.85 ) return &my_assgnm;
                   }
                // Potrei essere più tollerante accettando anche registri indici dello
                // stesso tipo con indici vicini, ma meglio evitare falsi positivi
               }
           } catch(...){}
        return nullptr;
       }

    //-----------------------------------------------------------------------
    //[[nodiscard]] std::string_view get_value_of(const std::string_view varlbl) const
    //   {
    //    if( const auto it=i_assignments.find(varlbl); it!=i_assignments.end() )
    //       {
    //        return it->second.value();
    //       }
    //    throw std::runtime_error( fmt::format("Variable {} not found in {}", varlbl, file_buf.path()) );
    //   }

    //-----------------------------------------------------------------------
    //void modify_value_if_present(const std::string_view varlbl, const std::string& new_val) noexcept
    //   {
    //    if( const auto it = i_assignments.find(varlbl); it!=i_assignments.end() )
    //       {
    //        it->second.modify_value(new_val);
    //       }
    //   }

    //-----------------------------------------------------------------------
    void overwrite_values_from(const File& other_file) noexcept
       {
        for( const auto& [his_varlbl, his_assgnm] : other_file.i_assignments )
           {
            if( his_varlbl == "vqMachSettingsVer"sv )
               {// Skipping: I'll keep my own version
               }
            else if( auto my_assgnm = get_field(his_varlbl); my_assgnm!=nullptr )
               {
                my_assgnm->modify_value(his_assgnm.value());
               }
            // Rilevo eventuale rinominazione della 'added_label()'
            else if( my_assgnm = detect_rename_of(his_assgnm); my_assgnm!=nullptr )
               {
                add_mod_issue( fmt::format("Renamed: {}={} => {}={} (verify)",his_varlbl,his_assgnm.value(),my_assgnm->added_label(),my_assgnm->value()) );
                my_assgnm->modify_value(his_assgnm.value());
               }
            else
               {
                add_mod_issue( fmt::format("Not found: {}={} (removed or renamed)",his_varlbl,his_assgnm.value()) );
               }
           }
       }


    //-----------------------------------------------------------------------
    void write(const fs::path outpth)
       {
        const std::string_view line_break = !i_lines.empty() &&
                                            i_lines.front().span().length()>1 &&
                                            i_lines.front().span()[i_lines.front().span().length()-2]=='\r'
                                            ? "\r\n"sv : "\n"sv;
        bool block_comment_notyetfound = true;
        sys::file_write fw( outpth.string() );
        for( const auto& line : i_lines )
           {
            if( line.assignment_ptr() && line.assignment_ptr()->is_value_modified() )
               {// This line is an assignment with modified value, reconstructing the line
                // Detect indentation
                const std::ptrdiff_t indent_len = line.assignment_ptr()->var_name().data() - line.span().data();
                assert(indent_len>=0);
                assert( line.span().length()>0 && line.span()[line.span().length()-1]=='\n' );

                fw << std::string_view(line.span().data(), static_cast<std::size_t>(indent_len))
                   << line.assignment_ptr()->var_name()
                   << " = "sv
                   << line.assignment_ptr()->value();

                if( !line.assignment_ptr()->comment().empty() )
                   {
                    fw << " # "sv << line.assignment_ptr()->comment();
                   }

                if( !line.assignment_ptr()->added_label().empty() )
                   {
                    fw << " '"sv << line.assignment_ptr()->added_label() << '\'';
                   }

                fw << line_break;
               }
            else if( block_comment_notyetfound && line.span().contains("[EndNote]") )
               {
                block_comment_notyetfound = false;
                // Adding some info on generated file
                fw << "    ("sv << sys::get_formatted_time_stamp() << " m32-pars-adapt, "sv << std::to_string(i_mod_issues.size()) << " issues)"sv << line_break;
                fw << line.span();
               }
            else
               {// Write original unmodified line
                fw << line.span();
               }
           }

        // Append modification issues
        for( const auto& issue : i_mod_issues )
           {
            fw << "# "sv << issue << line_break;
           }
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] std::size_t modified_values_count() const noexcept
       {
        std::size_t count = 0;
        for( const auto& [varlbl, assgnm] : i_assignments )
           {
            if( assgnm.is_value_modified() ) ++count;
           }
        return count;
       }

    [[nodiscard]] std::string info() const { return fmt::format("{} lines, {} assignments", i_lines.size(), i_assignments.size()); }
    [[nodiscard]] const std::string& path() const noexcept { return file_buf.path(); }

    //-----------------------------------------------------------------------
    void add_mod_issue(std::string&& issue) { i_mod_issues.emplace_back(issue); }
    [[nodiscard]] std::size_t mod_issues_count() const noexcept { return i_mod_issues.size(); }

 private:
    const sys::MemoryMappedFile file_buf; // File buffer
    std::vector<Line> i_lines; // Collected lines
    std::map<std::string_view,Assignment> i_assignments; // Assignments (name = value)
    std::vector<std::string> i_mod_issues; // Modifications problems
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


//---- end unit -------------------------------------------------------------
#endif
