#include <clang-c/CXCompilationDatabase.h>
#include <clang-c/Index.h>

#include <cxxopts.hpp>

#include <fmt/printf.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

bool verbose = false;

void processTranslationUnit(std::string_view fileName, const std::vector<std::string>& /* arguments */)
{
   if (verbose)
   {
      fmt::print("Processing {}\n", fileName);
   }
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
