# Generation

After the lexical, syntactical, and semantic analysis of the grammar-specification file, the generation phase follows. This phase still involves a number of semantic check, but because these now always dependent on the machine type, we have lumped them together with the actual internal representation and final output generation.

## Finite automata
Finite automata are the simplest (in terms of their inner working, not in terms of their construction) machines supported by Astir. They can parse regular languages specified through the use of the entire spectrum Astir's regular expressions, and produce both terminal and non-terminal output. Finite automata are further often used by parsers as dependency machines to perform selective regular lookahead where not supported by the machine.

It should be noted that finite automata do not support *reference recursion*, e.g. the following, albeit a perfectly valid context-free grammar for a regular language, will result in an error

```astir
production Atom = 'A';
production Tail = Body | empty;
production Body = Atom Tail;
```

due to the reference recursion on the path `Body-Tail-Body` or `Tail-Body-Tail` (the path cited in the error will depend on the order in which the statements are processed).

### Input and dependency machines
Finite automata accept only terminal input, be it raw or otherwise already processed input (say from another finite automaton). They can not have any machine dependencies (due to the rather simplistic nature of the DFA transition tables).

### Machine attribute defaults
The following are the defaults for finite automaton machine attributes

* `ProductionsTerminalByDefault` is `true`
* `ProductionsRootByDefault` is `true`
* `CategoriesRootByDefault` is `false`
* `AmbiguityResolvedByPrecedence` is set to `false`

### Internal NFA
After the basic semantic checks the finite automaton generator constructs an internal non-deterministic finite automaton. Originally (in one of the first completed commits) the Thompson's construction was used before the NFA was converted to a DFA and code generation began. This has since been changed to a custom processes that only distantly resembles Thompson's construction. Furthermore, the NFA-to-DFA conversion algorithm significantly deviates from the textbook one due to the presence of actions on transitions and states and the need for backtracking. While all epsilon-transitions are eventually eliminated with the calculation of epsilon-closures and concentration of epsilon-transition actions at accumulated states, the resulting finite automaton is still strictly not a DFA due to the multiple paths that the machine might have to take when trying to match an alternative and then resorting to backtracking. We call the result of the conversion process a pseudo-DFA (or better, Astir's DFA).

### Output generation
The output generated from the pseudo-DFA is mainly in the form of tables. There is the transition table, state finality table, transition-action table, and the state-action table, all of which are run by a generated boilerplate. The generated boilerplate has been written to be easily readable and contains numerous comments explaining why the things are done in the way they are. Feel free to check out the generated code for more information, we promise it won't be too painful.

## LL(k) and LL(finite) parsers
The LL(finite) parsers (or their semantically restricted versions, LL(k) parsers for all k greater than one) are predictive parsers parsing left-to-right the left-most derivation with arbitrary but necessarily finite lookahead.

When one talk's about LL(k) and LL(finite) parsers are one and the same thing in Astir on the generation level, with the only difference being that the LL(k) parsers will trigger an error anytime more than k units of lookahead is needed to disambiguate between two alternatives. 

### Input and dependency machines
LL(finite) parsers can accept arbitrary input, and reference arbitrary dependency machines.

### Machine attribute defaults
The following are the defaults for LL(finite) parser machine attributes

* `ProductionsTerminalByDefault` is `false`
* `ProductionsRootByDefault` is `false`
* `CategoriesRootByDefault` is `false`
* `AmbiguityResolvedByPrecedence` is set to `false`

### Internal lookahead register
The lookahead register computation of Astir's LL(finite) parsers roughly follows the LL(1) FIRST-and-FOLLOW strategy with the obvious dynamicized extension. The lookahead register is computed for all non-terminals (in the sense of LL(k) parsing theory) and then used extensively during the generation process.

### Output generation
LL(finite) parsers are generated as predictive recursive-descent parsers, in which every type-forming machine component has a separate parsing function. Patterns and regexes are generated in place, and any referenced components of dependency machines are parsed just-in-time with cached-lookahead.