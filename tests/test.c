/*

	test.c

*/

#include "filp.h"

int i_test=1;
char color[256];

int main(void)
{
	filp_startup();

	filp_exec("qq 100 set /* comentario */ $qq print");
	filp_exec("prn { $filp_compile_time print } set prn");
	filp_exec("path 'PATH' getenv set $path print '*\t*' print");

	filp_exec("'TERM' getenv 'linux' eq { '¡es linux!' print } if");

	filp_exec("false { 'dentro' print } unless");

	filp_exec("/* sé sumar!!! */ 5 10 + print");
	filp_exec("25 10 sub 15 eq { 'sé restar!!!' print } if");
	filp_exec("50 10 > { '50 sí es > 10' } { '50 no es > 10' } ifelse print");

	filp_exec("qq 0 set { qq $qq 1 add set $qq print $qq 10 == { exit } if } loop");
	filp_exec("10 { qq $qq 1 sub set $qq print } repeat");

	filp_shutdown();

	return(0);
}
