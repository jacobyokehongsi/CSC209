/*
 * This code is provided solely for the personal and private use of students
 * taking the CSC209H course at the University of Toronto. Copying for purposes
 * other than this use is expressly prohibited. All forms of distribution of
 * this code, including but not limited to public repositories on GitHub,
 * GitLab, Bitbucket, or any other online platform, whether as given or with
 * any changes, are expressly prohibited.
 *
 * Authors: Mustafa Quraish, Bianca Schroeder, Karen Reid
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2021 Karen Reid
 */

#include "dectree.h"

/**
 * Load the binary file, filename into a Dataset and return a pointer to 
 * the Dataset. The binary file format is as follows:
 *
 *     -   4 bytes : `N`: Number of images / labels in the file
 *     -   1 byte  : Image 1 label
 *     - NUM_PIXELS bytes : Image 1 data (WIDTHxWIDTH)
 *          ...
 *     -   1 byte  : Image N label
 *     - NUM_PIXELS bytes : Image N data (WIDTHxWIDTH)
 *
 * You can set the `sx` and `sy` values for all the images to WIDTH. 
 * Use the NUM_PIXELS and WIDTH constants defined in dectree.h
 */
Dataset *load_dataset(const char *filename) {
    // TODO: Allocate data, read image data / labels, return
    Dataset *ret = malloc(sizeof(Dataset));

    FILE *data_file;

    data_file = fopen(filename, "rb");

    fread(&ret->num_items, sizeof(int), 1, data_file);
    int n = ret->num_items;

    unsigned char labels[n];
    Image *img = malloc(sizeof(Image) * n);

    for (int i = 0; i < n; i++) {
        fread(&labels[i], sizeof(char), 1, data_file);
        img[i].sx = WIDTH;
        img[i].sy = WIDTH;

        unsigned char *tmp_img[NUM_PIXELS];
        fread(tmp_img, sizeof(char), NUM_PIXELS, data_file);
        img[i].data = *tmp_img;
    }

    fclose(data_file);
    ret->labels = labels;
    ret->images = img;

    return ret;
}

/**
 * Compute and return the Gini impurity of M images at a given pixel
 * The M images to analyze are identified by the indices array. The M
 * elements of the indices array are indices into data.
 * This is the objective function that you will use to identify the best 
 * pixel on which to split the dataset when building the decision tree.
 *
 * Note that the gini_impurity implemented here can evaluate to NAN 
 * (Not A Number) and will return that value. Your implementation of the 
 * decision trees should ensure that a pixel whose gini_impurity evaluates 
 * to NAN is not used to split the data.  (see find_best_split)
 * 
 * DO NOT CHANGE THIS FUNCTION; It is already implemented for you.
 */
double gini_impurity(Dataset *data, int M, int *indices, int pixel) {
    int a_freq[10] = {0}, a_count = 0;
    int b_freq[10] = {0}, b_count = 0;

    for (int i = 0; i < M; i++) {
        int img_idx = indices[i];

        // The pixels are always either 0 or 255, but using < 128 for generality.
        if (data->images[img_idx].data[pixel] < 128) {
            a_freq[data->labels[img_idx]]++;
            a_count++;
        } else {
            b_freq[data->labels[img_idx]]++;
            b_count++;
        }
    }

    double a_gini = 0, b_gini = 0;
    for (int i = 0; i < 10; i++) {
        double a_i = ((double)a_freq[i]) / ((double)a_count);
        double b_i = ((double)b_freq[i]) / ((double)b_count);
        a_gini += a_i * (1 - a_i);
        b_gini += b_i * (1 - b_i);
    }

    // Weighted average of gini impurity of children
    return (a_gini * a_count + b_gini * b_count) / M;
}

/**
 * Given a subset of M images and the array of their corresponding indices, 
 * find and use the last two parameters (label and freq) to store the most
 * frequent label in the set and its frequency.
 *
 * - The most frequent label (between 0 and 9) will be stored in `*label`
 * - The frequency of this label within the subset will be stored in `*freq`
 * 
 * If multiple labels have the same maximal frequency, return the smallest one.
 */
void get_most_frequent(Dataset *data, int M, int *indices, int *label, int *freq) {
    // TODO: Set the correct values and return
    *label = 0;
    *freq = 0;

    if (M <= 0) 
        return;

    char tmp[10] = {0};

    for (int i = 0; i < M; i++) {
        tmp[data->labels[indices[i]]]++;
    }

    int m = 0;
    for (int i = 0; i < 10; i++) {
        if (tmp[i] > m) {
            *label = i;
        }
    }

    *freq = tmp[*label];

    return;
}

/**
 * Given a subset of M images as defined by their indices, find and return
 * the best pixel to split the data. The best pixel is the one which
 * has the minimum Gini impurity as computed by `gini_impurity()` and 
 * is not NAN. (See handout for more information)
 * 
 * The return value will be a number between 0-783 (inclusive), representing
 *  the pixel the M images should be split based on.
 * 
 * If multiple pixels have the same minimal Gini impurity, return the smallest.
 */
int find_best_split(Dataset *data, int M, int *indices) {
    // TODO: Return the correct pixel
    double small = INFINITY;
    int pixel = 0;
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (small > gini_impurity(data, M, indices, i)) {
            small = gini_impurity(data, M, indices, i);
            pixel = i;
        }
    }

    return pixel;
}

/**
 * Create the Decision tree. In each recursive call, we consider the subset of the
 * dataset that correspond to the new node. To represent the subset, we pass 
 * an array of indices of these images in the subset of the dataset, along with 
 * its length M. Be careful to allocate this indices array for any recursive 
 * calls made, and free it when you no longer need the array. In this function,
 * you need to:
 *
 *    - Compute ratio of most frequent image in indices, do not split if the
 *      fration is greater than THRESHOLD_RATIO
 *    - Find the best pixel to split on using `find_best_split`
 *    - Split the data based on whether pixel is less than 128, allocate 
 *      arrays of indices of training images and populate them with the 
 *      subset of indices from M that correspond to which side of the split
 *      they are on
 *    - Allocate a new node, set the correct values and return
 *       - If it is a leaf node set `classification`, and both children = NULL.
 *       - Otherwise, set `pixel` and `left`/`right` nodes 
 *         (using build_subtree recursively). 
 */
DTNode *build_subtree(Dataset *data, int M, int *indices) {
    // TODO: Construct and return the tree
    DTNode *node = malloc(sizeof(DTNode));

    int label;
    int freq;
    get_most_frequent(data, M, indices, &label, &freq);

    if ((double) freq / (double) M >= THRESHOLD_RATIO) {
        //create leaf
        node->classification = label;
        node->left = NULL;
        node->right = NULL;
    } else {
        int pixel = find_best_split(data, M, indices);
        int a_count = 0;
        int b_count = 0;

        for (int i = 0; i < M; i++) {
            Image img = data->images[indices[i]];

            if (img.data[pixel] < 128) {
                a_count++;
            } else {
                b_count++;
            }
        }

        int a_indices[a_count];
        int b_indices[b_count];

        int a_tmp = 0;
        int b_tmp = 0;

        for (int i = 0; i < M; i++) {
            Image img = data->images[indices[i]];

            if (img.data[pixel] < 128) {
                a_indices[a_tmp++] = indices[i];
            } else {
                b_indices[b_tmp++] = indices[i];
            }
        }


        node->left = build_subtree(data, sizeof(a_indices) / sizeof(int), a_indices);
        node->right = build_subtree(data, sizeof(b_indices) / sizeof(int), b_indices);
        
    }
    free(node);

    return node;
}

/**
 * This is the function exposed to the user. All you should do here is set
 * up the `indices` array correctly for the entire dataset and call 
 * `build_subtree()` with the correct parameters.
 */
DTNode *build_dec_tree(Dataset *data) {
    // TODO: Set up `indices` array, call `build_subtree` and return the tree.
    // HINT: Make sure you free any data that is not needed anymore
    int n = data->num_items;
    int indices[n];

    for (int i = 0; i < n; i++) {
        indices[i] = i;
    }

    return build_subtree(data, n, indices);
}

/**
 * Given a decision tree and an image to classify, return the predicted label.
 */
int dec_tree_classify(DTNode *root, Image *img) {
    // TODO: Return the correct label
    if (root->left == NULL) {
        return root->classification;
    } else {
        if (img->data[root->pixel] < 128) {
            return dec_tree_classify(root->left, img);
        } else {
            return dec_tree_classify(root->right, img);
        }
    }
}

/**
 * This function frees the Decision tree.
 */
void free_dec_tree(DTNode *node) {
    // TODO: Free the decision tree

    return;
}

/**
 * Free all the allocated memory for the dataset
 */
void free_dataset(Dataset *data) {
    // TODO: Free dataset (Same as A1)

    return;
}
