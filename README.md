LART pour L'Art
===============

LART = LLVM Abstraction & Refinement Tool. The goal of this tool is to provide
LLVM-to-LLVM transformations that implement various program abstractions. The
resulting programs are, in terms of the instruction set, normal, concrete LLVM
programs that can be executed and analysed. Extra information about the
abstraction(s) in effect over a (fragment of) a program is inserted using
special LLVM intrinsic functions and LLVM metadata nodes.

LART provides both a standalone tool that processes on-disk bitcode files, as
well as a framework that can be integrated into complex LLVM-based tools. The
main motivation behind LART is to provide a "preprocessor" for LLVM-based model
checkers and other analysis tools, simplifying their job by reducing the
problem size without compromising soundness of the analyses.

The abstractions implemented by LART can be usually refined based on specific
instructions about which "part" of the abstraction is too rough (an abstraction
which is too rough will create spurious errors visible to subsequent analyses
but not present in the original program).

Abstractions for LLVM Bitcode
-----------------------------

The purpose of the entire exercise is to abstract away information from LLVM
bitcode, making subsequent analyses more efficient (at the expense of some
precision). To this end, we mainly need to be able to encode non-deterministic
choice in LLVM programs, which can be done simply through a special-purpose
function (similar to LLVM intrinsics). The function is named @lart.choice,
takes a pair of bounds as arguments and non-deterministically returns a value
that falls between those bounds.

This extension to LLVM semantics needs to be recognized by the downstream tool.
This is also the only crucial deviation from standard LLVM bitcode. Many
analysis tools will already implement a similar mechanism, either internally or
even with an external interface. Adapting tools without support for
@lart.choice to work with LART is usually very straightforward.

There are other special-purpose functions provided by LART, namely the
@lart.meta.* family, but as far as these instructions are concerned, most tools
will be able to safely ignore their existence, just like with existing
@llvm.dbg.* calls. Program transformations would be expected to retain those
calls in case LART is called in to refine an abstraction (each abstraction
provided by LART comes with a corresponding refinement procedure, which will
often need to find the @lart.meta calls inserted by the abstraction).

While most traditional abstraction engines work as interpreters, abstractions
can also be "compiled" into programs. Instead of (re-)interpreting instructions
symbolically, the symbolic instructions can be compiled. In case of predicate
abstraction, the resulting bitcode will directly manipulate and use predicate
valuations instead of concrete variables. As explained above, the important
difference is that the bitcode needs to make non-deterministic choices, since
some predicates may have indeterminate valuations (are both true and false).
Some variables could be even abstracted away entirely, and all tests on such
variables will yield both yes and no answers.

Sources of Non-Determinism
==========================

For analysis purposes, let's assume that we have a program which interacts with
its environment. Any of those interactions could lead to a number of different
outcomes observable by the program, and we enumerate those as non-deterministic
choice. This becomes a major problem quickly, as the choices compound
exponentially and the reachable state space of the program inflates in a
combinatorial explosion.

While part of this combinatorial explosion is unavoidable, in many practical
programs, most of it will be due to noise, choices mostly irrelevant with
regards to the programs' correctness. A typical example would be message
payloads in a reliable network protocol: it is important that the message
coming in is the same as the one coming out, but other than this simple fact,
the actual data is unimportant. In some cases, the programmer will supply this
information in some form: in a unit testsuite of the implementation, the
payload will be mostly dummy data. There will be a few cases of "tricky"
payloads, that could interfere with the protocol's data encoding and similar
issues. However, for the most part, the payloads would be "foo"s and "bar"s.

Basically, the programmer has intuitively abstracted away the payload in their
testcases. Unfortunately, we can't rely on programmers doing this manual
abstraction everywhere and for all kinds of data. While the programmer has
unique insight into the structure of the program that allows him to spot
abstraction opportunities, an automated tool has no such fairy at its disposal.

Arrays of Bytes
---------------

The tools that work with LLVM bitcode have a fairly limited understanding of
what is going on in a program. Data pours in with no obvious structure: the
program reads bytes from a file or a socket, usually in blocks through a
fixed-size buffer. A faithful "non-deterministic" simulation of the program's
environment would give a random-sized block of random bytes at each invocation
of the `read` syscall (bounded by the output buffer size, of course). The
program will inevitably look at the data, mostly byte-by-byte and make
decisions based on actual values of those bytes. Clearly, enumerating all those
possibilities is extremely expensive, and most of that work is completely
useless as well: vast majority of the inputs will quickly take the program down
a path that rightly rejects the input as garbage.

Structured Input
----------------

While analysis of "raw programs" is desirable, it is also very ambitious. A
middle ground can be struck by asking the programmer to provide verification
"drivers" -- basically, unit tests. However, in a traditional unit test,
everything is fixed: all inputs are hard-coded in the test itself. An extension
of this approach is to allow unit tests to specify ranges of values (say, an
arbitrary integer between 0 and 10) instead of constants. This is something
real-world programs often do, using various strategies to provide the inputs --
whether by generating random inputs (like QuickCheck), providing all "small"
inputs (using some metric on the inputs; like SmallCheck), using static
analysis to come up with "interesting" cases to test, etc. Of course, it's also
possible to use a model checker on such "open ended" test cases.

This compromise avoids most of the "garbage" inputs discussed in the previous
section. Yet it's still easy to write unit tests that cause (exhaustive) model
checkers to spin out of control, or to cause "deep" bugs that elude bounded
model checkers, even at impractical unrolling depths. The favourite type of a
bug with these symptoms is a failure at a boundary condition -- overflowing an
integral type, off-by-one errors near hard-coded application limits like
static buffer sizes, message size limits, etc. When debugging, programmers will
often artificially drop these limits to a small number. However, this usually
only happens after a problem is found accidentally -- a tool that is trying to
find the problem in the first place can't rely on the programmer to turn "deep"
problems into "shallow" ones manually.

Applications
============

In Explicit-State Model Checking
--------------------------------

For an explicit-state model checker, non-determinism is expensive "by default",
and it relies on abstraction to make the problem of open-ended programs (even
of the structured variety) tractable. Most, or all, input values need to be
abstracted away for an explicit-state model checker to work reasonably
efficiently.

In Bounded Model Checking
-------------------------

A bounded model checker defers the complexity of dealing with input
non-determinism to its backend decision procedure, usually an SMT solver. In
some cases, this SMT solver is in turn based on a simpler SAT decision
procedure. In case of STP, the SMT solver is based on abstraction-refinement,
bit-blasting "abstract" formulas into SAT problems (the abstraction is
especially useful for the array theory). The principle is that the SMT solver
has more insight into the problem (arrays and bitvectors) than its backend
procedure (SAT) can represent. The same principle can be replicated "one level
higher", in a bounded model checker, exploiting knowledge about the program
that is no longer present in the SMT representation.

One of the principles behind STP is to defer "expensive" transformations on the
problem into the already-abstracted stages of processing. In a bounded MC, loop
unrolling is such an expensive transformation -- abstractions that remove
entire loops from a program could have a substantial effect on the problem size
presented to the SMT solver (besides also saving memory and time on the MC side
of things). Indeed, in many cases, some loops in a program are entirely
redundant for a particular correctness criterion -- not unrolling those loops
and not feeding their unrolling to the SMT solver could save a lot of work;
even a particularly smart SMT solver will have a hard time figuring out that a
particular subset of the SMT problem represents a single loop and that the
entire lump would be a good candidate for SMT-level abstraction.

In essence, while program-level abstraction/refinement is not as crucial for a
bounded model checker as it is for an explicit-state one, it could
substantially improve its efficiency, especially over larger programs.

Implementation
==============

As mentioned earlier, a good implementation strategy seems to be a LLVM-to-LLVM
transformation, "compiling away" some details in the program, like exact values
of particular variables. This way, we can reuse the abstraction engine in many
existing tools.

The interesting questions that arise are: what kinds of abstractions to
implement and how to refine them. A first and in a way the simplest candidate
is to eliminate some variables entirely, as well as any values that depend on
them, and replacing all branches they affect with non-deterministic choice. The
crudest interesting refinement is then to put one of the variables back.

When variables associated with a particular loop are abstracted away in this
way, the loop will have no effect and can be optimised away by a (combination
of) pre-existing LLVM pass(es). If some but not all variables affected by a
loop are abstracted away, the loop may stay but become significantly
simpler. For choosing a variable to refine, a spurious counterexample could be
quite helpful, as it may reveal which (abstracted) variables participated in
the branching it took, although a crude refinement strategy could just pick
variables at random (of course a decision must be made that a refinement is
required, which means the a/r driver decided there is *some* spurious
counterexample, but it may not be available in a form suitable for the a/r
engine).

A more sophisticated abstraction technique is to replace concrete variables
with a set of predicates. This allows for subtler abstractions and also subtler
refinements. In case the abstraction fails, a counterexample is required to
make reasonable refinement though. A refinement may either add more predicates,
or possibly un-abstract a variable entirely (especially if it seems to already
have suspiciously many predicates associated).

In order to make better choices about refinements, the a/r engine can annotate
the LLVM bitcode with extra information, which would then show up in
counterexamples. Of particular interest are annotations about path constraints
on variables: if a spurious error occurs on a path guarded by `if (x > 1000)`,
it's very useful for predicate refinement to see this in the counterexample. An
intrinsic call, say `@llvm.dbg.assume` with a metadata node as a parameter
could serve that purpose fairly well. The job of the downstream tool is
basically to include these "semantic metadata" instructions in their
counterexample traces, so that the a/r engine can pick them up and base its
decisions on them. It's also up to the a/r engine to add relevant
@llvm.dbg.assume calls to the bitcode (additionally, the metadata nodes need to
be able to refer to "live" values in LLVM registers, which then need to be
passed as actual parameters to the assume; this is mostly an implementation
detail).

Incremental Refinement
----------------------

Transforming the LLVM bitcode obscures the relationships between various
abstractions/refinements of the same concrete program. This is especially true
if further transformation passes are applied after the abstraction. It would be
possible for the refinement process in the a/r engine to mark up the code
regions it has changed compared to the abstraction it was refining (this would
preclude use of standard LLVM transforms on the abstracted code though, as
those would not be able to preserve this kind of markup). Additionally, fairly
elaborate support would be required on the side of the model checker to make
use of such markup, and it's not clear if incremental verification would
out-weigh the benefit of stacking standard LLVM transforms after the
abstraction. After a non-incremental scheme is implemented, it might be
worthwhile to measure how much work is being re-done by the model checker upon
refinements, and possibly figure out a way to re-use it. This might be easier
in a bounded model checker than in an explicit-state one.

(An option for explicit-state model checker might be to arrange the predicates
in a bit-vector with "free" slots. While the predicate bitvector width does not
change, the model checker can re-use the visited set and start exploring from a
subset of already visited states, using the new program text. The a/r engine
would need to keep the bit-to-bit memory layout of the program intact, and we
would need to map program counters. Tricky, but might be doable.)

Planned Abstractions 
====================

(eventually, this will become "implemented abstractions")

TBD: Flesh out the details of the abstractions and refinements to (initially)
implement. Look at what parameters can be tuned in response to experimental
evaluation. Find stock LLVM transforms that will be useful to run on the
abstracted bitcode.
