#include <stdio.h>
#include <stdlib.h>
#include "knn.h"

unsigned char dataset[MAX_SIZE][NUM_PIXELS];
unsigned char labels[MAX_SIZE];

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s imagefile\n", argv[0]);
        exit(1);
    }
    printf("Loading, please wait\n");
    int res = load_dataset(argv[1], dataset, labels);
    printf("%d photos loaded.\n", res);
    for (int i = 0; i < res; i++) {
        print_image(dataset[i]);
        printf("\n%d is the label", (int)labels[i]);
    }
    return 0;
}
