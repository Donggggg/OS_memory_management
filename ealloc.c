#include "ealloc.h"

Manager manager; // 메모리 관리자 
Memory* new; // 메모리 리스트 관리용 포인터

// mmap 플래그 값
const int flag1 = PROT_READ | PROT_WRITE;
const int flag2 = MAP_ANONYMOUS | MAP_PRIVATE;

int get_memory_pool(int where, int pageNum) // 할당가능한 memory pool의 번호를 리턴
{
	if(where == ALLOC) { // alloc list에서 찾음
		for(int i = 0; i < NODENUM; i++)
			if(manager.pages[pageNum].alloc_used_list[i] == UNUSED) {
				manager.pages[pageNum].alloc_used_list[i] = USED;
				return i;
			}
	}
	else if(where == FREE) { // free list에서 찾음
		for(int i = 0; i < NODENUM; i++)
			if(manager.pages[pageNum].free_used_list[i] == UNUSED) {
				manager.pages[pageNum].free_used_list[i] = USED;
				return i;
			}
	}

	return -1;
}

void init_alloc()
{
	manager.page_num = 0;
}

void cleanup()
{
	int pageNum;
	Memory *cur, *next;

	for(pageNum = 0; pageNum < manager.page_num; pageNum++) {
		cur = manager.pages[pageNum].alloc_head;
		while(cur != NULL)
		{
			manager.pages[pageNum].alloc_used_list[cur->pool] = UNUSED;
			next = cur->next;
			cur = NULL;
			cur = next;
		}

		// free list 메모리 정리
		cur = manager.pages[pageNum].free_head;
		while(cur != NULL)
		{
			manager.pages[pageNum].free_used_list[cur->pool] = UNUSED;
			next = cur->next;
			cur = NULL;
			cur = next;
		}
	}

	manager.page_num = 0;
}

char* alloc(int size)
{
	int pageNum;
	char* mem;
	Memory* cur;

	if(size % MINALLOC != 0)
		return NULL;

	// 할당가능한 페이지 존재여부 검사
	for(pageNum = 0; pageNum < manager.page_num; pageNum++)
		if(manager.pages[pageNum].total_size >= size) {
			break;
		}

	// 할당가능한 페이지 존재하지 않으면 할당
	if(pageNum == manager.page_num) {
		manager.pages[pageNum].total_size = PAGESIZE;
		if((manager.pages[pageNum].page = mmap(0, PAGESIZE, flag1, flag2, -1, 0)) == MAP_FAILED) 
			return NULL;

		manager.pages[pageNum].alloc_head = NULL;
		manager.pages[pageNum].alloc_tail = NULL;

		for(int i = 0; i < NODENUM; i++){
			manager.pages[pageNum].alloc_used_list[i] = 0;
			manager.pages[pageNum].free_used_list[i] = 0;
		}

		// 할당되지 않은 메모리 초기화
		int pool = get_memory_pool(FREE, pageNum);
		new = &(manager.pages[pageNum].free_memory_pool[pool]);
		new->offset = 0;
		new->pool = pool;
		new->size = PAGESIZE;
		new->next = NULL;
		manager.pages[pageNum].free_head = new;
		manager.pages[pageNum].free_tail = new;

		manager.page_num++;
	}

	cur = manager.pages[pageNum].free_head;
	while(cur != NULL) 
	{
		if(cur->size >= size) { // 적절한 free를 찾으면 할당
			mem = manager.pages[pageNum].page + cur->offset;
			// alloc 메모리 관리
			int pool = get_memory_pool(ALLOC, pageNum);
			new = &(manager.pages[pageNum].alloc_memory_pool[pool]);
			new->offset = cur->offset;
			new->pool = pool;
			new->size = size;
			new->data = mem;
			new->next = NULL;
			manager.pages[pageNum].total_size -= new->size;

			if(manager.pages[pageNum].alloc_head == NULL) {
				manager.pages[pageNum].alloc_head = new;
				manager.pages[pageNum].alloc_tail = new;
			}
			else {
				manager.pages[pageNum].alloc_tail->next = new;
				manager.pages[pageNum].alloc_tail = new;
			}

			// free 메모리 관리
			cur->offset = cur->offset + size;
			cur->size = cur->size - size;

			if(cur->size == 0) { // free가 없으면
				manager.pages[pageNum].free_head = NULL;
				manager.pages[pageNum].free_tail = NULL;
				manager.pages[pageNum].free_used_list[cur->pool] = 0;
				cur = NULL;
			}

			return mem;
		}
		cur = cur->next;
	}
}

void dealloc(char* mem)
{
	int pageNum;
	Memory *cur, *cur2, *prev, *prev2;
	Memory *fore, *back, *back_prev, *fore_prev;

	for(pageNum = 0; pageNum < manager.page_num; pageNum++) {
		cur = manager.pages[pageNum].alloc_head;
		while(cur != NULL) 
		{
			if(cur->data ==  mem) { // 메모리를 찾으면 반납
				//memset(manager.page + cur->offset, 0, cur->size);
				manager.pages[pageNum].total_size += cur->size;
				fore_prev = prev2;

				// alloc 메모리가 1개인 경우 리셋 (여기서 free를 리셋을 안하고있긴함)
				if(manager.pages[pageNum].alloc_head->next == NULL) 
				{
					for(int i = 0; i < NODENUM; i++)
						manager.pages[pageNum].free_used_list[i] = UNUSED;
					int pool = get_memory_pool(FREE, pageNum);
					new = &(manager.pages[pageNum].free_memory_pool[pool]);
					new->offset = 0;
					new->pool = pool;
					new->size = PAGESIZE;
					new->next = NULL;
					manager.pages[pageNum].free_head = new;
					manager.pages[pageNum].free_tail = new;
				}
				else // 그 외의 경우 합병 케이스 탐색
				{
					cur2 = manager.pages[pageNum].free_head;
					fore = back = NULL;

					// 합병 케이스 탐색
					while(cur2 != NULL)
					{
						if(cur2->offset + cur2->size == cur->offset)
							fore = cur2;
						else if(cur2->offset == cur->offset + cur->size) {
							back = cur2;
							back_prev = prev;
						}

						prev= cur2;
						cur2 = cur2->next;
					}

					if(fore == NULL && back == NULL) { // 합병이 안되는 경우
						int pool = get_memory_pool(FREE, pageNum);
						new = &(manager.pages[pageNum].free_memory_pool[pool]);
						new->offset = cur->offset;
						new->pool = pool;
						new->size = cur->size;
						new->next = NULL;

						if(manager.pages[pageNum].free_head == NULL) {
							manager.pages[pageNum].free_head = new;
							manager.pages[pageNum].free_tail = new;
						}
						else {
							manager.pages[pageNum].free_tail->next = new;
							new = manager.pages[pageNum].free_tail;
						}
					}
					else if(fore != NULL && back == NULL) { // 앞의 free와 합병인 경우
						fore->size += cur->size; 
					}
					else if(fore == NULL && back != NULL) { // 뒤의 free와 합병인 경우
						back->size += cur->size; 
						back->offset -= cur->size;
					}
					else if(fore != NULL && back != NULL) { // 쌍방향 합병인 경우
						fore->size += cur->size + back->size; 

						// back free 리스트에서 제거
						if(back == manager.pages[pageNum].free_head) {
							manager.pages[pageNum].free_head = back->next;
						}
						else if(back == manager.pages[pageNum].free_tail)  {
							manager.pages[pageNum].free_tail = back_prev;
							manager.pages[pageNum].free_tail->next = NULL;
						}
						else {
							back_prev->next = back->next;
						}
						manager.pages[pageNum].free_used_list[back->pool] = 0;
						back = NULL;
					}
				}

				// cur을 alloc  리스트에서 제거
				if(cur == manager.pages[pageNum].alloc_head){
					manager.pages[pageNum].alloc_head = cur->next;
				}
				else if(cur == manager.pages[pageNum].alloc_tail) {
					manager.pages[pageNum].alloc_tail = fore_prev;
					manager.pages[pageNum].alloc_tail->next = NULL;
				}
				else{ 
					fore_prev->next = cur->next;
				}
				manager.pages[pageNum].alloc_used_list[cur->pool] = UNUSED;
				cur = NULL;

				return ;
			}

			prev2 = cur;
			cur = cur->next;
		}
	}
}

void print_alloc() // 디버깅용 함수(alloc 리스트 출력)
{
	int pageNum;
	Memory *cur;

	for(pageNum = 0; pageNum < manager.page_num; pageNum++) {
		cur = manager.pages[pageNum].alloc_head;
		while(cur!=NULL){
			printf("alloc>> offset:%d size:%d\n", cur->offset,cur->size);
			cur = cur->next;
		}
	}
}

void print_free() // 디버깅용 함수(free 리스트 출력)
{
	int pageNum;
	Memory *cur;

	for(pageNum = 0; pageNum < manager.page_num; pageNum++) {
		cur = manager.pages[pageNum].free_head;
		while(cur!=NULL){
			printf("free>> offset:%d size:%d\n", cur->offset,cur->size);
			cur = cur->next;
		}
	}
}
