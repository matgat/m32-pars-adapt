#pragma once
//  ---------------------------------------------
//  Parses my special json format adopted for
//  parameters overlays
//  ---------------------------------------------
//  #include "json_parser.hpp" // json::Parser
//  ---------------------------------------------
#include <vector>

#include "plain_parser_base.hpp" // plain::ParserBase
#include "json_node.hpp" // json::Node


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace json //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


/////////////////////////////////////////////////////////////////////////////
class Parser final : public plain::ParserBase<char>
{              using base = plain::ParserBase<char>;
 public:
    Parser(const std::string_view buf)
      : base(buf)
       {}

    //-----------------------------------------------------------------------
    void collect_childs_of(Node& n_parent, const int nest_lvl =0, const std::size_t line_start =1)
       {
        while( true )
           {
            base::skip_any_space();
            if( not base::has_codepoint() )
               {// No more data
                break;
               }
            else if( skip_possible_comment() )
               {
               }
            else if( got_sep() )
               {// Tolerating separators after values
                base::get_next();
               }
            else if( base::got('}') )
               {// Child block closed
                base::get_next();
                return;
               }
            else
               {
                // Here expecting one or more keys
                const auto keys = extract_keys();
                assert( not keys.empty() );

                // Get key/value separator
                assert( not base::got_space() );
                const char keyval_sep = base::curr_codepoint();
                if( not ascii::is_any_of<':','='>(keyval_sep) )
                   {
                    throw base::create_parse_error(std::format("Invalid separator '{}' after key \"{}\"", str::escape(keyval_sep), keys.back()));
                   }
                base::get_next();

                // I'll support comments also here
                base::skip_any_space();
                if( skip_possible_comment() )
                   {
                    base::skip_any_space();
                   }

                // Now expecting a value or further child block
                if( not base::has_codepoint() )
                   {
                    throw base::create_parse_error( std::format("Missing value of key \"{}\"", keys.back()) );
                   }
                else if( base::got('{') )
                   {// Collect child block
                    base::get_next();

                    // In this case I strictly enforce a colon
                    if( keyval_sep!=':' )
                       {
                        throw base::create_parse_error(std::format("Key \"{}\" must be followed by ':' and not by '{}'", keys.back(), keyval_sep));
                       }

                    if( keys.size()>1 )
                       {// I'll ensure a child for each key
                        Node n_tmp; // An empty bag to collect just this block
                        collect_childs_of(n_tmp, nest_lvl+1, base::curr_line());

                        // Copy retrieved child block to all keys
                        for( const auto& key : keys )
                           {
                            Node& n_child = n_parent.ensure_child( key );
                            n_child.insert_childs_of(n_tmp);
                           }
                       }
                    else
                       {
                        Node& child = n_parent.ensure_child( keys.front() );
                        collect_childs_of(child, nest_lvl+1, base::curr_line());
                       }
                   }
                else
                   {// A single value
                    if( keys.size()>1 )
                       {
                        throw base::create_parse_error("A value cannot have multiple keys");
                       }
                    Node& child = n_parent.ensure_child( keys.front() );
                    child.set_value( extract_value() );
                   }
               }
           }

        // No more data: Detect unclosed block
        if( nest_lvl>0 )
           {
            throw base::create_parse_error( std::format("Unclosed block (nesting level={}) opened at line {}", nest_lvl, line_start) );
           }
       }


 private:
    [[nodiscard]] static constexpr bool is_special_char(const char c) noexcept
       {
        return ascii::is_any_of<':','{','}',',',';','='>(c);
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] bool got_sep() const noexcept
       {
        return base::got_any_of<',',';'>();
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] bool skip_possible_comment()
       {
        if( base::eat("//") )
           {// Skip line comment
            base::skip_line();
            return true;
           }
        else if( base::eat("/*") )
           {// Skip block comment
            [[maybe_unused]] auto cmt = base::get_until("*/");
            return true;
           }
        return false;
       }


    //-----------------------------------------------------------------------
    // Extract a (possibly quoted) string. Ensures not empty
    [[nodiscard]] std::string_view extract_key()
       {
        struct local final
           {
            [[nodiscard]] static constexpr bool is_unexpected(const char c) noexcept
               {
                return is_special_char(c) or ascii::is_any_of<'\n',cend>(c);
               }

            [[nodiscard]] static std::string_view get_quoted_key(json::Parser& parser)
               {
                assert( parser.got('\"') );
                parser.get_next();
                return parser.get_until_and_skip(ascii::is<'\"'>, local::is_unexpected);
               }

            [[nodiscard]] static std::string_view get_unquoted_key(json::Parser& parser)
               {
                return parser.get_while( ascii::is_alnum_or_any_of<'_','-','+','.'> );
               }
           };

        assert( not base::got_space() );
        const std::string_view key = base::got('\"')
                                     ? local::get_quoted_key(*this)
                                     : local::get_unquoted_key(*this);
        if( key.empty() )
           {
            throw base::create_parse_error("No key found");
           }

        base::skip_any_space(); // Possible spaces after key
        return key;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::vector<std::string_view> extract_keys()
       {
        std::vector<std::string_view> keys;

        keys.push_back( extract_key() );

        // Extension: Support multiple keys (comma separated)
        while( base::got(',') )
           {
            base::get_next();
            base::skip_any_space(); // Possible spaces after comma
            keys.push_back( extract_key() );
           }

        return keys;
       }


    //-----------------------------------------------------------------------
    // Extract a (possibly quoted) string
    [[nodiscard]] std::string_view extract_value()
       {
        struct local final
           {
            [[nodiscard]] static std::string_view get_quoted_val(json::Parser& parser)
               {
                assert( parser.got('\"') );
                parser.get_next();
                return parser.get_until_and_skip(ascii::is<'\"'>, ascii::is_any_of<'\n',cend>);
               }

            [[nodiscard]] static constexpr bool is_end(const char c) noexcept
               {
                return ascii::is_space(c) or is_special_char(c);
               }

            [[nodiscard]] static std::string_view get_unquoted_val(json::Parser& parser)
               {
                return parser.get_until( is_end );
               }
           };

        assert( not base::got_space() );
        const std::string_view val = base::got('\"')
                                     ? local::get_quoted_val(*this)
                                     : local::get_unquoted_val(*this);
        return val;
       }
};




//---------------------------------------------------------------------------
// Parse json file
void parse(const std::string& file_path, const std::string_view buf, Node& root, fnotify_t const& notify_issue)
{
    Parser parser(buf);
    parser.set_on_notify_issue(notify_issue);
    parser.set_file_path( file_path );

    try{
        parser.collect_childs_of(root);
       }
    catch( parse::error& )
       {
        throw;
       }
    catch( std::exception& e )
       {
        throw parser.create_parse_error(e.what());
       }
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::





/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"json_parser"> json_parser_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("json::parse()") = []
   {
    const std::string_view buf =
        "// comment\n"
        "a,\"b\",c :\n"
        "   {\n"
        "    d,\"e\" :\n"
        "       {// comment\n"
        "        common: val\n"
        "       }\n"
        "   }\n"
        "\n"
        "n0: // comment\n"
        "   {\n"
        "    \"n1\" :\n"
        "       {\n"
        "        \"n1-0\" :\n"
        "           {// comment\n"
        "            \"key1\", key2 :\n"
        "               {\n"
        "                nam1 = val1\n"
        "                nam2: \"val2\"\n"
        "               }\n"
        "           }\n"
        "       }\n"
        "    n2:{\n"
        "        n2-0 :\n"
        "           {// comment\n"
        "            nam3:val3\n"
        "           } nam4=val4\n"
        "        nam5: val5\n"
        "       }\n"
        "   }\n"sv;

    json::Node root;
    try{
        struct issues_t final { int num=0; void operator()(std::string&& msg) noexcept {++num; ut::log << msg << '\n';}; } issues;
        json::parse("test", buf, root, std::ref(issues));
        ut::expect( ut::that % issues.num==0 ) << "no issues expected\n";
       }
    catch( parse::error& e )
       {
        ut::expect(false) << std::format("Exception: {} (line {})\n", e.what(), e.line());
       }

    //print(root);
    // ├a
    // │ ├d
    // │ │ ┕common:val
    // │ ┕e
    // │   ┕common:val
    // ├b
    // │ ├d
    // │ │ ┕common:val
    // │ ┕e
    // │   ┕common:val
    // ├c
    // │ ├d
    // │ │ ┕common:val
    // │ ┕e
    // │   ┕common:val
    // ┕n0
    //   ├n1
    //   │ ┕n1-0
    //   │   ├key1
    //   │   │ ├nam1:val1
    //   │   │ ┕nam2:val2
    //   │   ┕key2
    //   │     ├nam1:val1
    //   │     ┕nam2:val2
    //   ┕n2
    //     ├n2-0
    //     │ ┕nam3:val3
    //     ├nam4:val4
    //     ┕nam5:val5

    ut::expect( ut::that % root.string()=="{a:{d:{common:val},e:{common:val}},"
                                           "b:{d:{common:val},e:{common:val}},"
                                           "c:{d:{common:val},e:{common:val}},"
                                           "n0:{n1:{n1-0:{key1:{nam1:val1,nam2:val2},key2:{nam1:val1,nam2:val2}}},"
                                               "n2:{n2-0:{nam3:val3},nam4:val4,nam5:val5}}}"sv );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
