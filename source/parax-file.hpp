#ifndef GUARD_parax_file_hpp
#define GUARD_parax_file_hpp
//  ---------------------------------------------
//  Sipro par2kax file descriptor
//  ---------------------------------------------
#include <string_view>
#include <stdexcept>
#include <fmt/core.h> // fmt::format

#include "system.hpp" // sys::MemoryMappedFile, sys::file_write
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
        explicit Line(const std::string_view l, Field* const p) : i_LineSpan(l), i_FieldPtr(p) {}

        [[nodiscard]] const std::string_view& span() const noexcept { return i_LineSpan; }
        [[nodiscard]] std::string_view& span() noexcept { return i_LineSpan; }

        [[nodiscard]] const Field* field_ptr() const noexcept { return i_FieldPtr; }
        [[nodiscard]] Field* field_ptr() noexcept { return i_FieldPtr; }

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

        while( parser.has_data() )
           {
            const sipro::Line line = parser.next_line();
            Field* field_ptr = nullptr;

            if( parser.is_inside_note_block() )
               {// Ignoring notes
               }
            else if( line.tag() )
               {// Ignoring tags
               }
            else if( line.field() )
               {// Collect fields
                if( i_fields.contains(line.field().var_name()) )
                   {
                    issues.push_back( fmt::format("UDT: Duplicate field {} at line {}"sv, line.field().var_name(), i_lines.size()) );
                   }
                else
                   {
                    // Populate the map of fields
                    const auto ins = i_fields.try_emplace(line.field().var_name(), line.field(), i_lines.size());
                    field_ptr = &(ins.first->second); // Field associated to the collected line
                   }
               }

            i_lines.emplace_back( line.span(), field_ptr );
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
    [[nodiscard]] Assignment* get_field(const std::string_view key)
       {
        if( const auto it=i_fields.find(key); it!=i_fields.end() )
           {
            return &(it->second);
           }
        return nullptr;
       }

    //-----------------------------------------------------------------------
    void overwrite_values_from(const File& other_file) noexcept
       {
        for( const auto& [key, other] : other_file.i_fields )
           {
            if( const auto field = get_field(key) )
               {
                field->modify_value(other.value());
               }
            else
               {
                // Potrei cercare di rilevare rinominazioni delle label
                // controllando la corrispondenza ass.var_name(), ass.comment()
                // Anche con str::calc_similarity
                add_issue( fmt::format("Not found: {} = {}",key,other.value()) );
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

                if( !line.field_ptr()->comment().empty() )
                   {
                    fw << " # "sv << line.field_ptr()->comment();
                   }

                if( !line.field_ptr()->added_label().empty() )
                   {
                    fw << " '"sv << line.field_ptr()->added_label() << '\'';
                   }

                fw << line_break;
               }
            else if( block_comment_notyetfound && line.span().contains("[EndNote]") )
               {
                block_comment_notyetfound = false;
                // Adding some info on generated file
                fw << "    ("sv << sys::get_formatted_time_stamp() << " m32-pars-adapt, "sv << std::to_string(i_issues.size()) << " issues)"sv << line_break;
                fw << line.span();
               }
            else
               {// Write original unmodified line
                fw << line.span();
               }
           }

        // Append issues
        for( const auto& issue : i_issues )
           {
            fw << "# "sv << issue << line_break;
           }
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] std::size_t modified_values_count() const noexcept
       {
        std::size_t count = 0;
        for( const auto& [key, ass] : i_fields )
           {
            if( ass.is_value_modified() ) ++count;
           }
        return count;
       }

    [[nodiscard]] std::string info() const { return fmt::format("{} lines, {} fields", i_lines.size(), i_fields.size()); }
    [[nodiscard]] const std::string& path() const noexcept { return file_buf.path(); }

    //-----------------------------------------------------------------------
    void add_issue(std::string&& issue) { i_issues.emplace_back(issue); }
    [[nodiscard]] std::size_t issues_count() const noexcept { return i_issues.size(); }

 private:
    const sys::MemoryMappedFile file_buf; // File buffer
    std::vector<Line> i_lines; // Collected lines
    std::map<std::string_view,Field> i_fields; // Fields (name = value)
    std::vector<std::string> i_issues; // Notified problems
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


//---- end unit -------------------------------------------------------------
#endif
