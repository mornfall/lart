The tests in this directory need divine with a working compile --llvm support
to be available on PATH. This is used to compile C (and possibly C++) files to
standalone bitcode programs including all the required runtime support code.
While the programs themselves are fairly small, the resulting LLVM modules are
of moderate sizes, exercising the code fairly thoroughly.
