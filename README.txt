vcprompt
========

vcprompt is a little C program that prints a short string, designed to
be included in your prompt, with barebones information about the current
working directory for various version control systems.  It is designed
to be small and lightweight rather than comprehensive.

Currently, it has varying degrees of recognition for Mercurial, Git,
CVS, and Subversion working copies.

vcprompt has no external dependencies: it does everything with the
standard C library and POSIX calls.  It should work on any
POSIX-compliant system with a C99 compiler.

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

vcprompt prints nothing if the current directory is not managed by a
recognized VC system.


Format Strings
==============

You can customize the output of vcprompt using format strings, which can
be specified either on the command line or in the VCPROMPT_FORMAT
environment variable.  For example:

  vcprompt -f "%b"

and

  VCPROMPT_FORMAT="%b" vcprompt

are equivalent.

Format strings use printf-like "%" escape sequences:

  %n  name of the VC system managing the current directory
      (e.g. "cvs", "hg", "git", "svn")
  %b  current branch name
  %r  current revision
  %u  ? if there are any unknown files
  %m  + if there are any uncommitted changes (added, modified, or
      removed files)
  %%  a single % character

All other characters are expanded as-is.

The default format string is

  [%n:%b]

which is notable because it works with every supported VC system.  In
fact, some features are meaningless with some systems: there is no
single current revision with CVS, for example.  (And there is no good
way to quickly find out if there are any uncommitted changes or unknown
files, for that matter.)


Contributing
============

Patches are welcome.  Please follow these guidelines:

  * Ensure that the tests pass before and after your patch.
    If you cannot run the tests on a POSIX-compliant system,
    that is a bug: please let me know.

  * If at all possible, add a test whenever you fix a bug or implement a
    feature.  If you can write a test that has no dependencies (e.g. no
    need to execute "git" or "hg" or whatever), add it to
    tests/test-simple.  Otherwise, add it to the appropriate VC-specific
    test script, e.g. tests/test-git if it needs to be able to run git.

  * Keep the dependencies minimal: preferably just C99 and POSIX.
    If you need to run an external executable, make sure it makes
    sense: e.g. it's OK to run "git" in a git working directory, but
    *only* if we already know we are in a git working directory.

  * Performance matters!  I wrote vcprompt so that people wouldn't have
    to spawn and initialize and entire Python or Perl interpreter every
    time they execute a new shell command.  Using system() to turn
    around and spawn external commands -- especially ones that involve a
    relatively large runtime penalty like Python scripts -- misses the
    point of vcprompt.

    In fact, you'll find that vcprompt contains hacky little
    reimplementations of select bits and pieces of Mercurial, git,
    Subversion, and CVS precisely in order to avoid running external
    commands.  (And, in the case of Subversion, to avoid depending on a
    large, complex library.)

  * Stick with my coding style:
    - 4 space indents
    - no tabs
    - curly braces on the same line *except* when defining a function
    - C99 comments and variable declarations are OK, at least until
      someone complains that their compiler cannot handle them

  * Feel free to add yourself to the contributors list below.
    (If you don't do it, I'll probably forget.)

  
Author Contact
==============

vcprompt was written by Greg Ward <greg at gerg dot ca>.

The latest version is available from either of my public Mercurial
repositories:

  http://hg.gerg.ca/vcprompt/
  http://bitbucket.org/gward/vcprompt/overview


Other Contributors
==================

In chronological order:

  Daniel Serpell
  Jannis Leidel
  Yuya Nishihara
  KOIE Hidetaka
  Armin Ronache

Thanks to all!


Copyright & License
===================

Copyright (c) 2009, 2010, Gregory P. Ward.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
