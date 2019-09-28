#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT(x)
#include <stb_image.h>
#include <stretchy_buffer.h>

void * stb__sbgrowf(void *arr, int increment, int itemsize) {
   int dbl_cur = arr ? 2*stb__sbm(arr) : 0;
   int min_needed = stb_sb_count(arr) + increment;
   int m = dbl_cur > min_needed ? dbl_cur : min_needed;
   int *p = (int *) realloc(arr ? stb__sbraw(arr) : 0, itemsize * m + sizeof(int)*2);
   if (p) {
      if (!arr)
         p[1] = 0;
      p[0] = m;
      return p+2;
   } else {
	  puts("SB: OOM");
	  exit(100);
   }
}
