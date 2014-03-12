// -*- C++ -*- (c) 2014 Petr Rockai <me@mornfall.net>

#include <llvm/IR/Value.h>
#include <llvm/IR/Module.h>

#include <vector>
#include <deque>
#include <set>
#include <map>

namespace lart {
namespace aa {

struct Andersen {
    struct Node {
        bool queued;
        std::set< Node * > _pointsto;
        llvm::Value *_value;

        Node( llvm::Value *v ) : queued( false ), _value( v ) {}
    };

    struct Constraint {
        enum Type {
            Ref,   //  left = &right (alloc)
            Copy,  //  left =  right  (store)
            Deref, //  left = *right (load)
            Store, // *left =  right  (store)
            GEP    //  left =  right + offset (getelementptr)
        };
        /* TODO: GEP needs a representation for offsets */
        Node *left, *right;
    };

    /* each llvm::Value can have (at most) one associated Node */
    std::map< llvm::Value *, Node * > _nodes;
    std::vector< Constraint > _constraints;
    std::deque< Node * > _worklist;

    void build( llvm::Instruction &i ); // set up _nodes and _constraints
    void build( llvm::Module &m ); // set up _nodes and _constraints
    void solve( Constraint c ); // process the effect of a single constraint
    void solve( Node *n ); // process the effect of a single node
    void solve(); // compute points-to sets for all nodes
    void annotate( llvm::Module &m ); // build up metadata nodes
};

}
}
