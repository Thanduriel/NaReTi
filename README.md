# NaReTi
A script compiler that produces native code in runtime.

## features
* compiles int and float arithmetic
* custom types/ structs
* local and global variables
* calling native functions inside the script
* calling script functions from outside
* working examples are found in /scripts/
* some optimization: inlining, var/return substitution 

## dependencies

* [boost](http://www.boost.org/)
* [asmjit](https://github.com/kobalicek/asmjit)