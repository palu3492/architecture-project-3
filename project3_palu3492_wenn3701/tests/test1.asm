 lw 1 0 one
 lw 2 0 limit
 lw 3 0 zero
start add 3 1 3
 add 4 4 6
 nand 4 4 6
 beq 2 3 end
 beq 0 0 start
end halt
limit .fill 25
one .fill 1
zero .fill 0
