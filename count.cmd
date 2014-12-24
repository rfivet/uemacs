; count.cmd -- create a buffer with digit from 1 to n
set %i 1
!while &less %i 2000000
	insert-string %i
	newline
	set %i &add %i 1
!endwhile
write-file count.txt
exit-emacs
