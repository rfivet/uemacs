# README #

µEMACS (ue) on Cygwin/MSYS2/Linux/NetBSD/OpenBSD, based on uEmacs/PK (em)
from [kernel.org](https://git.kernel.org/pub/scm/editors/uemacs/uemacs.git/).

### Changes compare to uEmacs/PK ###

* Line termination detection with new buffer mode (either Unix or DOS).

* Encoding detection (ASCII, Extended ASCII, UTF-8 or Mixed).

* Some fixes related to size either unchecked or limited (strcpy,
  insert-string, filenames, $kill).

* Major refactoring of headers and file dependencies, hopefully to
  improve maintenance.

* Some defaults changed due to 'finger habits': ue instead of em, ^S in
  commands mapping...

### Unicode (UTF-8) support ###

* gcc limitation on Windows (__WCHAR_WIDTH__ 16).

* Display of double and zero width characters ongoing.

### How to build ###

* dependencies: (gcc || clang) && gmake && ncurses-devel.

* make
