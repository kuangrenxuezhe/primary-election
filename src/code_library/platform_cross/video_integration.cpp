#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
int main(int argc, char *argv[])
{
	int *p = (int*)malloc(1024*1024*256*sizeof(int));
	int i;
	DWORD tm = GetTickCount();
	for (i=0; i<1024*1024*256; i+=256) {
		p[i] += 1;
	}
	Sleep(1000);
	DWORD tt = GetTickCount() - tm;

	printf("%d\n", tt);

	return EXIT_SUCCESS;
}
/* ----------  end of function main  ---------- */

/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "video_integration.h"
int main()
{
	video_integration cc;
	cc.init();
	cc.add_lid(0,0,NULL,NULL);
	cc.del_lid(0,0);
	cc.add_sid(0,0,NULL,NULL);
	cc.del_sid(0,0);
	cc.add_uid(0,0);
	cc.del_uid(0,0);
	cc.output_beg();
	cc.output_end();
	var_4 tmp;
	cc.output_nid(0,NULL,NULL,0,NULL,0,tmp,NULL,0,tmp,NULL,0,tmp,NULL,NULL,0,tmp,NULL,NULL,0,tmp);

	return 0;
}
*/
