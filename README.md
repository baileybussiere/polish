# POLISH

A low-level stack-based programming language which is compiled to bytecode
to be executed by a virtual machine. The language is very weakly typed,
with different types simply corresponding to different ways to interpret
raw bytes on the stack or different ways to put data onto the stack.

There are 5 or 6 primitive data types of fixed size which can occupy the stack
and be operated on directly.

Here they are with their names (bold), prefixes, and size (in bytes):
* **character** C 1,
* **reduced integer** R 2,
* **integer** I 4,
* **long integer** L 8,
* **string** S [arbitrary size],
* and maybe **pointer** [no prefix] 8,

all of which may occupy the stack directly and may be operated on directly.

At this point the machine and compiler assume pointers fit in 8 bytes,
and the *pointer* type may be considered to be simply an alias for long.

## POLISH SOURCE CODE

Polish source code, which may be stored in a file with the suffix
`.pole` if desired, consists of a series of tokens.
A token may contain whitespace only if it begins and ends with the double-quote
character `"`. The first character of a token determines its lexing rules.

If the first character is an ascii letter or an underscore, the token ends on the
first character which is not an ascii letter, an underscore, or a number.
Such a token will then be matched to an operation by the compiler, or if no
match is found, the compiler will complain and it will be skipped.

If the first character is a ascii digit, the token ends on the first non-digit
character, and will be interpreted by the compiler as a numeric literal of the
default size (integer, 4 bytes) in base 10.

If the first character is `#`, the token ends on the next non-alphanumeric
character, will be interpreted as a numeric literal of a size and in a base
specified by a series of alphabetic characters immediately following the `#`:
the **format string**. This format string consists of 1 or 2 characters.
If one of the characters `cCrRiIlL`
(the numeric type prefixes) is encountered, the format string is assumed to
have ended, and the following alphanumeric characters will be parsed as a number.
If one of the characters `ubtqphsond`, their capital versions, or the character
`v` is encountered, the compiler uses a different base to parse the characters
following the format string:
* **u**nary (base 1, `1`)
* **b**inary (base 2, `01`)
* **t**ernary (base 3, `012`)
* **q**uaternary (base 4, `0123`)
* **p**ental (base 5, `01234`)
* **h**exal (base 6, `012345`)
* **s**eptenary (base 7, `0123456`)
* **o**ctal (base 8, `01234567`)
* **n**onary (base 9, `012345678`)
* **d**ecimal (base 10, `0123456789`)
* **v**igesimal (base 20, `0123456789aAbBcCdDeEfFgGhHiIjJ`),
where capital versions of the first 10 of these correspond to a base 10 higher,
including the...

To put numeric data onto the stack, simply write a number
