/**
 * @file list.h
 * @brief Linked list utilities for the RTOS
 *
 * This file defines a generic doubly-linked list implementation
 * for use throughout the RTOS.
 */

 #ifndef LIST_H
 #define LIST_H
 
 #include <stdint.h>
 #include <stddef.h>
 
 /* List node structure */
 typedef struct list_node {
     struct list_node* next;     /* Next node in list */
     struct list_node* prev;     /* Previous node in list */
     void* data;                 /* Pointer to node data */
 } list_node_t;
 
 /* List structure */
 typedef struct {
     list_node_t* head;          /* First node in list */
     list_node_t* tail;          /* Last node in list */
     uint32_t count;             /* Number of nodes in list */
 } list_t;
 
 /* Comparison function type for searching */
 typedef int (*list_compare_func_t)(const void* a, const void* b);
 
 /**
  * @brief Initialize a list
  * 
  * @param list List to initialize
  * @return int 0 on success, negative error code on failure
  */
 int list_init(list_t* list);
 
 /**
  * @brief Clear a list (remove all nodes)
  * 
  * @param list List to clear
  * @param free_data Flag to free node data pointers
  * @return int 0 on success, negative error code on failure
  */
 int list_clear(list_t* list, int free_data);
 
 /**
  * @brief Get number of nodes in list
  * 
  * @param list List to query
  * @return uint32_t Number of nodes
  */
 uint32_t list_count(const list_t* list);
 
 /**
  * @brief Check if list is empty
  * 
  * @param list List to check
  * @return int 1 if empty, 0 if not empty
  */
 int list_is_empty(const list_t* list);
 
 /**
  * @brief Add node to head of list
  * 
  * @param list List to add to
  * @param data Data pointer for new node
  * @return int 0 on success, negative error code on failure
  */
 int list_prepend(list_t* list, void* data);
 
 /**
  * @brief Add node to tail of list
  * 
  * @param list List to add to
  * @param data Data pointer for new node
  * @return int 0 on success, negative error code on failure
  */
 int list_append(list_t* list, void* data);
 
 /**
  * @brief Insert node after specified node
  * 
  * @param list List to modify
  * @param node Node to insert after
  * @param data Data pointer for new node
  * @return int 0 on success, negative error code on failure
  */
 int list_insert_after(list_t* list, list_node_t* node, void* data);
 
 /**
  * @brief Insert node before specified node
  * 
  * @param list List to modify
  * @param node Node to insert before
  * @param data Data pointer for new node
  * @return int 0 on success, negative error code on failure
  */
 int list_insert_before(list_t* list, list_node_t* node, void* data);
 
 /**
  * @brief Remove node from list
  * 
  * @param list List to modify
  * @param node Node to remove
  * @param free_data Flag to free node data pointer
  * @return int 0 on success, negative error code on failure
  */
 int list_remove(list_t* list, list_node_t* node, int free_data);
 
 /**
  * @brief Remove node from head of list
  * 
  * @param list List to modify
  * @param free_data Flag to free node data pointer
  * @return void* Data pointer from removed node, NULL if list empty
  */
 void* list_remove_head(list_t* list, int free_data);
 
 /**
  * @brief Remove node from tail of list
  * 
  * @param list List to modify
  * @param free_data Flag to free node data pointer
  * @return void* Data pointer from removed node, NULL if list empty
  */
 void* list_remove_tail(list_t* list, int free_data);
 
 /**
  * @brief Find node by data pointer
  * 
  * @param list List to search
  * @param data Data pointer to find
  * @return list_node_t* Matching node, NULL if not found
  */
 list_node_t* list_find(const list_t* list, const void* data);
 
 /**
  * @brief Find node using comparison function
  * 
  * @param list List to search
  * @param key Key to compare against node data
  * @param compare Comparison function
  * @return list_node_t* Matching node, NULL if not found
  */
 list_node_t* list_find_custom(const list_t* list, const void* key,
                               list_compare_func_t compare);
 
 /**
  * @brief Sort list using comparison function
  * 
  * @param list List to sort
  * @param compare Comparison function
  * @return int 0 on success, negative error code on failure
  */
 int list_sort(list_t* list, list_compare_func_t compare);
 
 /**
  * @brief Get head node of list
  * 
  * @param list List to query
  * @return list_node_t* Head node, NULL if list empty
  */
 list_node_t* list_head(const list_t* list);
 
 /**
  * @brief Get tail node of list
  * 
  * @param list List to query
  * @return list_node_t* Tail node, NULL if list empty
  */
 list_node_t* list_tail(const list_t* list);
 
 /**
  * @brief Get node at specified index
  * 
  * @param list List to query
  * @param index Index of node to get
  * @return list_node_t* Node at index, NULL if index out of bounds
  */
 list_node_t* list_at(const list_t* list, uint32_t index);
 
 /**
  * @brief Iterate over list nodes and apply function
  * 
  * @param list List to iterate
  * @param func Function to apply to each node's data
  * @param user_data User data to pass to function
  * @return int 0 on success, negative error code on failure
  */
 int list_foreach(const list_t* list, void (*func)(void* data, void* user_data), void* user_data);
 
 #endif /* LIST_H */