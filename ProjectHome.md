implement [addr2line](http://linux.die.net/man/1/addr2line) & [nm](http://linux.die.net/man/1/nm) for windows platform

## winaddr2line --help ##
```
Usage: winaddr2line [option(s)] [addr(s)]

 Convert addresses into line number/file name pairs.
 If no addresses are specified on the command line, they will be read from stdin
 The options are:

  -a --addresses         Show addresses
  -e --exe <executable>  Set the input file name (default is a.exe)
  -p --pretty-print      Make the output easier to read for humans
  -s --basenames         Strip directory names
  -f --functions         Show function names
  -C --demangle[=style]  Demangle function names
  -y --symbol-path=<direcoty_to_symbol>    (option specific to winaddr2line)
                         Set the directory to search for symbol file (.pdb)
  -h --help              Display this information
  -v --version           Display the program's version
```


## winnm --help ##
```
Usage: winnm [option(s)] [file(s)]
 List symbols in [file(s)] (a.exe by default).
 The options are:
  -A, --print-file-name  Print name of the input file before every symbol
  -C, --demangle         Decode low-level symbol names into user-level names
  -l, --line-numbers     Use debugging information to find a filename and
                         line number for each symbol
  -n, --numeric-sort     Sort symbols numerically by address
  -o                     Same as -A
  -h, --help             Display this information
  -V, --version          Display this program's version number
```
