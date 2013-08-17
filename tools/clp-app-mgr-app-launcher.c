#include <glib.h>
#include <stdio.h>
#include "../src/clp-app-mgr-lib.h"

int main(int argc, char *argv[])
{
	g_type_init();
	int ret;

	if (argc == 2) {
		ret = clp_app_mgr_exec(argv[1], NULL);
	}
	else if (argc > 2)
	{
		ret = clp_app_mgr_exec_argv(argv[1],argc-2,&argv[2]);

	}
	
	if( ret!=0 )
		g_print(" Ret=%d",ret);
	return ret;
}
