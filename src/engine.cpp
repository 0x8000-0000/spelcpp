/**
 * @file engine.cpp
 * @brief Spelling checker engine implementation
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

#include "engine.h"

#include <fmt/format.h>

#include <utility>

void splcp::SpellingEngine::observeDefinition(std::string_view tokenText,
                                              std::string_view sourceFile,
                                              size_t           lineNo,
                                              size_t           columnNo)
{
   std::string tokenString{tokenText};

   auto iter = m_identifiers.find(tokenString);
   if (iter != m_identifiers.end())
   {
      iter->second++;
      return;
   }

   fmt::print("Found definition for {} in {}:{}:{}\n", tokenText, sourceFile, lineNo, columnNo);

   m_identifiers.emplace(std::move(tokenString), 1);
}

void splcp::SpellingEngine::observeComment(std::string_view tokenText,
                                           std::string_view sourceFile,
                                           size_t           lineNo,
                                           size_t           columnNo)
{
   fmt::print("Found comment: {} at {}:{}:{}\n", tokenText, sourceFile, lineNo, columnNo);
}

void splcp::SpellingEngine::observeStringLiteral(std::string_view tokenText,
                                                 std::string_view sourceFile,
                                                 size_t           lineNo,
                                                 size_t           columnNo)
{
   fmt::print("Found literal: {} at {}:{}:{}\n", tokenText, sourceFile, lineNo, columnNo);
}
