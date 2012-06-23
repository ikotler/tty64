/*
 * gizmos.h
 */

#ifndef TOTAL_GIZMOS
	#define TOTAL_GIZMOS 4
#endif

char linux_x86_dummy[] = \
	"\xeb\xf7";	 		/* jmp shr -0x7 */

char linux_x86_adbg_int3[] = \
	"\x31\xc0"			/* xor eax, eax */
	"\x31\xdb"			/* xor ebx, ebx */
	"\x31\xc9"			/* xor ecx, ecx */
	"\xb0\x30"			/* mov al, 0x30 */
	"\xb3\x05"			/* mov bl, 0x5  */
	"\xe8\x00\x00\x00\x00"  	/* call next    */
	"\x59"				/* pop ecx 	*/
	"\x81\xc1\x10\x00\x00\x00"	/* add ecx, 0x10*/
	"\xcd\x80"			/* int 80h	*/
	"\xcc"				/* int 03h	*/
	"\xb0\x01"			/* mov al, 0x1	*/
	"\x31\xdb"			/* xor ebx, ebx	*/
	"\xcd\x80"			/* int 80h	*/
	"\xb0\x30"			/* mov al, 0x30 */
	"\xcd\x80"			/* int 80h	*/
	"\xe8\x00\x00\x00\x00"  	/* call next    */
	"\x58"				/* pop eax	*/
	"\x05\x08\x00\x00\x00"		/* add eax, 0x8 */
	"\x50"				/* push eax	*/
	"\xc3"				/* ret		*/
	"\xeb\xc7";		 	/* jmp -0xc7	*/

char linux_x86_adbg_pt[] = \
	"\x60"				/* pusha	*/
        "\x31\xc0"              	/* xor eax, eax */
        "\x31\xdb"              	/* xor ebx, ebx */
        "\x31\xc9"              	/* xor ecx, ecx */
        "\x31\xd2"              	/* xor edx, edx */
	"\x31\xf6"			/* xor esi, esi */
        "\xb0\x1a"              	/* mov al, 0x1a */
	"\xcd\x80"              	/* int 80h      */
	"\x85\xc0"			/* test, eax    */
	"\x74\x05"			/* jz 0x5	*/
	"\x31\xc0"			/* xor eax, eax */
	"\x40"				/* inc eax 	*/
        "\xcd\x80"              	/* int 80h      */
	"\x61"				/* popa		*/
	"\xeb\xde";			/* jmp -0xde	*/

char linux_x86_crypt_txt[] = \
	"\x60"				/* pusha		*/
	"\x89\xe5"			/* mov ebp,esp  	*/
	"\x81\xEC\x48\x00\x00\x00"	/* mov esp,0x48		*/
	"\x8d\x55\xb8"			/* lea edx,[ebp-0x48]  	*/
	"\x52"				/* push edx  		*/
	"\x68\x01\x54\x00\x00"		/* push dword 0x5401 	*/
	"\xe8\xbe\x00\x00\x00"		/* call 0xbe  		*/
	"\x8d\x45\xc4"			/* lea eax,[ebp-0x3c]  	*/
	"\x80\x08\x08"			/* or [eax],0x8 	*/
	"\x80\x28\x08"			/* sub [eax], 0x8 	*/
	"\x52"				/* push edx  		*/
	"\x68\x02\x54\x00\x00"		/* push dword 0x5402 	*/
	"\xe8\xaa\x00\x00\x00"		/* call 0xaa  		*/
	"\x81\xC4\x10\x00\x00\x00"	/* add esp,0x10		*/
	"\xe8\x0a\x00\x00\x00"		/* call 0x0a  		*/

	/* 
	 * PROMPT DB "Password: "
	 */

	"\x50\x61\x73\x73\x77\x6f\x72\x64\x3a\x20"

	"\xb8\x04\x00\x00\x00"		/* mov eax,0x4  	*/
	"\x5b"				/* pop ebx  		*/
	"\x8d\x0b"			/* lea ecx,[ebx]  	*/
	"\xbb\x01\x00\x00\x00"		/* mov ebx,0x1  	*/
	"\xba\x0a\x00\x00\x00"		/* mov edx,0xa  	*/
	"\xcd\x80"			/* int 80h  		*/
	"\x81\xEC\x11\x00\x00\x00"	/* sub esp,0x11		*/
	"\xb8\x03\x00\x00\x00"		/* mov eax,0x3  	*/
        "\x31\xdb"              	/* xor ebx, ebx 	*/
	"\x89\xe1"			/* mov ecx,esp  	*/
	"\xba\x11\x00\x00\x00"		/* mov edx,0x11  	*/
	"\xcd\x80"			/* int 80h  		*/
	"\x48"				/* dec eax		*/
	"\x50"				/* push eax  		*/
	"\x8d\x45\xc4"			/* lea eax,[ebp-0x3c]  	*/
	"\x81\x00\x08\x00\x00\x00"	/* add dword [eax],0x8 	*/
	"\x8d\x55\xb8"			/* lea edx,[ebp-0x48]  	*/
	"\x52"				/* push edx  		*/
	"\x68\x02\x54\x00\x00"		/* push dword 0x5402 	*/
	"\xe8\x52\x00\x00\x00"		/* call 0x52  		*/
	"\xb8\x04\x00\x00\x00"		/* mov eax,0x4		*/
	"\xbb\x01\x00\x00\x00"		/* mov ebx,0x1  	*/
	"\x68\x0a\x00\x00\x00"		/* push dword 0xa	*/
	"\x8d\x0c\x24"			/* lea ecx,[esp]  	*/
	"\xba\x01\x00\x00\x00"		/* mov edx,0x1		*/
	"\xcd\x80"			/* int 80h		*/
	"\x81\xc4\x0c\x00\x00\x00"	/* add esp,0xC		*/
	"\x80\x3c\x24\x01"		/* cmp byte [esp],0x1	*/
	"\x7c\x41"			/* jl 0x41		*/
	"\xe8\x3e\xff\xff\xff"		/* call -0x3e		*/
	"\xe8\x0c\x00\x00\x00"		/* call 0x0c		*/
	"\x81\xc4\x5d\x00\x00\x00"	/* add esp,0x5d		*/
	"\x61"				/* popa			*/
	"\xe9\x3b\xff\xff\xff"		/* jmp -0x3b		*/
	"\x55"				/* push ebp  		*/
	"\x89\xe5"			/* mov ebp,esp  	*/
        "\x31\xd2"              	/* xor edx, edx 	*/
	"\x3b\x55\x08"			/* cmp edx,[ebp+0x8]	*/
	"\x75\x02"			/* jne 0x2		*/
	"\x31\xd2"			/* xor edx, edx		*/
	"\xac"				/* lodsb		*/
	"\x32\x44\x15\x0C"		/* xor al,[ebp+edx+0xc] */
	"\x42"				/* inc edx		*/
	"\xaa"				/* stosb		*/
	"\xe2\xf0"			/* loop			*/
	"\xc9"				/* leave		*/
	"\xc3"				/* ret			*/
	"\x55"				/* push ebp  		*/
	"\x89\xe5"			/* mov ebp,esp  	*/
	"\xb8\x36\x00\x00\x00"		/* mov eax,0x36  	*/
	"\x31\xdb"			/* xor ebx,ebx  	*/
	"\x8b\x4d\x08"			/* mov ecx,[ebp+0x8]  	*/
	"\x8b\x55\x0c"			/* mov edx,[ebp+0xc]  	*/
	"\xcd\x80"			/* int 80h  		*/
	"\xc9"				/* leave   		*/
	"\xc3"				/* ret   		*/
	"\xb8\x01\x00\x00\x00"		/* mov eax,0x1  	*/
	"\xcd\x80";			/* int 80h  		*/
