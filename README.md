# POLISH

## Examples

`"Hello, World!\n" out sputf end`

`"What is your name? " out sputf in sgetf
"Hello, %s!\n" sfmt out sputf end`

`0 1 #L0
:start
  lswp dup ~+ swp
  "%i " sfmt out sputf
  lswp linc
  #L16 lcmp cinc
? @start
"\n" out sputf
end`

## Description

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

### TOKENIZATION

Polish source code, which may be stored in a file with the suffix
`.pole` if desired, consists of a series of tokens.
A token may contain whitespace only if it begins and ends with the double-quote
character `"`. All other whitespace characters are supressed, and do not cause
the compiler to emit instructions.
The first character of a token determines its lexing rules.

If the first character is an ascii letter or an underscore, the token ends at the
first character which is not an ascii letter, an underscore, or a number.
Such a token will then be matched to an operation by the compiler, or if no
match is found, the compiler will complain and it will be skipped.

If the first character is `:`, the token ends at the first non-alphanumeric,
non-underscore character, and is interpreted as a label definition.
The compiler will then remember the name of the label
(the characters following the `:` and in the token) and the label's position.

If the first character is `@`, the token ends at the first non-alphanumeric,
non-underscore character, and is interpreted as a jump to a label.
The sequence `? @label` is compiled as if `#L[label] lund ? jmp ldrp`
were written, where `[label]` is the numeric program pointer
value stored by the label; otherwise it is compiled as `#[label] jmp`.

If the first character is `"`, the token ends on the first non-escaped `"`,
and is interpreted as a string, whose bytes are to be pushed onto the stack
after a leading null byte. The standard escape sequences are supported.

If the first character is an ascii digit, the token ends on the first non-digit
character, and will be interpreted by the compiler as a numeric literal of the
default size (integer, 4 bytes) in base 10.

If the first character is `#`, the token ends on the next non-alphanumeric
character, and will be interpreted as a numeric literal of a size and in a base
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
where capital versions of the first 10 of these correspond to a base 10 higher, for example **q**uaternary (4) -> **Q**uattordecimal (14).
All the alphanumeric characters following the format string are the **numeric string**,
whose characters are interpreted as a series of digits.
The valid digits are set by the base, and if an invalid digit is
encountered in the numeric string an error is thrown.
Unary uses the character `1`; bases B for 2 <= B <= 10 use ascii
digits 0 through B - 1; bases 10 < B <= 20 use the ascii digits as
well as the first B - 10 letters of the english alphabet.

If the token begins with any other character, it is one character long,
and is interpreted as an operation. If the character is in `+ - * / ~ .`,
the corresponding operation in `add sub mul div und drp` is compiled,
otherwise an operation with opcode equal to the ascii encoding of the character
is compiled. Because such tokens must be one character long,
they can be chained without spaces; for example, `?~+` is equivalent to `? ~ +`.

### Operations

To see a list of named operations, see `src/common.h`.
The operations `?` and `!`, which have no long-form named equivalents,
are also recognized by the Polish VM.

Many operations have four or five different versions depending on how they
interpret the top of the stack as data.
The names of such operations will begin with a type prefix in `crils`
followed by a 3-letter base name;
`c` indicates the operation acts on characters,
`s` indicates that it acts on strings, etc.
For example, `ladd` takes the top 16 bytes of the stack,
adds the higher 8 bytes as a long to the lower 8 bytes as a long,
and pushes the 8-byte result to the stack,
causing the stack to become 8 bytes shorter;
`sput` traverses down the stack from the top until it finds a null byte,
takes all the bytes above, then takes 8 more bytes,
interpreting the lowest 8 bytes as a pointer and writing the rest of the
bytes as a string to the location indicated by the pointer.

Additionally, some functions have variants with and `f` postfixed;
such variants operate with files rather than pointers to memory locations.
For example, `rget` gets a reduced integer from a memory location
and pushes it onto the stack, while `rgetf` reads a reduced integer
from a file buffer and pushes it onto the stack.
