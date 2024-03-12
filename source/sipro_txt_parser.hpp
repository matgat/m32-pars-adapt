#pragma once
//  ---------------------------------------------
//  Parses Sipro text file format adopted for
//  CNC parameters and data files
//  ---------------------------------------------
//  #include "sipro_txt_parser.hpp" // sipro::TxtParser
//  ---------------------------------------------
#include "plain_parser_base.hpp" // plain::ParserBase
#include "string_utilities.hpp" // str::trim_right()


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sipro //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
class TxtParsedLine final
{
    enum class type : char
       {
        GENERIC // ...
       ,START_TAG // [Start...]
       ,END_TAG // [End...]
       ,ASSIGNMENT // nam = val # comment 'label'
       };

 private:
    std::string_view m_content;
    std::string_view m_name;
    std::string_view m_value;
    std::string_view m_comment;
    type m_type = type::GENERIC;

 public:
    constexpr void clear() noexcept
       {
        m_content = {};
        m_name = {};
        m_value = {};
        m_comment = {};
        m_type = type::GENERIC;
       }

    constexpr void set_as_tag(const std::string_view tag_content)
       {
        if( tag_content.starts_with("Start"sv) )
           {
            m_name = tag_content;
            m_name.remove_prefix(5);
            m_type = type::START_TAG;
           }
        else if( tag_content.starts_with("End"sv) )
           {
            m_name = tag_content;
            m_name.remove_prefix(3);
            m_type = type::END_TAG;
           }
        else
           {
            throw std::runtime_error( std::format("Invalid tag name {}", tag_content) );
           }
       }

    constexpr void set_as_assignment(const std::string_view var_name, const std::string_view value)
       {
        m_name = var_name;
        m_value = value;
        m_type = type::ASSIGNMENT;
       }


    [[nodiscard]] explicit constexpr operator bool() const noexcept { return not m_content.empty(); }

    [[nodiscard]] constexpr bool is_generic() const noexcept { return m_type==type::GENERIC; }

    [[nodiscard]] constexpr bool is_start_tag() const noexcept { return m_type==type::START_TAG; }
    [[nodiscard]] constexpr bool is_end_tag() const noexcept { return m_type==type::END_TAG; }
    [[nodiscard]] constexpr bool is_start_tag_of(const std::string_view sv) const noexcept { return is_start_tag() and m_name==sv; }
    [[nodiscard]] constexpr bool is_end_tag_of(const std::string_view sv) const noexcept { return is_end_tag() and m_name==sv; }

    [[nodiscard]] constexpr bool is_assignment() const noexcept { return m_type==type::ASSIGNMENT; }

    [[nodiscard]] constexpr std::string_view name() const noexcept { return m_name; }
    [[nodiscard]] constexpr std::string_view value() const noexcept { return m_value; }
    [[nodiscard]] constexpr std::string_view comment() const noexcept { return m_comment; }

    [[nodiscard]] constexpr bool has_comment() const noexcept { return not m_comment.empty(); }
    constexpr void set_comment(const std::string_view sv) noexcept { m_comment = sv; }

    [[nodiscard]] constexpr std::string_view content() const noexcept { return m_content; }
    constexpr void set_content(const std::string_view sv) noexcept { m_content = sv; }
};




/////////////////////////////////////////////////////////////////////////////
class TxtParser final : public plain::ParserBase<char>
{                 using base = plain::ParserBase<char>;
 private:
    TxtParsedLine m_line; // Current collected line
    std::size_t m_note_block_started_at_line = std::string_view::npos;

 public:
    TxtParser(const std::string_view buf)
      : base(buf)
       {}


    [[nodiscard]] TxtParsedLine const& next_line()
       {
        m_line.clear();
        const std::size_t start_offset = base::curr_offset();

        try{
            base::skip_blanks();
            if( is_inside_note_block() )
               {
                if( got_tag() )
                   {
                    collect_tag( m_line );
                    // Expecting just "EndNote" tag
                    if( not m_line.is_end_tag_of("Note"sv) )
                       {
                        throw base::create_parse_error("Unclosed [StartNote]", m_note_block_started_at_line);
                       }
                    set_outside_note_block();
                   }
                else
                   {// A line inside a [StartNote]/[EndNote] block
                    base::skip_line();
                   }
               }
            else if( got_line_comment_start() )
               {// A comment
                base::get_next();
                base::skip_line();
               }
            else if( got_tag() )
               {
                collect_tag( m_line );
                if( m_line.is_start_tag_of("Note"sv) )
                   {
                    set_inside_note_block( base::curr_line() );
                   }
               }
            else if( base::got_endline() )
               {// An empty line
                base::get_next();
               }
            else if( base::has_codepoint() )
               {
                collect_assignment( m_line );
               }
           }
        catch(parse::error&)
           {
            throw;
           }
        catch(std::exception& e)
           {
            throw base::create_parse_error(e.what());
           }

        m_line.set_content( base::get_view_between(start_offset, base::curr_offset()) );
        return m_line;
       }

    [[nodiscard]] bool is_inside_note_block() const noexcept { return m_note_block_started_at_line != std::string_view::npos; }
    void check_unclosed_note_block() const { if(is_inside_note_block()) throw base::create_parse_error("Unclosed [StartNote]", m_note_block_started_at_line); }

 private:
   void set_inside_note_block(const std::size_t line_idx) noexcept { m_note_block_started_at_line = line_idx; }
   void set_outside_note_block() noexcept { m_note_block_started_at_line = std::string_view::npos; }

    [[nodiscard]] bool got_line_comment_start() noexcept
       {
        return base::got('#');
       }

    [[nodiscard]] bool got_tag() const noexcept
       {
        return base::got('[');
       }

    void collect_tag(TxtParsedLine& line)
       {
        assert( got_tag() );
        base::get_next(); // Skip '['
        base::skip_blanks();
        const std::string_view tag_content = base::get_identifier();
        if( tag_content.empty() )
           {
            throw std::runtime_error("Empty tag name");
           }
        base::skip_blanks();
        if( not base::got(']') )
           {
            throw base::create_parse_error( std::format("Unclosed square bracket in tag [{}", tag_content) );
           }
        base::get_next(); // Skip ']'
        // Expecting nothing more
        base::skip_blanks();
        base::check_and_eat_endline();

        line.set_as_tag( tag_content );
       }

    void collect_assignment(TxtParsedLine& line)
       {
        assert( not base::got_space() );

        // [variable name]
        if( not base::got_alpha() )
           {
            throw base::create_parse_error("Expected a variable name");
           }
        const std::string_view var_name = base::get_identifier();

        // [assignment character]
        base::skip_blanks();
        if( not base::got('=') )
           {
            throw base::create_parse_error("Missing assignment character '='");
           }
        base::get_next(); // Skip '='

        // [value]
        base::skip_blanks();
        const std::string_view value = base::get_until(ascii::is_space_or_any_of<'#',cend>);
        if( value.empty() )
           {
            throw std::runtime_error("Empty value");
           }

        line.set_as_assignment(var_name, value);

        // [possible comment]
        base::skip_blanks();
        if( got_line_comment_start() )
           {
            base::get_next();
            base::skip_blanks();
            line.set_comment( str::trim_right(base::get_rest_of_line()) );
            //base::print("Collected assignment: {} = {} // {}\n", line.name(), line.value(), line.comment());
           }
        else
           {// Expecting a line end here
            base::check_and_eat_endline();
           }
       }
};


}//::::::::::::::::::::::::::::::::: sipro ::::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include "ansi_escape_codes.hpp" // ANSI_RED, ...
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"sipro_txt_parser"> sipro_txt_parser_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("basic") = []
   {
    const std::string_view buf =
        "# Comment\n"                         // [1]
        "[StartRoot]\n"                       // [2]
        "  # Comment\n"                       // [3]
        "  [StartNote]\n"                     // [4]
        "    A note\n"                        // [5]
        "    vn905 = 42 # Integer\n"          // [6]
        "  [EndNote]\n"                       // [7]
        "\n"                                  // [8]
        "  [StartFirst]\n"                    // [9]
        "    Integer = 1\n"                   // [10]
        "    String = \"name\"\n"             // [11]
        "    Number = 34.657 \n"              // [12]
        "  [EndFirst]\n"                      // [13]
        "  \n"                                // [14]
        "  [StartSecond]\n"                   // [15]
        "    [StartNote]\n"                   // [16]
        "      Inner note\n"                  // [17]
        "    [EndNote]\n"                     // [18]
        "\n"                                  // [19]
        "    vn904 = -42 # Integer \n"        // [20]
        "    # Comment\n"                     // [21]
        "    va2 = \"-content-\" # String\n"  // [22]
        "    vq10 = -5.3 #  A neg number\n"   // [23]
        "  [EndSecond]\n"                     // [24]
        "[EndRoot]\n"sv;                      // [25]

    sipro::TxtParser parser(buf);
    parser.set_on_notify_issue([](const std::string_view msg) -> void { ut::log << ANSI_BLUE "parser: " ANSI_DEFAULT << msg; });
    parser.set_file_path("test");

    std::size_t n_line = 0u;
    try{
        while( const sipro::TxtParsedLine& line = parser.next_line() )
           {
            //ut::log << ANSI_BLUE << line.label() << '=' << line.value() << ANSI_CYAN "(num " << n_line+1u << " line " << parser.curr_line() << ")\n" ANSI_DEFAULT;
            switch( ++n_line )
               {
                case  1: ut::expect( line.is_generic() ); break;
                case  2: ut::expect( line.is_start_tag_of("Root"sv) ); break;
                case  3: ut::expect( line.is_generic() ); break;

                case  4:
                    ut::expect( line.is_start_tag_of("Note"sv) );
                    ut::expect( parser.is_inside_note_block() );
                    break;

                case  5: ut::expect( line.is_generic() ); break;
                case  6: ut::expect( line.is_generic() ); break;

                case  7:
                    ut::expect( line.is_end_tag_of("Note"sv) );
                    ut::expect( not parser.is_inside_note_block() );
                    break;

                case  8: ut::expect( line.is_generic() ); break;
                case  9: ut::expect( line.is_start_tag_of("First"sv) ); break;

                case 10:
                    ut::expect( line.is_assignment() );
                    ut::expect( ut::that % line.name()=="Integer"sv );
                    ut::expect( ut::that % line.value()=="1"sv );
                    ut::expect( ut::that % line.comment()==""sv );
                    break;

                case 11:
                    ut::expect( line.is_assignment() );
                    ut::expect( ut::that % line.name()=="String"sv );
                    ut::expect( ut::that % line.value()=="\"name\""sv );
                    ut::expect( ut::that % line.comment()==""sv );
                    break;

                case 12:
                    ut::expect( line.is_assignment() );
                    ut::expect( ut::that % line.name()=="Number"sv );
                    ut::expect( ut::that % line.value()=="34.657"sv );
                    ut::expect( ut::that % line.comment()==""sv );
                    break;

                case 13: ut::expect( line.is_end_tag_of("First"sv) ); break;
                case 14: ut::expect( line.is_generic() ); break;
                case 15: ut::expect( line.is_start_tag_of("Second"sv) ); break;
                case 16: ut::expect( line.is_start_tag_of("Note"sv) ); break;
                case 17: ut::expect( line.is_generic() ); break;
                case 18: ut::expect( line.is_end_tag_of("Note"sv) ); break;
                case 19: ut::expect( line.is_generic() ); break;

                case 20:
                    ut::expect( line.is_assignment() );
                    ut::expect( ut::that % line.name()=="vn904"sv );
                    ut::expect( ut::that % line.value()=="-42"sv );
                    ut::expect( ut::that % line.comment()=="Integer"sv );
                    break;

                case 21: ut::expect( line.is_generic() ); break;

                case 22:
                    ut::expect( line.is_assignment() );
                    ut::expect( ut::that % line.name()=="va2"sv );
                    ut::expect( ut::that % line.value()=="\"-content-\""sv );
                    ut::expect( ut::that % line.comment()=="String"sv );
                    break;

                case 23:
                    ut::expect( line.is_assignment() );
                    ut::expect( ut::that % line.name()=="vq10"sv );
                    ut::expect( ut::that % line.value()=="-5.3"sv );
                    ut::expect( ut::that % line.comment()=="A neg number"sv );
                    break;

                case 24: ut::expect( line.is_end_tag_of("Second"sv) ); break;
                case 25: ut::expect( line.is_end_tag_of("Root"sv) ); break;

                default:
                    ut::expect(false) << std::format("Unexpected line: \"{}\"\n", line.content());
               }
           }
       }
    catch( parse::error& e )
       {
        ut::expect(false) << std::format("Exception: {} (line {}/{})\n", e.what(), n_line, e.line());
       }
    ut::expect( ut::that % n_line==25u ) << "lines number should match";
   };


ut::test("bad tag format") = []
   {
    const std::string_view buf =
        "\n"
        "[StartRoot\n"
        "\n"sv;

    sipro::TxtParser parser(buf);
    parser.set_file_path("bad-tag-format-test");

    try{
        while( parser.next_line() ) ;
        ut::expect(false) << "parsing a bad tag should throw";
       }
    catch( parse::error& e )
       {
        ut::expect( ut::that % e.line()==2u ) << "error should be at second line";
       }
   };


ut::test("bad tag name") = []
   {
    const std::string_view buf =
        "\n"
        "[Root]\n"
        "\n"sv;

    sipro::TxtParser parser(buf);
    parser.set_file_path("bad-tag-name-test");

    try{
        while( parser.next_line() ) ;
        ut::expect(false) << "parsing a bad tag should throw";
       }
    catch( parse::error& e )
       {
        ut::expect( ut::that % e.line()==3u ) << "error should be after second line";
       }
   };


ut::test("empty tag name") = []
   {
    const std::string_view buf =
        "\n"
        "[ ]\n"
        "\n"sv;

    sipro::TxtParser parser(buf);
    parser.set_file_path("empty-tag-test");

    try{
        while( parser.next_line() ) ;
        ut::expect(false) << "parsing an empty tag should throw";
       }
    catch( parse::error& e )
       {
        ut::expect( ut::that % e.line()==2u ) << "error should be at second line";
       }
   };


ut::test("random line") = []
   {
    const std::string_view buf =
        "\n"
        "  ;abc\n"
        "\n"sv;

    sipro::TxtParser parser(buf);
    parser.set_file_path("random-line-test");

    try{
        while( parser.next_line() ) ;
        ut::expect(false) << "parsing a random line should throw";
       }
    catch( parse::error& e )
       {
        ut::expect( ut::that % e.line()==2u ) << "error should be at second line";
       }
   };


ut::test("bad assignment") = []
   {
    const std::string_view buf =
        "\n"
        "  abc\n"
        "\n"sv;

    sipro::TxtParser parser(buf);
    parser.set_file_path("bad-assignment-test");

    try{
        while( parser.next_line() ) ;
        ut::expect(false) << "parsing an invalid assignment should throw";
       }
    catch( parse::error& e )
       {
        ut::expect( ut::that % e.line()==2u ) << "error should be at second line";
       }
   };


ut::test("empty value") = []
   {
    const std::string_view buf =
        "\n"
        "  abc = \n"
        "\n"sv;

    sipro::TxtParser parser(buf);
    parser.set_file_path("empty-value-test");

    try{
        while( parser.next_line() ) ;
        ut::expect(false) << "parsing an empty value should throw";
       }
    catch( parse::error& e )
       {
        ut::expect( ut::that % e.line()==2u ) << "error should be at second line";
       }
   };


ut::test("Unclosed [StartNote]") = []
   {
    const std::string_view buf =
        "\n"
        "  [StartNote]\n"
        "  [StartSomething]\n"
        "\n"sv;

    sipro::TxtParser parser(buf);
    parser.set_file_path("unclosed-note-test");

    try{
        while( parser.next_line() ) ;
        ut::expect(false) << "parsing an unclosed note should throw";
       }
    catch( parse::error& e )
       {
        ut::expect( ut::that % e.line()==3u ) << "error should be after second line";
       }
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
