# NaReTi
A script compiler that produces native code in runtime.

## currently working features
* int and float arithmetic
* custom types/ structs
* local and global variables
* calling native functions inside a script
* calling script functions from outside
* some optimization: inlining, var/return substitution
* generic types
 
Working examples can found in /scripts/.

## dependencies

* [boost](http://www.boost.org/)
* [asmjit](https://github.com/kobalicek/asmjit)