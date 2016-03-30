#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/slab.h>

// Node color 
#define RED 1
#define BLACK 0
#define DOUBLE_BLACK 2

// Node of the red-black tree
struct rb_node {
	struct rb_node *left, *right, *parent;
	int color;
}

// Initialize the rb tree
struct rb_node *rb_init(u64 runtime){
	
	struct rb_node *rb_root;
	rb_root->left = NULL;
	rb_root->right = NULL;
	rb_root->color = BLACK;
	rb_root->parent = NULL;
	// Need to replicate the vruntime? or use the value already present in vruntime field in sched_entity
	//rb_root->vruntime = runtime;		
	return rb_root;
} 

// Rotate the structure left
void rotate_left(struct rb_node *node){
	struct rb_node *child = node->right;
	
//	if(!child){
//		node->color = BLACK;
//		return;
//	}
		
	// grandparent node
	if (node == node->parent->left)
		node->parent->left = child;
	else
		node->parent->right = child;
	
	child->parent = node->parent;
	
	node->parent = child;
	node->right = child->left;
	
	if (child->left)
		child->left->parent = node;
	
	child->left = node;
}


// Rotate the structure right
void rotate_right(struct rb_node *node){
	struct rb_node *child = node->left;
	
	// grandparent node
	if (node == node->parent->left)
		node->parent->left = child;
	else
		node->parent->right = child;
	
	child->parent = node->parent;
	
	node->parent = child;
	node->left = child->right;
	
	if (child->right)
		child->right->parent = node;
	
	child->right = node;
}

// Inserting the new rb_node in the tree
void insert_node(struct rb_node *parent, struct rb_node *node, int left){
		
	if(!node){
		printk(KERN_ERR "Error seting up the rb_node\n");
		return;
	}
	
	node->left = NULL;
	node->right = NULL;
	
	if(!parent){
		node->color = BLACK;
		return;
	}

	node->color = RED;
	node->parent = parent;

	if (left == 1)	
		parent->left = node;
	else
		parent->right = node;

	// Check if the parent is painted BLACK, iy yes, then the tree is already balanced
	if(node->parent->color == BLACK)
		return;
	
	// In not, rearrange/repaint the rb-tree so that all the properties of the tree are intact
	while(node->parent->color == RED){
		struct rb_node *grandparent = node->parent->parent;
		struct rb_node *uncle;
		
		if(!grandparent)
			uncle = NULL;
		else
			// Get the uncle node
			uncle = (node->parent == grandparent->left)?grandparent->right:grandparent->left;
		
		if(uncle && uncle->color == RED){
			// Repaint parent and uncle node to BLACK, and grandparent node to RED
			node->parent->color = BLACK;
			uncle->color = BLACK;
			grandparent->color = RED;
			
			// Restart the process
			node = grandparent;
			if(!node){
				node->color = BLACK;
				break;
			}
			continue;
		}
		// If uncle node is BLACK 	
		else if ((node == node->parent->right) && (node->parent == grandparent->left)){
			rotate_left(node->parent);
			node = node->left;
		}
		else if ((node == node->parent->left) && (node->parent == grandparent->right)){
			rotate_right(node->parent);
			node = node->right;
		}

		node->parent->color = BLACK;
		grandparent = node->parent->parent;
		grandparent->color = RED;
		
		if(node == node->parent->left)
			rotate_right(grandparent);
		else
			rotate_left(grandparent);
	}
	
	return;

}

// Considers the deletion of only the leftmost node in the tree
void delete_node(struct rb_node *node){
	
	if (!node->parent)
		return;
	else if(node->parent->color == RED || (node->color == RED && !node->parent->right)){
		node->parent->left = NULL;
		// Irrespective of the color, be it RED or BLACK, rotate will not affect
		if(node->parent->right)
			rotate_left(node->parent);
	}
	else{
		node->parent->left = NULL;
		while(1){
			// Deleted node and parent nodes are black
			printk(KERN_ALERT "Handle the double black situation\n"):
			struct rb_node *sibling = node->parent->right;
			// sibling node is BLACK
			if(sibling->color == BLACK) {
			// left left case
				if((sibling->right && sibling->right->color == RED) || (sibling->left && sibling->left->color == RED && sibling->right && sibling->right->color == RED)){
					rotate_left(node->parent);
					sibling->right->color = BLACK;
				}
				else if (sibling->left && sibling->left->color == RED) {
					rotate_right(sibling);
					rotate_left(sibling->parent->parent);
					sibling->parent->color = BLACK;
				}
				else if((!sibling->left || sibling->left->color == BLACK) && (!sibling->right || sibling->right->color == BLACK)){
					sibling->color = RED;
					node = node->parent;
					continue;
				}
				//else if(!sibling->left && !sibling->right){
				//	rotate_left(sibling->parent);
				//	sibling->color = RED;
				//}
			}
			// sibling RED
			else{
				rotate_left(node->parent);
				if(!sibling->left || !sibling->right){
					node->parent->color = RED;
					sibling->color = BLACK;
				}
				else{
					node->parent->color = BLACK;
					if(node->parent->right)
						node->parent->right->color = RED;
				}
			}
			break;
		}
	}
	node->parent = NULL;
}


// Deleting the node from the rb-tree
