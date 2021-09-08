## floodmaz.cmd -- solve maze by painting wall on the right

# 6 set $seed
# either maze.cmd or sharpmaz.cmd
execute-file sharpmaz.cmd

set %thisbuf $cbufname
set %meml $curline
set %memc $curcol

set $curline 1
set $curcol 0
!gosub pushxy	#push stop position
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
	!gosub probe
	set $curcol &add %cc -1
	!gosub probe
	set $curline &add %ll 1
	set $curcol %cc
	!gosub probe
	set $curline &add %ll -1
	set $curcol %cc
	!gosub probe
# pop x y
	select-buffer stack
	beginning-of-file
	set %x $line
1	kill-to-end-of-line
	set %y $line
1	kill-to-end-of-line
	select-buffer %thisbuf
	set $curline %y
	set $curcol %x
!endwhile

set $curline %meml
set $curcol %memc
select-buffer stack
unmark-buffer
select-buffer %thisbuf
unmark-buffer
delete-buffer stack
!return

:probe
	!if &not &or &equ $curchar %NC &equ $curchar 32
	:pushxy					# push x y
		set %x $curcol
		set %y $curline
		select-buffer stack
		beginning-of-file
		insert-string %x
		newline
		insert-string %y
		newline
		select-buffer %thisbuf
	!endif
!return
