#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>      
#include <sys/types.h>  
#include <sys/wait.h>  
#include <string.h>
#include "knn.h"
#include <math.h>

/*****************************************************************************/
/* Do not add anything outside the main function here. Any core logic other  */
/*   than what is described below should go in `knn.c`. You've been warned!  */
/*****************************************************************************/

/**
 * main() takes in the following command line arguments.
 *   -K <num>:  K value for kNN (default is 1)
 *   -d <distance metric>: a string for the distance function to use
 *          euclidean or cosine (or initial substring such as "eucl", or "cos")
 *   -p <num_procs>: The number of processes to use to test images
 *   -v : If this argument is provided, then print additional debugging information
 *        (You are welcome to add print statements that only print with the verbose
 *         option.  We will not be running tests with -v )
 *   training_data: A binary file containing training image / label data
 *   testing_data: A binary file containing testing image / label data
 *   (Note that the first three "option" arguments (-K <num>, -d <distance metric>,
 *   and -p <num_procs>) may appear in any order, but the two dataset files must
 *   be the last two arguments.
 * 
 * You need to do the following:
 *   - Parse the command line arguments, call `load_dataset()` appropriately.
 *   - Create the pipes to communicate to and from children
 *   - Fork and create children, close ends of pipes as needed
 *   - All child processes should call `child_handler()`, and exit after.
 *   - Parent distributes the test dataset among children by writing:
 *        (1) start_idx: The index of the image the child should start at
 *        (2)    N:      Number of images to process (starting at start_idx)
 *     Each child should get at most N = ceil(test_set_size / num_procs) images
 *      (The last child might get fewer if the numbers don't divide perfectly)
 *   - Parent waits for children to exit, reads results through pipes and keeps
 *      the total sum.
 *   - Print out (only) one integer to stdout representing the number of test 
 *      images that were correctly classified by all children.
 *   - Free all the data allocated and exit.
 *   - Handle all relevant errors, exiting as appropriate and printing error message to stderr
 */
void usage(char *name) {
    fprintf(stderr, "Usage: %s -v -K <num> -d <distance metric> -p <num_procs> training_list testing_list\n", name);
}

int main(int argc, char *argv[]) {

    int opt;
    int K = 1;             // default value for K
    char *dist_metric = "euclidean"; // default distant metric
    int num_procs = 1;     // default number of children to create
    int verbose = 0;       // if verbose is 1, print extra debugging statements
    int total_correct = 0; // Number of correct predictions

    while((opt = getopt(argc, argv, "vK:d:p:")) != -1) {
        switch(opt) {
        case 'v':
            verbose = 1;
            break;
        case 'K':
            K = atoi(optarg);
            break;
        case 'd':
            dist_metric = optarg;
            break;
        case 'p':
            num_procs = atoi(optarg);
            break;
        default:
            usage(argv[0]);
            exit(1);
        }
    }

    if(optind >= argc) {
        fprintf(stderr, "Expecting training images file and test images file\n");
        exit(1);
    } 

    char *training_file = argv[optind];
    optind++;
    char *testing_file = argv[optind];

    // TODO The following lines are included to prevent compiler warnings
    // and should be removed when you use the variables.
    // (void)K;
    // (void)dist_metric;
    // (void)num_procs;
  
    // Set which distance function to use
    /* You can use the following string comparison which will allow
     * prefix substrings to match:
     * 
     * If the condition below is true then the string matches
     * if (strncmp(dist_metric, "euclidean", strlen(dist_metric)) == 0){
     *      //found a match
     * }
     */ 
  
    // TODO
    int dist_euclid = -1; // = 1 if euclidean else = 0 if cosine
    if (strncmp(dist_metric, "euclidean", strlen(dist_metric)) == 0 ||
    strncmp(dist_metric, "eucl", 4) == 0) {
        dist_euclid = 1;
        
    } else if ((strncmp(dist_metric, "cosine", strlen(dist_metric)) == 0 ||
    strncmp(dist_metric, "cos", 3) == 0)) {
        dist_euclid = 0;
    }


    // Load data sets
    if(verbose) {
        fprintf(stderr,"- Loading datasets...\n");
    }
    
    Dataset *training = load_dataset(training_file);
    if ( training == NULL ) {
        fprintf(stderr, "The data set in %s could not be loaded\n", training_file);
        exit(1);
    }

    Dataset *testing = load_dataset(testing_file);
    if ( testing == NULL ) {
        fprintf(stderr, "The data set in %s could not be loaded\n", testing_file);
        exit(1);
    }

    // Create the pipes and child processes who will then call child_handler
    if(verbose) {
        printf("- Creating children ...\n");
    }

    // TODO
    int fds1[num_procs][2];
    int fds2[num_procs][2];
    int status;

    for (int i = 0; i < num_procs; i++) {
        if (pipe(fds1[i]) == -1) {
            perror("pipe fds1");
            exit(1);
        } 
        if (pipe(fds2[i]) == -1) {
            perror("pipe fds2");
            exit(1);
        }
        int result = fork();
        if (result < 0) {
            perror("fork");
            exit(1);
        } else if (result == 0) {
            if (close(fds1[i][1]) == -1) {
                perror("close fds1");
                exit(1);
            }
            if (close(fds2[i][0]) == -1) {
                perror("close fds2");
                exit(1);
            }

            for (int k = 0; k < i; k++) {
                if (close(fds1[k][1]) == -1) {
                    perror("close fds1");
                    exit(1);
                }
                if (close(fds2[k][0]) == -1) {
                    perror("close fds2");
                    exit(1);
                }
            }

            if (dist_euclid == 1) { // euclidean
                child_handler(training, testing, K, distance_euclidean, fds1[i][0], fds2[i][1]);
                
            } else if (dist_euclid == 0) { // cosine
                child_handler(training, testing, K, distance_cosine, fds1[i][0], fds2[i][1]);
            }
            if (close(fds1[i][0]) == -1) {
                perror("close fds1");
                exit(1);
            }
            if (close(fds2[i][1]) == -1) {
                perror("close fds2");
                exit(1);
            }
            exit(0);
        }
    }
    // Distribute the work to the children by writing their starting index and
    // the number of test images to process to their write pipe

    // TODO
    int test_set_size = testing->num_items;
    int base_num = floor(test_set_size / num_procs);
    int modulo = test_set_size % num_procs;

    int starting_index = 0;
    for (int i = 0; i < num_procs; i++) {
        if (close(fds1[i][0]) == -1) {
            perror("close fds1");
            exit(1);
        }
        if (i < modulo) {
            if (write(fds1[i][1], &starting_index, sizeof(int)) == -1) {
                perror("write fds1");
                exit(1);
            }
            int num = base_num + 1;
            if (write(fds1[i][1], &num, sizeof(int)) == -1) {
                perror("write fds1");
                exit(1);
            }
            starting_index += num;
            if (close(fds1[i][1]) == -1) {
                perror("close fds1");
                exit(1);
            }
        } else {
            if (write(fds1[i][1], &starting_index, sizeof(int)) == -1) {
                perror("write fds1");
                exit(1);
            }
            if (write(fds1[i][1], &base_num, sizeof(int)) == -1) {
                perror("write fds1");
                exit(1);
            }
            starting_index += base_num;
            if (close(fds1[i][1]) == 1) {
                perror("close fds1");
                exit(1);
            }
        }
    }


    // Wait for children to finish
    if(verbose) {
        printf("- Waiting for children...\n");
    }

    // TODO
    for (int i = 0; i < num_procs; i++) {
        if (wait(&status) == -1) {
            perror("wait");
            exit(1);
        }
    }

    // When the children have finised, read their results from their pipe
 
    // TODO
    
    for (int i = 0; i < num_procs; i++) {
        if (close(fds2[i][1]) == -1) {
            perror("close fds2");
            exit(1);
        }
        int value;
        if (read(fds2[i][0], &value, sizeof(int)) == -1) {
            perror("read fds2");
            exit(1);
        }
        total_correct += value;
        if (close(fds2[i][0]) == -1) {
            perror("close fds2");
            exit(1);
        }
    }


    if(verbose) {
        printf("Number of correct predictions: %d\n", total_correct);
    }

    // This is the only print statement that can occur outside the verbose check
    printf("%d\n", total_correct);

    // Clean up any memory, open files, or open pipes
    // free my stuff

    // TODO
    free_dataset(training);
    free_dataset(testing);


    return 0;
}
