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
        //EVTLOG("collect_node: offset:{} char:{}", i, buf[i])
        // Skip comments
        while( i<siz )
           {
            skip_any_space();
            if( eat_line_comment_start() )
               {
                skip_line();
                continue;
               }
            else if( eat_block_comment_start() )
               {
                skip_block_comment();
                continue;
               }
            else if( i>=siz )
               {// No more data
                // Detect unclosed bracket
                if( nest_lvl>0 )
                   {
                    throw create_parse_error( fmt::format("Unclosed block (nesting level={}) opened at line {}", nest_lvl, line_start) );
                   }
                return;
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
                //if( keys.empty() ) throw ...; // Not necessary, already ensured
                //D1LOG("{}[{}] Found node \"{}\"\n", std::string(nest_lvl,'\t'), line, fmt::join(keys, ", "))

                // Now expecting a value or further childs
                skip_any_space();
                if( i>=siz )
                   {
                    throw create_parse_error( fmt::format("Missing value of key \"{}\"", keys.back()) );
                   }
                else if( buf[i]=='{' )
                   {// Collect subchilds
                    ++i; // Skip '{'

                    if( keys.size()>1 )
                       {// I'll ensure a child for each key
                        Node n_tmp; // An empty bag to collect just this block
                        collect_childs_of(n_tmp, nest_lvl+1, line);
                        //D2LOG("{}[{}] Found {} childs for \"{}\"\n", std::string(nest_lvl,'\t'), line, n_tmp.childs_count(), fmt::join(keys, ", "))

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
                        //D2LOG("{}[{}] Found {} childs for \"{}\"\n", std::string(nest_lvl,'\t'), line, n_tmp.childs_count(), keys.front())
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
                    //D2LOG("{}[{}] Assigned {} = {}\n", std::string(nest_lvl,'\t'), line, keys.front(), child.value())
                   }
               }
           }
       }


 private:

    //-----------------------------------------------------------------------
    [[nodiscard]] static bool is_json_char(const char c) noexcept
       {
        return c==';' || c==',' || c==':' || c=='{' || c=='}';
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] bool eat_line_comment_start() noexcept
       {
        if( i<(siz-1) && buf[i]=='/' && buf[i+1]=='/' )
           {
            i += 2; // Skip "//"
            return true;
           }
        return false;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] bool eat_block_comment_start() noexcept
       {
        if( i<(siz-1) && buf[i]=='/' && buf[i+1]=='*' )
           {
            i += 2; // Skip "/*"
            return true;
           }
        return false;
       }


    //-----------------------------------------------------------------------
    void skip_block_comment()
       {
        const std::size_t line_start = line; // Store current line
        const std::size_t i_start = i; // Store current position
        while( i<i_last )
           {
            if( buf[i]=='*' && buf[i+1]=='/' )
               {
                i += 2; // Skip "*/"
                return;
               }
            else if( buf[i]=='\n' )
               {
                ++line;
               }
            ++i;
           }
        throw create_parse_error("Unclosed block comment", line_start, i_start);
       }


    //-----------------------------------------------------------------------
    // Collect a quoted key (first " not yet eat)
    [[nodiscard]] std::string_view collect_quoted_key()
       {
        ++i; // Skip initial "
        const std::size_t i_start = i;
        while( i<siz )
           {
            if( buf[i]=='\"' )
               {
                ++i; // Skip final "
                return std::string_view(buf+i_start, i-i_start-1);
               }
            else if( buf[i]=='\n' || is_json_char(buf[i]) )
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
        const std::size_t i_start = i;
        while( i<siz && !std::isspace(buf[i]) )
           {
            if( is_json_char(buf[i]) )
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
        const std::string_view key = buf[i]=='\"' ? collect_quoted_key()
                                                  : collect_identifier();
        if( key.empty() )
           {
            throw create_parse_error( "Empty key" );
           }
        //D2LOG("[{}] Collected key \"{}\"\n", line, key)
        return key;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::vector<std::string_view> extract_keys()
       {
        std::vector<std::string_view> keys;

        keys.push_back( extract_key() ); // Note: This ensures keys not empty
        skip_any_space();
        // Support multiple keys (comma separated)
        while( i<siz && buf[i]==',' )
           {
            ++i; // Skip ','
            skip_any_space();
            keys.push_back( extract_key() );
            skip_any_space();
           }
        // Here I'm expecting a colon
        if( i<siz && buf[i]==':' )
           {
            ++i; // Eat ':'
           }
        else
           {
            //create_parse_error(fmt::format("Unexpected character \'{}\' after key \"{}\"", str::escape(buf[i]), keys.back()));
            create_parse_error(fmt::format("Key \"{}\" must be followed by ':'", keys.back()));
           }

        return keys;
       }

    //-----------------------------------------------------------------------
    // Extract a (possibly quoted) string
    [[nodiscard]] std::string_view extract_value()
       {
        const std::string_view val = buf[i]=='\"' ? collect_quoted_value()
                                                  : collect_unquoted_value();
        //D2LOG("[{}] Collected value \"{}\"\n", line, val)
        return val;
       }
};




//---------------------------------------------------------------------------
// Parse json file
void parse(const std::string& file_path, const std::string_view buf, Node& root, std::vector<std::string>& issues, const bool fussy)
{
    Parser parser(file_path, buf, issues, fussy);

    try{
        parser.collect_childs_of(root);
       }
    catch(parse_error&)
       {
        throw;
       }
    catch(std::exception& e)
       {
        throw parser.create_parse_error(e.what());
       }
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

//---- end unit -------------------------------------------------------------
#endif
