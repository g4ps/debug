#include <stdio.h>
#include "dump.h"
#include "dump.h"
#include <stdlib.h>
#include <unistd.h>


int
main(int argc, char **argv)
{
  FILE *fs = NULL;
  char *filename = NULL;
  int c;
  int proc = -1;
  while ((c = getopt(argc, argv, "p:f:")) != EOF) {
    switch(c) {
    case 'p':
      proc = atoi(optarg);
      break;
    case 'f':
      filename = optarg;
      fs = fopen(optarg, "r");
      if (fs == NULL) {
	perror(optarg);
	exit(1);
      }
      break;
    default:
      exit(1);
    }
  }
  /* if (proc < 0) { */
  /*   fprintf(stderr, "You have to supply proc number\n"); */
  /*   exit(1); */
  /* } */
  struct data_list *dl = NULL;
  struct wp_list *wp;  
  if (fs) {
    int ln = 0;
    char c;
    //dl = parse_input_file(fs, &ln, &c);
    wp = wp_parse_input_file(fs, &ln);
    if (wp == NULL) {
      fprintf(stderr, "parsing input file %s: error at line %d\n", filename, ln);
      exit(1);
    }
  }
  struct addr_space *sp = malloc(sizeof (struct addr_space));
  if (get_addr_space(proc, sp) < 0) {
    perror(argv[0]);
    exit(1);
  }
  init_wp_list(wp, sp);
  print_wp_list(wp);
  return 0;
}
