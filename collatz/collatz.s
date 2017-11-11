@ collatz.s

/* function to return the number of steps in the collatz sequence a number produces */
.global	collatz
collatz:
    @ TODO write this function
	@ r1 store the number to operate
	@ r2 is int 1 for AND
	@ r3 is int 3 for multiplication
	mov r1, r0
	mov r0, #0
	mov r2, #1
	mov r3, #3
.top:
	cmp r1, #1
	beq .finish
	and r4, r1, #1
	cmp r4, #0
	bne .odd
	b .even
.even:
	mov r1, r1, lsr #1
	add r0, r0, #1
	b .top
.odd:
	mul r1, r3, r1
	add r1, r1, #1
	add r0, r0, #1
	b .top
.finish:
    mov pc, lr

