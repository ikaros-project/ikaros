//
// File: KD_Tree.cc
// Author: Alexander Kolodziej
// Created: August 2007
//

#include "KD_Tree.h"
#include <string.h>
#include "IKAROS.h"

using namespace ikaros;

KD_Tree::KD_Tree(int dims, int class_sze, int dist_type)
{
    dimensions = dims;
    class_size = class_sze;
    distance_type = dist_type;

    sizeof_kd_node = int(sizeof(KD_Node*) * 4 +
                    sizeof(bool) +
                    sizeof(int) * 2 +
                    sizeof(float) * 2 +
                    sizeof(float) * (dimensions + class_size));

    root_kd_node = NULL;

    not_inserted_nodes = 0;
    auto_split_dimension = 0;

    verbose = false;
    check_for_clones = false;
    auto_rebuild = true;
    minimum_tree_size_for_rebuild = 100;
    unbalanced_tree_size_ratio_limit = 2.4;

    created_nodes = 0;
    destroyed_nodes = 0;
    nodes_marked_for_deletion = 0;

    min_dimension_values = (float*) malloc(sizeof(float) * dimensions);
    max_dimension_values = (float*) malloc(sizeof(float) * dimensions);
    mean_dimension_values = (float*) malloc(sizeof(float) * dimensions);
    hyperplane_min = (float*) malloc(sizeof(float) * dimensions);
    hyperplane_max = (float*) malloc(sizeof(float) * dimensions);
    hyperplane_closest_point = (float*) malloc(sizeof(float) * dimensions);

    neighbors_list_size = 128;
    neighbors_list = (KD_Node**) malloc(sizeof(KD_Node*) * neighbors_list_size);
    if (neighbors_list == NULL)
        printf("KD_Tree.KD_Tree() - ** ERROR ** Failed to malloc neighbors_list!\n");

    neighbors_matrix = (float**) malloc(sizeof(float*) * neighbors_list_size);
    if (neighbors_matrix == NULL)
        printf("KD_Tree.KD_Tree() - ** ERROR ** Failed to malloc neighbors_matrix!\n");
    for (int i = 0; i < neighbors_list_size; i++){
        neighbors_matrix[i] = (float*) malloc(sizeof(float) * (dimensions + class_size + 1));
        if (neighbors_matrix[i] == NULL)
            printf("KD_Tree.KD_Tree() - ** ERROR ** Failed to malloc neighbors_matrix[%i]!\n", i);
    }

    neighbors_found = 0;
}


KD_Tree::~KD_Tree()
{
    float ratio;
    int left_size, right_size;

    if (root_kd_node != NULL){
        left_size = root_kd_node->left_tree != NULL ? root_kd_node->left_tree->sub_tree_size : 1;
        right_size = root_kd_node->right_tree != NULL ? root_kd_node->right_tree->sub_tree_size : 1;
        ratio = (float)left_size / (float)right_size > 1 ? (float)left_size / (float)right_size : (float)right_size / (float)left_size;
        if (verbose){
            printf("KD_Tree.~KD_Tree() - Destroying root tree: size=%i(-%i) ratio=1:%.2f\n", root_kd_node->sub_tree_size, nodes_marked_for_deletion, ratio);
        }
        DestroyTreeRecurse(root_kd_node);
    }

    if (verbose){
        printf("KD_Tree.~KD_Tree() - Created nodes were: %i\n", created_nodes);
        printf("KD_Tree.~KD_Tree() - Destroyed nodes were: %i\n", destroyed_nodes);
    }

    free(neighbors_list);
    free(hyperplane_closest_point);
    free(hyperplane_max);
    free(hyperplane_min);
    free(mean_dimension_values);
    free(max_dimension_values);
    free(min_dimension_values);
}


/** rebuild the whole tree.
    actually a wrapper function that calls RebuildTree(root),
    but you never use that outside the object. explicit rebuilds
    of sub trees are only done after insertion if we want to make
    unbalanced (sub) trees balanced.
**/
bool
KD_Tree::RebuildTree()
{
    int i;

    if (root_kd_node == NULL){
        printf("KD_Tree.RebuildTree() - ** Warning ** There are no nodes to build tree from.\n");
        return false;
    }

    for (i = 0; i < dimensions; i++){
        min_dimension_values[i] = 0;
        max_dimension_values[i] = 0;
        mean_dimension_values[i] = 0;
    }

    RebuildTree(root_kd_node);

    if (root_kd_node != NULL)
        return true;
    else
        return false;
}


/** rebuilds a (sub) tree.
**/
KD_Node*
KD_Tree::RebuildTree(KD_Node *top_node)
{
    bool this_is_left_sub_tree = false;
    KD_Node *head_node, *parent, *new_top_node;

    parent = top_node->parent;

    // figure out if this is the root node, or if not if it is
    // a left or right sub tree
    if (parent == NULL){
        if (verbose)
            printf("KD_Tree.RebuildTree(ROOT) - Rebuilding tree with %i(-%i) nodes... \n", top_node->sub_tree_size, nodes_marked_for_deletion);
    }
    else{
        if (verbose)
            printf("KD_Tree.RebuildTree() - Rebuilding tree with %i/%i(-%i) nodes... \n", top_node->sub_tree_size, root_kd_node->sub_tree_size, nodes_marked_for_deletion);
        if (top_node == parent->left_tree)
            this_is_left_sub_tree = true;
        else
            this_is_left_sub_tree = false;
    }

    // deconstruct the tree to a linked list.
    head_node = MakeLinkedListFromTreeRecurse(top_node);
    if (head_node == NULL)
        return NULL;

    // build new tree from the linked list
    new_top_node = BuildTreeFromLinkedListRecurse(head_node);
    new_top_node->parent = parent;

    // set parent pointer and root_kd_node pointer if needed
    if (parent == NULL){
        root_kd_node = new_top_node;
        if (verbose)
            printf("KD_Tree.RebuildTree(ROOT) - Done. (new size %i(-%i) nodes)\n", root_kd_node->sub_tree_size, nodes_marked_for_deletion);
    }
    else{
        if (this_is_left_sub_tree)
            parent->left_tree = new_top_node;
        else
            parent->right_tree = new_top_node;
        if (verbose)
            printf("KD_Tree.RebuildTree() - Done. (new size %i/%i(-%i) nodes)\n", new_top_node->sub_tree_size, root_kd_node->sub_tree_size, nodes_marked_for_deletion);
    }

    return new_top_node;
}


/** builds a tree from a linked list...
**/
KD_Node*
KD_Tree::BuildTreeFromLinkedListRecurse(KD_Node *head_node)
{
    KD_Node *top_node;

    // after the split, a top node is returned, with two (possibly empty)
    // linked lists linked to the left_tree/right_tree pointers
    top_node = SplitLinkedList(head_node);
    top_node->sub_tree_size = 1;

    if (top_node->left_tree != NULL){
        top_node->left_tree = BuildTreeFromLinkedListRecurse(top_node->left_tree);
        top_node->left_tree->parent = top_node;
        top_node->sub_tree_size += top_node->left_tree->sub_tree_size;
    }

    if (top_node->right_tree != NULL){
        top_node->right_tree = BuildTreeFromLinkedListRecurse(top_node->right_tree);
        top_node->right_tree->parent = top_node;
        top_node->sub_tree_size += top_node->right_tree->sub_tree_size;
    }

    return top_node;
}


/** deconstructs a (sub) tree into a linked list.
    this is where all nodes marked for deletion get
    removed.
**/
KD_Node*
KD_Tree::MakeLinkedListFromTreeRecurse(KD_Node *top_node)
{
    KD_Node *last_node_in_list, *sub_list;

    // from the beginning the top node is both the first and the last node...
    last_node_in_list = top_node;
    last_node_in_list->next = NULL;

    // the sub lists returned by MakeLinkedListRecurse() contain a pointer
    // to the last node in the first node's ->parent pointer

    // append left if not empty
    if (top_node->left_tree != NULL){
        sub_list = MakeLinkedListFromTreeRecurse(top_node->left_tree);
        if (sub_list != NULL){
            last_node_in_list->next = sub_list;
            last_node_in_list = sub_list->parent;
        }
    }

    // append right if not empty
    if (top_node->right_tree != NULL){
        sub_list = MakeLinkedListFromTreeRecurse(top_node->right_tree);
        if (sub_list != NULL){
            last_node_in_list->next = sub_list;
            last_node_in_list = sub_list->parent;
        }
    }

    // discard this top node if it was marked as deleted
    if (top_node->deleted){
        sub_list = top_node->next;
        if (sub_list != NULL){
            sub_list->parent = last_node_in_list;
            sub_list->sub_tree_size = top_node->sub_tree_size -1;
        }
        DestroyNode(top_node);
        nodes_marked_for_deletion--;
        top_node = sub_list;
    }
    else
        top_node->parent = last_node_in_list;

    return top_node;
}


/** splits a linked list around a pivot and saves the two (possibly empty)
    lists in the returned top node's left_tree/right_tree pointers.
    this is used when (re) builing a tree from a linked list.
**/
KD_Node*
KD_Tree::SplitLinkedList(KD_Node *head_node)
{
    KD_Node *i, *pivot, *left_list, *right_list, *tmp;
    float iv;

    // get a pivot.
    // NOTE: i is ok to change the source of the ChoosePivot() function
    pivot = ChoosePivot(head_node);

    // now partition around the pivot
    iv = pivot->dimension_class_array[pivot->split_dimension];
    left_list = NULL;
    right_list = NULL;
    for (i = head_node; i != NULL;){
        if (i == pivot){
            i = i->next;
            continue;
        }
        if (i->dimension_class_array[pivot->split_dimension] <= iv){
            tmp = i;
            i = i->next;
            tmp->next = left_list;
            left_list = tmp;
        }
        else{
            tmp = i;
            i = i->next;
            tmp->next = right_list;
            right_list = tmp;
        }
    }

    pivot->left_tree = left_list;
    pivot->right_tree = right_list;

    return pivot;
}


/** choose a pivot node from a linked list of nodes.
    this function first finds the dimension with the
    widest spread, then finds the node that is closest
    to the mean in that dimension.
    that split dimension, and the size of the list, is
    saved in the node (must be! think of this if you want
    to change this code).
    the linked list that this function gets is a normal
    list where the ->next pointer points to the next node,
    and the last node's ->next pointer is set to NULL.
    this function is used for SplitLinkedList().
**/
KD_Node*
KD_Tree::ChoosePivot(KD_Node *head_node)
{
    KD_Node *i, *pivot;
    //KD_Node *prev_i, *prev_pivot;
    int d, list_size, widest_spread_dimension;
    float widest_spread_value, closest_mean_value_diff, iv, wsdm;

    // init the stats. these arrays are used solely for statistics
    // in this function.
    for (d = 0; d < dimensions; d++){
        min_dimension_values[d] = maxfloat;
        max_dimension_values[d] = -maxfloat;
        mean_dimension_values[d] = 0;
    }

    // figure out min, max and mean values for the instances that
    // we are about to partition
    list_size = 0;
    for (i = head_node; i != NULL; i = i->next){
        list_size++;
        for (d = 0; d < dimensions; d++){
            if (i->dimension_class_array[d] < min_dimension_values[d])
                min_dimension_values[d] = i->dimension_class_array[d];
            else
                max_dimension_values[d] = i->dimension_class_array[d];
            mean_dimension_values[d] += i->dimension_class_array[d];
        }
    }
    for (d = 0; d < dimensions; d++)
        mean_dimension_values[d] = mean_dimension_values[d] / (float)(list_size);

    // figure out which dimension among these elements that has the widest spread
    widest_spread_dimension = 0;
    widest_spread_value = max_dimension_values[0] - min_dimension_values[0];
    for (d = 1; d < dimensions; d++)
        if (max_dimension_values[d] - min_dimension_values[d] > widest_spread_value){
            widest_spread_value = max_dimension_values[d] - min_dimension_values[d];
            widest_spread_dimension = d;
        }

    // finally find the instance that is closest to the mean of the widest range dimension
    wsdm = mean_dimension_values[widest_spread_dimension]; // wsdm = widest spread dimension mean
    pivot = head_node;
    //prev_i = head_node;
    iv = pivot->dimension_class_array[widest_spread_dimension];
    closest_mean_value_diff = (iv-wsdm) < 0 ? -(iv-wsdm) : (iv-wsdm);
    for (i = head_node->next; i != NULL; i = i->next){
        iv = i->dimension_class_array[widest_spread_dimension];
        iv = (iv-wsdm) < 0 ? -(iv-wsdm) : (iv-wsdm);
        if (iv < closest_mean_value_diff){
            pivot = i;
            //prev_pivot = prev_i;
            closest_mean_value_diff = iv;
        }
        //prev_i = i;
    }

    // save the split dimension and the size of the list in the pivot node.
    pivot->split_dimension = widest_spread_dimension;
    pivot->sub_tree_size = list_size;

    return pivot;
}


/** inserts an instance. returns true if it worked, false otherwise.
    the instance could already be there (if we check for clones), or
    memory could not be given.
**/
bool
KD_Tree::InsertInstance(float *dimension_array, float *class_array)
{
    bool inserted_ok;
    KD_Node *node = CreateNode();

    if (node == NULL)
        return false;

    // set standard initial values
    node->left_tree = NULL;
    node->right_tree = NULL;
    node->parent = NULL;
    node->deleted = false;
    node->sub_tree_size = 1;
    node->split_dimension = -1;
    node->distance = -1;
    node->sub_tree_size = 1;

    // copy the dimension and class values
    memcpy( node->dimension_class_array, dimension_array, sizeof(float) * dimensions);
    memcpy( node->dimension_class_array + dimensions, class_array, sizeof(float) * class_size);

    // insert at root if the tree is empty
    if (root_kd_node == NULL){
        root_kd_node = node;
        root_kd_node->split_dimension = auto_split_dimension;
        auto_split_dimension = (auto_split_dimension+1) % dimensions;
        return true;
    }

    inserted_ok = InsertInstanceRecurse(root_kd_node, node);

    // RebuildIfNeeded() goes down the tree the same way as the InsertInstanceRecurse()
    // did and checks if there is a sub tree that is beg enough and unbalanced enough
    // to rebuild. if there is, it calls RebuildTree() with that node, and returns.
    // InsertInstanceRecurse() leaves it's trail via the ->next ponters.
    if (auto_rebuild &&  inserted_ok)
        RebuildIfNeeded(root_kd_node);

    return inserted_ok;
}


/** inserts an instance.
**/
bool
KD_Tree::InsertInstanceRecurse(KD_Node *tree_node, KD_Node *new_node)
{
    bool inserted_ok;

    if (check_for_clones)
        if (Equals(tree_node->dimension_class_array, new_node->dimension_class_array) && (!tree_node->deleted) ){
            if (verbose){
                printf("KD_Tree.InsertInstanceRecurse() - Node already exists, skipping! "); PrintNode(new_node); printf("\n");
            }
            DestroyNode(new_node);
            return false;
        }

    // leaf node? if so, we set an arbitrary split dimension
    if (tree_node->sub_tree_size == 1){
        tree_node->split_dimension = auto_split_dimension;
        auto_split_dimension = (auto_split_dimension + 1) % dimensions;
    }

    // see if this instance belongs in the left or righ sub tree, and then
    // check if that sub tree is NULL. if its NULL, insert the instance there
    // otherwise call ourselves recursively with that sub tree.
    if (new_node->dimension_class_array[tree_node->split_dimension] <= tree_node->dimension_class_array[tree_node->split_dimension])
        if (tree_node->left_tree == NULL){
            tree_node->left_tree = new_node;
            new_node->parent = tree_node;
            tree_node->sub_tree_size++;
            tree_node->next = NULL; // for RebuildIfNeeded()
            inserted_ok = true;
        }
        else{
            inserted_ok = InsertInstanceRecurse(tree_node->left_tree, new_node);
            if (inserted_ok){
                tree_node->sub_tree_size++;
                tree_node->next = tree_node->left_tree; // for RebuildIfNeeded()
            }
            else
                tree_node->next = NULL; // for RebuildIfNeeded()
        }
    else
        if (tree_node->right_tree == NULL){
            tree_node->right_tree = new_node;
            new_node->parent = tree_node;
            tree_node->sub_tree_size++;
            tree_node->next = NULL; // for RebuildIfNeeded()
            inserted_ok = true;
        }
        else{
            inserted_ok = InsertInstanceRecurse(tree_node->right_tree, new_node);
            if (inserted_ok){
                tree_node->sub_tree_size++;
                tree_node->next = tree_node->right_tree; // for RebuildIfNeeded()
            }
            else
                tree_node->next = NULL; // for RebuildIfNeeded()
        }

    return inserted_ok;
}


bool
KD_Tree::DeleteInstance(float *instance)
{
    bool tf;

    if (root_kd_node == NULL)
        return false;

    tf = DeleteInstanceRecurse(root_kd_node, instance);

    if (tf)
        nodes_marked_for_deletion++;

    return tf;
}


bool
KD_Tree::DeleteInstanceRecurse(KD_Node *top_node, float *instance)
{
    int sd;

    if (Equals(top_node->dimension_class_array, instance)){
        top_node->deleted = true;
        return true;
    }

    sd = top_node->split_dimension;

    if (instance[sd] > top_node->dimension_class_array[sd]){
        if (top_node->right_tree != NULL)
            return DeleteInstanceRecurse(top_node->right_tree, instance);
        else
            return false;
    }

    if (top_node->left_tree != NULL)
        return DeleteInstanceRecurse(top_node->left_tree, instance);
    else
        return false;
}


/** check if a (sub) tree needs rebuilding. this depends on if the ratio between
    the left_tree and the right_tree are above a certain value, and if the sub
    tree is large enough. waste of efforts to rebuild trees with, say, 5 nodes, no?
**/
void
KD_Tree::RebuildIfNeeded(KD_Node *tree_node)
{
    float ratio, left_size, right_size;

    if (tree_node == NULL || tree_node->sub_tree_size < minimum_tree_size_for_rebuild)
        return;

    // if one sub tree is NULL, just pick a sane value of 1 for it.
    // cant very well divide by zero, can we?
    left_size = tree_node->left_tree != NULL ? tree_node->left_tree->sub_tree_size : 1;
    right_size = tree_node->right_tree != NULL ? tree_node->right_tree->sub_tree_size : 1;
    ratio = (float)left_size / (float)right_size > 1 ? (float)left_size / (float)right_size : (float)right_size / (float)left_size;

    if (ratio > unbalanced_tree_size_ratio_limit){
        RebuildTree(tree_node);
        return;
    }
    else
        RebuildIfNeeded(tree_node->next);

}


/** malloc a node.
**/
KD_Node*
KD_Tree::CreateNode()
{
    KD_Node *node = (KD_Node*) malloc(sizeof_kd_node);
    if (node == NULL){
        printf("KD_Tree.CreateNode() - ** ERROR ** Failed to get memory!");
        return NULL;
    }
    created_nodes++;
    return node;
}


/** free a node.
**/
void
KD_Tree::DestroyNode(KD_Node *node)
{
    free(node);
    destroyed_nodes++;
}


/** destroy (free) a whole (sub) tree.
**/
void
KD_Tree::DestroyTreeRecurse(KD_Node *top_node)
{
    if (top_node->left_tree != NULL)
        DestroyTreeRecurse(top_node->left_tree);

    if (top_node->right_tree != NULL)
        DestroyTreeRecurse(top_node->right_tree);

    DestroyNode(top_node);
}


/** see if two dimension arrays are equal.
**/
inline bool
KD_Tree::Equals(float *instance, float *trget)
{
    int i;

    for (i = 0; i < dimensions; i++)
        if (instance[i] != trget[i])
            return false;

    return true;
}


/** compute distance between two dimension arrays.
    uses either euclidian or manhattan type distance.
    this is decided at construction of the tree. it
    is the last parameter: EUCLIDIAN or MANHATTAN.
**/
float
KD_Tree::Distance(float *instance, float *target_)
{
    int i;
    float a, b, d = 0;

    switch (distance_type){
        case EUCLIDIAN:
            for (i = 0; i < dimensions; i++){
                a = target_[i];
                b = instance[i];
                d += (a-b) * (a-b);
            }
            d = ikaros::sqrt(d);
            break;

        case MANHATTAN:
            for (i = 0; i < dimensions; i++){
                a = target_[i];
                b = instance[i];
                d += (a-b) > 0 ? (a-b) : - (a-b);
            }
            break;
    }
    return d;
}


/** calls FindKNearestNeighborsRecurse() which finds tries to
    find k_amnt neighbors and store them in neighbors_list.
    then the dimension and class values from those nodes are
    copied to a **float (matrix) and the pointer to that is
    returned.
**/
float**
KD_Tree::FindKNearestNeighbors(float *tgt, int k_amnt)
{
    KD_Node **new_neighbors_list;
    float **new_neighbors_matrix;
    int i;

    // make the neighbors_list and neighbors_matrix larger if needed
    if (k_amnt > neighbors_list_size){
        neighbors_list_size += 128;

        // make neighbors_list larger, return NULL if fails
        new_neighbors_list = (KD_Node**) realloc(neighbors_list, sizeof(KD_Node*) * neighbors_list_size);
        if (new_neighbors_list == NULL){
            neighbors_list_size -= 128;
            printf("KD_Tree.FindKNeighbors() - ** ERROR ** Failed to reallocate neighbors_list from %i to %i !\n", neighbors_list_size, neighbors_list_size+128);
            return NULL;
        }
        neighbors_list = new_neighbors_list;

        // make neighbors_matrix larger. if fails, realloc neighbors_list
        // back to previous size and return NULL
        new_neighbors_matrix = (float**) realloc(neighbors_matrix, sizeof(float*) * neighbors_list_size);
        if (new_neighbors_matrix == NULL){
            neighbors_list_size -= 128;
            printf("KD_Tree.FindKNeighbors() - ** ERROR ** Failed to reallocate neighbors_matrix from %i to %i !\n", neighbors_list_size, neighbors_list_size+128);
            neighbors_list = (KD_Node**)realloc(neighbors_list, neighbors_list_size);
            if (neighbors_list == NULL)
                printf("KD_Tree.FindKNeighbors() - ** ERROR ** Failed to reallocate neighbors_list back to %i !\n", neighbors_list_size);
            return NULL;
        }
        neighbors_matrix = new_neighbors_matrix;

        // allocate the new sub arrays for neighbors_matrix.
        // if it fails, realloc neighbors_list and neighbors_matrix
        // back to previous size and return NULL
        for (i = neighbors_list_size-128; i < neighbors_list_size; i++){
            neighbors_matrix[i] = (float*) malloc(sizeof(float) * (dimensions + class_size + 1));
            if (neighbors_matrix[i] == NULL){
                printf("KD_Tree.KD_Tree() - ** ERROR ** Failed to malloc neighbors_matrix[%i]!\n", i);
                neighbors_list_size -= 128;

                for (i = i - 1; i > neighbors_list_size; i--)
                    free(neighbors_matrix[i]);

                neighbors_list = (KD_Node**)realloc(neighbors_list, neighbors_list_size);
                if (neighbors_list == NULL)
                    printf("KD_Tree.FindKNeighbors() - ** ERROR ** Failed to reallocate neighbors_list back to %i !\n", neighbors_list_size);

                neighbors_matrix = (float**)realloc(neighbors_matrix, neighbors_list_size);
                if (neighbors_matrix == NULL)
                    printf("KD_Tree.FindKNeighbors() - ** ERROR ** Failed to reallocate neighbors_matrix back to %i !\n", neighbors_list_size);

                return NULL;
            }
        }

        if (verbose)
            printf("KD_Tree.FindKNeighbors() - Reallocated neighbors_list/neighbors_matrix from %i to %i.\n", neighbors_list_size-128, neighbors_list_size);
        return FindKNearestNeighbors(tgt, k_amnt);
    }

    // set some standard initial values
    comparisons = 0;
    target = tgt;
    k_amount = k_amnt;
    neighbors_found = 0;
    farthest_neighbor_so_far = -1;

    // null the neighbors list...
    for (i = 0; i < neighbors_list_size; i++)
        neighbors_list[i] = NULL;

    // the hyper plane is the area where we are currently looking.
    // so set it to maximum size in the beginning.
    for (i = 0; i < dimensions; i++){
        hyperplane_min[i] = -maxfloat;
        hyperplane_max[i] = maxfloat;
    }

    // search if there are some nodes in the tree
    if (root_kd_node != NULL)
        FindKNearestNeighborsRecurse(root_kd_node);

    // fill the return matrix
    FillNeighborsMatrix();

    return neighbors_matrix;
}


/** finds k_amnt (closest) neighbors in the tree and stores
    pointers to them in the neighbors_list. if there are not
    k_amnt nodes in the tree, the remaining pointers are set
    to NULL.
**/
void
KD_Tree::FindKNearestNeighborsRecurse(KD_Node *node)
{
    KD_Node *nearer, *farther;
    int sd, i;
    float distance_to_hyperplane, tmp;
    float *hyperplane_tmp1, *hyperplane_tmp2;

    // calculate distance to this node
    node->distance = Distance(node->dimension_class_array, this->target);
    comparisons++;

    // if we havent found k neighbors yet, immediately add this node
    // to the neighbors_list.
    if ( (neighbors_found < k_amount) && (!node->deleted) ){
        neighbors_list[neighbors_found] = node;
        farthest_neighbor_so_far = neighbors_found; // not true. but set it so we can calc
                                                    // distance to some neighbor. see the last
                                                    // if statement in this function
        neighbors_found++;

        // as soon as we have found k neighbors, we need to know which of
        // them is the farthest one
        if (neighbors_found == k_amount)
            farthest_neighbor_so_far = GetIndexToFarthestNeighbor();
    }
    else
        // ok, so we have already found k neighbors. then check if this node
        // is closer to the target than the farthest neighbor so far, and
        // save it at that place if it is.
        // then find the new farthest neighbor for next save.
        if ( (node->distance < neighbors_list[farthest_neighbor_so_far]->distance) && (!node->deleted) ){
            neighbors_list[farthest_neighbor_so_far] = node;
            farthest_neighbor_so_far = GetIndexToFarthestNeighbor();
        }

    if (node->sub_tree_size == 1) // leaf node?
        return;

    sd = node->split_dimension;

    // decide which sub tree is nearer, and set the hyperplane_tmp
    // variables so that we make the hyperplane smaller from the right direction
    // so to speak.
    // if the left tree is nearer, then we should make the hyperplane smaller
    // from the max edge, etc...
    if (this->target[sd] <= node->dimension_class_array[sd]){
        nearer = node->left_tree;
        farther = node->right_tree;
        hyperplane_tmp1 = hyperplane_min;
        hyperplane_tmp2 = hyperplane_max;
    }
    else{
        nearer = node->right_tree;
        farther = node->left_tree;
        hyperplane_tmp1 = hyperplane_max;
        hyperplane_tmp2 = hyperplane_min;
    }

    // look in left tree if not empty
    if (nearer != NULL){
        tmp = hyperplane_tmp2[sd];
        hyperplane_tmp2[sd] = node->dimension_class_array[sd];
        FindKNearestNeighborsRecurse(nearer);
        hyperplane_tmp2[sd] = tmp;
    }

    // check if the farther tree is not empty. if it is not, then
    // calculate the (closest) distance to its hyperplane, and if that
    // is smaller than the farthest neighbor found so far, searh it.
    // also, if we havent found k neighbors yet, search it anyway.
    if (farther != NULL){
        tmp = hyperplane_tmp1[sd];
        hyperplane_tmp1[sd] = node->dimension_class_array[sd];

        for (i = 0; i < dimensions; i++)
            if (target[i] < hyperplane_min[i])
                hyperplane_closest_point[i] = hyperplane_min[i];
            else
                if (target[i] > hyperplane_max[i])
                    hyperplane_closest_point[i] = hyperplane_max[i];
                else
                    hyperplane_closest_point[i] = target[i];

        distance_to_hyperplane = Distance(target, hyperplane_closest_point);

        // FIRST check distance, THEN if there are k neighbors. why? in the long run this if will be true
        // much more often because of the distance than of that we havent found k neighbors yet.
        if ( (distance_to_hyperplane < neighbors_list[farthest_neighbor_so_far]->distance) || (neighbors_found < k_amount) )
            FindKNearestNeighborsRecurse(farther);

        hyperplane_tmp1[sd] = tmp;
    }

    return;
}


int
KD_Tree::GetIndexToFarthestNeighbor()
{
    int i, r;
    if (neighbors_list[0] == NULL)
        return -1;
    r = 0;
    for (i = 1; i < neighbors_found; i++)
        if (neighbors_list[i]->distance > neighbors_list[r]->distance)
            r = i;
    return r;
}


int
KD_Tree::GetIndexToNearestNeighbor()
{
    int i, r;
    if (neighbors_list[0] == NULL)
        return -1;
    r = 0;
    for (i = 1; i < neighbors_found; i++)
        if (neighbors_list[i]->distance < neighbors_list[r]->distance)
            r = i;
    return r;
}


bool
KD_Tree::SetMinimumTreeSizeForRebuild(int nodes)
{
    if (nodes < 10){
        printf("KD_Tree.SetMinimumTreeSizeForRebuild() - Cant set to less than size 10!\n");
        return false;
    }

    minimum_tree_size_for_rebuild = nodes;
    return true;
}


bool
KD_Tree::SetUnbalancedTreeSizeRatioLimit(float ratio)
{
    if (ratio < 1.5){
        printf("KD_Tree.SetUnbalancedTreeSizeRatioLimit() - Cant set to less than 1.5!\n");
        return false;
    }

    unbalanced_tree_size_ratio_limit = ratio;
    return true;
}


void
KD_Tree::SetCheckForClones(bool true_or_false)
{
    check_for_clones = true_or_false;
}


void
KD_Tree::SetAutoRebuild(bool true_or_false)
{
    auto_rebuild = true_or_false;
}


void
KD_Tree::SetVerbose(bool true_or_false)
{
    verbose = true_or_false;
}


/** fills the neighbors_matrix **float with the found neighbors.
    if k_amount neighbors were not found, the remaining distance
    values are set to -1.
**/
void
KD_Tree::FillNeighborsMatrix()
{
    int i;

    for (i = 0; i < k_amount; i++)
        if (neighbors_list[i] == NULL)
            neighbors_matrix[i][dimensions + class_size] = -1.0;
        else{
            memcpy(neighbors_matrix[i], neighbors_list[i]->dimension_class_array, sizeof(float) * (dimensions + class_size));
            neighbors_matrix[i][dimensions + class_size] = neighbors_list[i]->distance;
        }
}


void
KD_Tree::PrintKNearestNeighbors()
{
    int i;
    printf("KD_Tree.PrintKNearestNeighbors()\n");
    if (neighbors_found == 0)
        printf("  [ zero neighbors found ]\n");
    for (i = 0; i < neighbors_found; i++){
        if (neighbors_list[i] == NULL)
            printf("  [ NULL (error) ]\n");
        else{
            printf("  "); PrintNode(neighbors_list[i]); printf("\n");
        }
    }
}




/** debug functions below **/


void
KD_Tree::PrintTree()
{
    printf("KD_Tree.PrintTree(root) ----------------------\n");
    this->PrintTreeRecurse(this->root_kd_node, 0, 'T');
    printf("----------------------------------------------\n");
}


void
KD_Tree::PrintTree(KD_Node *top_node)
{
    printf("--------- SUB TREE ---------\n");
    this->PrintTreeRecurse(top_node, 0, '?');
    printf("--------------------------------\n");
}


void
KD_Tree::PrintTreeRecurse(KD_Node *node, int depth, char c)
{
    int i;

    for (i = 0; i < depth; i++)
        printf(" ");

    printf("%c", c); PrintNode(node); printf("\n");

    if (node->left_tree != NULL)
        PrintTreeRecurse(node->left_tree, depth + 3, 'L');
    if (node->right_tree != NULL)
        PrintTreeRecurse(node->right_tree, depth + 3, 'R');
}


void
KD_Tree::PrintArray(char *title, float *array, int size)
{
    int i;

    if (title != NULL)
        printf("%s: ", title);

    printf("[");
    for (i = 0; i < size-1; i++)
        printf("%.4f ", array[i]);
    printf("%.4f]", array[size-1]);
}


void
KD_Tree::PrintNode(KD_Node *node)
{
    int i;

    printf("[");
    for (i = 0; i < dimensions + class_size - 1; i++)
        printf("%.4f, ", node->dimension_class_array[i]);
    printf(" %.4f  sd=%i dist=%.4f L=%p R=%p P=%p N=%p D=%i  sub=%i]", node->dimension_class_array[dimensions+class_size-1],
        node->split_dimension, node->distance, (void *)(node->left_tree), (void *)(node->right_tree), (void *)(node->parent), (void *)(node->next), node->deleted, node->sub_tree_size);
}


void
KD_Tree::PrintLinkedList(char *title, KD_Node *head)
{
    KD_Node *i;

    if (title == NULL)
        printf("*** linked list ***\n");
    else
        printf("*** linked list: %s ***\n", title);

    for (i = head; i != NULL; i = i->next){
        printf("  %p ", (void *)i); PrintNode(i);
        printf("  p=%p  n=%p\n", (void *)(i->parent), (void *)(i->next));
    }
    printf("*******************\n");
}


void
KD_Tree::CheckAllParentPointers(KD_Node *top_node)
{
    if (top_node->left_tree != NULL){
        if (top_node->left_tree->parent != top_node)
            printf("PARENTERROR L\n");
        CheckAllParentPointers(top_node->left_tree);
    }

    if (top_node->right_tree != NULL){
        if (top_node->right_tree->parent != top_node)
            printf("PARENTERROR R\n");
        CheckAllParentPointers(top_node->right_tree);
    }
}

