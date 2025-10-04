# Making my own shell
This repo contains the source code that I am using to build my own POSIX compliant shell. Currently, it is a work in progress with the current shell I am working on being implemented in C. Currently the code is in a single source file, but as the file is getting relatively large now, will be splitting into several header and .c files shortly before continuing with the implementation.

## Table Of Contents:
1. [Usage](#usage)
2. [Bultin commands](#builtin-commands)
3. [Running a program](#running-a-program)


### Usage:
1. Clone this repo
2. You should have either a gcc or clang compiler installed. Once installed, you should be able to simply run `cc shell.c -o shell`
3. Run `./shell`. Voìla! You're in the shell.

### Builtin commands

**1. exit** - The classic starter (other than a constant loop printing out a dollar sign of course). This command exits the shell, with the integer provided being returned to the parent shell in which the shell is started.

Example usage:

```
$ exit 1
```


**2. echo** - Prints out whatever you provide to the shell (literally "echo's" it back to you). Recently added support for single quotes, so exact commands are printed out within those, otherwise spaces are shrunk down into single spaces.

Example usage and output:

```
$echo hello   world
hello world
$echo 'hello   world'
hello   world
```

**3. type** - prints out the type of a command, whether it exists in the `PATH` environment variable, is a local builtin, or is not a valid command at all.

Example usage and output:

```
$ type echo
echo is a shell builtin
$ type cat
cat is /bin/cat
$ type hello
hello: not found
```

**4. pwd** - Prints the present working directory

Example usage:
```
 pwd
/Users/YOUR_WORKING_DIRECTORY
```


**5. cd** - Changes to the appropriate directory  (relative, absolute and root paths works)

Example usage:

```
cd ~
pwd
YOUR_ROOT_DIRECTORY
```


### Running a program
Any program in your PATH variable paths is runnable by the shell. Normal unix commands like `ls` should work, and you can also use `git` in the shell! If some command doesn't work, you can simply check if it is in your PATH variable with the `type` builtin, example usage shown above.


# Additional features
- Escaping with backlashes added outside quotes
Example usage:

```
$ echo hello \ \ world
hello  world
```

- Escaping with backslash inside double quotes added (specifically \ and " can be escaped)
Example usage:
```
$ echo "hello\"insidequotes"script\"
hello"insidequotesscript"
```


Reference used for implementation:
https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html
