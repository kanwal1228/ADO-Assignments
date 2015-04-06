#include "storage_mgr.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
void initStorageManager (void)
{

}

/* if file already exists it will return RC_OK. If file does not */
RC createPageFile (char *fileName)
{
    int i = 0;

    FILE *fp;
    char header[50];

    if( access( fileName, F_OK ) != -1 )
    {
        printf("file already exists\n");
        return RC_OK;
    }
    else
    {
    	fp = fopen(fileName, "wb");
    	strcpy(header,fileName);

    	if (fp != NULL)
    	{
			strcat(header,"-1-0");

			while(header[i]!= '\0')
			{
				i++;
			}

			fwrite( header, sizeof(char), i, fp);
			fputc('\n', fp );

			for( i=0; i < PAGE_SIZE; i++ )
				fputc( '\0', fp );
			fclose(fp);
			printf("File created\n");
			return RC_OK;
    	}
    	else
    	{
    		printf("file could not be created\n");
    		return RC_FILE_NOT_FOUND;
    	}
	}
}

RC openPageFile (char *fileName, SM_FileHandle *fHandle)
{
    FILE *fp;
    char str[50];
    int i = 0;
    int j = 0;
    int k = 0;
    char arrayptr[3][20];

    int orig_Loc = 0,fileSize = 0;

    SM_FileHandle fHandleLocal;

    if( access( fileName, F_OK ) == -1 )
    {
    	return RC_FILE_NOT_FOUND;
    }

    fp = fopen(fileName,"rb+");

    if (fp != NULL )
    {
    	fseek(fp,0,SEEK_SET);
    	fgets(str,50,fp);
    	if(str[0] != '\0')																//if file is empty
    	{
    		for(i = 0 ; i<sizeof(str); i++)
			{
				if(str[i]!='-' && str[i] != '\0')
					arrayptr[j][k++] = str[i];
				else if (str[i] == '\0') break;
				else if((str[i] == '-'))
				{
					arrayptr[j][k] = '\0';
					j++;
					k=0;
				}
			}

    		//fHandleLocal = (SM_FileHandle *) malloc(sizeof(SM_FileHandle));
    		fHandleLocal.fileName = (char *) malloc(strlen(fileName));
			memcpy(fHandleLocal.fileName,fileName,strlen(fileName));
			//memcpy(fHandleLocal.fileName,fileName,sizeof(arrayptr[0]));
			fHandleLocal.totalNumPages = atoi(arrayptr[1]);
			//get no. records from total file size
			//dividing the file size by the fixed record size
			//store the original location
			orig_Loc = ftell(fp);
			//move to the last of the file
			fseek(fp,0,SEEK_END);
			//get the file position
			fileSize = ftell(fp);
			//setting to pointer to the original position
			fseek(fp,orig_Loc,SEEK_SET);
			fHandleLocal.totalNumPages = fileSize/4096;
			fHandleLocal.curPagePos = atoi(arrayptr[2]);
			fHandleLocal.mgmtInfo = fp;

			memcpy(fHandle,&fHandleLocal,sizeof(SM_FileHandle));
			free(fHandleLocal.fileName);
			return RC_OK;
    	}
    	else
    	{
    		return RC_READ_NON_EXISTING_PAGE;
    	}

    }
    else
    	return RC_FILE_NOT_FOUND;
}

RC closePageFile (SM_FileHandle *fHandle)
{
    FILE *fp;
    if (fHandle == NULL)
    {
        printf("Please enter a valid File Handle\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    else
    {
    fp = fHandle->mgmtInfo;
    fclose(fp);
    printf("File has been Closed Successfully\n");
    return RC_OK;
    }
}


RC destroyPageFile (char *fileName)
{
	int fileRemove;

	fileRemove = remove(fileName);

	if(fileRemove == 0)
    {
		printf("File Removed Successfully\n");
		return RC_OK;
    }
	else
    {
		printf("File Could not be removed\n");
		return RC_FILE_NOT_FOUND;
    }
}


/*
 * To read a specific page pointed by the pageNum variable in a given file
 */

RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	size_t bytes_read;																	// no. of bytes read
	int errorCode;

	if((pageNum + 1) > fHandle->totalNumPages)
	{
		return RC_READ_NON_EXISTING_PAGE;
	}

	errorCode = fseek(fHandle->mgmtInfo,(pageNum - fHandle->curPagePos)*PAGE_SIZE,SEEK_CUR);					//moving the file pointer to the relative position
																												//with respect to the current position
	if(errorCode != 0)
	{
		return RC_READ_NON_EXISTING_PAGE;
	}

	bytes_read = fread(memPage,PAGE_SIZE,1,fHandle->mgmtInfo);				// retrieving the page pointed out by the pointer

	if(bytes_read != 1)
	{
		return RC_READ_NON_EXISTING_PAGE;
	}
	fHandle->curPagePos = pageNum + 1;
	return RC_OK;
}

/*
 * the curPagePos variable in the fHandle points to the current page position
 * curPagePos is the current position of pointer in the file
 * FHandle is used to handle the file
*/

int getBlockPos (SM_FileHandle *fHandle)
{
	int blockPos = 0;
	blockPos = ftell(fHandle->mgmtInfo)/PAGE_SIZE;
	return (blockPos -1);
}

/*
 * Reading the first block in the file.
 * Check for no. of blocks in file.
 * Move the pointer to the beginning of the file
 */

RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	size_t bytes_read;																	// no. of bytes read
	int errorCode = 0;

	if(fHandle->totalNumPages == 0)														// if no blocks are present in file
	{
		return RC_READ_NON_EXISTING_PAGE;
	}

	if(fHandle->curPagePos >= 0)														//if pointer is not at the beginning of the file
	{
		errorCode = fseek(fHandle->mgmtInfo,-fHandle->curPagePos*PAGE_SIZE,SEEK_CUR);					//moving the file pointer to the required position
	}


	if(errorCode != 0)
	{
		return RC_READ_NON_EXISTING_PAGE;
	}

	bytes_read = fread(memPage,PAGE_SIZE,1,fHandle->mgmtInfo);				// retrieving the page pointed out by the pointer

	if(bytes_read != 1)
	{
		return RC_READ_NON_EXISTING_PAGE;
	}
	fHandle->curPagePos = 1;
	return RC_OK;

}

/*
 * If the file pointer is at first page or there are no pages throw error
 * move pointer to the previous page if no error
 */

RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	size_t bytes_read;																	// no. of bytes read
	int errorCode;

	if(fHandle->totalNumPages == 0 || fHandle->curPagePos == 0)
	{
		return RC_READ_NON_EXISTING_PAGE;
	}

	errorCode = fseek(fHandle->mgmtInfo,(-1)*PAGE_SIZE,SEEK_CUR);					//moving the file pointer to the previous block from the current block

	if(errorCode != 0)
	{
		return RC_READ_NON_EXISTING_PAGE;
	}

	bytes_read = fread(memPage,PAGE_SIZE,1,fHandle->mgmtInfo);				// retrieving the page pointed out by the pointer

	if(bytes_read != 1)
	{
		return RC_READ_NON_EXISTING_PAGE;
	}
	return RC_OK;

}

/*
 * Reading the current block pointed by the curPagePos variable
 */

RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	size_t bytes_read;																	// no. of bytes read

	bytes_read = fread(memPage,PAGE_SIZE,1,fHandle->mgmtInfo);				// retrieving the page pointed out by the pointer

	if(bytes_read != 1)
	{
		return RC_READ_NON_EXISTING_PAGE;
	}
	fHandle->curPagePos++;
	return RC_OK;

}

/*
 * If the file pointer is at the last block of the file then throw error
 * else move pointer to the next block
 */

RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	size_t bytes_read;																	// no. of bytes read
	int errorCode;

	if(fHandle->totalNumPages == 0 || fHandle->totalNumPages < fHandle->curPagePos+1)
	{
		return RC_READ_NON_EXISTING_PAGE;
	}

	errorCode = fseek(fHandle->mgmtInfo,(fHandle->curPagePos+1)*PAGE_SIZE,SEEK_CUR);					//moving the file pointer to the required position

	if(errorCode != 0)
	{
		return RC_READ_NON_EXISTING_PAGE;
	}

	bytes_read = fread(memPage,PAGE_SIZE,1,fHandle->mgmtInfo);				// retrieving the page pointed out by the pointer

	if(bytes_read != 1)
	{
		return RC_READ_NON_EXISTING_PAGE;
	}
	fHandle->curPagePos = (fHandle->curPagePos)+2;
	return RC_OK;

}
/*
 * Moves pointer to the end of the file and then reads the last block of file
 */

RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	size_t bytes_read;																	// no. of bytes read
	int errorCode;

	if(fHandle->totalNumPages == 0)
	{
		return RC_READ_NON_EXISTING_PAGE;
	}

	errorCode = fseek(fHandle->mgmtInfo, (fHandle->totalNumPages - (fHandle->curPagePos + 2))*PAGE_SIZE,SEEK_CUR);					//moving the file pointer to the required position

	if(errorCode != 0)
	{
		return RC_READ_NON_EXISTING_PAGE;
	}

	bytes_read = fread(memPage,PAGE_SIZE,1,fHandle->mgmtInfo);				// retrieving the page pointed out by the pointer

	if(bytes_read != 1)
	{
		return RC_READ_NON_EXISTING_PAGE;
	}
	return RC_OK;
}

/*
 * This method is used write a block in the file at a given position
 */

RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	size_t bytes_write;																	// no. of bytes read
	int errorCode;

	/*
	 * Write code to check whether appropriate no. of blocks are there or not present before the
	 * writing the block at the specified position
	 */

	errorCode = fseek(fHandle->mgmtInfo,(pageNum - fHandle->curPagePos)*PAGE_SIZE,SEEK_CUR);					//moving the file pointer to the required position

	if(errorCode != 0)
	{
		return RC_WRITE_FAILED;
	}

	bytes_write = fwrite(memPage,PAGE_SIZE,1,fHandle->mgmtInfo);				// retrieving the page pointed out by the pointer

	if(bytes_write != 1)
	{
		return RC_WRITE_FAILED;
	}

	fHandle->totalNumPages++;
	fHandle->curPagePos = pageNum + 1;
	return RC_OK;

}

/*
 * This method writes the block at the current pointer location
 */

RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	size_t bytes_write;																	// no. of bytes written
	int cursorPos = getBlockPos(fHandle);
	bytes_write = fwrite(memPage,PAGE_SIZE,1,fHandle->mgmtInfo);				// retrieving the page pointed out in the file

	if(bytes_write != 1)
	{
		return RC_WRITE_FAILED;
	}

	if(fHandle->totalNumPages <= (cursorPos + 1))
	{
		fHandle->curPagePos = cursorPos++;
	}
	else
	{
		fHandle->curPagePos = cursorPos++;
		fHandle->totalNumPages++;
	}

	return RC_OK;

}


/*
 * This method writes an empty block at the end of the file.
 */
RC appendEmptyBlock (SM_FileHandle *fHandle)
{
	int i;
	int errorCode;

	if(fHandle->totalNumPages >= (fHandle->curPagePos +1))
	{
		errorCode = fseek(fHandle->mgmtInfo, (fHandle->totalNumPages - fHandle->curPagePos)*PAGE_SIZE,SEEK_CUR);					//moving the file pointer to the last of file
		if(errorCode != 0)
		{
			return RC_READ_NON_EXISTING_PAGE;

		}
	}

	for(i =0 ;i<4096; i++)
	{
		fputc('\0',fHandle->mgmtInfo);
	}

	fHandle->totalNumPages++;
	fHandle->curPagePos = fHandle->totalNumPages-1;
	return RC_OK;
}

/*
 * Update the number of pages in the variable numberOfPages if it is less then
 * the total number of pages in the fHandle variable
 */

RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle)
{
	while(numberOfPages<fHandle->totalNumPages)
	{
		appendEmptyBlock (fHandle);
	}
	return RC_OK;
}
