vcprompt: version control information in your prompt
====================================================

This is a fork of Greg Ward's [vcprompt](https://bitbucket.org/gward/vcprompt):

> vcprompt is designed to be small and lightweight rather than comprehensive. Currently, it has varying degrees of support for Mercurial, Git, Subversion, CVS, and Fossil working copies.

And this makes sense because thing which executes every time I press `<Enter>` should be lighting fast to avoid annoying delay before printing next command line prompt!

Problem with original vcprompt is **slow Mercurial support** - when `%m` or `%u` used in format string (to show unknown/modified files) it have to run `hg status`, which took **0.1 sec** on my fast enough workstation (_Intel® Core™ i7-2600K overclocked at 4.6GHz_) on empty repository.

There is also another similar project: [djl/vcprompt](https://github.com/djl/vcprompt), but it's even more slower: about **0.2 sec** when both `%m` and `%u` used and **0.1 sec** when only one of these formats used _(is it run `hg status` twice?!)_.

So, I've implemented fast Perl script, which is able to detect unknown/modified status of Mercurial repository in **0.002 sec**… but it doesn't correct in all cases (intentionally, for speed) - sometimes (when `.hg/dirstate` cache outdated) it report presence of modified files when there is no modified files. This can be fixed by updating cache, for ex.  by running `hg status`. Anyway, it's usually safer to think for a second there are modified files while there is no modified files, than opposite.  So, for me in this case speed is more important than correctness.

Another performance issue with original vcprompt is not using git cache for untracked files (user can enable it with `git update-index --untracked-cache`). I've fixed this, and this result in 3 times speedup on large repository (Gentoo portage) when `%u` used.

On Gentoo Linux it can be easily installed from `powerman` overlay:

```
# layman -a powerman
# emerge vcprompt
```

Installation/usage documentation is in original [README.txt](README.txt).
