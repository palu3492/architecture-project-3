 lw 2 1 3
 lw 1 0 ten
start add 1 2 1
 beq 0 1 2
 beq 0 0 start
 sw 2 1 0
 noop
done halt
ten .fill 10
neg2 .fill -2
stAdd .fill start
