// -*- C++ -*- (c) 2014 Petr Rockai <me@mornfall.net>

#include <llvm/PassManager.h>

namespace lart {
namespace aa {

enum Type { Andersen };

struct Pass : llvm::ModulePass
{
    static char ID;
    Pass( Type t ) : llvm::ModulePass( ID ) {}
    virtual ~Pass() {}

    bool runOnModule( llvm::Module &m ) {
        return false;
    }
};

}
}
