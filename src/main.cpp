#include <clang-c/CXCompilationDatabase.h>
#include <clang-c/Index.h>

#include <cxxopts.hpp>

#include <fmt/printf.h>

#include <filesystem>
#include <string>

int main(int argc, char* argv[])
{
   try
   {
      std::string projectDirectory;
      bool        verbose = false;

      cxxopts::Options options("spelcpp", "Spelling Checker for C/C++ Source Code");

      options.add_options()("help", "Print help")("v,verbose", "Verbose");

      options.add_options("Input")("d,dir", "Directory", cxxopts::value<std::string>(projectDirectory));

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

      if (verbose)
      {
         fmt::print("Processing {}\n", compilationDatabaseFile);
      }

      CXCompilationDatabase_Error ccderror      = CXCompilationDatabase_NoError;
      const auto                  dirName       = std::filesystem::canonical(projectPath).string();
      CXCompilationDatabase compilationDatabase = clang_CompilationDatabase_fromDirectory(dirName.data(), &ccderror);
      if (CXCompilationDatabase_NoError != ccderror)
      {
         fmt::print("Error: Failed to open compilation database: {}\n", ccderror);
         return 1;
      }

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
