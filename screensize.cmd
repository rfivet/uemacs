# Visualize Screen Dimensions
select-buffer screensize
insert-string &cat $curwidth &cat "x" $pagelen
insert-string &rig "---------+" &sub 10 $curcol
&sub &div $curwidth 10 1 insert-string "---------+"
insert-string &lef "1234567890" &mod $curwidth 10
&sub $pagelen 3 execute-command-line "insert-string &cat ~n &add $curline 1"
beginning-of-file
unmark-buffer
write-message $line
