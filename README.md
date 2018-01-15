# Automaon

A finite automaton -- a simple state machine which accepts or rejects words. A word is any finite sequence of letters from an alphabet. An alphabet is any finite set. If w is a word , then by |w| we denote its length, and by w[i], 0 <= i < |w|, its i-th letter. Moreover, the symbol ε denotes an unique empty word, i.e. a word of length 0.

Formally, an automaton A is a tuple (L, E, U, F, qI, T), where L is an alphabet, E is a set of existential states, U is a set of universal states, Q ≝ E ∪ U is the set of all states of A, F∈Q is the set of final states, qI is the unique initial state, and T: Q × L → P(Q) is the transition function.

Intuitively, an automaton recognises a word in the following way: the automaton starts in the initial state and reads the word, letter by letter. When the automaton is in state q and sees a letter a then it ``chooses'' the next state from the set T(q,a) and proceeds. If q is universal then every such choice has to lead to acceptance, is q is existential then only one has to lead to acceptance. Acceptance, in this context, requires that the above process after reading the last letter of the word ends up in a final state.

More formally: the above process can be described as a tree. A run of the automaton run on word w of length d is a tree of height d+1. (For instance, a run on the epty word has unique node -- the root -- at the depth 0.) The root ε of the run is labelled run[ε] = qI and every node u in the tree satisfies the following: if node u is at depth i, is labelled q and

T(q, w[i]) = ∅ then u is a leaf;
T(q, w[i]) ≠ ∅ and the state q is existential then u has exactly one child labelled with a state belonging to the set T(q, w[i]);
T(q, w[i]) ≠ ∅ and q universal then node u has exactly one p-labelled child for every state p ∈ T(q, w[i]) .
We say that a run is accepting if the leafs' labels on depth d are final states and the labels of the remaining leafs are universal states. A word is accepted by an automaton if there is an accepting run. If there is non then the automaton rejects the word.
The acceptance of a word can also be described as a recursive function. An automaton accepts a word w if the following function bool accept(string w, string r)

```c++

// this function accepts the word @w continuing the process from the last node of the path @r 
// belonging to some run of the automaton
bool accept(string w, string r):

  d := |r|-1;

  if (d ≥ |w|)
    //we have processed every letter in @w; accept if the last state is accepting
    return (r[d] ∈ F);

  if (r[d] ∈ E)
    // the last state is existential so accept if there
    // is an accepting continuation of the run @r
    return (∃q∈T(r[d],w[d]) accept(w,rq))

  // otherwise, the last state is universal (r[d] ∈ U)
  // and we have to accept every possible continuation
  return (∀q∈T(r[d],w[d]) accept(w,rq))

```
  
returns true when called as accept(w,qI).
Pozor! Variables w,r are words and rq is the conctatenation of the word r and the letter (also a state) q. Intuitively, r can be seen as a path in a run.