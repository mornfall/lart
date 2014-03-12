// -*- C++ -*- (c) 2014 Petr Rockai <me@mornfall.net>

#include <lart/aa/andersen.h>

namespace lart {
namespace aa {

void Andersen::solve( Constraint c ) {
    std::set< Node * > updated;

    // ...

    if ( updated != c.left->_pointsto ) {
        c.left->_pointsto = updated;
        if ( !c.left->queued ) {
            _worklist.push_back( c.left );
            c.left->queued = true;
        }
    }
}

void Andersen::solve( Node *n ) {
    /* TODO: optimize */
    std::vector< Constraint >::iterator i;
    for ( i = _constraints.begin(); i != _constraints.end(); ++i )
        if ( i->left == n || i->right == n )
            solve( *i );
}

void Andersen::solve() {
    while ( !_worklist.empty() ) {
        Node *n = _worklist.front();
        _worklist.pop_front();
        n->queued = false;
        solve( n );
    }
}

void Andersen::build( llvm::Instruction &i ) {
    // build constraints
}

void Andersen::build( llvm::Module &m ) {
    for ( auto v = m.global_begin(); v != m.global_end(); ++ v )
        ;

    for ( auto &f : m )
        for ( auto &b: f )
            for ( auto &i : b )
                build( i );
}

void Andersen::annotate( llvm::Module &m ) {
    // build metadata
}

}
}
