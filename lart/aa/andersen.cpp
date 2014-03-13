// -*- C++ -*- (c) 2014 Petr Rockai <me@mornfall.net>

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>

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
            for ( auto x : c.right->_pointsto )
                copyout( x->_pointsto, updated );
            break;
        case Constraint::Copy:
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
        _amls.back()->aml = true;
        constraint( Constraint::Ref, i, _amls.back() );
    }

    if ( llvm::isa< llvm::StoreInst >( i ) )
        constraint( Constraint::Store, i.getOperand( 1 ), i.getOperand( 0 ) );

    if ( llvm::isa< llvm::LoadInst >( i ) )
        constraint( Constraint::Deref, &i, i.getOperand( 0 ) );

    if ( llvm::isa< llvm::BitCastInst >( i ) ||
         llvm::isa< llvm::IntToPtrInst >( i ) ||
         llvm::isa< llvm::PtrToIntInst >( i ) )
        constraint( Constraint::Copy, &i, i.getOperand( 0 ) );

    /* TODO: gep */
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

typedef llvm::ArrayRef< llvm::Value * > ValsRef;
using llvm::MDNode;

MDNode *Andersen::annotate( llvm::Module &m, Node *n, std::set< Node * > &seen )
{
    MDNode *mdn;

    /* already converted */
    if ( _mdnodes.count( n ) )
        return _mdnodes.find( n )->second;

    /* loop has formed, insert a temporary node and let the caller handle this */
    if ( seen.count( n ) ) {
        if ( _mdtemp.count( n ) )
            return _mdtemp.find( n )->second;
        mdn = MDNode::getTemporary( m.getContext(), ValsRef() );
        _mdtemp.insert( std::make_pair( n, mdn ) );
        return mdn;
    }

    seen.insert( n );
    int id = seen.size();

    /* make the points-to set first */
    {
        std::set< Node * > &pto = n->_pointsto;
        llvm::Value **v = new llvm::Value *[ pto.size() ], **vi = v;

        int i = 0;
        for ( Node *p : pto )
            *vi++ = annotate( m, p, seen );

        mdn = MDNode::get( m.getContext(), ValsRef( v, pto.size() ) );
    }

    /* now make the AML node */
    if ( n->aml ) {
        llvm::Value **v = new llvm::Value *[3];
        v[0] = llvm::ConstantInt::get( m.getContext(), llvm::APInt( 32, id ) );
        v[1] = _rootctx;
        v[2] = mdn;
        llvm::ArrayRef< llvm::Value * > vals( v, 3 );
        mdn = MDNode::get( m.getContext(), vals );
    }

    _mdnodes.insert( std::make_pair( n, mdn ) );

    /* close the cycles, if any */
    if ( _mdtemp.count( n ) ) {
        _mdtemp.find( n )->second->replaceAllUsesWith( mdn );
        _mdtemp.erase( n );
    }

    return mdn;
}

void Andersen::annotate( llvm::Instruction &insn, std::set< Node * > &seen )
{
    llvm::Module &m = *insn.getParent()->getParent()->getParent();

    if ( _nodes.count( &insn ) )
        insn.setMetadata( "aa_def", annotate( m, _nodes[ &insn ], seen ) );

    int size = 0;
    std::vector< Node * > ops;
    for ( int i = 0; i < insn.getNumOperands(); ++i ) {
        llvm::Value *op = insn.getOperand( i );
        if ( !_nodes.count( op ) )
            continue;
        ops.push_back( _nodes[ op ] );
        size += ops.back()->_pointsto.size();
    }

    llvm::Value **v = new llvm::Value *[ size ], **vi = v;
    for ( Node *n : ops )
        for ( Node *p : n->_pointsto )
            *vi++ = annotate( m, p, seen );

    insn.setMetadata( "aa_use", MDNode::get( m.getContext(), ValsRef( v, size ) ) );
}

void Andersen::annotate( llvm::Module &m ) {
    llvm::ArrayRef< llvm::Value * > ctxv(
        llvm::MDString::get( m.getContext(), "lart.aa-root-context" ) );
    _rootctx = MDNode::get( m.getContext(), ctxv );

    std::set< Node * > seen;

    for ( auto &f : m )
        for ( auto &b: f )
            for ( auto &i : b )
                annotate( i, seen );
    assert( _mdtemp.empty() );
}

}
}
