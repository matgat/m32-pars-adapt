#ifndef GUARD_json_parser_hpp
#define GUARD_json_parser_hpp
//  ---------------------------------------------
//  Parses my special json format adopted for
//  parameters overlays
//  ---------------------------------------------
#include <vector>
#include <fmt/ranges.h> // To print std::vector

#include "basic-parser.hpp" // BasicParser
#include "json-node.hpp" // json::Node


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace json //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


/////////////////////////////////////////////////////////////////////////////
class Parser final : public BasicParser
{
 public:
    Parser(const std::string& pth, const std::string_view dat, std::vector<std::string>& lst, const bool fus)
      : BasicParser(pth,dat,lst,fus) {}

    //-----------------------------------------------------------------------
    void collect_childs_of(Node& n_parent, const int nest_lvl =0, const std::size_t line_start =1)
       {
        try{//EVTLOG("collect_childs_of: offset:{} char:{}", i, buf[i])
            while( true )
               {
                skip_any_space();
                if( i>=siz )
                   {// No more data!
                    break;
                   }
                else if( skip_possible_comment() )
                   {
                    continue;
                   }
                else if( buf[i]==',' || buf[i]==';' )
                   {// Tolerating these separators after values
                    ++i; // Skip ','
                    continue;
                   }
                else if( buf[i]=='}' )
                   {// Childs block closed
                    ++i; // Skip '}'
                    return;
                   }
                else
                   {
                    // Here expecting one or more keys
                    const auto keys = extract_keys();
                    assert( !keys.empty() );
                    //DLOG1("{}[{}] Found node \"{}\"\n", std::string(nest_lvl,'\t'), line, fmt::join(keys, ", "))
                    // What I'm expecting after a key?

                    // Get key/value separator
                    if( i>=siz )
                       {
                        throw create_parse_error(fmt::format("No data after key \"{}\"", keys.back()));
                       }
                    assert( !std::isspace(buf[i]) );
                    const char separator = buf[i++];
                    if( separator!=':' && separator!='=' ) // Extension: Also '='
                       {
                        throw create_parse_error(fmt::format("Invalid separator '{}' after key \"{}\"", str::escape(separator), keys.back()));
                       }

                    // I'll support comments also here
                    skip_any_space();
                    if( skip_possible_comment() )
                       {
                        skip_any_space();
                       }

                    // Now expecting a value or further childs
                    if( i>=siz )
                       {
                        throw create_parse_error( fmt::format("Missing value of key \"{}\"", keys.back()) );
                       }
                    else if( buf[i]=='{' )
                       {// Collect subchilds
                        ++i; // Skip '{'

                        // In this case I strictly enforce a colon
                        if( separator!=':' )
                           {
                            throw create_parse_error(fmt::format("Key \"{}\" must be followed by ':'", keys.back()));
                           }

                        if( keys.size()>1 )
                           {// I'll ensure a child for each key
                            Node n_tmp; // An empty bag to collect just this block
                            collect_childs_of(n_tmp, nest_lvl+1, line);
                            //DLOG2("{}[{}] Found {} childs for \"{}\"\n", std::string(nest_lvl,'\t'), line, n_tmp.childs_count(), fmt::join(keys, ", "))

                            // Copy retrieved subchilds to all childs
                            for( const auto& key : keys )
                               {
                                Node& n_child = n_parent.ensure_child( key );
                                n_child.insert_childs_of(n_tmp);
                               }
                           }
                        else
                           {
                            Node& child = n_parent.ensure_child( keys.front() );
                            collect_childs_of(child, nest_lvl+1, line);
                            //DLOG2("{}[{}] Found {} childs for \"{}\"\n", std::string(nest_lvl,'\t'), line, n_tmp.childs_count(), keys.front())
                           }
                       }
                    else
                       {// A single value
                        if( keys.size()>1 )
                           {
                            throw create_parse_error("A value cannot have multiple keys");
                           }
                        Node& child = n_parent.ensure_child( keys.front() );
                        child.set_value( extract_value() );
                        //DLOG2("{}[{}] Assigned {} = {}\n", std::string(nest_lvl,'\t'), line, keys.front(), child.value())
                       }
                   }
               }

            // No more data: Detect unclosed block
            if( nest_lvl>0 )
               {
                throw create_parse_error( fmt::format("Unclosed block (nesting level={}) opened at line {}", nest_lvl, line_start) );
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
       }


 private:

    //-----------------------------------------------------------------------
    [[nodiscard]] static bool is_special_char(const char c) noexcept
       {
        return c==':' || c=='{' || c=='}' || c==',' || c==';' || c=='=';
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] bool skip_possible_comment()
       {
        if( i<(siz-1) && buf[i]=='/' )
           {// Possible comment detected
            if( buf[i+1]=='/' ) [[likely]]
               {// Line comment detected
                i += 2; // Skip "//"
                skip_line();
                return true;
               }
            else if( buf[i+1]=='*' ) [[unlikely]]
               {// Block comment detected
                throw create_parse_error("Block comments are not supported, use //");
                //i += 2; // Skip "/*"
                //// Skip block comment
                //const std::size_t line_start = line; // Store current line
                //const std::size_t i_start = i; // Store current position
                //while( i<i_last )
                //   {
                //    if( buf[i]=='*' && buf[i+1]=='/' )
                //       {
                //        i += 2; // Skip "*/"
                //        return true;
                //       }
                //    else if( buf[i]=='\n' )
                //       {
                //        ++line;
                //       }
                //    ++i;
                //   }
                //throw create_parse_error("Unclosed block comment", line_start, i_start);
               }
           }
        return false;
       }


    //-----------------------------------------------------------------------
    // Collect a quoted key (first " not yet eat)
    [[nodiscard]] std::string_view collect_quoted_key()
       {
        assert( i<siz && buf[i]=='\"' );
        ++i; // Skip initial "
        const std::size_t i_start = i;
        while( i<siz )
           {
            if( buf[i]=='\"' )
               {
                ++i; // Skip final "
                return std::string_view(buf+i_start, i-i_start-1);
               }
            else if( buf[i]=='\n' || is_special_char(buf[i]) )
               {
                break;
               }
            ++i;
           }
        throw create_parse_error("Unclosed key (\" expected)");
       }


    //-----------------------------------------------------------------------
    // Collect a quoted value (first " not yet eat)
    [[nodiscard]] std::string_view collect_quoted_value()
       {
        assert( i<siz && buf[i]=='\"' );
        const std::size_t i_start = i; // Including the double quote
        ++i; // Skip initial "
        while( i<siz )
           {
            if( buf[i]=='\"' )
               {
                ++i; // Skip final ", but including it
                return std::string_view(buf+i_start, i-i_start);
               }
            else if( buf[i]=='\n' )
               {
                break;
               }
            ++i;
           }
        throw create_parse_error("Unclosed value (\" expected)");
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::string_view collect_unquoted_value()
       {
        assert( i<siz && !std::isspace(buf[i]) );
        const std::size_t i_start = i;
        while( i<siz && !std::isspace(buf[i]) )
           {
            if( is_special_char(buf[i]) )
               {
                throw create_parse_error( fmt::format("Character '{}' not allowed in unquoted value",buf[i]) );
               }
            ++i;
           }
        return std::string_view(buf+i_start, i-i_start);
       }


    //-----------------------------------------------------------------------
    // Extract a (possibly quoted) string. Ensures not empty
    [[nodiscard]] std::string_view extract_key()
       {
        assert( i<siz && !std::isspace(buf[i]) );
        const std::string_view key = buf[i]=='\"' ? collect_quoted_key()
                                                  : collect_identifier();
        skip_any_space(); // Possible spaces after key

        if( key.empty() )
           {
            throw create_parse_error( "Empty key" );
           }
        //DLOG2("[{}] Collected key \"{}\"\n", line, key)
        return key;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::vector<std::string_view> extract_keys()
       {
        std::vector<std::string_view> keys;

        keys.push_back( extract_key() ); // Note: This ensures keys not empty
        // Extension: Support multiple keys (comma separated)
        while( i<siz && buf[i]==',' )
           {
            ++i; // Skip ','
            skip_any_space(); // Possible spaces after comma
            keys.push_back( extract_key() );
           }
        // What I'm expecting here? I'll let the main cycle deal with that
        return keys;
       }

    //-----------------------------------------------------------------------
    // Extract a (possibly quoted) string
    [[nodiscard]] std::string_view extract_value()
       {
        const std::string_view val = buf[i]=='\"' ? collect_quoted_value()
                                                  : collect_unquoted_value();
        //DLOG2("[{}] Collected value \"{}\"\n", line, val)
        // What I'm expecting here? I'll let the main cycle deal with that
        return val;
       }
};




//---------------------------------------------------------------------------
// Parse json file
void parse(const std::string& file_path, const std::string_view buf, Node& root, std::vector<std::string>& issues, const bool fussy)
{
    Parser parser(file_path, buf, issues, fussy);
    parser.collect_childs_of(root);
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

//---- end unit -------------------------------------------------------------
#endif
