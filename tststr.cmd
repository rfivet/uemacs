; Insert long environment variables [will be truncated to NSTRING - 1 (127)]
insert-string $PATH
newline
; Insert string with escaped characters
insert-string "hello, world~n"
newline
; Insert long quoted string [will be truncated to NSTRING - 2 (126)]
insert-string "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"
next-line
; Insert long tokens [will be truncated to NSTRING - 1 (127)]
insert-string 1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
next-line
insert-string _________1_________2_________3_________4_________5_________6_________7_________8_________9_________0_________1_________2_________3
next-line
; Create and insert string variable until size exceed string limit [will be truncated to NSTRING - 1 (127)
set %nam 123
set %expect &len %nam
!while &equ &len %nam %expect
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
write-file %nam
insert-string &cat "Filename: " $cfname
newline
insert-string "Filename length: "
insert-string &len $cfname
save-file
beginning-of-file
