# Mimiker's documentation structure

We decided to keep documentation of the project on three separate levels,
reflecting their supposed frequency of changes and distance from the actual
source code. We also have a separate onboarding note for newcomers (not
mentioned here). Programmer's responsibility is to keep all these docs up-to
date. We expect that the in-code documentation will be under constant
development.  

## Levels of documentation

- Infrastructure documentation
- Kernel design documentation
- Implementation documentation

## Infrastructure documentation

### What's the purpose?

Documents helping project members find their way in the infrastructure.
They are stored in a form of informative documents, such as

- project directory structure
- coding and running tests
- test framework note
- build system note
- toolchain note

#### In what form is it?

All these docs are supposed to reside in `/wiki/docs` in a form of `Markdown`
files.

#### Where to read?

Under "Wiki" tab on project's GitHub page, then you find the document that
interests you on sidebar on the right-hand side of the screen in the right
section.

## Kernel design documentation

### What's the purpose?

To document design of all kernel subsystems, i.e. we register all design
decisions and give high-level description of algorithms used by a given
subsystem.  We also point out if the subsystem implementation is simplified in
comparison to the desired one. This is important, since we need to keep track on
code that should be rewritten when the system matures. These docs are
supplemented by the auxiliary document, explaining how kernel subsystems
compose.

A subsystem design documentation describes its key algorithms, data structures,
resource management and locking policy.

### In what form is it?

Kept in a separate `Markdown` files meant to reside in `wiki/docs` folder.

### Where to read?

Under "Wiki" tab on project's GitHub page, then you find the document that
interests you on sidebar on the right-hand side of the screen.

## Implementation documentation

- in-code documentation
- subsystems interface documentation
- coding conventions
- tags and code documenting conventions
- project contributing guidelines (using GitHub - create PR, make CR)

### What's the purpose?

To help kernel programmers in crafting coherent readable and clean code that
easily integrates with our codebase. That means documenting interfaces,
coding conventions, rules to make successful commits and useful code reviews.
This includes the lowest level of in-source code documentation.

### In what form is it?

Either as comments and tags in source files or in folder `wiki/docs`
as `Markdown` files.

### Where to read?

Function descriptions are supposed to be found in header
files in `/include` folder. Documents can be found under "Wiki"
tab on project's GitHub page, there you will find the document
that interests you on sidebar on the right-hand side of the screen.

