
type source(node).
type sink(node, int).
type linear pluggedIn(node, node).
type load(node, sum float).

type extern rand().
type RANK_VAL = 0.1.

!pluggedIn(Sink, Source) :-
	!unplugged(Sink),
	edge(Sink, Source),
	sink(Sink, _),
	source(Source).

!pluggedIn(Sink, Source), load(Source, Amt) :-
	!pluggedIn(Sink, Source),
	sink(Sink, Amt).

!pluggedIn(Sink, NewSource) :-
	!pluggedIn(Sink, OldSource),
	edge(Sink, NewSource),
	load(OldSource, OldSourceAmt), OldSourceAmt > 1, // old source is overloaded
	load(NewSource, NewSourceAmt),
	sink(Sink, Amt),
	NewSourceAmt + Amt < OldSourceAmt || rank() < RANK_VAL.


