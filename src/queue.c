Queue *newQueue(){
	Queue *new;
	new = malloc(sizeof(Queue));
	assert(new != NULL);
	new->head = NULL;
	new->tail = NULL;
	new->size = 0;
	return new;
}

Node *newNode(int x, int y){
	Node *new = malloc(sizeof(Node));
	assert(new != NULL);
	new->x = x;
	new->y = y;
	new->next = NULL;
	return new;
}


void printNode(Node *n){
	fprintf(stderr, "x: %d, y: %d --> ", n->x, n->y);
}

void printQueue(Queue *qp){
	if( qp == NULL ) return;
	Node *n = qp->head;
	while(  n != NULL ){
		printNode(n);
		n = n->next;
	}
	fprintf(stderr, "\n");
}

void enqueue(Queue *qp, int x, int y){
	
	// fprintf(stderr, "here.0\n");
	Node *new = newNode(x, y);
	if( qp->size == 0 ){
		qp->size = 1;
		qp->head = new;
		qp->tail = new;
		// fprintf(stderr, "here.1\n");
	}else{ /* qp->size > 0 */
		qp->tail->next = new;
		qp->tail = new;
		qp->size++;
		// fprintf(stderr, "here.2\n");
	}
	printQueue(qp);
}

Node *dequeue(Queue *qp){
	if( qp->size < 2) return qp->head;

	Node *n = qp->head;
	qp->head = qp->head->next;
	qp->size--;
	if( qp->size == 1) qp->tail = qp->head;

	return n;
}
