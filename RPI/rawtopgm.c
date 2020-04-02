/* gcc -Wall -Wextra -pedantic rawtopgm.c -o rawtopgm
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(int argc, char *argv[])
{
  char buf[384000],*p,*q;
  int len, W[61], H[61];
  FILE *src;

  W[15]=320; W[16]=W[30]=W[60]=640;
  H[16]=120; H[15]=H[30]=240; H[60]=480;

  assert(argc <= 2 || !"rawtopgm [file.raw]");
  src = (argc == 1 || strcmp(argv[1], "-") == 0) ? stdin : fopen(argv[1], "rb");

       if (             1 == fread(buf, 384000, 1, src))  len=384000;
  else if (rewind(src), 1 == fread(buf, 307200, 1, src))  len=307200;
  else if (rewind(src), 1 == fread(buf, 153600, 1, src))  len=153600;
  else if (rewind(src), 1 == fread(buf,  81920, 1, src))  len= 81920;
  else if (rewind(src), 1 == fread(buf,  76800, 1, src))  len= 76800;
  else assert(!"unknown raw length");
  
  assert(0 == fread(buf, 1, 1, src));
  assert(feof(src));
  fclose(src);

  if (len>307200)
  {
    for(p=q=buf; p<buf+len; ++p)
    {
      *q++ = *p++;
      *q++ = *p++;
      *q++ = *p++;
      *q++ = *p++;
    }
    len = 307200;
  }
  fprintf(stdout,"P5\n%d %d\n255\n", W[len/5120], H[len/5120]);
  fwrite(buf, len, 1, stdout);

  return 0;
}
