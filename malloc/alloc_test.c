#include <stdio.h>

#include "alloc.c"
//My custom tester for alloc

void printFreeNodes() {
	printf("-----------------\n");
	printf("\tPrinting all freeNodes...\n\tfreeHead = %p\n\tfreeTail = %p\n", freeHead, freeTail);
	freeNode_t * cur = freeHead;
	size_t i = 0;
	while(cur) {
		printf("\tFreeNode %ld:\n\t\tmem = %p\n\t\tprev = %p\n\t\tnext = %p\n", i, cur->mem, cur->prev, cur->next);
		cur = cur->next;
		i++;
	}
}

void printNodes(int test, const char * msg) {
	printf("TEST%d: printing all memNodes %s...\nhead = %p\ntail = %p\n", test, msg, head, tail);
	if(head) printf("\thead->next = %p", head->next);
	if(tail) printf(";      tail->next = %p\n", tail->next);
	else puts("");
	memNode_t * cur = head;
	size_t i = 0;
	while(cur) {
		printf("Node %ld:\n\tused = %d\n\tsize = %ld\n\tusedBytes = %ld\n\tprev = %p\n\tnext = %p\n", i, cur->used, cur->size, cur->usedBytes, cur->prev, cur->next);
		cur = cur->next;
		i++;
	}
	printFreeNodes();
	printf("==============================\n");
}

void checkNull(void * ptr, const char * name) {
	if(!ptr) printf("%s was null; malloc failed!\n", name);
	else printf("%s = addr. %p\n", name, ptr);
}

int main(int argc, char * argv[]) {
	//Unless debugging, ONLY call malloc, alloc, realloc, and free!
	//Test1: Check creation, join, free
	printf("MEMNODE_SIZE = %ld = 0x%x\n", (size_t) MEMNODE_SIZE, MEMNODE_SIZE);
	int * hi = malloc(sizeof(int));
	checkNull(hi, "hi");
	int * test2 = malloc(3 * sizeof(int));
	checkNull(test2, "test2");
	int * test3 = malloc(2 * sizeof(int));
	checkNull(test3, "test3");
	printNodes(1, "BEFORE free");
	free(test2);
	printNodes(1, "AFTER free test2");
	free(hi);
	printNodes(1, "AFTER free hi");
	free(test3);
	printNodes(1, "AFTER free test3; everything should be free'd");
	//Test2: Check adding after join and frees
	int *big = malloc(5 * 1024 * 1024 * sizeof(int *));
	printNodes(2, "after malloc BIG");
	checkNull(big, "big");
	free(big);
	printNodes(2, "after free big");
	//Test3: Check split
	int * medium = malloc(30);
	checkNull(medium, "medium");
	printNodes(3, "after malloc medium");
	//Test4: realloc to grow within bounds
	medium = realloc(medium, 45);
	checkNull(medium, "medium");
	printNodes(4, "after realloc medium to 45 bytes");
	//Test5: realloc to grow out of bounds; need new mem location and copy data
	int * small = malloc(sizeof(int));
	checkNull(small, "small");
	medium = realloc(medium, 50);
	checkNull(medium, "medium");
	printNodes(5, "after alloc small and realloc medium to 50 bytes");
	//Test6: tester-2.c
	/*memNode_destroyAll();
	head = NULL;
	freeHead = NULL;
	malloc(1);

  int i;
  int **arr = malloc(5 * 1024 * 1024 * sizeof(int *));
  if (arr == NULL) {
    printf("Memory failed to allocate!\n");
    return 1;
  }
	printNodes(6, "after malloc arr (huge!)");
  for (i = 0; i < 4; i++) {
  	printf("\ti = %d\n", i);
   arr[i] = malloc(sizeof(int));
   checkNull(arr[i], "arr[i]");
    if (arr[i] == NULL) {
      printf("Memory failed to allocate!\n");
      return 1;
    }
    *(arr[i]) = i; //the problem lies with corruption.
    printNodes(6, "after small malloc AND assignment");
    printf("\t*(arr[%d]) = %d\n", i, *(arr[i]));
  }*/
  	//Test7: assign to data (easy)
  	/*int * assign = malloc(sizeof(int));
  	checkNull(assign, "assign");
  	printNodes(7, "BEFORE assign");
  	*assign = 37;
  	printNodes(7, "AFTER assign 37");
  	printf("*assign = %d\n", *assign);
  	//Test 8: assign to data (more complicated)
  	int * intArr = malloc(2 * sizeof(int));
  	checkNull(intArr, "intArr");
  	printNodes(8, "BEFORE assigning");
  	intArr[0] = 3;
  	intArr[1] = 70;
  	printNodes(8, "AFTER assigning");
  	printf("intArr[0] = %d; intArr[1] = %d\n", intArr[0], intArr[1]);
  	//Test 9: assign to pointer array AND data (HARD)
  	memNode_destroyAll();
	head = NULL;
  	int ** intPtrArr = malloc(2 * sizeof(int*));
  	checkNull(intPtrArr, "intPtrArr");
  	printNodes(9, "BEFORE malloc intPtrArr[0, 1]");
  	intPtrArr[0] = malloc(sizeof(int));
  	checkNull(intPtrArr[0], "intPtrArr[0]");
  	printNodes(9, "after malloc intPtrArr[0] but BEFORE ASSIGN");
  	*intPtrArr[0] = 74;
  	printf("\t*intPtrArr[0] = %d\n", *intPtrArr[0]);
  	printNodes(9, "AFTER assign intPtrArr[0]");
  	intPtrArr[1] = malloc(sizeof(int));
  	checkNull(intPtrArr[0], "intPtrArr[1]");
  	printNodes(9, "after malloc intPtrArr[1] but BEFORE ASSIGN");
  	*intPtrArr[1] = 274;
  	printf("\t*intPtrArr[1] = %d\n", *intPtrArr[1]);
  	printNodes(9, "AFTER assign intPtrArr[1]");*/
  	//
  	
  	//
	return 0;
}
