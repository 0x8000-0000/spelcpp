/**
 * @file engine.h
 * @brief Interface for the spelling checker engine
 * @copyright 2020 Florin Iucha
 */

/*
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef SPELCPP_ENGINE_H_INCLUDED
#define SPELCPP_ENGINE_H_INCLUDED

#include <string>
#include <string_view>
#include <unordered_map>

namespace splcp
{
class SpellingEngine
{
public:
   /** Indicates that a definition has been observed by the tokenizer
    *
    * @param tokenText is the text of the definition token
    * @param sourceFile is the absolute path to the file where the token is located
    * @param lineNo is the line number where the token starts
    * @param columnNo is the colum number where the token starts
    */
   void observeDefinition(std::string_view tokenText, std::string_view sourceFile, size_t lineNo, size_t columnNo);

   /** Indicates that a string literal has been observed by the tokenizer
    *
    * @param tokenText is the text of the literal token
    * @param sourceFile is the absolute path to the file where the token is located
    * @param lineNo is the line number where the token starts
    * @param columnNo is the colum number where the token starts
    */
   void observeStringLiteral(std::string_view tokenText, std::string_view sourceFile, size_t lineNo, size_t columnNo);

   /** Indicates that a comment has been observed by the tokenizer
    *
    * @param tokenText is the text of the comment token
    * @param sourceFile is the absolute path to the file where the token is located
    * @param lineNo is the line number where the token starts
    * @param columnNo is the colum number where the token starts
    */
   void observeComment(std::string_view tokenText, std::string_view sourceFile, size_t lineNo, size_t columnNo);

   /*
    * TODO: macros?
    */
private:
   std::unordered_map<std::string, size_t> m_identifiers;
};
} // namespace splcp

#endif // SPELCPP_ENGINE_H_INCLUDED
