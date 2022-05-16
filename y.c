/*-
 * DO NOT MODIFY THIS FILE
 */

#include <stdio.h>

void yyerror(char *s)
{
	fprintf(stderr, "%s\n", s);
}

#undef yywrap

int yywrap()
{
	return 1;
}
