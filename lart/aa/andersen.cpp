// -*- C++ -*- (c) 2014 Petr Rockai <me@mornfall.net>

#include <llvm/IR/Instructions.h>

#include <lart/aa/andersen.h>

namespace lart {
namespace aa {

void Andersen::push( Node *n ) {
    if ( n->queued )
        return;

    _worklist.push_back( n );
    n->queued = true;
}

Andersen::Node *Andersen::pop() {
    Node *n = _worklist.front();
    _worklist.pop_front();
    n->queued = false;
    return n;
}

template< typename Cin, typename Cout >
void copyout( Cin &in, Cout &out ) {
    std::copy( in.begin(), in.end(), std::inserter( out, out.begin() ) );
}

void Andersen::solve( Constraint c ) {
    std::set< Node * > updated = c.left->_pointsto;

    switch ( c.t ) {
        case Constraint::Ref:
            updated.insert( c.right );
            break;
        case Constraint::Deref:
            copyout( c.right->_pointsto, updated );
            break;
        case Constraint::Store:
            for ( auto x : c.left->_pointsto ) {
                copyout( c.right->_pointsto, x->_pointsto );
                push( x );
            }
            break;
    }

    if ( updated != c.left->_pointsto ) {
        c.left->_pointsto = updated;
        push( c.left );
    }
}

void Andersen::solve( Node *n ) {
    /* TODO: optimize */
    std::vector< Constraint >::iterator i;
    for ( i = _constraints.begin(); i != _constraints.end(); ++i )
        if ( i->left == n || i->right == n )
            solve( *i );
}

void Andersen::solve()
{
    for ( auto i : _nodes )
        push( i.second );
    for ( auto i : _amls )
        push( i );

    while ( !_worklist.empty() )
        solve( pop() );
}

void Andersen::build( llvm::Instruction &i ) {

    if ( llvm::isa< llvm::AllocaInst >( i ) ) {
        _amls.push_back( new Node );
        constraint( Constraint::Ref, i, _amls.back() );
    }

    if ( llvm::isa< llvm::StoreInst >( i ) )
        constraint( Constraint::Store, i.getOperand( 1 ), i.getOperand( 0 ) );

    if ( llvm::isa< llvm::LoadInst >( i ) )
        constraint( Constraint::Deref, &i, i.getOperand( 0 ) );

    /* TODO: copy, gep */
    /* TODO: heap variables (malloc &c.) */
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
