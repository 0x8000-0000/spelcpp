Implementation Notes
====================

Extracting Symbols
------------------

Symbols are extracted from source code using the Clang tooling library.


Processing
----------

1. Run a parsing step to build the AST and locate all symbol definitions.
The symbol definitions are more important than "all tokens" because the other
tokens could be defined in external headers upon which your project has no
control. This produces two associative array: one which maps from a symbol
name to its first encountered location, and a second one which maintains the
counts for each symbol name.

2. Take the set of symbol definition names and split them down assuming the
usual [cC]amelCase and snake\_case conventions. Eliminate known abbreviations
using a dictionary. Convert to lower case and eliminate duplicates to create
the symbol definition subtoken set.

3. Run a tokenization step to extract the comments and the string literals.

4. Process the comments by:

   * converting all punctuation to space,

   * tokenizing on space boundary,

   * eliminating all strings that already appear in the symbol definitions.

   Similar to step 2 above, split down the tokens further assuming [cC]amelCase
   and snake\_case.

5. Process the string literals in the same way as comments.


Checking the spelling
---------------------

After collecting all the unique strings to be checked, filter out the tokens
that are present in the project dictionary then use an external spelling
checker library such as aspell or hunspell to perform the checking.
