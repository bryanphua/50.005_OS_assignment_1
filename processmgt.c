#define _POSIX_SOURCE

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<string.h>
#include<signal.h>


#define INELIGIBLE 0
#define READY 1
#define RUNNING 2
#define FINISHED 3
#define FAILED 4

#define MAX_LENGTH 1024
#define MAX_PARENTS 10
#define MAX_CHILDREN 10
#define MAX_NODES 50

#define DEBUG 1

typedef struct node {
  int id;
  char prog[MAX_LENGTH];
  char *args[MAX_LENGTH/2 + 1];
  int num_args;
  char input[MAX_LENGTH];
  char output[MAX_LENGTH];
  int parents[MAX_PARENTS];
  int num_parents;
  int children[MAX_CHILDREN];
  int num_children;
  int status;
  pid_t pid;
} node_t;

void print_node_info(struct node n){
	int i;

	printf("Node id: %d\n  prog - %s", n.id, n.prog);

    // io
	printf("\n   in: %s\n  out: %s", n.input, n.output);

    // arguments
	printf("\n  Arguments (%d): ", n.num_args);
	for (i=0; i<n.num_args; i++) printf("%s, ", n.args[i]);
	//for (i=0; n.args[i]!=(char*)NULL && i<MAX_LENGTH/2+1; i++) printf("%s, ", n.args[i]);

    // parents
	printf("\n  Parents (%d): ", n.num_parents);
	for (i=0; i<n.num_parents; i++) printf("%d, ", n.parents[i]);

    // children
	printf("\n  Children (%d): ", n.num_children);
	for (i=0; i<n.num_children; i++) printf("%d, ", n.children[i]);

	printf("\n\n");
}

void print_all_nodes_info(struct node* nodes, int numnodes){
	for (int i=0; i<numnodes; i++) print_node_info(nodes[i]);
}

/**
 * Search for tokens in the string s, separated by the characters in 
 * delimiters. Populate the string array at *tokens.
 *
 * Return the number of tokens parsed on success, or -1 and set errno on 
 * failure.
 */
int parse_tokens(const char *s, const char *delimiters, char ***tokens) {
  const char *s_new;
  char *t;
  int num_tokens;
  int errno_copy;

  /* Check arguments */
  if ((s == NULL) || (delimiters == NULL) || (tokens == NULL)) {
    errno = EINVAL;
    return -1;
  }

  /* Clear token array */
  *tokens = NULL;
  /* Ignore initial segment of s that only consists of delimiters */
  s_new = s + strspn(s, delimiters);

  /* Make a copy of s_new (strtok modifies string) */
  t = (char *) malloc(strlen(s_new) + 1);
  if (t == NULL) {
    return -1;    
  }
  strcpy(t, s_new);
  /* Count number of tokens */
  num_tokens = 0;
  if (strtok(t, delimiters) != NULL) {
    for (num_tokens = 1; strtok(NULL, delimiters) != NULL; num_tokens++) ;
  }

  /* Allocate memory for tokens */
  *tokens = (char**) malloc((num_tokens + 1)*sizeof(char *));
  if (*tokens == NULL) {
    errno_copy = errno;
    free(t);  // ignore errno from free
    errno = errno_copy;  // retain errno from malloc
    return -1;
  }

  /* Parse tokens */
  if (num_tokens == 0) {
    free(t);
  } else {
    strcpy(t, s_new);
    **tokens = strtok(t, delimiters);
    for (int i=1; i<num_tokens; i++) {
      *((*tokens) +i) = strtok(NULL, delimiters);      
    }
  }
  *((*tokens) + num_tokens) = NULL;  // end with null pointer

 
  return num_tokens;
}

void free_parse_tokens(char **tokens) {
  if (tokens == NULL) {
    return;    
  }
  
  if (*tokens != NULL) {
    free(*tokens);    
  }

  free(tokens);
}

/**
 * Parse the input line at line, and populate the node at node, which will
 * have id set to id.
 * 
 * Return 0 on success or -1 and set errno on failure.
 */
int parse_input_line(char *line, int id, node_t *node) {
  char **strings;  // string array
  char **arg_list;  // string array
  char **child_list;  // string array
  int a;

  /* Split the line on ":" delimiters */
  if (parse_tokens(line, ":", &strings) == -1) {
    perror("Failed to parse node information");
    return -1;
  }

  /* Parse the space-delimited argument list */
  if (parse_tokens(strings[0], " ", &arg_list) == -1) {
    perror("Failed to parse argument list");
    free_parse_tokens(strings);
    return -1;
  }

  /* Parse the space-delimited child list */
  if (parse_tokens(strings[1], " ", &child_list) == -1) {
    perror("Failed to parse child list");
    free_parse_tokens(strings);
    return -1;
  }

  /* Set node id */
  node->id = id;
  fprintf(stderr, "... id = %d\n", node->id);

  /* Set program name */
  strcpy(node->prog, arg_list[0]);
  fprintf(stderr, "... prog = %s\n", node->prog);

  /* Set program arguments */
  for (a = 0; arg_list[a] != NULL; a++) {
    node->args[a] = arg_list[a];
    node->num_args++;
    fprintf(stderr, "... arg[%d] = %s\n", a, node->args[a]);
  }
  node->args[a] = NULL;
  fprintf(stderr, "... arg[%d] = %s\n", a, node->args[a]);

  /* Set input file */
  strcpy(node->input, strings[2]);
  fprintf(stderr, "... input = %s\n", node->input);
  
  /* Set output file */
  strcpy(node->output, strings[3]);
  fprintf(stderr, "... output = %s\n", node->output);
    
  /* Set child nodes */
  node->num_children = 0;
  if (strcmp(child_list[0], "none") != 0) {
    for (int c = 0; child_list[c] != NULL; c++) {
      if (c < MAX_CHILDREN) {
        if (atoi(child_list[c]) != id) {
          node->children[c] = atoi(child_list[c]);
          fprintf(stderr, "... child[%d] = %d\n", c, node->children[c]);
          node->num_children++;
        } else {
          perror("Node cannot be a child of itself");
          return -1;
        }
      } else {
        perror("Exceeded maximum number of children per node");
        return -1;
      }
    }
  }
  fprintf(stderr, "... num_children = %d\n", node->num_children);
  //free_parse_tokens(arg_list);
  //free_parse_tokens(strings);
  //free_parse_tokens(child_list);


  return 0;
}

/**
 * Parse the file at file_name, and populate the array at n.
 * 
 * Return the number of nodes parsed on success, or -1 and set errno on
 * failure.
 */
int parse_graph_file(char *file_name, node_t *node) {
  FILE *f;
  char line[MAX_LENGTH];
  int id = 0;
  int errno_copy;

  /* Open file for reading */
  fprintf(stderr, "Opening file...\n");
  f = fopen(file_name, "r");
  if (f == NULL) {
    perror("Failed to open file");
    return -1;
  }

  /* Read file line by line */
  fprintf(stderr, "Reading file...\n");
  while (fgets(line, MAX_LENGTH, f) != NULL) {
    strtok(line, "\n");  // remove trailing newline

    /* Parse line */
    fprintf(stderr, "Parsing line %d...\n", id);
    if (parse_input_line(line, id, node) == 0) {
      node++;  // increment pointer to point to next node in array
      id++;  // increment node ID
      if (id >= MAX_NODES) {
        perror("Exceeded maximum number of nodes");
        return -1;
      }
    } else {
      perror("Failed to parse input line");
      return -1;
    }
  }

  /* Handle file reading errors and close file */
  if (ferror(f)) {
    errno_copy = errno;
    fclose(f);  // ignore errno from fclose
    errno = errno_copy;  // retain errno from fgets
    perror("Error reading file");
    return -1;
  }

  /* If no file reading errors, close file */
  if (fclose(f) == EOF) {
    perror("Error closing file");
    return -1;  // stream was not successfully closed
  }
  
  /* If no file closing errors, return number of nodes parsed */  
  return id;
}

/**
 * Parses the process tree represented by nodes and determines the parent(s)
 * of each node.
 */
int parse_node_parents(node_t *nodes, int num_nodes) {
  node_t *child;
  for(int i=0; i<num_nodes; i++) {
    for(int j=0; j<nodes[i].num_children; j++) {
      child = &(nodes[nodes[i].children[j]]);

      // set parent and increment num_parents
      child->parents[child->num_parents] = i;
      child->num_parents++;
    }
  }
  return 0;
}

/**
 * Checks the status of each node in the process tree represented by nodes and 
 * verifies whether it can progress to the next stage in the cycle:
 *
 * INELIGIBLE -> READY -> RUNNING -> FINISHED
 *
 * Returns the number of nodes that have finished running, or -1 if there was 
 * an error.
 */
int parse_node_status(node_t *nodes, int num_nodes) {
  pid_t p, pstat;
  node_t *parent;

  int pstatus;
  int finished=0;
  for(int i=0; i<num_nodes; i++) {
    //initialise default values
    nodes[i].status = INELIGIBLE;
    nodes[i].pid =0;
    //set nodes with no parents to ready and run
    if(nodes[i].num_parents==0) {
      nodes[i].status = READY;
    }
   }

    while(finished !=1){
	
	
	//run nodes that are ready
        for(int i=0; i<num_nodes; i++) {
		if (nodes[i].status == READY){
		printf("forking node %d\n",i);	
		p = fork();
      		if ( p == -1 ) { // error forking return -1
        		perror("error forking");
			return -1;
      		} else if ( p == 0 ) { //child process
			//execute command, kill parent on error
			printf("Executing this\n");
			if (strcmp(nodes[i].input,"stdin")!=0){
				freopen(nodes[i].input,"r",stdin);
			}
			if (strcmp(nodes[i].output,"stdout")!=0){
				freopen(nodes[i].output,"w",stdout);	
			}
			if(execvp(nodes[i].args[0], nodes[i].args)==-1){

				
         			perror("error exec, killing parent");
				kill(getppid(),0);
			}
      		} else {
      			nodes[i].pid = p;
			nodes[i].status = RUNNING;
			printf("waiting for node %d..........",i);
			wait(&pstatus);
			nodes[i].status = FINISHED;
			printf("FINISHED!!\n");
      		}
	}
	}
	//check running processes status update status if finished
	/*for(int i=0; i<num_nodes; i++){
		if (nodes[i].status == RUNNING){
			printf("getting pstat of node %d\n",i);
			pstat = waitpid(nodes[i].pid,&pstatus,WNOHANG);
			if (pstat == nodes[i].pid){
				nodes[i].status = FINISHED;
			}
			if (pstat = -1){
				perror("error getting pstat");
				return -1;
			}
		}
	}*/
	//check all parent nodes of ineligible nodes, update status to READY if parents are all finished
	int newstatus;
	for (int i=0; i<num_nodes; i++){
		if (nodes[i].status == INELIGIBLE){
			
			printf("checking ineligable node %d\n",i);
			newstatus = READY;
   			for (int j=0; j<nodes[i].num_parents; j++) {
        			parent = &(nodes[nodes[i].parents[j]]);
        			if (parent->status != FINISHED){
					newstatus = INELIGIBLE;
				}	
        		}
			nodes[i].status = newstatus;
		}
	}
    	//check status
	finished =1;
	for (int i =0;i<num_nodes;i++){
		if (nodes[i].status != FINISHED){
			finished =0;
		}
	}
	
  }//endwhile 
  return 0;
}

node_t *allnodes;

int print_proc_recurse(node_t root, int depth) {
    for (int j=0; j<depth; j++) printf("   ");
    if (depth) printf("|- ");
    printf("%d: %s\n", root.id, root.prog);
    for (int i=0; i<root.num_children; i++) {
        print_proc_recurse(allnodes[root.children[i]], depth+1);
    }
    return 0;
}

/**
 * Prints the process tree represented by nodes to standard error.
 *
 * Returns 0 if printed successfully.
 */
int print_process_tree(node_t *nodes, int num_nodes) {
  allnodes = nodes;
  for(int i=0; i<num_nodes; i++) {
    if(nodes[i].num_parents==0) print_proc_recurse(nodes[i], 0);
  }
  return 0;
}

/**
 * Takes in a graph file and executes the programs in parallel.
 */
int main(int argc, char *argv[]) {
  node_t nodes[MAX_NODES];
  int num_nodes;
  int num_nodes_finished;
  char* filename;	

  /* Check command line arguments */
  //accepts 1 argument -> filename of graph
  if (argc==2) {
    filename = argv[1];	
  } else {
    perror("requires one argument\n");
    return -1;
  }

  /* Parse graph file */
  fprintf(stderr, "Parsing graph file...\n");
  num_nodes = parse_graph_file(filename, nodes);

  if (num_nodes<0) {
    perror("error parsing graph file");
  } else {
    fprintf(stderr,"\nnumber of nodes = %d\n\n", num_nodes);
  }

  /* Parse nodes for parents */
  fprintf(stderr, "Parsing node parents...\n");
  if (parse_node_parents(nodes, num_nodes)!=0) {
    fprintf(stderr,"error parsing node parents");
  } 
  
  //Test: check children and parents of all nodes
  if (DEBUG) print_all_nodes_info(nodes, num_nodes);

  /* Print process tree */
  fprintf(stderr, "\nProcess tree:\n");
  if (print_process_tree(nodes, num_nodes)!=0) {
    fprintf(stderr,"error printing process tree");
  }

  /* Run processes */
  fprintf(stderr, "Running processes...\n");
 num_nodes_finished = parse_node_status(nodes, num_nodes);
  
  if (num_nodes_finished<0) {
    perror("Error executing processes");
    return EXIT_FAILURE;
  } else {
    fprintf(stderr, "All processes finished. Exiting.\n");
    return EXIT_SUCCESS;
  }
}
