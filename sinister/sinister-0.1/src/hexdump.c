/*
 * hexdump.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include "hex_ext.h"
#include "utilities.h"
#include "consts.h"

static hex_stack *stack;

/*
 * hexdump_initialization, initialization of hex dump viewer
 * * bir, bytes, # of bytes in a row
 * * s_addr, given initial address
 * * e_addr, given final address
 */

int hexdump_initialization(int bir, void *s_addr, void *e_addr) {

	stack = calloc(1, sizeof(hex_stack));
	
	if (!stack) {
		perror("calloc");
		return -1;
	}

	stack->bir = bir;
	stack->baddr = s_addr;
	stack->eaddr = e_addr;

	return 1;
}

/*
 * hexdump__finialization, finialization of hex dump viewer
 */

void hexdump_finialization(void) {
	hex_entry *ptr, *ptr_nxt;

	if (!stack || !stack->head) {
		return ;
	}	

	ptr = stack->head;

	while (ptr) {
		ptr_nxt = ptr->next;
		free(ptr);
		ptr = ptr_nxt;
	}	

	return ;
}

/*
 * hexdump_getcurrentptr, get current pointer in the stack
 */

__inline__ void *hexdump_getcurrentptr(void) {
	hex_entry *tmp;

	if (stack->head == NULL) {

		tmp = calloc(1, sizeof(hex_entry));

		if (!tmp) {
			perror("calloc");
			return NULL;
		}

		stack->head = stack->tail = tmp;

	} else {

		stack->tail->next = calloc(1, sizeof(hex_entry));

		if (!stack->tail->next) {
			perror("calloc");
			return NULL;
		}

		tmp = stack->tail->next;
		stack->tail = tmp;
	}

	return tmp;
}

/*
 * hexdump_recursive_psh, recursive data disassemabler
 * * data, given data
 * * shifts, # of shifts
 */

int hexdump_recursive_psh(long data, int shifts) {
	hex_entry *cur;

	cur = hexdump_getcurrentptr();

	if (!cur) {
		return -1;
	}
	
	cur->data = (data & 0xFF);
	stack->totbytes++;

	if (shifts+1 < PTRACE_WORD) {
		return hexdump_recursive_psh((data >> 8), shifts+1);
	}

	return 1;
}

/*
 * hexdump_push, push data into hexdump stack
 * * data, given data
 */

int hexdump_push(long data) {
	return hexdump_recursive_psh(data, 0);
}

/*
 * hexdump_display, generate hex dump from stack
 */

int hexdump_display(void) {
	int idx, pad_length = 0;
	char *hdisplay, c;
	hex_entry *ptr;

	if (stack->baddr == stack->eaddr) {
		return 0;
	}

	if (stack->totbytes < stack->bir) {

		if ( ( stack->totbytes < addr_substruct(stack->baddr, stack->eaddr) ) ||
		     ( addr_substruct(stack->baddr, stack->eaddr) > stack->bir ) )
		{
			return 0;
		}
	}
	
	hdisplay = calloc(1, stack->bir);

	if (!hdisplay) {
		perror("calloc");
		return 0;
	}

	printf("%p: \t", stack->baddr);

	ptr = stack->head;

	for (idx = 0; (idx < stack->bir && ptr); idx++) {

		printf("%02x ", (ptr->data & 0xFF));

		c = ptr->data;

		if (!isgraph(ptr->data)) {
                	c = '.';
		}
		
		hdisplay[idx] = c;

		ptr = ptr->next;
		free(stack->head);

		stack->head = ptr;
		stack->totbytes--;

		stack->baddr = addr_increase(stack->baddr, 1);

		if (stack->baddr == stack->eaddr) {
			pad_length = stack->bir - idx - 1;
			break;
		}
	}

	while (pad_length) {
		printf("   ");
		hdisplay[idx] = ' ';
		idx++;
		pad_length--;
	}
	
	printf("| %s\n", hdisplay);

	free(hdisplay);

	return 1;
}
