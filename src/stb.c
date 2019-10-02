#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT(x)
#include <SIMPLE/3rd-party/stb_image.h>
#include <SIMPLE/3rd-party/stretchy_buffer.h>

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

void sb_remove(void ***array, void *item) {
    if (!(*array))
        return;
    void **new_array = NULL;
    int c = sb_count(*array);
    for (int i = 0; i < c; i++) {
        if ((*array)[i] == item) {
            for (int j = 0; j < i; j++) {
                sb_push(new_array, (*array)[j]);
            }
            for (int j = i + 1; j < c; j++) {
                sb_push(new_array, (*array)[j]);
            }
            sb_free(*array);
            *array = new_array;
            break;
        }
    }
}

void sb_remove_i(void ***array, int index) {
    if (!(*array))
        return;
    void **new_array = NULL;
    int c = sb_count(*array);
    for (int i = 0; i < c; i++) {
        if (i == index) {
            for (int j = 0; j < i; j++) {
                sb_push(new_array, (*array)[j]);
            }
            for (int j = i + 1; j < c; j++) {
                sb_push(new_array, (*array)[j]);
            }
            sb_free(*array);
            *array = new_array;
            break;
        }
    }
}