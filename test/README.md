The tests in this directory need a recent version of divine (with a working
compile --llvm support, as well as with LLVM verification support) to be
available on PATH.

DIVINE is used to compile C (and possibly C++) files to standalone bitcode
programs including all the required runtime support code.  While the programs
themselves are fairly small, the resulting LLVM modules are of varying size,
depending on which parts of the standard library are linked in.

Additionally, we use `divine verify -p pointsto` to check that the metadata
generated by lart's AA is sound. Support for this is only available in
development versions of DIVINE.
