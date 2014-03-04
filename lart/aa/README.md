Alias Analysis in LART
======================

The alias analyses implemented in LART are fairly expensive, and usually can't
provide on-the-fly answers. Instead, they compute a whole-module analysis that
gives points-to information for all "top level" values (mainly LLVM registers)
in the program, and for any "abstract memory locations" these values could be
pointing to.

As a rule, all alias analyses implemented in LART are "may" analyses, and
represent their result using points-to sets, where a particular pointer may
take any of the values in its points-to set. If this set is singleton, the
information is fully precise. Most importantly, the analyses guarantee that if
a particular location is *not* in a points-to set, the pointer must not take
that value.

Abstract Memory
---------------

In a running program, each memory (heap) allocation will result in a concrete
memory location being created in the program's address space and used. For the
purpose of alias analysis, we need to abstract those concrete memory locations
in some fashion, as it is impossible to statically compute the set of concrete
locations. The common abstraction used for this is assigning a single abstract
location to every callsite of a memory allocation routine.

In LART, the creation of abstract memory locations is a choice to be made by a
particular analysis. Initially, we will use the common approach of using one
abstract location for each static call that allocates memory.

Abstract memory locations are represented as unique integers.

Context and Flow Sensitivity
----------------------------

An alias analysis can be global, computing a single conservative "may point to"
solution for the entire program, meaning that for a particular pointer, it will
never "fall out" of its points-to set for the entire lifetime of the program.
In many cases, this analysis will be overly pessimistic. To that end, there are
two common ways in which to refine this global view. One computes distinct
solutions for each call-site (this is called context sensitivity), and another
for each control-flow location (this is called flow sensitivity). These two are
somewhat orthogonal: an analysis can be neither, one of them or both. 

Metadata Format
---------------

As outlined above, the analyses produce a significant quantity of data and
could require fairly long time to do so. Moreover, it is desirable that this
data be readily available to external tools, even though LART itself is not
going to be part of LLVM in foreseeable future. As such, providing analysis
results in a well-defined format embedded inside LLVM bitcode as metadata which
standard LLVM libraries and tools can read seems to be a good compromise.

This makes the analysis results persistent, and easily re-usable without a
requirement for run-time linking or exporting stable APIs from LART.

LLVM metadata is structured as a graph with labelled edges and nodes being
tuples of primitive values. The basic idea of the format is to represent
points-to sets as metadata nodes. Abstract memory locations are represented by
nodes carrying their ID. Points-to sets are represented by a single node, with
outgoing edges for each element of that set. The members of the points-to sets
are then represented again as metadata nodes, pointing to the associated
abstract memory location (the possible target of the pointer), the type of data
this points-to link represents (especially whether it can be in turn a pointer)
and for pointers-to-pointers, an edge to another points-to set for the pointer
living in that particular memory location with validity span covering that of
the parent points-to set.
