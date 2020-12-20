#include <clang-c/CXCompilationDatabase.h>
#include <clang-c/Index.h>

#include <cxxopts.hpp>

#include <fmt/printf.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

bool verbose = false;

struct Accumulator
{
   int tokens = 0;

   int level = 0;

   std::unordered_map<std::string, unsigned> identifiers;
};

void processCursor(CXCursor clangCursor, Accumulator& accumulator)
{
   if (clang_isCursorDefinition(clangCursor) != 0)
   {
      CXSourceLocation location = clang_getCursorLocation(clangCursor);

      if (clang_Location_isInSystemHeader(location))
      {
         return;
      }

      /*
       * Further filter out if the symbol is in a header outside of the
       * project space.
       */
      if (!clang_Location_isFromMainFile(location))
      {
         return;
      }

      CXString    name = clang_getCursorSpelling(clangCursor);
      std::string tokenString{clang_getCString(name)};
      clang_disposeString(name);

      if (tokenString.empty())
      {
         return;
      }

      auto iter = accumulator.identifiers.find(tokenString);
      if (iter != accumulator.identifiers.end())
      {
         iter->second++;
         return;
      }

      CXString definitionLocation;
      unsigned line;
      unsigned column;
      clang_getPresumedLocation(location, &definitionLocation, &line, &column);

      fmt::print(
         "Found definition for {} in {}:{}:{}\n", tokenString, clang_getCString(definitionLocation), line, column);

      clang_disposeString(definitionLocation);

      accumulator.identifiers.emplace(std::move(tokenString), 1);
   }
}

CXChildVisitResult visitTranslationUnit(CXCursor cursor, CXCursor /* parent */, CXClientData clientData)
{
   auto* accumulator = reinterpret_cast<Accumulator*>(clientData);

   processCursor(cursor, *accumulator);
   accumulator->level++;

   accumulator->tokens++;
   clang_visitChildren(cursor, visitTranslationUnit, clientData);

   accumulator->level--;
   return CXChildVisit_Continue;
}

void processLiteral(const CXTranslationUnit& translationUnit, const CXToken& token)
{
   CXSourceLocation tokenLocation = clang_getTokenLocation(translationUnit, token);

   if (clang_Location_isInSystemHeader(tokenLocation))
   {
      return;
   }

   CXString tokenText = clang_getTokenSpelling(translationUnit, token);

   CXString literalLocation;
   unsigned line;
   unsigned column;
   clang_getPresumedLocation(tokenLocation, &literalLocation, &line, &column);

   fmt::print("Found literal: {} at {}:{}:{}\n",
              clang_getCString(tokenText),
              clang_getCString(literalLocation),
              line,
              column);

   clang_disposeString(literalLocation);

   clang_disposeString(tokenText);
}

void processComment(const CXTranslationUnit& translationUnit, const CXToken& token)
{
   CXSourceLocation tokenLocation = clang_getTokenLocation(translationUnit, token);

   if (clang_Location_isInSystemHeader(tokenLocation))
   {
      return;
   }

   CXString commentBlock = clang_getTokenSpelling(translationUnit, token);

   CXString commentLocation;
   unsigned line;
   unsigned column;
   clang_getPresumedLocation(tokenLocation, &commentLocation, &line, &column);

   fmt::print("Found comment: {} at {}:{}:{}\n",
              clang_getCString(commentBlock),
              clang_getCString(commentLocation),
              line,
              column);

   clang_disposeString(commentLocation);
   clang_disposeString(commentBlock);
}

void processTranslationUnit(std::string_view fileName, const std::vector<std::string>& arguments)
{
   if (verbose)
   {
      fmt::print("Processing {}\n", fileName);
   }

   auto clangIndex = clang_createIndex(/* excludeDeclarationsFromPCH = */ 0,
                                       /* displayDiagnostics         = */ 0);

   CXTranslationUnit translationUnitPtr = nullptr;

   const unsigned parsingOptions = CXTranslationUnit_DetailedPreprocessingRecord | CXTranslationUnit_KeepGoing |
                                   CXTranslationUnit_CreatePreambleOnFirstParse |
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
      Accumulator accumulator;
      CXCursor    cursor = clang_getTranslationUnitCursor(translationUnitPtr);
      clang_visitChildren(cursor, visitTranslationUnit, &accumulator);

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
            processComment(translationUnitPtr, tokens[ii]);
            break;

         case CXToken_Literal:
            processLiteral(translationUnitPtr, tokens[ii]);
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

      if (result.count("help"))
      {
         fmt::print("{}\n", options.help({"", "Input"}));
         return 0;
      }

      if (result.count("verbose"))
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
               arguments.push_back(argumentText);
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
