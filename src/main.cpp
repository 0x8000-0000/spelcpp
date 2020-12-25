/**
 * @file main.cpp
 * @brief Application entry point; process program arguments then dispatch
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

#include <clang-c/CXCompilationDatabase.h>
#include <clang-c/Index.h>

#include <cxxopts.hpp>

#include <fmt/printf.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bool verbose = false;

std::string_view getStringView(CXString clangString)
{
   const auto* cStr = clang_getCString(clangString);
   return {cStr, strlen(cStr)};
}

void processCursor(CXCursor clangCursor, splcp::SpellingEngine& engine)
{
   if (clang_isCursorDefinition(clangCursor) != 0)
   {
      CXSourceLocation location = clang_getCursorLocation(clangCursor);

      if (clang_Location_isInSystemHeader(location) != 0)
      {
         return;
      }

      /*
       * TODO: Further filter out if the symbol is in a header outside of the
       * project space.
       *
       * Right now this check is too coarse it filters out all header files.
       */
      if (clang_Location_isFromMainFile(location) == 0)
      {
         return;
      }

      CXString tokenText = clang_getCursorSpelling(clangCursor);

      auto tokenString = getStringView(tokenText);

      if (tokenString.empty())
      {
         clang_disposeString(tokenText);
         return;
      }

      CXString definitionLocation;
      unsigned line   = 0;
      unsigned column = 0;
      clang_getPresumedLocation(location, &definitionLocation, &line, &column);

      engine.observeDefinition(tokenString, getStringView(definitionLocation), line, column);

      clang_disposeString(definitionLocation);
      clang_disposeString(tokenText);
   }
}

CXChildVisitResult visitTranslationUnit(CXCursor cursor, CXCursor /* parent */, CXClientData clientData)
{
   auto* engine = reinterpret_cast<splcp::SpellingEngine*>(clientData);

   processCursor(cursor, *engine);
   clang_visitChildren(cursor, visitTranslationUnit, clientData);

   return CXChildVisit_Continue;
}

void processLiteral(const CXTranslationUnit& translationUnit, const CXToken& token, splcp::SpellingEngine& engine)
{
   CXSourceLocation tokenLocation = clang_getTokenLocation(translationUnit, token);

   if (clang_Location_isInSystemHeader(tokenLocation) != 0)
   {
      return;
   }

   CXString   tokenText   = clang_getTokenSpelling(translationUnit, token);
   const auto tokenString = getStringView(tokenText);

   if (tokenString.empty() || (tokenString[0] != '"'))
   {
      clang_disposeString(tokenText);
      return;
   }

   CXString literalLocation;
   unsigned line   = 0;
   unsigned column = 0;
   clang_getPresumedLocation(tokenLocation, &literalLocation, &line, &column);

   engine.observeStringLiteral(tokenString, getStringView(literalLocation), line, column);

   clang_disposeString(literalLocation);
   clang_disposeString(tokenText);
}

void processComment(const CXTranslationUnit& translationUnit, const CXToken& token, splcp::SpellingEngine& engine)
{
   CXSourceLocation tokenLocation = clang_getTokenLocation(translationUnit, token);

   if (clang_Location_isInSystemHeader(tokenLocation) != 0)
   {
      return;
   }

   CXString commentText = clang_getTokenSpelling(translationUnit, token);

   CXString commentLocation;
   unsigned line   = 0;
   unsigned column = 0;
   clang_getPresumedLocation(tokenLocation, &commentLocation, &line, &column);

   engine.observeComment(getStringView(commentText), getStringView(commentLocation), line, column);

   clang_disposeString(commentLocation);
   clang_disposeString(commentText);
}

void processTranslationUnit(std::string_view fileName, const std::vector<std::string>& arguments)
{
   if (verbose)
   {
      fmt::print("Processing {}\n", fileName);
   }

   auto* clangIndex = clang_createIndex(/* excludeDeclarationsFromPCH = */ 0,
                                        /* displayDiagnostics         = */ 0);

   CXTranslationUnit translationUnitPtr = nullptr;

   const unsigned parsingOptions = static_cast<unsigned>(CXTranslationUnit_DetailedPreprocessingRecord) |
                                   static_cast<unsigned>(CXTranslationUnit_KeepGoing) |
                                   static_cast<unsigned>(CXTranslationUnit_CreatePreambleOnFirstParse) |
                                   clang_defaultEditingTranslationUnitOptions();

   std::vector<const char*> argPointers;
   argPointers.reserve(arguments.size());
   for (const auto& str : arguments)
   {
      argPointers.push_back(str.data());
   }

   const CXErrorCode parseError = clang_parseTranslationUnit2(
      /* CIdx                  = */ clangIndex,
      /* source_filename       = */ fileName.data(),
      /* command_line_args     = */ argPointers.data(),
      /* num_command_line_args = */ static_cast<int>(argPointers.size()),
      /* unsaved_files         = */ nullptr,
      /* num_unsaved_files     = */ 0,
      /* options               = */ parsingOptions,
      /* out_TU                = */ &translationUnitPtr);

   if ((parseError == CXError_Success) && (nullptr != translationUnitPtr))
   {
      /*
       * Collect all the definitions
       */
      splcp::SpellingEngine engine;
      CXCursor              cursor = clang_getTranslationUnitCursor(translationUnitPtr);
      clang_visitChildren(cursor, visitTranslationUnit, &engine);

      CXSourceRange range = clang_getCursorExtent(cursor);

      /*
       * Collect the comments
       */
      CXToken* tokens    = nullptr;
      unsigned numTokens = 0;
      clang_tokenize(translationUnitPtr, range, &tokens, &numTokens);

      for (unsigned ii = 0; ii < numTokens; ++ii)
      {
         switch (clang_getTokenKind(tokens[ii]))
         {
         case CXToken_Comment:
            processComment(translationUnitPtr, tokens[ii], engine);
            break;

         case CXToken_Literal:
            processLiteral(translationUnitPtr, tokens[ii], engine);
            break;

         default:
            break;
         }
      }

      clang_disposeTokens(translationUnitPtr, tokens, numTokens);
   }

   if (nullptr != translationUnitPtr)
   {
      clang_disposeTranslationUnit(translationUnitPtr);
   }

   clang_disposeIndex(clangIndex);
}

int main(int argc, char* argv[])
{
   try
   {
      std::string projectDirectory;

      cxxopts::Options options("spelcpp", "Spelling Checker for C/C++ Source Code");

      options.add_options()("help", "Print help")("v,verbose", "Verbose")(
         "positional",
         "Positional arguments: these are the arguments that are entered "
         "without an option",
         cxxopts::value<std::vector<std::string>>());

      options.add_options("Input")("d,dir", "Directory", cxxopts::value<std::string>(projectDirectory));

      options.parse_positional({"positional"});

      const auto result = options.parse(argc, argv);

      if (result.count("help") > 0)
      {
         fmt::print("{}\n", options.help({"", "Input"}));
         return 0;
      }

      if (result.count("verbose") > 0)
      {
         verbose = true;
      }

      if (projectDirectory.empty())
      {
         fmt::print("Error: Project directory is missing.\n\n");
         fmt::print("{}\n", options.help({"Input"}));
         return 1;
      }

      const std::filesystem::path projectPath{projectDirectory};
      if (!std::filesystem::exists(projectPath))
      {
         fmt::print("Error: Cannot find the specified project directory '{}'\n", projectDirectory);
         return 1;
      }

      const auto compilationDatabaseFile = projectPath / "compile_commands.json";
      if (!std::filesystem::exists(compilationDatabaseFile))
      {
         fmt::print("Error: Cannot find a compilation database in the specified project directory '{}'\n",
                    projectDirectory);
         return 1;
      }

      if (result.count("positional") == 0)
      {
         fmt::print("Error: Missing input files.\n");
         return 1;
      }

      const auto& positionalArguments = result["positional"].as<std::vector<std::string>>();

      if (verbose)
      {
         fmt::print("Processing {}\n", compilationDatabaseFile);

         fmt::print("Looking for\n");
         for (const auto& str : positionalArguments)
         {
            fmt::print("   {}\n", str);
         }
      }

      CXCompilationDatabase_Error ccderror      = CXCompilationDatabase_NoError;
      const auto                  dirName       = std::filesystem::canonical(projectPath).string();
      CXCompilationDatabase compilationDatabase = clang_CompilationDatabase_fromDirectory(dirName.data(), &ccderror);
      if (CXCompilationDatabase_NoError != ccderror)
      {
         fmt::print("Error: Failed to open compilation database: {}\n", ccderror);
         return 1;
      }

      CXIndex index = clang_createIndex(/* excludeDeclarationsFromPCH = */ 0,
                                        /* displayDiagnostics         = */ 0);

      CXCompileCommands compileCommands = clang_CompilationDatabase_getAllCompileCommands(compilationDatabase);

      const auto compilationCount = clang_CompileCommands_getSize(compileCommands);

      if (verbose)
      {
         fmt::print("Project contains {} translation unit(s).\n", compilationCount);
      }

      for (size_t ii = 0; ii < compilationCount; ++ii)
      {
         CXCompileCommand compileCommand = clang_CompileCommands_getCommand(compileCommands, ii);

         CXString    dirNameString    = clang_CompileCommand_getDirectory(compileCommand);
         CXString    fileNameCxString = clang_CompileCommand_getFilename(compileCommand);
         std::string fileNameString{clang_getCString(fileNameCxString)};

         if (verbose)
         {
            fmt::print("  {}\n", fileNameString);
         }

         /*
          * TODO: check if the filename matches one of the positional arguments
          */

         const unsigned           argCount = clang_CompileCommand_getNumArgs(compileCommand);
         std::vector<std::string> arguments;
         arguments.reserve(argCount);

         bool skipFileNames = false;

         for (size_t jj = 0; jj < argCount; ++jj)
         {
            if (skipFileNames)
            {
               skipFileNames = false;
               continue;
            }

            CXString    cxString     = clang_CompileCommand_getArg(compileCommand, jj);
            const char* argumentText = clang_getCString(cxString);

            if ((argumentText[0] == '-') && ((argumentText[1] == 'c') || (argumentText[1] == 'o')))
            {
               skipFileNames = true;
            }
            else
            {
               arguments.emplace_back(argumentText);
            }

            clang_disposeString(cxString);
         }

         /*
          * process
          */

         processTranslationUnit(fileNameString, arguments);

         clang_disposeString(fileNameCxString);
         clang_disposeString(dirNameString);
      }

      clang_CompileCommands_dispose(compileCommands);
      clang_disposeIndex(index);
      clang_CompilationDatabase_dispose(compilationDatabase);
   }
   catch (const cxxopts::OptionParseException& ope)
   {
      fmt::print("Failed to parse command-line options: {}\n", ope.what());
      return 1;
   }
   catch (...)
   {
      fmt::print("Unexpected exception caught in main!\n");
      return 1;
   }

   return 0;
}
