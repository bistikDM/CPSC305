@ uppercase.s

/* function to convert a given string to all uppercase */
.global	uppercase
uppercase:
    @ TODO - write this function
	@ r4 is the counter
	@ r5 temp space of r0
	@ r6 uppercased
	sub sp, sp, #16
	str lr, [sp]
	str r4, [sp, #4]
	str r5, [sp, #8]
	str r6, [sp, #12]
	mov r4, #0
	mov r5, r0
.top:
	ldrb r0, [r5, r4]
	cmp r0, #0
	beq .end
	bl toupper
	strb r0, [r6, r4]
	add r4, r4, #1
	b .top
.end:
	strb r0, [r6, r4]
	mov r0, r6
	ldr lr, [sp]
	ldr r4, [sp, #4]
	ldr r5, [sp, #8]
	ldr r6, [sp, #12]
	add sp, sp, #16
    mov pc, lr 

