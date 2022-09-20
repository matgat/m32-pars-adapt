#ifndef GUARD_sipro_parser_hpp
#define GUARD_sipro_parser_hpp
//  ---------------------------------------------
//  Parser of Sipro m32 text file format adopted
//  for cnc parameters and data files
//  ---------------------------------------------
#include "basic-parser.hpp" // BasicParser


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sipro //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
// Descriptor of a tag (in the buffer)
class Tag final
{
 public:
    [[nodiscard]] operator bool() const noexcept { return !i_Name.empty(); }

    [[nodiscard]] std::string_view name() const noexcept { return i_Name; }
    void set_name(std::string_view s)
       {
        if( s.empty() )
           {
            throw std::runtime_error("Empty tag name");
           }
        else if( s.starts_with("Start") )
           {
            //i_start = true;
            s.remove_prefix(5);
           }
        else if( s.starts_with("End") )
           {
            i_start = false;
            s.remove_prefix(3);
           }
        else
           {
            throw std::runtime_error( fmt::format("Invalid tag name {}",s) );
           }
        i_Name = s;
       }

    [[nodiscard]] bool is_start() const noexcept { return i_start; }
    [[nodiscard]] bool is_end() const noexcept { return !i_start; }

    [[nodiscard]] bool is_start_of(const std::string_view s) const noexcept { return i_start && i_Name==s; }
    [[nodiscard]] bool is_end_of(const std::string_view s) const noexcept { return !i_start && i_Name==s; }

 private:
    std::string_view i_Name;
    bool i_start = true;
};



/////////////////////////////////////////////////////////////////////////////
// Descriptor of an assignment entry (in the buffer)
class Assignment final
{
 public:
    [[nodiscard]] operator bool() const noexcept { return !i_Value.empty(); }

    [[nodiscard]] std::string_view var_name() const noexcept { return i_Name; }
    void set_var_name(const std::string_view s)
       {
        if( s.empty() || !std::isalpha(s.front()) )
           {
            throw std::runtime_error( fmt::format("Invalid variable name {}",s) );
           }
        i_Name = s;
       }

    [[nodiscard]] std::string_view value() const noexcept { return i_Value; }
    void set_value(const std::string_view s)
       {
        if( s.empty() )
           {
            throw std::runtime_error("Empty value");
           }
        i_Value = s;
       }

    [[nodiscard]] std::string_view comment() const noexcept { return i_Comment; }
    void set_comment(const std::string_view s) noexcept { i_Comment = s; }
    [[nodiscard]] bool has_comment() const noexcept { return !i_Comment.empty(); }

    [[nodiscard]] std::string_view added_label() const noexcept { return i_CommentLabel; }
    void set_added_label(const std::string_view s) noexcept { i_CommentLabel = s; }
    [[nodiscard]] bool has_added_label() const noexcept { return !i_CommentLabel.empty(); }


 private:
    std::string_view i_Name;
    std::string_view i_Value;
    std::string_view i_Comment;
    std::string_view i_CommentLabel;
};



/////////////////////////////////////////////////////////////////////////////
// Descriptor of a line event (in the buffer)
class Line final
{
 public:
    [[nodiscard]] const std::string_view& span() const noexcept { return i_LineSpan; }
    [[nodiscard]] std::string_view& span() noexcept { return i_LineSpan; }

    [[nodiscard]] const Tag& tag() const noexcept { return i_Tag; }
    [[nodiscard]] Tag& tag() noexcept { return i_Tag; }

    [[nodiscard]] const Assignment& assignment() const noexcept { return i_Assignment; }
    [[nodiscard]] Assignment& assignment() noexcept { return i_Assignment; }

 private:
    std::string_view i_LineSpan;
    Tag i_Tag;
    Assignment i_Assignment;
};



/////////////////////////////////////////////////////////////////////////////
class Parser final : public BasicParser
{
 public:
    Parser(const std::string& pth, const std::string_view dat, std::vector<std::string>& lst, const bool fus)
      : BasicParser(pth,dat,lst,fus) {}

    //-----------------------------------------------------------------------
    [[nodiscard]] Line next_line()
       {
        Line curr_line;
        const std::size_t i_start = i; // To collect line span

        try{
            skip_blanks();
            if( i_inside_note_block )
               {
                if( is_tag() )
                   {
                    collect_tag( curr_line.tag() );
                    // Expecting just "EndNote" tag
                    if( curr_line.tag().is_end_of("Note"sv) )
                       {
                        i_inside_note_block = false;
                       }
                    else
                       {
                        notify_error("Unclosed [StartNote]"sv);
                       }
                   }
                else
                   {// A line inside a [StartNote]/[EndNote] block
                    skip_line();
                   }
               }
            else if( eat_line_end() )
               {// An empty line
               }
            else if( eat_line_comment_start() )
               {// A comment
                skip_line();
               }
            else if( is_tag() )
               {
                collect_tag( curr_line.tag() );

                if( curr_line.tag().is_start_of("Note"sv) )
                   {
                    i_inside_note_block = true;
                   }
               }
            else
               {
                collect_assignment( curr_line.assignment() );
               }
           }
        catch(parse_error&)
           {
            throw;
           }
        catch(std::exception& e)
           {
            throw create_parse_error(e.what());
           }

        curr_line.span() = std::string_view(buf+i_start, i-i_start);
        return curr_line;
       }

    [[nodiscard]] bool is_inside_note_block() const noexcept { return i_inside_note_block; }


 private:

    //-----------------------------------------------------------------------
    [[nodiscard]] bool eat_line_comment_start() noexcept
       {
        if( i<siz && buf[i]=='#' )
           {
            ++i; // Skip '#'
            return true;
           }
        return false;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] bool is_tag() const noexcept
       {
        return i<siz && buf[i]=='[';
       }

    //-----------------------------------------------------------------------
    //[[nodiscard]] Tag collect_tag() { Tag tag; collect_tag(tag); return tag; }
    void collect_tag(Tag& tag)
       {
        ++i; // Skip '['
        skip_blanks();
        tag.set_name( collect_identifier() );

        skip_blanks();
        if( i>=siz || buf[i]!=']' )
           {
            throw create_parse_error( fmt::format("Unclosed tag [{}", tag.name()) );
           }
        ++i; // Skip ']'
        //D2LOG("[{}] Collected tag [{}]\n", line, tag.name())

        // Expecting nothing more
        skip_blanks();
        if( !eat_line_end() )
           {
            notify_error("Unexpected content after tag: {}", str::escape(skip_line()));
           }
       }

    //-----------------------------------------------------------------------
    void collect_assignment(Assignment& namval)
       {
        // [variable name]
        // Contract: blanks already skipped
        if( i>=siz || !std::isalpha(buf[i]) )
           {
            throw create_parse_error("Expected a variable name");
           }
        namval.set_var_name( collect_identifier() );

        // [assignment character]
        skip_blanks();
        if( i>=siz || buf[i]!='=' )
           {
            throw create_parse_error("Missing assignment character '='");
           }
        ++i; // Skip '='

        // [value]
        skip_blanks();
        namval.set_value( collect_token() );

        // [comment and register label]
        skip_blanks();
        if( eat_line_comment_start() )
           {
            skip_blanks();
            // Collect the comment
            if( i<siz && buf[i]!='\n' )
               {
                const std::size_t i_txt_start = i; // Start of comment text
                std::size_t i_txt_end = i; // One-past-end of comment text (for right trim)
                do {
                    if( buf[i]=='\n' )
                       {// Line finished
                        break;
                       }
                    else
                       {
                        if( !is_blank(buf[i]) ) i_txt_end = ++i;
                        else ++i;
                       }
                   }
                while( i<siz );

                namval.set_comment( std::string_view(buf+i_txt_start, i_txt_end-i_txt_start) );
               }
           }

        // Expecting a line end here
        if( !eat_line_end() )
           {
            notify_error("Unexpected content after assignment: {}", str::escape(skip_line()));
           }

        // [register label]
        // Extract possible register label expressed as: # comment 'label'
        if( namval.comment().ends_with('\'') )
           {
            const std::size_t i_lbl_end = namval.comment().size(); // One-past-end of label
            std::size_t i_lbl_start = i_lbl_end-1; // One-prev-start of label
            while( i_lbl_start>0 && is_identifier(namval.comment()[--i_lbl_start]) ) ;
            if( namval.comment()[i_lbl_start]=='\'' )
               {
                namval.set_added_label( std::string_view(namval.comment().data()+i_lbl_start+1, i_lbl_end-i_lbl_start-2) );
                // Trim spaces for comment text end
                do{ --i_lbl_start; } while( is_blank(namval.comment()[i_lbl_start]) );
                namval.set_comment( std::string_view(namval.comment().data(), i_lbl_start+1) );
               }
            else
               {
                notify_error("Assignment {}={} has invalid label", namval.var_name(), namval.value());
               }
           }

        //D2LOG("[{}] Collected assignment: {} = {} // {} '{}'\n", line, namval.var_name(), namval.value(), str::iso_latin1_to_utf8(namval.comment()), namval.added_label())
       }

 private:
    bool i_inside_note_block = false;
};



//---------------------------------------------------------------------------
// Parse a sipro file
//void parse(const std::string& file_path, const std::string_view buf, std::map<DefineBuf>& defs, std::vector<std::string>& issues, const bool fussy)
//{
//    Parser parser(file_path, buf, issues, fussy);
//    while( const Line l = parser.next_line() )
//       {
//       }
//}



}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


//---- end unit -------------------------------------------------------------
#endif
