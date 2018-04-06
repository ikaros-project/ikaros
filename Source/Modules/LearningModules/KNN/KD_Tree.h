//
// File: KD_Tree.h
// Author: Alexander Kolodziej
// Created: August 2007
//

#include "IKAROS.h"

#ifndef KD_TREE_H_INCLUDED
#define KD_TREE_H_INCLUDED

#define EUCLIDIAN 0
#define MANHATTAN 1

// if this is defined, each time an instance is inserted it will
// be checked that this instance is not already in the tree.

#define CHECK_FOR_CLONES


typedef struct kd_node
{
    struct kd_node  *left_tree;
    struct kd_node  *right_tree;
    struct kd_node  *parent;

    struct kd_node  *next;

    bool            deleted;

    int             sub_tree_size;
    int             split_dimension;

    float           distance;

                    // this variable needs to be at the end since its an array
                    // of variable length
    float           dimension_class_array[1];
} KD_Node;


class KD_Tree
{
public:

    KD_Tree(int dimensions, int class_size, int distance_type);
    ~KD_Tree(void);

                // rebuilds the whole tree. returns true unless all nodes had been
                // marked for deletion, or unless the tree was empty to begin with.
    bool        RebuildTree(void);
                // inserts an instance/point/node. the dimension_array contains the coordinates
                // and the class_array contains the class values
                // returns true if insertion was successfull. returns false if no memory
                // could be allocated or if we are checking for clones and there already
                // was an instance with these coordinates in the tree.
    bool        InsertInstance(float *dimension_array, float *class_array);
                // marks an instance/point/node for deletion. this is done the next time
                // a rebuild of a sub tree that contains that node occurs
                // returns true if the instance was found, false otherwise.
    bool        DeleteInstance(float *dimension_array);
                // prints the whole tree (including nodes marked for deletion)
    void        PrintTree(void);
                // prints a table of the neighbors found the last time FindKNearestNeighbors() was called
    void        PrintKNearestNeighbors(void);
                // finds k_amount neighbors to the target. the *target is a float array containing
                // the targets coordinates.
                // returns: an array of pointers to float arrays. each of these float arrays
                // the following: the coordinates, the class values, the distance
                // each float array simply consists of the above mentioned appended, in the
                // mentioned order.
                // if the tree contained less than k_amount neighbors the last float value of those
                // remaining neighbors (that is, the distance) is set to -1.0.
    float**     FindKNearestNeighbors(float *target, int k_amount);
                // calculates the distance between two instances/points
                // each of the arguments is an array of coordinates
                // hmm, why is this public???
    float       Distance(float *instance, float *target);

                // returns the iondexes to the closest or farthest neighbor from the
                // last call to FindKNearestNeighbors()
    int         GetIndexToNearestNeighbor(void);
    int         GetIndexToFarthestNeighbor(void);

                // if auto_rebuild is set to true, then when an instance is inserted
                // the tree is checked along the path of insertion to see if any sub trees
                // have become unbalanced, and if they have they are rebuilt.
    void        SetAutoRebuild(bool true_or_false);
                // see above +
                // sub trees smaller than this size are not rebuilt
    bool        SetMinimumTreeSizeForRebuild(int nodes);
                // see above +
                // sub trees are considered unbalanced when the ratio between their
                // left and righ sub trees is bigger than 1:ratio
                // note: empty left or right sub trees are considered to have size 1
    bool        SetUnbalancedTreeSizeRatioLimit(float ratio);
                // if check_for_clones is set to true, the insertion function checks to
                // see if the instance already is in the tree.
    void        SetCheckForClones(bool true_or_false);
                // verbose, you get some info...
    void        SetVerbose(bool true_or_false);

private:

    KD_Node     *root_kd_node;

    int         dimensions;
    int         class_size;
    int         distance_type;

    KD_Node*    RebuildTree(KD_Node *top_node);
    KD_Node*    MakeLinkedListFromTreeRecurse(KD_Node *top_node);
    KD_Node*    SplitLinkedList(KD_Node *head_node);
    KD_Node*    ChoosePivot(KD_Node *head_node);
    KD_Node*    BuildTreeFromLinkedListRecurse(KD_Node *head_node);
    void        RebuildIfNeeded(KD_Node *tree_node);
    void        PrintTreeRecurse(KD_Node *node, int depth, char c);
    void        FindKNearestNeighborsRecurse(KD_Node *node);
    void        FillNeighborsMatrix(void);
    int         comparisons;

    bool        InsertInstanceRecurse(KD_Node *tree_node, KD_Node *new_node);
    bool        DeleteInstanceRecurse(KD_Node *top_tree, float *dimension_array);

    bool        Equals(float *instance, float *target);

    KD_Node*    CreateNode(void);
    void        DestroyNode(KD_Node *node);
    void        DestroyTreeRecurse(KD_Node *top_node);

    KD_Node     **neighbors_list;
    float       **neighbors_matrix;
    float       *hyperplane_min;
    float       *hyperplane_max;
    float       *hyperplane_closest_point;
    float       *target;
    int         neighbors_list_size;
    int         neighbors_found;
    int         farthest_neighbor_so_far;
    int         k_amount;

    KD_Node     **node_list;
    float       *min_dimension_values;
    float       *max_dimension_values;
    float       *mean_dimension_values;
    int         sizeof_kd_node;
    int         node_list_size;

    int         nodes_in_tree;
    int         not_inserted_nodes;
    int         auto_split_dimension;

    bool        verbose;
    bool        check_for_clones;
    bool        auto_rebuild;
    int         minimum_tree_size_for_rebuild;
    float       unbalanced_tree_size_ratio_limit;

    int         created_nodes;
    int         destroyed_nodes;
    int         nodes_marked_for_deletion;

    // debug
    void        PrintTree(KD_Node *top_node);
    void        PrintNode(KD_Node *node);
    void        PrintLinkedList(char *title, KD_Node *head);
    void        PrintArray(char *title, float *array, int size);
    void        RecurseTree(void);
    void        RecurseTreeRecursion(KD_Node *node);
    void        CheckAllParentPointers(KD_Node *top_node);

};



#endif // KD_TREE_H_INCLUDED
