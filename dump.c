#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "dump.h"

int
getmemfd(pid_t pid)    //get file desctiptor of pid's memory file in proc
{
  char *buf = malloc(BUFSIZ);
  memset(buf, 0, BUFSIZ);
  sprintf(buf, "/proc/%d/mem", pid);
  int fd = open(buf, O_RDWR);
  free(buf);
  return fd;
}

int
getmapsfd(pid_t pid)    //get file desctiptor of pid's maps file in proc
{
  char *buf = malloc(BUFSIZ);
  memset(buf, 0, BUFSIZ);
  sprintf(buf, "/proc/%d/maps", pid);
  int fd = open(buf, O_RDWR);
  free(buf);
  return fd;
}



long  //returns position of first occurence of *key in *buf, -1 otherwise
d_search(void *v_key, void *v_buf, size_t k_siz, size_t b_siz)
{
  char *key = v_key;
  char *buf = v_buf;
  long state = 0;
  long  k_pos = 0;
  for (long i = 0; i < b_siz; i++) {
    if (key[k_pos] == buf[i]) {
      k_pos++;
      if (k_pos == k_siz)
	return i - k_siz + 1;
    }
    else
      k_pos = 0;
  }
  return -1;
}


unsigned long   //Converts proceding  hexdigits in  *line to ulong
stoul(char *line)
{
  unsigned long ret = 0;
  for (; isxdigit(*line); line++) {
    unsigned long dig;
    char ch = *line;
    if (ch >= '0' && ch <= '9') {
      dig =  ch - '0';
    }
    else if (ch >= 'a' && ch <= 'f') 
      dig = 10 + (ch - 'a');
    else if (ch >= 'A' && ch <= 'F')
      dig = 10 + (ch - 'A');
    else
      return 0;
    ret = ret * 16 + dig;
  }
  return ret;
}

long   //reads memory region in file fd into buffer buf
read_region(int fd, struct mem_region* reg,
	    char* buf, size_t b_siz)
{
  if (reg -> size > b_siz)
    return -1;
  if (lseek(fd, reg -> lower, SEEK_SET) < 0)
    return -2;
  memset(buf, 0, b_siz);
  long n;
  if ((n = read(fd, buf, reg -> size)) < 0)
    return -3;
  return n;
}

void
parse_line(char *line, struct mem_region* buf) //parse line in /proc/*/maps and put data to buf
{
  buf -> lower = stoul(line);
  while (*line != '-')
    line++;
  line++;
  buf -> upper = stoul(line);
  while (*line != ' ')
    line++;
  line++;
  buf -> size = buf -> upper - buf -> lower;
  for (int i = 0; i < 4; i++) {
    buf -> access[i] = line[i];
  }
  buf -> access[4] = '\0';
  while (*line != '[' && *line != '/')
    line++;
  int pos = 0;
  while (line[pos] != '\n' &&  line[pos] != '\0') {
    buf -> path[pos] = line[pos];
    pos++;
  }
}

int //creates struct addr_space in *buf with info about pid
get_addr_space(pid_t pid, struct addr_space* buf)
{
  int mem_fd;
  if ((mem_fd = getmemfd(pid)) < 0)
    return -1;  
  int maps_fd;
  if ((maps_fd = getmapsfd(pid)) < 0)
    return -1;
  FILE* maps_str = fdopen(maps_fd, "r");
  if (maps_str == NULL)
    return -1;
  size_t size = BUFSIZ;
  char *s_buf = malloc(size);
  if (buf == NULL)
    return -1;
  buf -> regions = malloc(sizeof(struct mem_reg_list));
  struct mem_reg_list *temp = buf -> regions;
  struct mem_reg_list *prev;
  while (getline(&s_buf, &size, maps_str) != EOF) {
    struct mem_region *m_buf = malloc(sizeof (struct mem_region));
    parse_line(s_buf, m_buf);
    temp -> region = m_buf;
    temp -> next = malloc(sizeof(struct mem_reg_list));
    prev = temp;
    temp = temp -> next;
  }
  free(temp);
  prev -> next = NULL;
  free(s_buf);
  buf -> pid = pid;
  buf -> mem_fd = mem_fd;
  return 0;
}

struct mem_region*
iter_region(struct addr_space* sp)
{
  static struct addr_space *prev = NULL;
  static int n_reg = 0;
  if (sp == NULL) {
    prev = NULL;
    return NULL;
  }
  if (prev == NULL || prev != sp) {
    prev = sp;
    n_reg = 0;
  }
  struct mem_reg_list *temp = sp -> regions;
  for (int i = 0; i < n_reg; i++) {
    temp = temp -> next;
    if (temp == NULL)
      return NULL;    
  }
  n_reg++;
  return temp -> region;
}

void
free_ptr_list(struct ptr_list* list)
{
  if (list == NULL)
    return;
  struct ptr_list* next;
  while ((next = list -> next) != NULL) {
    free (list);
    list = next;
  }
  free(list);
}

struct ptr_list*
find_data_in_region (int mem_fd, void* v_key, size_t k_siz, struct mem_region* reg, int *found)
{
  (*found) = 0;
  struct ptr_list *prev = NULL;
  struct ptr_list *ret = NULL;
  char *buf = malloc(reg -> size);
  if (buf == NULL)
    return NULL;  
  char *start = buf;
  lseek(mem_fd, reg -> lower, SEEK_SET);
  long n = read(mem_fd, buf, reg -> size);
  if (n <= 0)
    return NULL;
  char *key = v_key;
  long pos = 0;
  long als = 0;
  while ((pos = d_search(v_key, (char *)buf, k_siz, n)) >= 0) {
    (*found)++;
    if (prev == NULL) {
      ret = prev = malloc(sizeof(struct ptr_list));
      prev -> ptr = (void*)(reg -> lower + pos + als);
      prev -> next = NULL;
    }
    else {
      prev -> next = malloc(sizeof(struct ptr_list));
      prev = prev -> next;
      prev -> ptr = (void*)(reg -> lower + pos + als);
      prev -> next = NULL;
    }
    als += (pos + k_siz);
    buf += (pos + k_siz);
    n -= (pos + k_siz);;
  }
  free(start);
  return ret;
}

struct ptr_list*
find_data_in_space(void *key, size_t size, struct addr_space *sp, int *tf)
{
  struct ptr_list *ret, *curr, *prev;
  ret = curr = prev = NULL;
  for (struct mem_region *reg = iter_region(sp); reg != NULL; reg = iter_region(sp)) {
    int found = 0;
    curr = find_data_in_region(sp -> mem_fd, key, size, reg, &found);
    if (found == 0)
      continue;
    else if (found == -1) {
      (*tf) = -1;
      return NULL;
    }
    (*tf) += found;
    if (ret == NULL) {
      ret = prev = curr;      
    }
    else {
      prev -> next = curr;
    }
    while (prev -> next) {
      prev = prev -> next;
    }
  }
  iter_region(NULL);
  return ret;
}



void
print_hex_data(void *data, size_t size)
{
  char *c = data;
  char d;
  for (int i = 0; i < size; i++) {
    d = c[i];
    printf("%hhX", d);
  }
  printf("\n");
}

void
print_data_list(struct data_list* d)
{
  int n = 0;
  while (d) {
    n++;
    print_hex_data(d -> data, d -> size);
    d = d -> next;
  }
  printf("\n");
}

void
print_ptr_list(struct ptr_list *p)
{
  while (p) {
    printf("\t%p\n", p -> ptr);
    p = p -> next;
  }
}


void
print_region(struct mem_region *r)
{
  printf("Region %s:\n", r -> path);
  printf("Bounds: %p - %p\n", (void*)r -> lower, (void*)r -> upper);
  printf("Total size in bytes: %d\n", r -> size);
  printf("Access: %s\n\n", r -> access);
  printf("------------------------------------------\n\n");
}


void
print_wp_list(struct wp_list *l)
{
  int i = 1;
  l = l -> next;
  while (l) {
    printf("%d:\t\n", i);
    print_watch_point(l -> point);
    l = l -> next;
    printf("**************************\n");
    i++;
  }
  printf("--------------------------------------------------------------------------------\n\n");
}

void
print_watch_point(const struct watch_point *p)
{
  printf("Type: ");
  char *s_type;
  switch(p -> type) {
  case 'g':
    s_type = "double";
    break;
  case 'd':
    s_type = "int";
    break;
  case 'l':
    s_type = "long";
    break;
  case 'c':
    s_type = "char";
    break;
  case 's':
    s_type = "string";
    break;
  case 'r':
    s_type = "raw";
    break;    
  }
  char c = p -> type;
  printf("%s;\n", s_type);
  printf("Size: %d;\n", p -> data -> size);
  void *t = p -> data -> data;
  printf("Contents: \"");
  if (c == 'g')
    printf("%g", *((double*) t) );
  else if (c == 'd')
    printf("%d", *((int*) t));
  else if (c == 'c')
    printf("%c", *((char*)t) );
  else if (c == 'l')
    printf("%ld", *((long*)t) );
  else if (c == 's')
    printf("%s", ((char*) t) );
  else if (c == 'r')
    print_hex_data(t, p -> data -> size);
  else if (c == 'f')
    printf("%f", *( (float*) t));
  printf("\"\n");
  printf("Located in:\n");
  if (p -> ptrs) {
    print_ptr_list(p -> ptrs);
  }
  else
    printf("nowhere\n");
}

struct wp_list*
combine_wp_lists(struct wp_list *l1, struct wp_list *l2)
{
  
}


void
remove_ptr_from_list(struct ptr_list* l, void *p)
{
  struct ptr_list *prev = NULL;
  while(l) {
    if (l -> ptr == p) {
      if (prev) 
	prev -> next = l -> next;
      free(l);
      return;       
    }
    l = l -> next;
  }
}


int
rm_drifted_watchpoint(struct watch_point *p, struct addr_space *sp)
{
  int rm_total = 0;
  struct ptr_list *ptr = p -> ptrs, *start;
  start = ptr;
  while (ptr) {
    lseek(sp -> mem_fd, (off_t)ptr -> ptr, SEEK_SET);
    char *buf = malloc(p -> data -> size);
    read(sp -> mem_fd, buf, p -> data -> size);    
    if (memcmp(buf, p -> data -> data, p -> data -> size) != 0) {
      void *t = ptr -> ptr;
      ptr = ptr -> next;
      remove_ptr_from_list(start, t);
      rm_total++;
    }
    else
      ptr = ptr -> next;    
  }
}


struct ptr_list*
init_watchpoint(struct watch_point* p, struct addr_space *sp)
{
  int n;
  p -> ptrs = find_data_in_space(p -> data -> data, p -> data -> size, sp, &n);
  if (p -> ptrs == NULL)
    return NULL;
  p -> sp = sp;
  return p -> ptrs;
}

void
init_wp_list(struct wp_list *l, struct addr_space *sp)
{
  l = l -> next;
  while (l) {
    init_watchpoint(l -> point, sp);
    l = l -> next;
  }
}


