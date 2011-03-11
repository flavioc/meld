
%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%% DATABASE %%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%

% nodes.
node(1).
node(2).
node(3).
% ...

% initial beliefs.
belief(1, [6, 8]).

% potentials.
potential(1, [5, 6]).
% ...

% edges.
edgeVal(1, 2, [2, 3]).
edgeVal(2, 1, [3, 2]).
edgeVal(1, 3, [5, 4]).
edgeVal(3, 1, [5, 4]).
% ...

%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%% RULES %%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%

inMessage(Node, Vector) :-
   edgeVal(_, Node, Vector).

belief(Node, normalize(Sum)) :-
   findall(Vector, inMessage(Node, Vector), List),
   potential(Node, Potential),
   sumLists([List, Potential], Sum).

cavity(Source, Dest, normalize(minusLists(Belief, InMessage))) :-
   belief(Source, Belief),
   edgeVal(Dest, Source, InMessage).

newEdgeVal(Source, Dest, damp(normalize(convolve(Cav)), Out)) :-
   cavity(Source, Dest, Cav),
   edgeVal(Source, Dest, Out).

%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%% CONTROL CODE %%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%

computeNewMessages(Node) :-
   edgeVal(Node, Neighbor, OldMessage),
   newEdgeVal(Node, Neighbor, NewMessage),
   retract(edgeVal(Node, Neighbor, OldMessage)),
   asserta(edgeVal(Node, Neighbor, NewMessage)),
   fail.
computeNewMessages(_).

computeFirstBelief(Node, Delta) :-
   belief(Node, FirstBelief),
   computeNewMessages(Node),
   belief(Node, NewBelief),
   retract(belief(Node, FirstBelief)),
   asserta(belief(Node, NewBelief)),
   beliefDelta(FirstBelief, NewBelief, Delta).
   
doSplashBpTrees([]) :- !.
doSplashBpTrees([Tree | Trees]) :-
   doSplashBpTree(Tree),
   doSplashBpTrees(Trees).
   
doSplashBpLoop([]) :- !. % finished
doSplashBpLoop(Deltas) :-
   buildSplashTrees(Deltas, Trees),
   doSplashBpTrees(Trees, NewDeltas),
   doSplashBpLoop(NewDeltas).
   
doSplashBP :-
   findall(Node-Delta, (node(Node), computeFirstBelief(Node, Delta)), Deltas),
   doSplashBpLoop(Deltas).
   
?- doSplashBP.