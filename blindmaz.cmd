## blindmaz.cmd -- solve maze by walking a left-handed blind mouse

#7 set $seed
# either maze.cmd, sharpmaz.cmd or floodmaz.cmd
execute-file floodmaz.cmd

set %meml $curline	# save entrance location
set %memc $curcol
set %x &add %memc 1
set %y %meml
end-of-line
set %stopcol &sub $curcol 1

set %DX0 1
set %DY0 0
set %DX1 0
set %DY1 1
set %DX2 -1
set %DY2 0
set %DX3 0
set %DY3 -1

set %dotc &asc "•"	# alternatively use "."

store-procedure probe
	set %OX &ind &cat "%DX" %nD
	set %OY &ind &cat "%DY" %nD
	set %nx &add %x %OX
	set %ny &add %y %OY
	set $curline %ny
	set $curcol %nx
	!if &or &equ $curchar 32 &equ $curchar %dotc
		!if &equ $curchar 32
			set %C %dotc
		!else
			set %C &asc " "		# erase when backtracking (or highlight)
		!endif
		set %D %nD
		set $curchar %C
		set $curline %y
		set $curcol %x
		set $curchar %C
		set %x &add %nx %OX
		set %y &add %ny %OY
		set $curline %y
		set $curcol %x
		set %res TRUE
	!else
		set %res FALSE
	!endif
	update-screen
!endm

set %D 0		# looking EAST
!while &les %x %stopcol
	set %nD &mod &add %D 3 4				# Can go left?
	run probe
	!if &seq %res FALSE
		set %nD %D							# Can go straight?
		run probe
		!if &seq %res FALSE
			set %nD &mod &add %D 1 4		# Can go right?
			run probe
			!if &seq %res FALSE
				set %D &mod &add %D 2 4		# Go back!
			!endif
		!endif
	!endif
!endwhile
set $curline %meml
set $curcol %memc
unmark-buffer
