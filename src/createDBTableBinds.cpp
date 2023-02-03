/*
    The createDBTableBinds()'s function gets the data for all the tables in a specified database and
    writes separate binds (from binds.hpp) object builder functions for each of them to source code
    files.

    The function declarations to a .h/.hpp file and their definitions to a .cpp file using
    the paths that are specified in the function arguments.

*/

#include "createDBTableBinds.h"

#include <cctype>
#include <iterator>

#include "binds.hpp"
#include "getDBTables.h"
#include "typeCreationStrings.h"

void replaceWord( std::string& str, const std::string& pattern, const std::string& replacement ) {
    size_t pos = str.find( pattern );
    if ( pos != std::string::npos ) {
        size_t size = pattern.size();
        str.replace( pos, size, replacement );
    }
    else {  // In case I made a mistake in the typeCreationStrings TurnerMap
        if ( pattern == "NAME" ) {
            std::cout << pattern << " not found in " << str << '\n';
        }
    }
};

namespace {

    void setDeclHeaderAndFooter( std::ostringstream& declaration_header, std::ostringstream& declaration_footer,
                                 std::string_view db_name ) {
        std::string upper_db_name;
        std::transform( db_name.begin(), db_name.end(), std::back_inserter( upper_db_name ), ::toupper );
        std::string included_macro = std::string( "INCLUDED_" ) + upper_db_name + "BINDS_H";

        declaration_header << "// This file was generated by createDBTableBinds() function\n"
                           << "#ifndef " << included_macro << '\n'
                           << "#define " << included_macro << "\n\n"
                           << "#include \"binds.hpp\"\n\n\n";

        declaration_footer << "#endif //" << included_macro << '\n';
    }

    std::ostringstream createDefinitionHeader( std::string_view includeStr ) {
        std::ostringstream os;
        os << "// This file was generated by createDBTableBinds() function\n"
           << "#include \"" << includeStr << "\"\n\n";
        return os;
    }

    void setFileBodies( std::ostringstream& declaration_body, std::ostringstream& definition_body,
                        const std::vector<Table>& tables, unsigned long buff_size ) {
        std::for_each( tables.begin(), tables.end(), [&]( const auto& table ) {
            std::string funcReq = std::string( "Binds<InputCType> " ) + table.name + "InputBinds()";
            std::string funcRes = std::string( "Binds<OutputCType> " ) + table.name + "OutputBinds()";
            declaration_body << funcReq << ";\n" << funcRes << ";\n\n";

            std::ostringstream requestStream;
            requestStream << funcReq << "{\n    return Binds<InputCType>( make_vector<InputCType>( \n        ";
            std::ostringstream responseStream;
            responseStream << funcRes << "{\n    return Binds<OutputCType>( make_vector<OutputCType>( \n        ";
            int count = 0;
            std::for_each( table.fields.begin(), table.fields.end(), [&]( const auto& field ) {
                auto typeVariations = typeCreationStrings.at( field.externalType );
                std::string makeInputStr, makeOutputStr;
                if ( ( field.flags & UNSIGNED_FLAG ) == static_cast<int>( UNSIGNED_FLAG ) ) {
                    makeInputStr = typeVariations.cTypeInputUnsigned;
                    makeOutputStr = typeVariations.cTypeOutputUnsigned;
                }
                else {
                    makeInputStr = typeVariations.cTypeInputSigned;
                    makeOutputStr = typeVariations.cTypeOutputSigned;
                }

                replaceWord( makeInputStr, "NAME", field.name );
                replaceWord( makeOutputStr, "NAME", field.name );
                replaceWord( makeInputStr, "BUFF_SIZE", std::to_string( buff_size ) );
                replaceWord( makeOutputStr, "BUFF_SIZE", std::to_string( buff_size ) );
                requestStream << ( count++ < 2 ? "" : ",\n        " ) << makeInputStr;
                responseStream << ( count++ < 2 ? "" : ",\n        " ) << makeOutputStr;
            } );
            requestStream << " ));\n}\n";
            responseStream << " ));\n}\n\n";
            definition_body << requestStream.str() << responseStream.str();
        } );
    }

    void writeDeclarationFile( const std::ostringstream& declaration_header, const std::ostringstream& declaration_body,
                               const std::ostringstream& declaration_footer, std::string_view fileName ) {
        std::ofstream hFile( fileName.data() );
        hFile << declaration_header.str() << declaration_body.str() << declaration_footer.str();
        hFile.close();
    }

    void writeDefinitionFile( const std::ostringstream& definition_header, const std::ostringstream& definition_body,
                              std::string_view fileName ) {
        std::ofstream cppFile( fileName.data() );
        cppFile << definition_header.str() << definition_body.str();
        cppFile.close();
    }
}  // namespace

// In this function the term 'header' is used in the HTML sense as being the top of a page, declaration_header refers to
// the top of a .h/.hpp file and definition_header the top of a .cpp file.
void createDBTableBinds( std::string_view host, std::string_view user, std::string_view password,
                         std::string_view database, std::string_view declFile, std::string_view defnFile,
                         std::string_view includeStr, unsigned long buff_size ) {

    std::ostringstream declaration_header, declaration_footer;
    setDeclHeaderAndFooter( declaration_header, declaration_footer, database );

    std::ostringstream definition_header = createDefinitionHeader( includeStr );

    std::ostringstream declaration_body, definition_body;
    auto tables = getDBTables( host, user, password, database );
    setFileBodies( declaration_body, definition_body, tables, buff_size );

    writeDeclarationFile( declaration_header, declaration_body, declaration_footer, declFile );

    writeDefinitionFile( definition_header, definition_body, defnFile );
}