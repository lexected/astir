# Something regular

The ["Hello Binary!"](#/hello_binary) section gave you a first taste of parsing regular languages using the regular expressions combined with the astir grammar. We saw how to configure a finite automaton as a plain recognizer, how to set it up to capture more information from the input stream and actually serve as a tokenizer, and we hinted on the more general problem of abstracting hierarchical entities within a grammar and the challenges we are likely to face when trying to do so.

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
DecimalNumber = ('0' 'd')? ['0' - '9']+;
HexadecimalNumber = '0' 'x' ['0' - '9' 'a'-'f']+;
```

> The above grammar allows empty lines but requires that every number is followed by a comma, something not strictly necessary in the most common comprehension of the more general CSV format. 
> Note further that whitespace characters are not permitted by this grammar.

## Atomic and Sequential Literals
Forgetting about the above grammar for a second, imagine that you are writing a tokenizer that needs to recognize some Welsh town names. While `Backside Wood` or `Three Cocks` would be relatively easy to spell out character by character, others like `Llanfairpwllgwyngyllgogerychwyrndrobwllllantysiliogogogoch` could be [more of a challenge](https://www.youtube.com/watch?v=fHxO0UdpoxM). Double quotes are here for that purpose: the finite automaton, or in fact also any other astir parsing machine, will match `"Pant-sod"` as `'P' 'a' 'n' 't' '-' 's' 'o' 'd'`, etc.

There is a subtle difference between the two types of strings, roughly equivalent to the difference in C or C++: single quotes require the raw representation of the input token to exactly match the string, whereas the double quotes denote the desire for the string to be matched character-by-character.

So, in the above grammar, `"0b"` would be equivalent to `'0' 'b'`, and `'0b'` would never be matched against as the input tokens are just the raw single-byte characters `0` and `b`, separated and in this order. On the other hand, if a preprocessing machine of some sort, say `PrefixPreprocessor`, concatenated prefixes such as `0b` or `0x` into a new class of tokens and let all other characters proceed to our `CSNTokenizer` unchanged, `"0b"` would never be matched against, whereas `'0b'` would be a necessity to match the concatenated prefixes.

We shall therefore, for the sake of simplicity, adjust our grammar to use double-quotes where appropriate.

```astir
BinaryNumber = "0b" ['0' '1']+;
OctalNumber = "0o" ['0' - '7']+;
DecimalNumber = ("0d")? ['0' - '9']+;
HexadecimalNumber = "0x" ['0' - '9' 'a'-'f']+;
```

> Observe that we have added parentheses around the character sequence `"0d"`. It is the astir's own tokenizer that splits double-quoted strings into individual strings, meaning that this input formation is perfomed long before the meaning of the quantifiers `?` or `*` is recognized, and as such `"0d"?` would have been seen by astir's parser as `'0' 'd'?` rather than the `('0' 'd')?` that we want. 

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

The power of categories goes beyond them being a syntactic sugar: in the generated parser code, `Number` will a type separate to `BinaryNumber` or `HexadecimalNumber`, but both of them will inherit it. Any grammar rules further within the `CSNTokenizer` or in machines using `CSNTokenizer` will be able to refer to both the category and derived production, and the output code will mirror the choice in the typing of the category/production/pattern fields. 

No other parts of the grammar specification need to be modified for the newly-introduced category `Number` to work.

## Attributability 
Tokenization (and more generally parsing) is more than just mere recognition of grammar patterns that occur in input. When a pattern is recognized we often want to be able to `capture` some parts of the pattern and store the captured input somewhere appropriate. In this light we partition the statements into attributable and non-attributable, and we also consider whether they are type-forming or not.

Categories, productions, and patterns (declared with the `pattern` keyword) are the **attributable** statements. Attributable statements can belong to (inherit) categories (using the colon notation, see [above](#categories)) and list *fields* (or attributes, hence the name) that actions in their rules can modify. Standalone regular expressions (declared with the `regex` keyword) are **non-attributable**. They can not belong to categories, they can list no fields, and the regular expression associated with them may contain no actions at all. 

> We say that a statement (obviously attributable) _lists_ a field if it is among the fields declared in the curly brackets following the statements declaration.
> We say that a field is _owned_ by a statement if the statement lists the field and the statement is type-forming.
> We say that a field is _available_ to a statement if the statement either lists the field or if it is available to any of the categories the statement inherits.

Categories and productions are **type-forming**. They represent a type in the generated output (e.g. a `class` in C++ or TypeScript). Since both of them are attributable, they can list fields, but unlike patterns, they *possess* their fields, meaning that the corresponding generated output type will contain memebers/fields that will hold information assigned to them by actions. 
Patterns and regexes are **non-type-forming**. Rather than corresponding to a type in the output, their parsing constraint on the language is invoked independently every time they are used (as is probably best seen in the generated output for LL(k) parsers). Furthermore, whenever a pattern is used it is imperative that all the fields listed by the statement referencing it are available to the statement. Finally, whenever a pattern inherits at least one category, a similar limitation is in place -- every field listed by the pattern must also be among the fields available to all categories it inherits.

### Fields
Fields of attributable statements correspond to the notion of fields or member variables in object-oriented languages. Astir's fields have two built-in _storage_ types, `flag` and `raw`, and two kinds of _plurality_, `item` and `list`, for all custom types. A custom type is introduced to the context by a relevant type-forming statement.

Below we give an example of an extension of the above grammar in which statements list some fields that we shall use in just a moment.

```astir
production File {
    Line list lines;
    Line lastLine;
} = (Line '\n')* Line;

production Line {
    Number list numbers;
} = (Number ',')*;

```astir
category Number {
    raw valueString;
};

production BinaryNumber : Number = '0' 'b' ['0' '1']+;
production OctalNumber : Number = '0' 'o' ['0' - '7']+;
production DecimalNumber {
    flag isExplicit;
} : Number = ('0' 'd')? ['0' - '9']+;
production HexadecimalNumber : Number = '0' 'x' ['0' - '9' 'a'-'f']+;
```

### Actions on `flag`s
Flag fields can be either `flag`ged or `unflag`ged. You can always assume that the flag field is in the "unflagged" state until flagged. In C++ this corresponds to a `bool` field being `false` until it has been set to `true` by the `flag` action.

Here is an example of the `flag` action in use to modify the `isExplicit` field of `DecimalNumber`.

```astir
production DecimalNumber {
    flag isExplicit;
} : Number = ('0' 'd'@flag:isExplicit)? ['0' - '9']+;
```

In the runtime of the generated finite automaton the specific instance of `DecimalNumber` representing an example input of `0d12` will have the field `isExplicit` set to true, whereas an instance representing example input `22` would see the field remain in the default `false` state.

Note that no action is executed unless the entire rule is matched. For example, having the first part of the rule modified as `('0'@flag:isExplicit 'd')?` work on the input `077` would still, in the runtime of the generated finite automaton or any other parser, yield an instance of `DecimalNumber` with `isExplicit` in the default false. Sure, the greedy regex parser would try to match `'0' 'd'` first, but upon failing on the second character it would resort to skipping the entire bracket `(...)?` and continue with `['0' - '9']+` -- exactly as one would expect from regular expressions with greedy repetitive matching. 

### Actions on `raw` fields
Raw fields roughly correspond to strings in most languages. One can `capture`, `append`, or `prepend` parts of input to a raw field, and for completeness, raw fields can also be emptied by the `empty` action.

In our example grammar for comma-separated numbers we have declared the raw field `valueString` to store the string of the number, regardless of its format, for semantic processing. Note that in the terminology defined above, the `Number` category *owns* `valueString`, but the field is also available to all productions that inherit the category, namely `BinaryNumber`, `OctalNumber`, `DecimalNumber`, and `HexadecimalNumber`. As an available field it can be modified by actions in these productions.

We shall now give code for the obvious behaviour in which the value string is simply directly captured.

```astir
production BinaryNumber : Number = '0' 'b' ['0' '1']+@capture:valueString;
production OctalNumber : Number = '0' 'o' (['0' - '7']+)@capture:valueString;
```

> Notice that the use of parentheses makes no difference for the `capture` action in `OctalNumber` as the only thing in the parentheses is the repetitive regex `['0' - '7']+`. It is, however, believed, that `(['0' - '7']+)@capture:valueString` may be somewhat more readable.

But one can do better than this. Sometimes, we want to be selective about which raw subparts of a more complex regex we capture. See the use of the `append` action below.

```astir
production DecimalNumber {
    flag isExplicit;
} : Number = ('0' 'd')? (['0' - '9']@append:valueString)+;
production HexadecimalNumber : Number = '0' 'x' (['0' - '9' 'a'-'f']@append:valueString)+;
```

As a tailnote, `raw` can be captured not only in machines that operate on `raw` input, but also in any machine that operate on machine input where the machine only uses root terminals. The input stream, however, must not contain any outputs of ignored roots. More on that later.

### Actions on `item` fields
Item fields do not have a type on their own -- they need a name of a type-forming statement for their storage specification to be complete. When declaring an item field the keyword `item` is optional, e.g.
```astir
// TFAE (the following are equivalent)
production File {
    ...
    Line lastLine;
} = ...

production File {
    ...
    Line item lastLine;
} = ...
```
The type-forming statement referenced does not have to be a statement within the current machine. Indeed, all types created by type-forming statements of the input machine (the one following the `on` keyword in the machine declaration) or machine dependencies (the machines listed after the `uses` keyword), their dependencies, etc..

Item fields can be `set` and `unset`, but only by references to the type-forming statement of their type or, in case of categories, by references to type-forming statements that inherit that category (directly or indirectly). Similar to the behaviour of `flag` fields, every item field is considered to be in the "`unset`" state until set. Here is an example from our CSN grammar

```astir
production File {
    Line list lines;
    Line lastLine;
} = (Line '\n')* Line@set:lastLine
```

### Actions on `list` fields
List fields represent aggregates. As `item` fields they need a type created by a type-forming statement to complete their storage specification, but unlike them the keyword `list` is compulsory in the declaration. 

```astir
production File {
    Line list lines;
    Line item lastLine;
} = ...;
```

One can `push`, `pop`, and `clear` list fields. `push` and `pop` operate in the conventional stack sense (i.e. `push` adds an element and `pop` removes the last element added). `clear` is similar to `empty` for `raw` fields and simply `pop`s all elements until the are none left in the list. In our particular case the `push` action comes handy twice

```astir
production File {
    Line list lines;
    Line lastLine;
} = (Line@append:lines '\n')* Line@append:lines@set:lastLine;

production Line {
    Number list numbers;
} = (Number@append:numbers ',')*;
```

> As the above example demonstrates, multiple actions can be attached to a single term in a regular expression. These actions can naturally be heterogenous in their type (e.g. one can `@flag:aFlag@append:aRawList@push:aTerminalList@clear:someOtherList` with no trouble). 

The complete actioned grammar is now

```astir
production File {
    Line list lines;
    Line lastLine;
} = (Line@append:lines '\n')* Line@append:lines@set:lastLine;

production Line {
    Number list numbers;
} = (Number@append:numbers ',')*;

category Number {
    raw valueString;
};

production BinaryNumber : Number = ('0' 'b' ['0' '1']+)@capture:valueString;
production OctalNumber : Number = ('0' 'o' ['0' - '7']+)@capture:valueString;
production DecimalNumber {
    flag isExplicit;
} : Number = ('0' 'd'@flag:isExplicit)? (['0' - '9']+)@capture:valueString;
production HexadecimalNumber : Number = '0' 'x' (['0' - '9' 'a'-'f']+)@capture:valueString;
```

## Patterns
Patterns are attributable non-type-forming statements. As such they are as powerful as productions but can be used to avoid cluttering your syntax-tree type hierarchy with types that don't really fit and span several super- or sub-types (in terms of inheritance relationships). If you are familiar with the concept of interfaces (in C# or Java, say) then patterns could be considered the exact opposite of interfaces: while interfaces introduce a type and the interface (literally) to access the data of that type, patterns do not introduce a type but come with an implementation that can be selectively plugged in into other types. Astir's categories can easily be used as interfaces, and when used in that manner they are perfectly complemented by patterns, although that is not meant to be their main use.

While in our example CSN grammar it indeed makes sense to have `BinaryNumber` ... `HexadecimalNumber` as separate types (especially when thinking about the implementation of the back end), we shall for the sake of an example convert all those productions to patterns. Note that the raw field `valueString`, although present in the parent category `Number`, will have to be explicitly listed as the `pattern`'s field as well to comply with the semantic rules that we touched on [above](#attributability).

```astir
category Number {
    raw valueString;
};

pattern BinaryNumber {
    raw valueString;
} : Number = ('0' 'b' ['0' '1']+)@capture:valueString;
pattern OctalNumber {
    raw valueString;
} : Number = ('0' 'o' ['0' - '7']+)@capture:valueString;
pattern DecimalNumber {
    raw valueString;
    flag isExplicit;
} : Number = ('0' 'd'@flag:isExplicit)? (['0' - '9']+)@capture:valueString;
pattern HexadecimalNumber{
    raw valueString;
} : Number = '0' 'x' (['0' - '9' 'a'-'f']+)@capture:valueString;
```

And just like that -- all the sub-types of `Number` will disappear in the generated output and the only type exposed to the developer on the back end will be `Number`.

## A note on backtracking
While many conventional (older) lexer generations avoid backtracking by placing restrictions on what sort of regular expressions can be recognized by the output tokenizer and specifying strict precedence rules in order to get a pure deterministic finite automaton (DFA) on the output, astir does no such thing. When it comes to astir's finite automata, the output pseudo-DFA will always try to match the longest string possible even if it has to resort to backtracking, unless of course instructed otherwise. 

This does lead to decreased performance in some cases, and the users are actively encouraged to use all the means available for controlling backtracking to increase performance where necessary.

## Terminality
The output of an astir-generated parser (or finite automaton, if you think of those two terms as being disjoint) may be terminal or nonterminal. In particular, type-forming statements can be terminal or non-terminal, with categories being always non-terminal and productions being either of the two.

Terminals, unlike non-terminals, carry around

* the corresponding raw input agains which they were matched, even when generated on non-raw input
* their type information that is also directly available

... which usually means that their are somewhat more expensive in terms of their memory footprint. 

There usually isn't any difference between terminals and non-terminals when used within the same machine. Terminal input, when supported by the other machine, may give performance boost to the machine it is fed to, as it is usually easier to deduce the type of the input production from the explicit type information of terminals than from a type cast/check. This difference often fades when the terminal is referred to by its category, but not always (as it is with finite automata).

Since finite automata are meant to have high performance when parsing regular languages, they only accept terminal input (because of their internal tabular construction) but may output both terminals and non-terminals. LL(k) parsers may (or may not, depending on the exact context) use the faster terminal type recognition when supplied with terminal input, and may again hjhave both terminal and non-terminal output. 

Overall, using terminals for lexing/primary parsing might be advantageous than using non-terminals, especially when you need to refer to some of the input tokens by their raw contents. But the exact details depend on the machines used, and the user is strongly encouranged to read the appropriate reference documentation if implementation performance is of their concern.

Terminality of productions can be explicitly specified by the keywords `terminal`/`nonterminal`. If neither is present, the machine's default applies. For finite automata the default is always `terminal`.

Example

```astir
terminal production Line {
    Number list numbers;
} = (Number@append:numbers ',')*;
```

## The final grammar
```astir
production File {
    Line list lines;
    Line lastLine;
} = (Line@append:lines '\n')* Line@append:lines@set:lastLine;

production Line {
    Number list numbers;
} = (Number@append:numbers ',')*;

category Number {
    raw valueString;
};

pattern BinaryNumber {
    raw valueString;
} : Number = ('0' 'b' ['0' '1']+)@capture:valueString;
pattern OctalNumber {
    raw valueString;
} : Number = ('0' 'o' ['0' - '7']+)@capture:valueString;
pattern DecimalNumber {
    raw valueString;
    flag isExplicit;
} : Number = ('0' 'd'@flag:isExplicit)? (['0' - '9']+)@capture:valueString;
pattern HexadecimalNumber{
    raw valueString;
} : Number = '0' 'x' (['0' - '9' 'a'-'f']+)@capture:valueString;
```