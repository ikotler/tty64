/*
 * shells.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <elf.h>

#include "shbuf.h"
#include "shells.h"
#include "gizmos.h"
#include "gfx.h"
#include "elf_ext.h"
#include "preprocs.h"
#include "postprocs.h"
#include "x86.h"

static gizmo_shcode gizmos[TOTAL_GIZMOS] = \
{
	{ "x86" , "Linux" , "Dummy Gizmo"             , 2   , linux_x86_dummy     , PREPROC_SIMPLE_LEN   , gizmo_preproc_simple   , NULL } ,
	{ "x86" , "Linux" , "INT 3h / Anti Debugging" , 50  , linux_x86_adbg_int3 , PREPROC_SIMPLE_LEN   , gizmo_preproc_simple   , NULL } ,
	{ "x86" , "Linux" , "PTRACE / Anti Debugging" , 27  , linux_x86_adbg_pt   , PREPROC_SIMPLE_LEN   , gizmo_preproc_simple   , NULL } ,
	{ "x86" , "Linux" , "Crypto Gizmo"            , 240 , linux_x86_crypt_txt , PREPROC_CRYPTTXT_LEN , gizmo_preproc_crypttxt , gizmo_postproc_crypttxt } 
};

/*
 * shells_init, initialization
 */

int shells_init(void) {
	return TOTAL_GIZMOS;
}

/*
 * shells_foreach, perform foreach loop 
 * * cb, callback function on entry
 */

void shells_foreach(void (*cb)(int, gizmo_shcode)) {
	int j;

	for (j = 0; j < TOTAL_GIZMOS; j++) {
		cb(j, gizmos[j]);
	}

	return ;
}

/*
 * shells_shrtdesc, output short info on gizmo
 * * idx, given gizmo #
 * * gizmo, given gizmo
 */

void shells_shrtdesc(int idx, gizmo_shcode gizmo) {
	printf("\t #%d: %s/%s - %s (%d bytes)\n", idx, gizmo.platform, gizmo.arch, gizmo.desc, gizmo.sh_len);
	return ;
}

/*
 * shells_fulldesc, output full infon on gizmo
 * * idx, given gizmo #
 * * gizmo, given gizmo
 */

void shells_fulldesc(int idx, gizmo_shcode gizmo) {
	int bytes, j = 0;

	gfx_box("(#%d) %s/%s - %s", idx, gizmo.platform, gizmo.arch, gizmo.desc);

	printf("\nchar GIZMOCODE[%d] =\n", gizmo.sh_len);
	printf("\t\"");

	for (bytes = 0; bytes < gizmo.sh_len; bytes++) {

		j++;

		printf("\\x%02x", gizmo.sh[bytes] & 0xFF);
		
		if (j == 16 || bytes == gizmo.sh_len-1) {
			
			if (bytes == gizmo.sh_len-1) {
				
				printf("\";\n\n");
				break;
			}

			printf("\"\n\t\"");
			j = 0;
		}
	}
	
	return ;
}

/*
 * shells_get, fetch gizmo
 * * idx, given gizmo index
 */

gizmo_shcode *shells_get(int idx) {
	return (gizmo_shcode *) &gizmos[idx];
}
