#ifndef 
#define IMAGE

typedef struct Node {
    int x;
    int y;
    struct Node *next;
} Node;

typedef struct Queue {
	struct Node *head;
	struct Node *tail;
	int size;
} Queue;

Queue *newQueue();
Node *newNode();

void printNode(Node *n){
	fprintf(stderr, "x: %d, y: %d --> ", n->x, n->y);
}

void printQueue(Queue *qp){

}

void enqueue(Queue *qp, int x, int y){

}

Node *dequeue(Queue *qp){

}

#endif