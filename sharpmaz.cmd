## sharpmaz.cmd -- redraw a block maze using line characters

execute-file maze.cmd

set %meml $curline
set %memc $curcol
end-of-line
set %ec &sub $curcol 1
end-of-file
set %el &sub $curline 1
previous-line
set %spaces $line
beginning-of-file
set %old $line
set $line %spaces
next-line
!while &less $curline %el
	set $curcol 1
	!while &less $curcol %ec
		!if &not &equ $curchar 32
			set %v 0
			previous-line
			!if &not &equ $curchar 32
				set %v &add %v 1
			!endif
			next-line
			backward-character
			!if &not &equ $curchar 32
				set %v &add %v 2
			!endif
			2 forward-character
			!if &not &equ $curchar 32
				set %v &add %v 4
			!endif
			next-line
			backward-character
			!if &not &equ $curchar 32
				set %v &add %v 8
			!endif

			previous-line
			# alternatively use single width "╳╵╴┘╶└─┴╷│┐┤┌├┬┼"
			set $curchar &asc &mid "╳╹╸┛╺┗━┻╻┃┓┫┏┣┳╋" &add %v 1 1
		!endif
		forward-character
	!endwhile
	next-line
!endwhile

beginning-of-file
set $line %old
set $curline %meml
set $curcol %memc
