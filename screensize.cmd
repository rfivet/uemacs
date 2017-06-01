# Visualize Screen Dimensions
insert-string &cat $curwidth &cat "x" $pagelen
insert-string &rig "---------+" &sub 10 $curcol
&sub &div $curwidth 10 1 insert-string "---------+"
insert-string &lef "1234567890" &mod $curwidth 10
end-of-file
&sub $pagelen 3 execute-command-line "insert-string &cat $curline ~n"
beginning-of-file
unmark-buffer
write-message $line
