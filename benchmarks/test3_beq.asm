 lw 1 0 one
 lw 2 0 limit
 lw 3 0 zero
start add 3 1 3
 beq 0 0 n1
n1 beq 0 0 n2
n2 beq 0 0 n3
n3 beq 0 0 n4
n4 beq 2 3 end
 beq 0 0 start
end halt
limit .fill 100
one .fill 1
zero .fill 0