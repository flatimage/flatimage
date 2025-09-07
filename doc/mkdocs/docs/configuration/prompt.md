# Prompt

## Setting a Custom Prompt

You can change the value of the PS1 variable for a custom prompt for your
application, the `Appendix` contains the options reference.

```bash
$ ./app.flatimage fim-env set 'PS1="[\#] \A \W -> "'
$ ./app.flatimage fim-exec bash
[1] 15:05 ~ ->
```

## Appendix

Details for `PS1` configuration:

```txt
PROMPTING
       When  executing interactively, bash displays the primary prompt PS1 when it is ready to read a command, and the secondary prompt PS2 when it needs more
       input to complete a command.  Bash displays PS0 after it reads a command but before executing it.  Bash displays PS4 as described above before  tracing
       each command when the -x option is enabled.  Bash allows these prompt strings to be customized by inserting a number of backslash-escaped special char‐
       acters that are decoded as follows:
              \a     an ASCII bell character (07)
              \d     the date in "Weekday Month Date" format (e.g., "Tue May 26")
              \D{format}
                     the  format is passed to strftime(3) and the result is inserted into the prompt string; an empty format results in a locale-specific time
                     representation.  The braces are required
              \e     an ASCII escape character (033)
              \h     the hostname up to the first ‘.'
              \H     the hostname
              \j     the number of jobs currently managed by the shell
              \l     the basename of the shell's terminal device name
              \n     newline
              \r     carriage return
              \s     the name of the shell, the basename of $0 (the portion following the final slash)
              \t     the current time in 24-hour HH:MM:SS format
              \T     the current time in 12-hour HH:MM:SS format
              \@     the current time in 12-hour am/pm format
              \A     the current time in 24-hour HH:MM format
              \u     the username of the current user
              \v     the version of bash (e.g., 2.00)
              \V     the release of bash, version + patch level (e.g., 2.00.0)
              \w     the value of the PWD shell variable ($PWD), with $HOME abbreviated with a tilde (uses the value of the PROMPT_DIRTRIM variable)
              \W     the basename of $PWD, with $HOME abbreviated with a tilde
              \!     the history number of this command
              \#     the command number of this command
              \$     if the effective UID is 0, a #, otherwise a $
              \nnn   the character corresponding to the octal number nnn
              \\     a backslash
              \[     begin a sequence of non-printing characters, which could be used to embed a terminal control sequence into the prompt
              \]     end a sequence of non-printing characters

       The command number and the history number are usually different: the history number of a command is its position in the history list, which may include
       commands restored from the history file (see HISTORY below), while the command number is the position in the sequence of commands executed  during  the
       current  shell session.  After the string is decoded, it is expanded via parameter expansion, command substitution, arithmetic expansion, and quote re‐
       moval, subject to the value of the promptvars shell option (see the description of the shopt command under SHELL BUILTIN  COMMANDS  below).   This  can
       have unwanted side effects if escaped portions of the string appear within command substitution or contain characters special to word expansion.
```
