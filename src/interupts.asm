global isr_table
; 0 ..= 255

; registers to save:
; RAX, RBX, RCX, RDX, RSI, RDI,
; RBP, R8, R9, R10, R11, R12, R13, R14, R15

%macro push_all 0
	push rax
	push rbx
	push rcx
	push rdx
	push rsi
	push rdi
	push rbp
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
%endmacro

%macro pop_all 0
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rbp
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
%endmacro

extern interrupt_handler

interupt_handler_common:
	mov rax, 1
	mov rbx, 2
	mov rcx, 3
	mov rdx, 4
	mov rsi, 5
	mov rdi, 6
	mov rbp, 7
	mov r8,  8
	mov r9,  9
	mov r10, 10
	mov r11, 11
	mov r12, 12
	mov r13, 13
	mov r14, 14
	mov r15, 15
	push_all
	
	mov rdi, rsp
	call interrupt_handler

	pop_all
	add rsp, 16
	sti
	iretq


%macro isr_noerror 1
interupt_handler_%1:
	push qword 0 ; dummy errorcode
	push qword %1; interurupt number
	cli
	jmp interupt_handler_common
%endmacro

%macro isr_error 1
interupt_handler_%1:
	push qword %1; interurupt number
	cli
	jmp interupt_handler_common
%endmacro

%assign i 0
%rep 256
%if (i == 8 || (i >= 10 && i <= 14) || i == 17 || i == 21)
isr_error i
%else
isr_noerror i
%endif
%assign i i+1
%endrep

%macro INTERRUPT_NAME 1
	dq interupt_handler_%1
%endmacro

isr_table:
%assign i 0
%rep 256
INTERRUPT_NAME i
%assign i i+1
%endrep

