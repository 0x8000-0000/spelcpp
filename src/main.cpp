#include <clang-c/CXCompilationDatabase.h>
#include <clang-c/Index.h>

#include <cxxopts.hpp>

#include <fmt/printf.h>

#include <filesystem>

int main(int argc, char* argv[])
{
   try
   {
      cxxopts::Options options(argv[0], " - Spelling Checker for C/C++ Source Code");
      options.positional_help("[optional args]").show_positional_help();

      options.allow_unrecognised_options().add_options()("help", "Print help");

      const auto result = options.parse(argc, argv);

      if (result.count("help"))
      {
         fmt::print("{}\n", options.help({"", "Group"}));
         return 0;
      }
   }
   catch (...)
   {
      fmt::print("Unexpected exception caught in main!\n");
      return 1;
   }

   return 0;
}
