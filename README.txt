vcprompt
========

vcprompt is a little C program that prints a short string, designed to
be included in your prompt, with barebones information about the current
working directory for various version control systems.  It is designed
to be small and lightweight rather than comprehensive.

Currently, it knows how to get (and print) the current branch for CVS,
Git, and Mercurial working copies.  Eventually, I plan to add support
for Subversion and Bazaar, and give it the (optional) ability to also
indicate how dirty the working copy is, i.e. whether there are unknown
files and whether there are local changes (modified/removed/added
files).

vcprompt currently has no external dependencies: it does everything with
the standard C library.  I suspect this will not last long: e.g. to
determine anything about a Subversion working copy will probably require
using the Subversion C API.

To compile vcprompt:

  make

To install it:

  cp -p vcprompt ~/bin

To use it with bash, just call it in PS1:

  PS1='\u@\h:\w $(vcprompt)\$'

To use it with zsh, you need to enable shell option PROMPT_SUBST, and
then do similarly to bash:

  setopt prompt_subst
  PROMPT='[%n@%m] [%~] $(vcprompt)'
