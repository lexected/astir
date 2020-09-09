# About

## What is it?

Astir is a parser generator -- it takes a grammar as input and outputs code that, when run, recognizes lexical and syntactical structures on its input. For the sake of simplicity we shall refer to this code as to *parser code* or just *parser*. This parser code can then be extended to process the structures recognized in some meaningful way.

As a parser generator it is *output-*, rather than *input-* centred. Traditionally, parser generators have been designed with generality (in terms of the breadth of the variety of languages their output could recognize) and parsing performance in mind. While the development of astir actively eyed both of these characteristics, it was also designed with extreme care to make sure that

- a single grammar specification could be used to generate **human-friendly** parsers in **multiple languages**,
- the output of any generated parser made proper and appropriately extensive use of the **object-oriented** paradigm,
- the output parsers themselves were **not limited to** using **just one parsing algorithm** or one "true" type and structure of input.

Astir thus comes with its own grammar-specification language, which

- makes **full use** backtracking **regular expressions**,
- exhaustively captures **inheritance** and **population** relationships among grammar productions,
- allows for parser output construction in a completely **target-language-neutral** way.

## What is it good for?

The generator output can, depending on its configuration, process an arbitrary raw byte stream or a(n only very lightly restricted) generic target-language-structure stream. This output parser can then recognize any context-free grammar within the limitations of the pre-selected target algorithmic machinery.

A rather specific use of astir can be found under the [Getting started] (#getting_started) section of this documentation where we employ astir to generate a parser for the C11 language and hint on how the generated code can easily be elaborated on in a way that allows us to close the project up into a full-blown C compiler.

But it gets better than that. As a way of example, the LL(star) algorithm can easily recognize even some of the context-sensitive languages, while the LR(0) support allows the output parsers to recognize languages no left-to-right parser could. To learn more about astir's true limitations please see the code generation reference.

> **Simply put**\
> Astir may feasibly be your parser generator of choice for any 21st-century language-processing (in the conventional, not machine-learning sense) task.

## How is it different?

You might be familiar with tools such as *lex*, *bison*, or *ANTLR*, all of which generate code for parsers of some feasible regular or context-free languages. To reiterate on some of the points made above, in this context, astir allows you to

- write object-oriented context-free grammars,
- use just one grammar file for all target languages, and
- modularly draw on a database of various parsing algorithms in the quest to efficiently parse intrisically complex syntactic structures.