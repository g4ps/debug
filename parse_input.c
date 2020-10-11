#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include "dump.h"


static uint8_t
getbyte(char *l, int *state)
{
  uint8_t ret = 0;
  (*state) = 0;
  for (int i = 0; i < 2; i++) {
    ret *= 16;
    if (l[i] >= '0' && l[i] <= '9') {
      ret += l[i] - '0';
    }
    else if (l[i] >= 'a' && l[i] <= 'f') {
      ret += l[i] - 'a' + 10;
    }
    else if (l[i] >= 'A' && l[i] <= 'F') {
      ret += l[i] - 'A' + 10;
    }
    else {
      (*state) = -1;
      return 0;
    }
  }
}



struct watch_point*
parse_input_wp_line(char *line, int *status)
{
  struct watch_point *ret;
  ret = malloc(sizeof(struct watch_point));
  ret -> ptrs = NULL;
  ret -> sp = NULL;
  while (isspace(*line)) {
    line++;    
  }
  (*status) = 0;
  ret -> data = malloc(sizeof (struct data_instance));
  if ((*line) == '#') {
    (*status) = 1;
    free(ret);
    return NULL;
  }
  char *i_buf = malloc(BUFSIZ);
  int i_pos = 0;
  char *data = malloc(BUFSIZ);
  int size = 0;
  while(!isspace(*line)) {      
    i_buf[i_pos] = (*line);
    if ((*line) == ']') {
      line++;
      break;
    }
    line++;
    i_pos++;
  }
  while (isspace(*line)) {
    line++;
  }
  if (strcmp(i_buf, "[double]") == 0) {
    ret -> type = 'g';
    if (sscanf(line, "%g", (double*)data) != 1) {
      (*status) = -1;
      free(i_buf);
      free(data);
      return NULL;
    }
    size = sizeof(double);
  }
  else if (strcmp (i_buf, "[char]") == 0) {
    ret -> type = 'c';
    if (sscanf(line, "%c", data) != 1) {
      (*status) = -1;
      free(i_buf);
      free(data);
      return NULL;
    }
    size = sizeof(char);
  }
  else if (strcmp (i_buf, "[long]") == 0) {
    ret -> type = 'l';
    if (sscanf(line, "%ld", (long*)data) != 1) {
      (*status) = -1;
      free(i_buf);
      free(data);
      return NULL;
    }
    size = sizeof(long);
  }
  else if (strcmp (i_buf, "[int]") == 0) {
    ret -> type = 'i';
    if (sscanf(line, "%d", (int*)data) != 1) {
      (*status) = -1;
      free(i_buf);
      free(data);
      return NULL;
    }
    size = sizeof(int);      
  }
  else if (strcmp (i_buf, "[string]") == 0) {
    ret -> type = 's';
    while ((*line) != '\"')
      line++;
    line++;
    int temp = 0;
    while ((*line) != '\"' && temp < BUFSIZ) {
      (*status) = -1;
      data[temp] = (*line);
      line++;
      temp++;
    }
    if (temp == BUFSIZ) {
      (*status) = -1;
      free(i_buf);
      free(data);
      return NULL;
    }
    data[temp + 1] = '\0';
    size = temp;
  }
  else if (strcmp (i_buf, "[raw]") == 0) {
    ret -> type = 'r';
    int state = 0;
    while (state == 0 && isxdigit(*line) &&
	   size < BUFSIZ) {
      data[size] = getbyte(line, &state);
      line += 2;
      size++;
    }
    if (size == BUFSIZ || state == -1) {
      (*status) = -1;
      free(i_buf);
      free(data);
      return NULL;
    }
  }
  else if (strcmp (i_buf, "[float]") == 0) {
    ret -> type = 'f';
    if (sscanf(line, "%f", (float*)data) != 1) {
      (*status) = -1;
      free(i_buf);
      free(data);
      return NULL;
    }
    size = sizeof(float);
  }
  else {
    (*status) = -1;
    free(i_buf);
    free(data);
    return NULL;
  }
  ret -> data -> data = malloc(size);
  memcpy (ret -> data -> data, data, size);
  ret -> data -> size = size;
  return ret;
}

int
add_wp_to_list(struct wp_list* wpl,
	       struct watch_point *wp)
{
  if (wpl == NULL || wp == NULL)
    return -1;
  while (wpl -> next) {
    wpl = wpl -> next;
  }
  wpl -> next = malloc(sizeof(struct wp_list));
  wpl -> next -> next = NULL;
  wpl -> next -> prev = wpl;
  wpl -> next -> root = wpl -> root;
  wpl -> next -> point = wp;  
  return 0;
}

struct wp_list*
wp_parse_input_file(FILE *fs, int *ln)
{
  struct wp_list *list = malloc(sizeof(struct wp_list));
  list -> root = list;
  list -> next = NULL;
  list -> prev = NULL;
  list -> point = NULL;
  (*ln) = 0;
  size_t l_size = BUFSIZ;
  char *line = malloc(l_size);  
  while (getline(&line, &l_size, fs) != EOF) {
    (*ln)++;
    int status = 0;
    struct watch_point *p = parse_input_wp_line(line, &status);
    if (p == NULL) {
      if (status < 0) {
	return NULL;
      }
      else {
	continue;
      }
    }   
    if (add_wp_to_list(list, p) < 0)
      return NULL;    
  }
  return list;
}

struct data_list*
parse_input_file (FILE* fs, int *ln, char *t)
{
  (*ln) = 0;
  struct data_list *prev, *ret = NULL;
  prev = NULL;
  size_t l_size = BUFSIZ;
  char *line = malloc(l_size);
  char *s = line;
  while (getline(&line, &l_size, fs) != EOF){
    (*ln)++;
    while (isspace(*line)) {
      line++;    
    }
    if ((*line) == '#')
      continue;
    char *i_buf = malloc(BUFSIZ);
    int i_pos = 0;
    char *data = malloc(BUFSIZ);
    int size = 0;
    while(!isspace(*line)) {      
      i_buf[i_pos] = (*line);
      if ((*line) == ']') {
	line++;
	break;
      }
      line++;
      i_pos++;
    }
    while (isspace(*line)) {
      line++;
    }
    if (strcmp(i_buf, "[double]") == 0) {
      (*t) = 'g';
      if (sscanf(line, "%g", (double*)data) != 1)
	return NULL;
      size = sizeof(double);
    }
    else if (strcmp (i_buf, "[char]") == 0) {
      (*t) = 'c';
      if (sscanf(line, "%c", data) != 1)
	return NULL;
      size = sizeof(char);
    }
    else if (strcmp (i_buf, "[long]") == 0) {
      (*t) = 'l';
      if (sscanf(line, "%ld", (long*)data) != 1)
	return NULL;
      size = sizeof(long);
    }
    else if (strcmp (i_buf, "[int]") == 0) {
      (*t) = 'i';
      if (sscanf(line, "%d", (int*)data) != 1)
	return NULL;
      size = sizeof(int);      
    }
    else if (strcmp (i_buf, "[string]") == 0) {
      (*t) = 's';
      while ((*line) != '\"')
	line++;
      line++;
      int temp = 0;
      while ((*line) != '\"') {
	data[temp] = (*line);
	line++;
	temp++;
      }
      data[temp + 1] = '\0';
      size = temp;
    }
    else if (strcmp (i_buf, "[raw]") == 0) {
      (*t) = 'r';
      int state = 0;
      while (state == 0 && isxdigit(*line)) {
	data[size] = getbyte(line, &state);
	line += 2;
	size++;
      }
      if (state == -1) {
	return NULL;
      }
    }
    else if (strcmp (i_buf, "[float]") == 0) {
      (*t) = 'f';
      if (sscanf(line, "%f", (float*)data) != 1)
	return NULL;
      size = sizeof(float);
    }
    else {
      return NULL;
    }
    void *t = malloc(size);
    memcpy(t, data, size);
    free(data);
    if (ret == NULL) {
      ret = malloc(sizeof(struct data_list));
      ret -> size = size;
      ret -> data = t;
      ret -> next = NULL;
      prev = ret;
    }
    else {
      prev -> next = malloc(sizeof(struct data_list));
      prev = prev -> next;
      prev -> data = t;
      prev -> size = size;
      prev -> next = NULL;      
    }
    line = s;
    memset(s, 0, BUFSIZ);
  }
  free(s);
  return ret;
}
