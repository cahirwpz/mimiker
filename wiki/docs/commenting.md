# How do we comment our code?

- We use [Markdown](https://guides.github.com/pdfs/markdown-cheatsheet-online.pdf)
  syntax inside C-style comments.

## Tags

Tags are meant to help us mark important things in our code.
When you are about to write a comment that requires special
attention, please add an adequate tag to mark it.

Wildcards used below:

- `?a`: author's nick
- `?s`: name of source from which code have been taken
- `?t`: term to be described

List of tags:

- `FIXME(?a)`: info about needed fixes, that we are aware about,
   but we leave it be
- `TODO(?a)`: info about some unimplemented features for
   the time being, less obliging than GitHub Issue
- `XXX(?a)`: explanation for some non-obvious part of code,
   perhaps a hack
- `START OF ?s CODE` and `END OF ?s CODE`: marks foreign part of code,
   you **must** leave info about where is it taken from along
   with specific link
- `INFO(?t)`: describes given term, must occur exactly once in the code
