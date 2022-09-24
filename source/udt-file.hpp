#ifndef GUARD_udt_file_hpp
#define GUARD_udt_file_hpp
//  ---------------------------------------------
//  Sipro udt file descriptor
//  Consists in a list of name=value assignments
//  ---------------------------------------------
#include <string_view>
#include <stdexcept>
#include <fmt/core.h> // fmt::format

#include "system.hpp" // sys::MemoryMappedFile, sys::edit_text_file, sys::file_write
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

        //[[nodiscard]] std::string build_line(const std::string_view orig_line) const noexcept
        //   {
        //    return i_Assignment.has_added_label() ? fmt::format("{} = {} # {} '{}'"sv, i_Assignment.var_name(), i_NewVal, i_Assignment.comment(), i_Assignment.added_label())
        //                                          : fmt::format("{} = {} # {}"sv, i_Assignment.var_name(), i_NewVal, i_Assignment.comment() );
        //   }

        [[nodiscard]] std::string_view var_name() const noexcept { return i_Assignment.var_name(); }
        [[nodiscard]] std::string_view value() const noexcept { return i_NewVal.empty() ? i_Assignment.value() : i_NewVal; }
        [[nodiscard]] std::string_view comment() const noexcept { return i_Assignment.comment(); }
        [[nodiscard]] std::string_view added_label() const noexcept { return i_Assignment.added_label(); }

        [[nodiscard]] bool is_value_modified() const noexcept { return !i_NewVal.empty(); }
        [[nodiscard]] std::string_view modified_value() const noexcept { return i_NewVal; }
        void modify_value(const std::string& new_val) { i_NewVal = new_val; }

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

        [[nodiscard]] const std::string_view& span() const noexcept { return i_LineSpan; }
        [[nodiscard]] std::string_view& span() noexcept { return i_LineSpan; }

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
        try{
            sipro::Parser parser(file_buf.path(), file_buf.as_string_view(), parse_issues, true);

            while( parser.end_not_reached() )
               {
                const sipro::Line line = parser.next_line();
                Assignment* asgnm_ptr = nullptr;

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
                        issues.push_back( fmt::format("Unlabeled variable {} at line {}"sv, line.assignment().var_name(), i_lines.size()) );
                       }
                    else if( i_assignments.contains(line.assignment().added_label()) )
                       {
                        issues.push_back( fmt::format("Duplicate variable {} at line {}"sv, line.assignment().added_label(), i_lines.size()) );
                       }
                    else
                       {
                        // Populate the map of assignments
                        const auto ins = i_assignments.try_emplace(line.assignment().added_label(), line.assignment(), i_lines.size());
                        asgnm_ptr = &(ins.first->second); // Pointer to the constructed assignment to be asosciated with the collected line
                       }
                   }

                i_lines.emplace_back( line.span(), asgnm_ptr );
               }
           }
        catch( parse_error& e)
           {
            sys::edit_text_file( e.file_path(), e.line(), e.pos() );
            throw;
           }

        // Append parsing issues to overall issues list
        if( !parse_issues.empty() )
           {
            const std::string prefix{ pth.filename().string() };
            for(const auto& issue_entry : parse_issues )
               {
                issues.push_back( fmt::format("[{}]: {}", prefix , issue_entry) );
               }
           }
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] auto get_value_of(const std::string_view varlbl) const
       {
        const auto it = i_assignments.find(varlbl);
        if( it==i_assignments.end() )
           {
            throw std::runtime_error( fmt::format("Variable {} not found in {}", varlbl, file_buf.path()) );
           }
        return it->second.value();
       }

    //-----------------------------------------------------------------------
    void modify_value(const std::string_view varlbl, const std::string& new_val)
       {
        const auto it = i_assignments.find(varlbl);
        if( it==i_assignments.end() )
           {
            throw std::runtime_error( fmt::format("Variable {} not found in {}", varlbl, file_buf.path()) );
           }
        else
           {
            it->second.modify_value(new_val);
           }
       }

    //-----------------------------------------------------------------------
    void write(const fs::path outpth)
       {
        sys::file_write fw( outpth.string() );
        for( const auto& line : i_lines )
           {
            if( line.assignment_ptr() && line.assignment_ptr()->is_value_modified() )
               {// This line is an assignment with modified value, reconstructing the line
                // Detect indentation
                const std::ptrdiff_t indent_len = line.assignment_ptr()->var_name().data() - line.span().data();
                assert(indent_len>=0);

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

                // Respect the original line break
                assert( line.span().length()>0 && line.span()[line.span().length()-1]=='\n' );
                if( line.span().length()>1 && line.span()[line.span().length()-2]=='\r' )
                   {// Windows line break
                    fw << '\r';
                   }
                fw << '\n';
               }
            else
               {// Write line as it was
                fw << line.span();
               }
           }
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] std::size_t modified_values_count() const noexcept
       {
        std::size_t count = 0;
        for( const auto& [key, ass] : i_assignments )
           {
            if( ass.is_value_modified() ) ++count;
           }
        return count;
       }

    [[nodiscard]] std::string info() const { return fmt::format("{} lines, {} assignments ({} modified)", i_lines.size(), i_assignments.size(), modified_values_count()); }


 private:
    const sys::MemoryMappedFile file_buf; // File buffer
    std::vector<Line> i_lines; // Collected lines
    std::map<std::string_view,Assignment> i_assignments; // Assignments (name = value)
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


//---- end unit -------------------------------------------------------------
#endif
