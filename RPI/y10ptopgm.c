/* gcc -Wall -Wextra -pedantic y10ptopgm.c -o y10ptopgm
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(int argc, char *argv[])
{
  char buf[384000],*p,*q;
  FILE *src;

  assert(argc <= 2 || !"y10ptopgm [file.y10p]");
  src = (argc == 1 || strcmp(argv[1], "-") == 0) ? stdin : fopen(argv[1], "rb");

  assert(1 == fread(buf, 384000, 1, src));
  assert(0 == fread(buf, 1, 1, src));
  assert(feof(src));
  fclose(src);

  for(p=q=buf; p<buf+384000; ++p)
  {
    *q++ = *p++;
    *q++ = *p++;
    *q++ = *p++;
    *q++ = *p++;
  }
  fprintf(stdout,"P5\n640 480\n255\n");
  fwrite(buf, 640*480, 1, stdout);

  return 0;
}
