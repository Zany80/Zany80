#include <ZanyFS.h>

//void malloc_init() {
	//fs_free = fs_root;
	//fs_free->next = fs_end;
	//fs_free->next_free = 0;
//}

void *malloc(size_t size, zanyfs_t *file_system) {
	
	header_t *h;
	header_t **f;
	
	if(!size || size + offsetof(header_t, next_free) < size)
		return 0;
	size += offsetof(header_t, next_free);
	if(size < sizeof(header_t))
		size = sizeof(header_t);
	
	for(h = file_system->free, f = &file_system->free; h; f = &(h->next_free), h = h->next_free) {
		size_t blocksize = (char *)(h->next) - (char *)h;
		if(blocksize >= size) {
			if(blocksize >= size + sizeof(struct header)) {
				header_t *const newheader = (header_t *const)((char *)h + size);
				newheader->next = h->next;
				newheader->next_free = h->next_free;
				*f = newheader;
				h->next = newheader;
			}
			else {
				*f = h->next_free;
			}
			return(&(h->next_free));
		}
	}

	return 0;
}
