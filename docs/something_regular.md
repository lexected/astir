# Something regular

The ["Hello Binary!"](#/hello_binary) gave you a first taste of parsing regular languages using the regular expressions combined with the astir grammar. We saw how to configure a finite automaton as a plain recognizer, how to set it up to capture more information from the input stream and actually serve as a tokenizer, and we hinted on the more general problem of abstracting hierarchical entities within a grammar and the challenges we are likely to face when trying to do so.

Wanting this part of the Getting Started guide not to depend on the previous one we will pose an entirely new parsing task and show how to make full use of the astirs finite automata's capabilities.

Our task shall be to parse a file of comma-separated numbers, but as a small twist, we will allow the numbers to be in the binary, octal, decimal, or hexadecimal form. We will start with a naive pure-production regular grammar, and then continue to tweak it while demonstrating astir's

* two types of string sequences,
* categories,
* fields of attributable statements,
* patterns as productions that are not type-forming,
* backtracking,
* terminality, non-terminality, and how it interacts with categories.

Without further ado, this is the grammar

```astir
File = (Line '\n')* Line;

Line = (Number ',')*;

Number =
    BinaryNumber
    | OctalNumber
    | DecimalNumber
    | HexadecimalNumber
    ;

BinaryNumber = '0' 'b' ['0' '1']+;
OctalNumber = '0' 'o' ['0' - '7']+;
DecimalNumber = '0' 'd'? ['0' - '9']+;
HexadecimalNumber = '0' 'x' ['0' - '9' 'a'-'f']+;

```
> The above grammar allows empty lines but requires that every number is followed by a comma, something not strictly necessary in the most common comprehension of the more general CSV format. 
> Note further that the whitespace is not permitted by this grammar.

## Atomic and Sequential Literals
Forgetting about the above grammar for a second, imagine that you are writing a tokenizer that needs to recognize some Welsh town names. While `Backside Wood` or `Three Cocks` would be relatively easy to spell out character by character, others like `Llanfairpwllgwyngyllgogerychwyrndrobwllllantysiliogogogoch` could be [more of a challenge](https://www.youtube.com/watch?v=fHxO0UdpoxM). Double quotes are here for that purpose: the finite automaton, or in fact also any other astir parsing machine, will match `"Pant-sod"` as `'P' 'a' 'n' 't' '-' 's' 'o' 'd'`, etc.

There is a subtle difference between the two types of strings, roughly equivalent to the difference in C or C++: single quotes require the raw representation of the input token to exactly match the string, whereas the double quotes denote the desire for the string to be matched character-by-character.

So, in the above grammar, `"0b"` would be equivalent to `'0' 'b'`, and `'0b'` would never be matched against as the input tokens are just the raw single-byte characters `0` and `b`, separated and in this order. On the other hand, if a preprocessing machine of some sort, say `PrefixPreprocessor`, concatenated prefixes such as `0b` or `0x` into a new class of tokens and let all other characters proceed to our `CSNTokenizer` unchanged, `"0b"` would never be matched against, whereas `'0b'` would be a necessity to match the concatenated prefixes.

We shall therefore, for the sake of simplicity, adjust our grammar to use double-quotes where appropriate.

```astir
BinaryNumber = "0b" ['0' '1']+;
OctalNumber = "0o" ['0' - '7']+;
DecimalNumber = "0d"? ['0' - '9']+;
HexadecimalNumber = "0x" ['0' - '9' 'a'-'f']+;
```

## Categories
Have a look at the definition of the production rule `Number`

```astir 
Number =
    BinaryNumber
    | OctalNumber
    | DecimalNumber
    | HexadecimalNumber
    ;
```

It serves as a mere convenience grouping of actual productions rather than a production on its own. Moreoever, from the object-oriented point of view, decimal or hexadecimal numbers are really just some more specific classes of numbers, and although they are represented in a different way, they still serve the same purpose -- to represent a numerical value.

Astir allows you to avoid this type of productions via the use of categories:

```astir
category Number;

BinaryNumber : Number = "0b" ['0' '1']+;
...
HexadecimalNumber : Number = "0x" ['0' - '9' 'a'-'f']+;
```

The power of categories goes beyond them being a syntactic sugar: in the generated parser code, `Number` will a type separate to `BinaryNumber` or `HexadecimalNumber`, but both of them will inherit it. Any grammar rules further within the `CSNTokenizer` or in machines using it will be able to refer to both the category and derived production, and the output code will mirror the choice in the typing of the category/production/pattern fields. 

No other parts of the grammar specification need to be modified for the newly-introduced category `Number` to work.

## Attributable statements and actions