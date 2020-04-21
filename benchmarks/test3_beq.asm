 lw 1 0 one # load reg1 with 1
 lw 2 0 limit # load reg2 with 100
 lw 3 0 zero # load reg3 with 0
start add 3 1 3 # var ++
 beq 0 0 n1
n1 beq 0 0 n2
n2 beq 0 0 n3
n3 beq 0 0 n4
n4 beq 2 3 end # if equal limit then end
 beq 0 0 start # go back to the beginning of the loop
end halt
limit .fill 100
one .fill 1
zero .fill 0