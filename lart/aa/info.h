// -*- C++ -*- (c) 2014 Petr Rockai <me@mornfall.net>

#include <llvm/IR/Module.h>

namespace lart {
namespace aa {

/*
 * This class provides convenient access to the metadata that encodes pointer
 * information in LLVM bitcode files. The class itself does not cache anything,
 * all queries reflect the current state stored in the associated Module.
 */

struct Info {
    Info( Module *m );

private:
    Module *_module;
};

}
}
