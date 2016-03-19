; Insert long environment variables [will be truncated to NSTRING - 1 (127)]
insert-string &env PATH
newline
insert-string $PATH
newline
insert-string &cat $PATH $PATH
newline
set %mypath $PATH
insert-string %mypath
newline
insert-string &cat "Length of $PATH: " &len $PATH
newline
insert-string &cat "Length of %mypath: " &cat &len %mypath ~n
; Insert string with escaped characters
insert-string "hello, world~n"
newline
; Insert 512 long token [will be truncated to sizeof istring buffer - 2 (510)]
insert-string 0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF
newline
; Insert 512 long quoted string [will be truncated to sizeof istring buffer - 3 (509)]
insert-string "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
newline
; Insert long quoted string [will be truncated to NSTRING - 2 (126)]
insert-string "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"
next-line
; Insert long tokens [will be truncated to NSTRING - 1 (127)]
insert-string 1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
next-line
insert-string _________1_________2_________3_________4_________5_________6_________7_________8_________9_________0_________1_________2_________3
next-line
; Create and insert string variable until size exceed 1024 characters or variable get truncated
set %nam 123
set %expect &len %nam
!while &and &les %expect 1024 &equ &len %nam %expect
    insert-string %nam
    newline
    set %nam &cat %nam %nam
    set %expect &tim %expect 2
!endwhile
insert-string %nam
newline
insert-string &cat "Actual: " &len %nam
newline
insert-string &cat "Expected: " %expect
newline
; Use the variable as filename [will be truncated to NFILEN - 1 (79)]
set %nam &mid %nam 1 255
write-file %nam
insert-string &cat "Filename: " $cfname
newline
insert-string "Filename length: "
insert-string &len $cfname
end-of-file
; Create a line longer than 1 kill block (250), 2 * 127 + 21 = 255
insert-string 1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
insert-string 1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
insert-string "#12345678901234567890"
; kill and yank
beginning-of-line
kill-to-end-of-line
kill-to-end-of-line
yank
yank
; insert kill variable (up to 127 characters), was 25 before fix
insert-string $kill
save-file
beginning-of-file
set-mark
end-of-file
copy-region
insert-string $kill
redraw-display
