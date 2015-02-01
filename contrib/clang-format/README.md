Mercurial integration
----

To integrate the hg-clang-format script with mercurial put the following alias
definition into the local `.hg/hgrc` file:

```
[alias]
clang-format = !$($HG root)/contrib/clang-format/hg-clang-format $@
```

Usage: hg clang-format [rev]
