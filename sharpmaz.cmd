execute-file maze.cmd

end-of-line
set %ec &sub $curcol 1
end-of-file
set %el &sub $curline 1
set $curline %el
set %spaces $line
set $curline 1
set %old $line
set $line %spaces
set %l 2
!while &less %l %el
	set $curline %l
	set %c 1
	!while &less %c %ec
		set $curcol %c
		!if &not &equ $curchar 32
			set %v 0
			set $curline &sub %l 1
			set $curcol %c
			!if &not &equ $curchar 32
				set %v &add %v 1
			!endif
			set $curline %l
			set $curcol &sub %c 1
			!if &not &equ $curchar 32
				set %v &add %v 2
			!endif
			set $curcol &add %c 1
			!if &not &equ $curchar 32
				set %v &add %v 4
			!endif
			set $curline &add %l 1
			set $curcol %c
			!if &not &equ $curchar 32
				set %v &add %v 8
			!endif

			set $curline %l
			set $curcol %c
			set $curchar &asc &mid "╳╵╴┘╶└─┴╷│┐┤┌├┬┼" &add %v 1 1
		!endif
		set %c &add %c 1
	!endwhile
	set %l &add %l 1
!endwhile
set $curline 1
set $line %old
set $curline 3
set $curcol 1
