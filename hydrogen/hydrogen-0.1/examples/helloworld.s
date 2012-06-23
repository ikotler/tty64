;
; helloworld.s, your favorite greeting
;

;
; Override nasm nature to generate 16bit binaries
;

BITS 32

;
; Entry Point
;

call _start

	DB "Hello World",0xA
	
_start:

	;
	; write(1, "Hello World\n", 12)
	;


	mov eax, 4
	mov ebx, 1
	pop ecx
	mov edx, 12

	;
	; exit(0)
	;

	mov eax, 1
	xor ebx, ebx
	int 80h

