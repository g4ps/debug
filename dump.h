#ifndef MY_DEBUG
#define MY_DEBUG 1
#include <unistd.h>
#include <stdint.h>


struct
mem_region
{
  unsigned long upper;
  unsigned long lower;
  unsigned long size;
  char access[5];
  char path[100];  
};

struct
mem_reg_list
{
  struct mem_region* region;
  struct mem_reg_list* next;
};

struct
addr_space
{
  int mem_fd;
  pid_t pid;
  struct mem_reg_list* regions;
  int n_reg;
};

struct
ptr_list
{
  void *ptr;
  struct ptr_list *next;
};


struct
data_instance
{
  void* data;
  size_t size;
};

struct
data_list
{
  void *data;
  size_t size;
  struct data_list* next;
};

struct
watch_point
{
  struct data_instance *data;
  struct ptr_list *ptrs;
  char type;
  struct addr_space *sp;
};

struct
wp_list
{
  struct watch_point *point;
  struct wp_list *next;
  struct wp_list *prev;
  struct wp_list *root;
};

void print_ptr_list(struct ptr_list *list);
//prints ptr_list;

void free_ptr_list(struct ptr_list* list);
//free structure ptr_list

struct ptr_list* find_data_in_region(int mem_fd, void *key,
				     size_t k_siz, struct mem_region* reg, int *f);
//find key (size of k_siz) in region reg , while mem_fd points to mem file
//and set f to total keys found in region

struct ptr_list* find_data_in_space(void *key, size_t size, struct addr_space *sp, int *f);
//find key (size of size) in space sp, and set f to how many found

int getmemfd(pid_t pid);   //get file descriptor for mem file in proc
//for according process

int getmapsfd(pid_t pid);   //same thing for maps file

long d_search(void *key, void *buf, size_t k_size, size_t b_size);
//searches for key (size of k_size) in buf (size of b_size)
//and returns position of first byte of key in buf

unsigned long stoul (char *line); //converts hexdigits in char* to ulong

long read_region(int mem_fd, struct mem_region *reg, char *buf, size_t buf_siz);
//read region of reg from mem_fd to buf(size of buf_siz)

void parse_line(char *line, struct mem_region *buf);
//parse line in /proc/*/maps into *buf
  
int get_addr_space(pid_t pid, struct addr_space* buf);
//fills buf with info about pid

struct mem_region* iter_region (struct addr_space *sp);
//iteratively go through all reions in address space sp
//if sp == NULL, then restart iteration

void print_region(struct mem_region *r);
//prints information about r in stdout

void print_hex_data(void *data, size_t size);
//print buffer data (size of size) in hexademical notation
//simular to hexdump program

void print_data_list(struct data_list *d);
//prints data_list

struct data_list* parse_input_file(FILE* fs, int *lin, char *h);
//parses input file and retruns buffer or NULL if something is wrong;
//lin returns number of lines parsed, or number of troubled line
//in case of error

void print_watch_point(const struct watch_point*);
//prints structure watch_point

struct wp_list*  wp_parse_input_file(FILE *fs, int *lin);
//parse input file fs and get wp_list of watchpoints from input file fs

int add_wp_to_list (struct wp_list *, struct watch_point*);
//add watchpoint to the list ow watchpoints

struct watch_point* parse_input_wp_line (char *line, int *status);
//parse a line and make watchpoint without initialising it
//returns NULL and sets status < 0 in case of an error
//return NULL and sets status = 0 in case of comment line

struct ptr_list* initwatch_point(struct watch_point*, struct addr_space*);
//initialise watchpoint in addr_space with data from watch_point
//returns ptr_list of matching points from addr_space

void print_wp_list(struct wp_list*);
//print wp list;

void init_wp_list(struct wp_list *l, struct addr_space *sp);
//initialize whole list of watchpoints


#endif
