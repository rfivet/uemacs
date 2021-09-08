## blindmaz.cmd -- solve maze by walking a left-handed blind mouse

#7 set $seed
# either maze.cmd, sharpmaz.cmd or floodmaz.cmd
execute-file floodmaz.cmd

set %dotc &asc "•"	# alternatively use "."
set $curchar %dotc
set %x &add $curcol 1
set %y $curline
end-of-line
set %stopcol &sub $curcol 1

# X-Y offset for absolute direction: east, south, west, north
set %DX0 1
set %DY0 0
set %DX1 0
set %DY1 1
set %DX2 -1
set %DY2 0
set %DX3 0
set %DY3 -1

set %absD 0							# absolute direction: looking EAST
!while &les %x %stopcol
# try move on left, right or front
	set %relD 3						# 3, 0, 1, 2 == left, front, right, back
	!while &not &equ %relD 2
		set %newD &mod &add %absD %relD 4
		set %offX &ind &cat "%DX" %newD
		set %offY &ind &cat "%DY" %newD
		set %nx &add %x %offX
		set %ny &add %y %offY
		set $curline %ny
		set $curcol  %nx
		!if &or &equ $curchar 32 &equ $curchar %dotc
			!if &equ $curchar 32
				set %C %dotc
			!else
				set %C &asc " "		# erase (or highlight) when backtracking
			!endif
			set %absD %newD
			set $curchar %C
			set $curline %y
			set $curcol  %x
			set $curchar %C
			set %x &add %nx %offX
			set %y &add %ny %offY
			update-screen
			!goto moveon
		!endif
		set %relD &mod &add %relD 1 4
	!endwhile
# else turn around
	set %absD &mod &add %absD 2 4	#	 face back!
:moveon
!endwhile

set $curline %y
set $curcol  %x
unmark-buffer
