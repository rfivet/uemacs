# record seed
# 5 set $seed
set %S $seed

# setup direction offsets
set %D1 0
set %D2 1
set %D3 0
set %D4 -1
set %D5 0

# draw the maze layout
$curwidth insert-string " "
newline
set %w &sub $curwidth 2
!if &equ 0 &mod %w 2
	set %w &sub %w 1
!endif
insert-string " "
%w insert-string "▓"
insert-string " "
set %buf $line
set %l &sub $pagelen 4
!if &equ 1 &mod %l 2
	set %l &sub %l 1
!endif
set %cnt %l
!while &less 0 %cnt
	set %cnt &sub %cnt 1
	newline
	insert-string %buf
!endwhile
newline
set %w &add %w 1
%w insert-string " "
set %l &add %l 3

# draw the exit
set $curline &sub %l 2
set $curcol &sub %w 1
set $curchar 32

# draw the maze
set %x 2
set %y 3
set $curline %y
set $curcol %x
set $curchar 32
set %flags 0
set %cnt &tim &sub &div %w 2 1 &sub &div %l 2 1 
!while &les 1 %cnt
	!if &or &equ %flags 15 &not &equ $curchar 32
		set %flags 0
		set %y &add %y 2
		!if &equ %y %l
			set %y 3
			set %x &add %x 2
			!if &equ %x %w
				set %x 2
			!endif
		!endif
	!else
		set %D &rnd 4
		set %OX &ind &cat "%D" %D
		set %OY &ind &cat "%D" &add %D 1
		set $curline &add %y &tim 2 %OY
		set %i &add %x &tim 2 %OX
		set $curcol %i
		!if &equ $curchar 32
			!if &equ %D 3		# turn direction into bitmask {1,2,4,8}
				set %D 8
			!endif
			set %flags &bor %flags %D	# mark direction as checked
		!else
			set $curchar 32
			set %y $curline		# update current position
			set %x %i
			set $curline &sub %y %OY	# erase path between old and cur pos
			set $curcol &sub %x %OX
			set $curchar 32
			set %flags 0
			set %cnt &sub %cnt 1
		!endif		
	!endif
	set $curline %y
	set $curcol %x
!endwhile

# id maze
beginning-of-file
kill-to-end-of-line
insert-string &cat " Maze " &cat %w &cat "x" &cat &sub %l 2 &cat " #" %S
write-message $line
&sub $curwidth $curcol insert-string " "

# draw the entrance
set $curline 3
set $curcol 1
set $curchar 32
