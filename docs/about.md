# About

## What is it?

Astir is a parser generator -- it takes a grammar specification on input and produces code that, when run, recognizes lexical and syntactical structures on its own input. We refer to this code as to *parser code* or just *parser*. This parser code can then be extended to process the structures recognized in some meaningful way (i.e. compiled).

As a parser generator it is *output-*, rather than *input-*centred. Traditionally, parser generators have been designed with generality (in terms of the breadth of the variety of grammars their output could recognize) and parsing performance in mind. While the development of Astir actively eyed both of these characteristics, it was also designed with extreme care to make sure that

- a single grammar specification could be used to generate **human-friendly** (where possible) parsers in **multiple languages**,
- the output of any generated parser made proper and appropriately extensive use of the **object-oriented** paradigm,
- the output parsers themselves were **not limited to** using **just one parsing algorithm** or one "true" type and structure of input.

Astir thus comes with its own grammar-specification language, which

- makes **full use** of **regular expressions**,
- exhaustively captures **inheritance** and **population** relationships among grammar productions,
- allows for parser output construction in a completely **target-language-neutral** way.

## What is it good for?

The generator output can, depending on its configuration, process an arbitrary raw byte stream or a(n only very lightly restricted) generic target-language-structure stream. This output parser can then recognize any context-free grammar within the limitations of the pre-selected target algorithmic machinery.

To learn more about Astir's limitations please see the code generation reference.

## How is it different?

You might be familiar with tools such as *lex*, *bison*, or *ANTLR*, all of which generate code for parsers of some feasible regular or context-free languages. To reiterate on some of the points made above, in this context, Astir allows you to

- write object-oriented context-free grammars,
- use just one grammar file for all target languages, and
- modularly draw on a database of various parsing algorithms in the quest to efficiently parse intrisically complex syntactic structures.