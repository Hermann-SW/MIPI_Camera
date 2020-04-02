/* gcc -Wall -Wextra -pedantic spreadpgm.c -o spreadpgm
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(int argc, char *argv[])
{
  unsigned char buf[15+640*480],*p,m=0;
  int width, height;
  double sc;
  FILE *src;

  assert(argc <= 2 || !"spreadpgm [file.pgm]");
  src = (argc == 1 || strcmp(argv[1], "-") == 0) ? stdin : fopen(argv[1], "rb");

  assert(1 == fread(buf, 15, 1, src));
  assert(2 == sscanf(buf, "P5\n%d %d\n255\n", &width, &height));
  if (height%16)
    height += (16-(height%16));
  assert(1 == fread(buf+15, width*height, 1, src));
  assert(0 == fread(buf, 1, 1, src));
  assert(feof(src));
  fclose(src);

  for(p=buf+15; p<buf+15+width*height; ++p)
    if (*p > m)
      m = *p;

  sc = 254.0 / m;

  for(p=buf+15; p<buf+15+width*height; ++p)
    *p = sc * *p;

  fwrite(buf, 15+width*height, 1, stdout);

  return 0;
}
