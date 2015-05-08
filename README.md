# README #

Ciguë is µEMACS (ue) on Cygwin/Linux, based on uEmacs/PK (em) from kernel.org.

### Changes compare to uEmacs/PK ###
* Line termination detection with new buffer mode (either Unix or DOS).
* Encoding detection (ASCII, Extended ASCII, UTF-8 or Mixed).
* Some fixes related to size either unchecked or limited (strcpy, insert-string, filenames, $kill).
* Major refactoring of headers and file dependencies, hopefully to improve maintenance.
* Reactivation of target 'source' and 'depend' in Makefile.
* Some defaults changed due to 'finger habits': ue instead of em, ^S in commands mapping...

### How to build ###
* dependencies: ncurses.
* make depend ; make
* MINGW32 target is experimental and lacks screen/kbd support.

### Badges ###
[![Coverity Status](https://scan.coverity.com/projects/4449/badge.svg)](https://scan.coverity.com/projects/4449)
