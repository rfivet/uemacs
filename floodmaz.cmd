# 6 set $seed
execute-file maze.cmd

set %thisbuf $cbufname

store-procedure pushxy				# push x y
	set %x $curcol
	set %y $curline
	select-buffer stack
	beginning-of-file
	insert-string %x
	newline
	insert-string %y
	newline
	select-buffer %thisbuf
!endm

store-procedure popxy				# pop x y
	select-buffer stack
	beginning-of-file
	set %x $line
1	kill-to-end-of-line
	set %y $line
1	kill-to-end-of-line
	select-buffer %thisbuf
	set $curline %y
	set $curcol %x
!endm

set $curline 1
set $curcol 0
run pushxy	#push stop position
set %x 1
set $curline 4
set $curcol %x
set %OC $curchar
set %NC &asc "█"
!while &not &equ %x 0
	set $curchar %NC
	set %cc $curcol
	set %ll $curline
	set $curcol &add %cc 1
	!if &equ $curchar %OC
		run pushxy
	!endif
	set $curcol &add %cc -1
	!if &equ $curchar %OC
		run pushxy
	!endif
	set $curline &add %ll 1
	set $curcol %cc
	!if &equ $curchar %OC
		run pushxy
	!endif
	set $curline &add %ll -1
	set $curcol %cc
	!if &equ $curchar %OC
		run pushxy
	!endif
	run popxy	
!endwhile
set $curline 3
set $curcol 1
select-buffer stack
unmark-buffer
select-buffer %thisbuf
unmark-buffer
delete-buffer stack
