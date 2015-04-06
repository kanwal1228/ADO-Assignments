/*
 * buffer_mgr.c
 *
 *  Created on: Mar 22, 2015
 *      Author: kanwal
 */
#include "buffer_mgr.h"
#include <stdlib.h>
#include <stdio.h>
#include "dt.h"
#include "storage_mgr.h"
#include <string.h>

#define DUPFILE "duplicate_file.bin"

typedef struct List_ele{
	char *data;
	struct List_ele *next;
	struct List_ele *previous;
	bool dirty;
	int fixCount;
	int pageNum;
	int index;
	int counter;

}List_ele;

typedef struct List {

	int elements;											// to initialize the no. of pages
	List_ele *first;										// pointers to traverse the linked list
	List_ele *last;
	List_ele *curPos;
	int readCount;
	int writeCount;
	int insertAt;

}List;

//to display list of elements

void display_List(List *list)
{
	List_ele *next = NULL;

	next = list->first;
	while(next)
	{
		printf("Data: %s- Fix Count: %d- Dirty: %d- PageNum: %d\n",next->data,next->fixCount,next->dirty,next->pageNum);
		next = next->next;
	}
}

List_ele* search_IndexRec(List *list,int index)
{
	List_ele *next = NULL;

	next = list->first;
	while(next)
	{
		if(next->index == index)
		{
			return next;
		}
		next = next->next;
	}
	return NULL;
}

int insertAllowed(List *list,int index,int MAXBUFF)
{
	List_ele *element = NULL;
	int counter = 0;

	for(counter = 0;counter<MAXBUFF;counter++)
	{
		element = search_IndexRec(list,index);

		if(element->fixCount != 0)
		{
			index++;
			if(index>=MAXBUFF)
			{
				index = 0;
			}
		}
		else
		{
			return index;
		}
	}

	return -1;
}

int getLRUPosIns(List *list)
{
	List_ele *element = NULL;
	int index = -1;
	int counterVal = -1;

	element = list->first;
	counterVal = element->counter;
	index = element->index;

	while(element)
	{
		element = element->next;
		if(element && (element->counter > counterVal))
		{
			counterVal = element->counter;
			index = element->index;
		}
	}
	return index;
}

//this method writes the dirty record to the disk
RC updateDTrecord(BM_BufferPool *const bm)
{
	SM_FileHandle fp_orig; //,fp_dup;
	SM_PageHandle ph_orig = NULL;
	List_ele *del_ele = NULL;
	List *list = NULL;
	list = bm->mgmtData;

	//open original file
	ph_orig = (SM_PageHandle ) malloc(sizeof(SM_PageHandle)*4096);
	openPageFile(bm->pageFile,&fp_orig);

	del_ele = search_IndexRec(list,list->insertAt);
	if(del_ele)
	{
		if(writeBlock(del_ele->pageNum, &fp_orig,del_ele->data) != RC_OK)
		{
			return RC_WRITE_FAILED;
		}
		else
		{
			del_ele->dirty = FALSE;
		}
	}
	else
	{
		return RC_INVALID_LIST;
	}

	//close the original file
	if(closePageFile (&fp_orig) != RC_OK)
	{
		return RC_FILE_NOT_CLOSED;
	}

	free(ph_orig);
	list->writeCount++;
	return RC_OK;

}

//inserting the element in the list
void push(List *const list,List_ele *element,int MAX_BUFF)
{
	List_ele *strt_Ptr = NULL;
	strt_Ptr = list->first;

	if(list->elements < MAX_BUFF)
	{
		while(strt_Ptr != NULL)
		{
			if(strt_Ptr->index == list->insertAt)
			{
				memcpy(strt_Ptr->data,element->data,sizeof(char *)*PAGE_SIZE);
				strt_Ptr->dirty = element->dirty;
				strt_Ptr->fixCount = element->fixCount;
				strt_Ptr->pageNum = element->pageNum;
				list->elements++;
				list->insertAt = strt_Ptr->index + 1;
				if(list->insertAt >= MAX_BUFF)
				{
					list->insertAt = 0;
				}
				list->curPos = strt_Ptr;
				break;
			}
			strt_Ptr = strt_Ptr->next;
		}
	}
	else
	{
		list->elements--;
		push(list,element,MAX_BUFF);
	}

}

void updateLRUCounter(List *list,int index)
{
	List_ele *strt_Ptr = NULL;
	strt_Ptr = list->first;
	while(strt_Ptr)
	{
		if(strt_Ptr->pageNum != NO_PAGE && strt_Ptr->index != index)
		{
			strt_Ptr->counter = strt_Ptr->counter + 1;
		}

		strt_Ptr = strt_Ptr->next;
	}

}

//inserting elements based on LRU strategy
void push_LRU(List *const list,List_ele *element,int MAX_BUFF)
{
	List_ele *strt_Ptr = NULL;
	strt_Ptr = list->first;

	if(list->elements < MAX_BUFF)
	{
		while(strt_Ptr != NULL)
		{
			if(strt_Ptr->index == list->insertAt)
			{
				memcpy(strt_Ptr->data,element->data,sizeof(char *)*PAGE_SIZE);
				strt_Ptr->dirty = element->dirty;
				strt_Ptr->fixCount = element->fixCount;
				strt_Ptr->pageNum = element->pageNum;
				strt_Ptr->counter = 0;
				list->elements++;
				//update counter of other elements
				updateLRUCounter(list,strt_Ptr->index);
				list->insertAt = strt_Ptr->index + 1;
				if(list->insertAt >= MAX_BUFF)
				{
					list->insertAt = 0;
				}
				list->curPos = strt_Ptr;
				break;
			}
			strt_Ptr = strt_Ptr->next;
		}
	}
	else
	{
		list->elements--;
		push_LRU(list,element,MAX_BUFF);
	}

}


RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
		  const int numPages, ReplacementStrategy strategy,
		  void *stratData)
{
	int i = 0;
	List_ele *element = NULL;
	List *list = NULL;

	if(numPages)
	{
		element = (List_ele *) malloc ( sizeof(List_ele));										//initializing the first element in the list
		element->next = NULL;
		element->previous = NULL;
		element->data = malloc(sizeof(char *)*PAGE_SIZE);
		element->dirty = FALSE;
		element->fixCount = 0;
		element->pageNum = NO_PAGE;																	//set deffault pagenum to -1 to indicate no page
		element->index = 0;
		element->counter = -1;

		list = (List *) malloc ( sizeof(List));
		list->first = element;																		//adding the element to the list
		list->last = element;
		list->readCount = 0;
		list->writeCount = 0;
	}

	for(i = 1; i<numPages; i++)
	{
		element = (List_ele *) malloc ( sizeof(List_ele));										//adding other elements to the list
		element->next = NULL;
		element->previous = list->last;
		element->data = malloc(sizeof(char *)*PAGE_SIZE);
		element->dirty = FALSE;
		element->fixCount = 0;
		element->pageNum = NO_PAGE;																//set default pagenum to -1 to indicate no page
		element->index = i;
		element->counter = -1;

		list->last->next = element;																	//updating the previous element's next pointer
		list->last = element;																		//updating the list pointer to the newly added element

	}

	list->elements = 0;
	list->insertAt = 0;
	bm->mgmtData = list;
	bm->numPages = numPages;
	bm->pageFile = malloc(sizeof(char*)*strlen(pageFileName));
	memcpy(bm->pageFile,pageFileName,sizeof(char*)*strlen(pageFileName));
	//bm->pageFile = pageFileName;
	bm->strategy = strategy;
	return RC_OK;
}

RC shutdownBufferPool(BM_BufferPool *const bm)
{
	List *list = NULL;
	List_ele *ele_Ptr = NULL;
	list = bm->mgmtData;
	//ele_Ptr = list->first;
	int index = list->insertAt;
	RC returnCode;
	int i;

	for(i=0;i<bm->numPages;i++)													//Check for any dirty pages
	{
		if(++index>bm->numPages-1)
					index=0;
		ele_Ptr = search_IndexRec(list,index);


		if(ele_Ptr->dirty)
		{
			if(ele_Ptr->fixCount == 0)
			{
				//write the page to the disk
				list->insertAt=index;
				returnCode = updateDTrecord(bm);

				//check for any error
				if(returnCode != RC_OK)
				{
					//return error in case of any error
					return returnCode;
				}
			}
			else
			{
				return RC_FIXCOUNT_NONZERO;
			}
		}
	}


	//After writing dirty pages to the disk if any
	//Begin to deallocate the memory occupied by the list
	//Set the pointer to the start of the list

	ele_Ptr = list->first;

	while(ele_Ptr)
	{
		list->first = ele_Ptr->next;

		free(ele_Ptr);

		ele_Ptr = list->first;
	}
	free(list);

	return RC_OK;

}

RC forceFlushPool(BM_BufferPool *const bm)
{
	List *list = NULL;
	List_ele *ele_Ptr = NULL;
	list = bm->mgmtData;
	ele_Ptr = list->first;
	RC returnCode;
	//BM_PageHandle *page;

	while(ele_Ptr)															//Check for any dirty pages
	{
		if(ele_Ptr->dirty)
		{
			if(ele_Ptr->fixCount == 0)
			{
				//write the page to the disk
				returnCode = updateDTrecord(bm);

				//check for any error
				if(returnCode != RC_OK)
				{
					//return error in case of any error
					return returnCode;
				}
				ele_Ptr->dirty = FALSE;
			}
		}

		ele_Ptr = ele_Ptr->next;
	}

	return RC_OK;
}


//search the given node in the list

List_ele* search_element(List *list ,int pageNum)
{
	List_ele *ptr = NULL;

	ptr = list->first;

	while(ptr)
	{
		if(ptr->pageNum == pageNum)
		{
			return ptr;
		}

		ptr = ptr->next;
	}

	//return null in case no page is found
	return NULL;
}

List_ele* fetchPage(BM_BufferPool *const bm, PageNumber pageNum)
{
	SM_FileHandle fh;
	SM_PageHandle memPage = NULL;

	List *list = NULL;
	List_ele *ele_Ptr = NULL;

	if(openPageFile (bm->pageFile, &fh) == RC_OK)
	{
		memPage = (SM_PageHandle) malloc(PAGE_SIZE*(sizeof(char*)));
		if(readBlock (pageNum, &fh, memPage) == RC_OK)
		{
			ele_Ptr = (List_ele *) malloc(sizeof(List_ele));
			ele_Ptr->next = NULL;
			ele_Ptr->previous = NULL;
			ele_Ptr->data = memPage;
			ele_Ptr->dirty = FALSE;
			ele_Ptr->fixCount = 1;
			ele_Ptr->pageNum = pageNum;
			list = bm->mgmtData;
			list->readCount = list->readCount + 1;
			closePageFile(&fh);
			return ele_Ptr;
		}
		else
		{
			//close the file
			closePageFile(&fh);
		}
	}

	return NULL;
}

// Buffer Manager Interface Access Pages
//this record marks the record given in the page handle as dirty in the linked list

RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	//search for the record to marked dirty
	List *list = NULL;
	list = bm->mgmtData;
	List_ele *ele_Ptr = NULL;

	//check whether list contains the element or not
	ele_Ptr = search_element(list ,page->pageNum);
	if(ele_Ptr == NULL)
	{
		return RC_INVALID_LIST;
	}

	ele_Ptr->dirty = TRUE;
	ele_Ptr = NULL;
	return RC_OK;

}

//this method reduces the fix count for the given page;
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	//search for the record to marked dirty
	List *list = NULL;
	list = bm->mgmtData;
	List_ele *ele_Ptr = NULL;
	//check whether list contains the element or not

	ele_Ptr = search_element(list ,page->pageNum);
	if(ele_Ptr == NULL)
	{
		return RC_INVALID_LIST;
	}

	ele_Ptr->fixCount = ele_Ptr->fixCount - 1;
	ele_Ptr = NULL;
	return RC_OK;
}
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const index)
{
	List *list = NULL;
	list = bm->mgmtData;
	List_ele *ele_Ptr = NULL;

	//check whether list contains the element or not
	ele_Ptr = search_element(list,index->pageNum);

	if(ele_Ptr == NULL)
	{
		return RC_INVALID_PAGE;
	}

	list->writeCount++;
	list->insertAt = index->pageNum;
	return updateDTrecord(bm);

}

RC FIFO(BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum)
{
	List *list = NULL;
	list = bm->mgmtData;
	List_ele *ele_Ptr = NULL;
	List_ele *new_Element = NULL;
	//List_ele *free_Mem = NULL;
	List_ele *last_element = NULL;
	RC returnCode;
	SM_FileHandle *fHandle = NULL;
	int insertAt = 0;

	//check whether list contains the element or not
	ele_Ptr = search_element(list,pageNum);
	if(ele_Ptr == NULL)
	{
		//in case element does not exists in list
		//get page from the disk
		new_Element = fetchPage(bm,pageNum);

		if(new_Element == NULL)
		{
			//in case the page does not exists on the disk
			//return RC_READ_NON_EXISTING_PAGE;
			//because of the test case we will have to update the disk file with missing block
			//putting an empty block
			//open the file
			fHandle = (SM_FileHandle *)malloc(sizeof(fHandle));

			if(openPageFile (bm->pageFile, fHandle) == RC_OK )
			{
				//successfully opened the file now write the data to file
				if(appendEmptyBlock(fHandle) == RC_OK)
				{
					closePageFile(fHandle);
					new_Element = fetchPage(bm,pageNum);
					list->readCount = list->readCount - 1;

				}
				else
				{
					closePageFile(fHandle);
				}

			}
		}

		// check whether element fetched from the disk can be inserted into the list or not
		// check list capacity

		if(list->elements >= bm->numPages)
		{
			// in case the list is full remove the element from the list
			//FIFO or LRU POP remains the same
			//check for fix count and dirty records
			//remove element to insert the new record

			insertAt = insertAllowed(list,list->insertAt,bm->numPages);

			if(insertAt == -1)
			{
				return RC_CLIENTS_CONNECTED;
			}

			last_element = search_IndexRec(list,insertAt);
			if(last_element->dirty == TRUE )
			{
				returnCode = updateDTrecord(bm);
				//list->writeCount++;
			}
			else
			{
				returnCode = RC_OK;
			}

			if(returnCode == RC_OK)
			{
				//in case record is written successfully,increase write count
				//insert the new record to the page
				list->insertAt = insertAt;
				push(list,new_Element,bm->numPages);
				//deallocate the memory not required
				page->data = list->curPos->data;
				page->pageNum = list->curPos->pageNum;
				free(new_Element);

			}
			else
			{
				//in case of any error throw the same
				return returnCode;
			}

		}
		else
		{
			//in case the list is empty insert the element into the list
			//FIFO or LRU push remains the same
			push(list, new_Element,bm->numPages);
			page->data = list->curPos->data;
			page->pageNum = list->curPos->pageNum;
		}

	}
	else
	{
		ele_Ptr->fixCount = ele_Ptr->fixCount + 1;
		page->data = ele_Ptr->data;
		page->pageNum = ele_Ptr->pageNum;
		list->insertAt = ele_Ptr->index + 1;
	}

	return RC_OK;
}

RC LRU(BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum)
{
	List *list = NULL;
	list = bm->mgmtData;
	List_ele *ele_Ptr = NULL;
	List_ele *new_Element = NULL;
	SM_FileHandle *fHandle = NULL;
	int insertAt = 0;

	//check whether list contains the element or not
	ele_Ptr = search_element(list,pageNum);

	if(ele_Ptr == NULL)
	{
		//in case element does not exists in list
		//get page from the disk
		new_Element = fetchPage(bm,pageNum);

		if(new_Element == NULL)
		{
			//in case the page does not exists on the disk
			//return RC_READ_NON_EXISTING_PAGE;
			//because of the test case we will have to update the disk file with missing block
			//putting an empty block
			//open the file
			fHandle = (SM_FileHandle *)malloc(sizeof(fHandle));

			if(openPageFile (bm->pageFile, fHandle) == RC_OK )
			{
				//successfully opened the file now write the data to file
				if(appendEmptyBlock(fHandle) == RC_OK)
				{
					closePageFile(fHandle);
					new_Element = fetchPage(bm,pageNum);
					list->readCount = list->readCount - 1;
				}
				else
				{
					closePageFile(fHandle);
				}

			}
		}
		// check whether element fetched from the disk can be inserted into the list or not
		// check list capacity

		if(list->elements >= bm->numPages)
		{
			// in case the list is full remove the element from the list
			//check for fix count and dirty records
			//remove element to insert the new record

			insertAt = getLRUPosIns(list);

			if(insertAt != -1)
			{
				//in case record is written successfully,increase write count
				//insert the new record to the page
				list->insertAt = insertAt;
				push_LRU(list,new_Element,bm->numPages);
				//deallocate the memory not required
				page->data = list->curPos->data;
				page->pageNum = list->curPos->pageNum;
				free(new_Element);
			}
			else
			{
				//in case of any error throw the same
				return RC_INVALID_INSERT_POS;
			}

		}
		else
		{
			//in case the list is empty insert the element into the list
			//FIFO or LRU push remains the same
			push_LRU(list,new_Element,bm->numPages);
			page->data = list->curPos->data;
			page->pageNum = list->curPos->pageNum;
		}

	}
	else
	{
		ele_Ptr->fixCount = ele_Ptr->fixCount + 1;
		page->data = ele_Ptr->data;
		page->pageNum = ele_Ptr->pageNum;
		//list->insertAt = ele_Ptr->index + 1;
		ele_Ptr->counter = 0;
		updateLRUCounter(list,ele_Ptr->index);
		list->insertAt = getLRUPosIns(list);
		printf("Write at : %i\n",list->insertAt);
	}

	return RC_OK;
}

RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,
	    const PageNumber pageNum)
{
	RC returnCode;
	switch (bm->strategy)
	{
		case RS_FIFO :	returnCode = FIFO(bm,page,pageNum);
						break;

		case RS_LRU :	returnCode = LRU(bm,page,pageNum);
						break;
	case RS_CLOCK:
		break;
	case RS_LFU:
		break;
	case RS_LRU_K:
		break;
	default : printf("Invalid replacement strategy\n");
	return RC_FILE_NOT_FOUND;
	}

	return returnCode;

}

// Statistics Interface

//returns an array of pageNumber values for the given frames

PageNumber *getFrameContents (BM_BufferPool *const bm)
{
	List_ele *element = NULL;
	List *list;
	PageNumber *frameContent,frameCount;
	int i;

	list = bm->mgmtData;
	element = list->first;
	frameCount = bm->numPages;

	frameContent = malloc(frameCount* sizeof(int*));

	for(i= 0;i<frameCount;i++)
	{
		if(element->pageNum != NO_PAGE)
		{
			*(frameContent+i) = element->pageNum;
		}
		else
		{
			*(frameContent+i) = NO_PAGE;
		}

		element = element->next;
	}

	return frameContent;

}

//returns an array of boolean values for the given page frames

bool *getDirtyFlags (BM_BufferPool *const bm)
{
	List_ele *element = NULL;
	List *list;
	PageNumber frameCount;
	int i;
	bool *dirtyFlag;

	list = bm->mgmtData;
	element = list->first;
	frameCount = bm->numPages;

	dirtyFlag = malloc(frameCount* sizeof(bool *));

	for(i= 0;i<frameCount;i++)
	{
		*(dirtyFlag+i) = element->dirty;
		element = element->next;
	}

	return dirtyFlag;

}

//returns an array of fix counts for the given page frames

int *getFixCounts (BM_BufferPool *const bm)
{
	List_ele *element = NULL;
	List *list = NULL;
	PageNumber frameCount;
	int i;
	int *fixCount;

	list = bm->mgmtData;
	element = list->first;
	frameCount = bm->numPages;

	fixCount = malloc(frameCount* sizeof(int *));

	for(i= 0;i<frameCount;i++)
	{
		*(fixCount+i) = element->fixCount;
		element = element->next;
	}

	return fixCount;

}

//returns the no. of read operation performed on the given file
//stored in readCount variable

int getNumReadIO (BM_BufferPool *const bm)
{
	List *list = NULL;
	list = bm->mgmtData;
	return list->readCount;
}

//returns the no. of write operation performed on the given file
//stored in writeCount variable

int getNumWriteIO (BM_BufferPool *const bm)
{
	List *list = NULL;
	list = bm->mgmtData;
	return list->writeCount;
}
