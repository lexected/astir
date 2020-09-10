# Something regular

The ["Hello Binary!"](#/hello_binary) gave you a first taste of parsing a regular language using the regular expressions combined with the astir grammar. We learned to configure a finite automaton as a plain recognizer, how to set it up to capture more information from the input stream and actually serve as a tokenizer, and we hinted on the more general problem of abstracting hierarchical entities within a grammar and the challenges we are likely to face when trying to do so.

Wanting this part of the Getting Started guide not to depend on the previous one we will pose an entirely new parsing task and show how to make full use of the astirs finite automata's capabilities.

# Simplified English Gramamr
There is a simplified Chinese out there, so why couldn't we have a simplified English as well? Below we give a pure production grammar to pin down what exactly we shall mean by "simplified English" (denoted `SE`) in this document. We will then work our way through some of the key features of astir that can be applied in the context of finite automata and modify our grammar to be a better fit for our purposes.

```astir



```