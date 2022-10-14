#ifndef GUARD_parax_file_hpp
#define GUARD_parax_file_hpp
//  ---------------------------------------------
//  Sipro par2kax.txt file descriptor
//  ---------------------------------------------
#include <stdexcept>
#include <string_view>
#include <map>
#include <fmt/core.h> // fmt::format

#include "system.hpp" // sys::MemoryMappedFile, sys::file_write
#include "string-utilities.hpp" // str::unquoted
#include "time-stamp.hpp" // sys::get_formatted_time_stamp()
#include "sipro-parser.hpp" // sipro::Parser


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace parax //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


/////////////////////////////////////////////////////////////////////////////
class File final
{
    /////////////////////////////////////////////////////////////////////////
    class Field final
       {
        public:
            explicit Field(const sipro::Assignment a, const std::size_t l) : i_Assignment(a), i_LineIdx(l) {}

            [[nodiscard]] std::string_view var_name() const noexcept { return i_Assignment.var_name(); }
            [[nodiscard]] std::string_view value() const noexcept { return i_NewVal.empty() ? i_Assignment.value() : i_NewVal; }

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
            explicit Line(const std::string_view l) : i_LineSpan(l), i_FieldPtr(nullptr) {}

            [[nodiscard]] std::string_view span() const noexcept { return i_LineSpan; }

            [[nodiscard]] const Field* field_ptr() const noexcept { return i_FieldPtr; }
            [[nodiscard]] Field* field_ptr() noexcept { return i_FieldPtr; }
            void set_field_ptr(Field* const p) noexcept { i_FieldPtr = p; }

        private:
            std::string_view i_LineSpan;
            Field* i_FieldPtr;
       };

 public:
    explicit File(const fs::path& pth, std::vector<std::string>& issues)
      : file_buf{pth.string()}
       {
        std::vector<std::string> parse_issues;
        sipro::Parser parser(file_buf.path(), file_buf.as_string_view(), parse_issues, true);

        // A context for collecting axis fields
        class CurrAxBlock final
           {
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

                [[nodiscard]] operator bool() const noexcept { return m_inside; }
                [[nodiscard]] std::string_view tag_name() const noexcept { return m_tagname; }
                [[nodiscard]] std::size_t line_idx() const noexcept { return m_line_idx; }
                [[nodiscard]] const auto& collected_fields() const noexcept { return m_collected_fields; }
                [[nodiscard]] auto& mutable_collected_fields() noexcept { return m_collected_fields; }

            private:
                std::string_view m_tagname;
                std::size_t m_line_idx = 0;
                std::map<std::string_view,Field> m_collected_fields;
                bool m_inside = false;
           } curr_ax_block;

        while( parser.has_data() )
           {
            const sipro::Line line = parser.next_line();

            if( curr_ax_block )
               {// Collecting the fields of an axis
                if( parser.is_inside_note_block() )
                   {// Ignoring notes
                   }
                else if( line.tag() )
                   {// Detect Ax definition block end [End###Ax]
                    if( line.tag().is_end_of(curr_ax_block.tag_name()) )
                       {
                        curr_ax_block.end();
                        if( curr_ax_block.collected_fields().empty() )
                           {
                            parse_issues.push_back( fmt::format("No fields collected in axis block at line {}"sv, curr_ax_block.line_idx()) );
                           }
                        // I need the axis name to store the collected fields
                        else if( const Field* const ax_name_field = get_field(curr_ax_block.collected_fields(),"Name") )
                           {
                            const std::string_view ax_name = str::unquoted(ax_name_field->value());
                            if( i_axblocks.contains(ax_name) )
                               {
                                parse_issues.push_back( fmt::format("Duplicate axis name {} in block at line {}"sv, ax_name, curr_ax_block.line_idx()) );
                               }
                            else
                               {
                                const auto [it, inserted] = i_axblocks.insert({ax_name, std::move(curr_ax_block.mutable_collected_fields())});
                                assert(curr_ax_block.collected_fields().empty()); // After move should be empty
                                if( !inserted )
                                   {
                                    parse_issues.push_back( fmt::format("Axis block {} at line {} was not inserted"sv, ax_name, curr_ax_block.line_idx()) );
                                   }
                               }
                           }
                        else
                           {
                            parse_issues.push_back( fmt::format("Name not found in axis block at line {}"sv, curr_ax_block.line_idx()) );
                           }
                       }
                    else if( line.tag().name() != "Note"sv )
                       {
                        parse_issues.push_back( fmt::format("Unexpected tag {} at line {}"sv, line.tag().name(), i_lines.size()+1) );
                       }
                   }
                else if( line.assignment() )
                   {// Collect axis fields
                    if( curr_ax_block.collected_fields().contains(line.assignment().var_name()) )
                       {
                        parse_issues.push_back( fmt::format("Duplicate field {} at line {}"sv, line.assignment().var_name(), i_lines.size()+1) );
                       }
                    else
                       {
                        // Populate the map of fields
                        const auto [it, inserted] = curr_ax_block.mutable_collected_fields().try_emplace(line.assignment().var_name(), line.assignment(), i_lines.size());
                        if( !inserted )
                           {
                            parse_issues.push_back( fmt::format("Field {} at line {} was not inserted"sv, line.assignment().var_name(), i_lines.size()+1) );
                           }
                       }
                   }
               }
            else
               {// Searching for a [Start###Ax] tag
                if( parser.is_inside_note_block() )
                   {// Ignoring notes
                   }
                else if( line.tag() )
                   {// Detect entering Ax definition block [Start###Ax]
                    if( line.tag().is_start() && line.tag().name().ends_with("Ax") )
                       {
                        curr_ax_block.start(line.tag().name(), i_lines.size()+1);
                       }
                    // Ignoring other tags
                   }
                else if( line.assignment() )
                   {
                    parse_issues.push_back( fmt::format("Unexpected field {} at line {}"sv, line.assignment().var_name(), i_lines.size()+1) );
                   }
               }

            // All lines are collected to reproduce the original file
            i_lines.emplace_back( line.span() );
           }

        // Associate the field pointers to lines
        for( auto& [axid, axblock] : i_axblocks )
           {
            for( auto& [var_name, field] : axblock )
               {
                assert(field.line_index()<i_lines.size());
                i_lines[field.line_index()].set_field_ptr(&field);
               }
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
    [[nodiscard]] std::map<std::string_view,Field>* get_axfields(const std::string_view axid) noexcept
       {
        if( auto it=i_axblocks.find(axid); it!=i_axblocks.end() )
           {
            return &(it->second);
           }
        return nullptr;
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] static const Field* get_field(const std::map<std::string_view,Field>& fields, const std::string_view key) noexcept
       {
        if( auto it=fields.find(key); it!=fields.end() )
           {
            return &(it->second);
           }
        return nullptr;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] static Field* get_field(std::map<std::string_view,Field>& fields, const std::string_view key) noexcept
       {
        if( auto it=fields.find(key); it!=fields.end() )
           {
            return &(it->second);
           }
        return nullptr;
       }

    //-----------------------------------------------------------------------
    void write(const fs::path outpth, const std::string_view mach_type)
       {
        const std::string_view line_break = !i_lines.empty() &&
                                            i_lines.front().span().length()>1 &&
                                            i_lines.front().span()[i_lines.front().span().length()-2]=='\r'
                                            ? "\r\n"sv : "\n"sv;
        bool block_comment_notyetfound = true;
        sys::file_write fw( outpth.string() );
        for( const auto& line : i_lines )
           {
            if( line.field_ptr() && line.field_ptr()->is_value_modified() )
               {// This line is a field with modified value, reconstructing the line
                // Detect indentation
                const std::ptrdiff_t indent_len = line.field_ptr()->var_name().data() - line.span().data();
                assert(indent_len>=0);
                assert( line.span().length()>0 && line.span()[line.span().length()-1]=='\n' );

                fw << std::string_view(line.span().data(), static_cast<std::size_t>(indent_len))
                   << line.field_ptr()->var_name()
                   << " = "sv
                   << line.field_ptr()->value();

                //if( !line.field_ptr()->comment().empty() )
                //   {
                //    fw << " # "sv << line.field_ptr()->comment();
                //   }

                fw << line_break;
               }
            else if( block_comment_notyetfound && line.span().contains("[EndNote]") )
               {
                block_comment_notyetfound = false;
                // Adding some info on generated file
                fw << "    "sv << sys::get_formatted_time_stamp() << " m32-pars-adapt, "sv << std::to_string(i_mod_issues.size()) << " issues"sv << line_break;
                fw << "    Machine: "sv << mach_type << line_break;
                for( const auto& issue : i_mod_issues )
                   {
                    fw << "      ."sv << issue << line_break;
                   }
                fw << line.span();
               }
            else
               {// Write original unmodified line
                fw << line.span();
               }
           }

       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::size_t modified_values_count() const noexcept
       {
        std::size_t count = 0;
        for( auto& [axid, axblock] : i_axblocks )
           {
            for( auto& [var_name, field] : axblock )
               {
                if( field.is_value_modified() ) ++count;
               }
           }
        return count;
       }

    [[nodiscard]] std::string info() const { return fmt::format("{} lines, {} axes", i_lines.size(), i_axblocks.size()); }
    [[nodiscard]] const std::string& path() const noexcept { return file_buf.path(); }

    //-----------------------------------------------------------------------
    void add_mod_issue(std::string&& issue) { i_mod_issues.emplace_back(issue); }
    [[nodiscard]] std::size_t mod_issues_count() const noexcept { return i_mod_issues.size(); }

 private:
    const sys::MemoryMappedFile file_buf; // File buffer
    std::vector<Line> i_lines; // Collected lines
    std::map<std::string_view,std::map<std::string_view,Field>> i_axblocks; // Axes fields
    std::vector<std::string> i_mod_issues; // Notified problems
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


//---- end unit -------------------------------------------------------------
#endif
