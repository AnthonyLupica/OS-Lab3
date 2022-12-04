// ACADEMIC INTEGRITY PLEDGE
//
// - I have not used source code obtained from another student nor
//   any other unauthorized source, either modified or unmodified.
//
// - All source code and documentation used in my program is either
//   my original work or was derived by me from the source code
//   published in the textbook for this course or presented in
//   class.
//
// - I have not discussed coding details about this project with
//   anyone other than my instructor. I understand that I may discuss
//   the concepts of this program with other students and that another
//   student may help me debug my program so long as neither of us
//   writes anything during the discussion or modifies any computer
//   file during the discussion.
//
// - I have violated neither the spirit nor letter of these restrictions.
//
//
//
// Signed: Anthony Lupica  Date: 12/2/2022

//filesys.c
//Based on a program by Michael Black, 2007
//Revised 11.3.2020 O'Neil

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// refactored function provided with the seed file that 
// prints the disc usage map.
// *Not to be called for submission version*
void discMap(char map[]);

// list the files on disc
void listFiles(char dir[]);

// Goes through all the validation work of determining if the file 
// specified by fileName exists, and is printable.
// If so, the starting sector and number of sectors used are stored
void findPrintableFile(char dir[], char* fileName, int* sectorStart, int* sectorCount);

// following a succesful run through findPrintableFile, fileName's contents
// in floppy are printed to the console
void printFile(FILE *floppy, int sectorStart, int sectorCount);

// recursive function to print large numbers with comma separators
void commaSeparate(unsigned int n);

// the function searches for a free directory. If one exists, a fileName entry is written
// to the directory, along with file extension .t. It returns the offset of the free directory,
// and -1 if there wasn't one
int findFree(char dir[], char* fileName);

// A text string is prompted for and the file specified in findfree is 
// created with the text string. The directory and map are updated.
void writeFile(int directoryOffset, char dir[], char map[], FILE *floppy);

void deleteFile(char dir[], char map[], char* fileName);

// total number of available bytes (considering both occupied/unoccupied sectors)
const unsigned int TRACKED_BYTES = 261632;

int main(int argc, char* argv[])
{
	// validate the number of command args
	if (argc < 2 || argc > 3)
	{
		printf("The number of command line arguments received does not map to any of the available commands\n\n");
		printf("Available commands: \n");
		printf("./filesys D filename --> Delete the named file fromt the disc\n");
		printf("./filesys L          --> List the files on the disc\n");
		printf("./filesys M filename --> Create a text file, and store it to disc\n");
		printf("./filesys P filename --> Read the named file, and print it to screen\n");

		exit(1);
	}
	
	// validate the content of argv[1]: 4 available commands 
	// argv[1][0] is the first char of the string stored in argv[1]. Since we are expecting 
	// a single char, we should validate the string length too
	if ((argv[1][0] != 'L' && argv[1][0] != 'l' && argv[1][0] != 'D' && argv[1][0] != 'd' &&
	    argv[1][0] != 'M' && argv[1][0] != 'm' && argv[1][0] != 'P' && argv[1][0] != 'p') || strlen(argv[1]) != 1)
	{
		printf("'%s' is not one of the four available commands\n\n", argv[1]);

		printf("Available commands: \n");
		printf("./filesys D filename --> Delete the named file fromt the disc\n");
		printf("./filesys L          --> List the files on the disc\n");
		printf("./filesys M filename --> Create a text file, and store it to disc\n");
		printf("./filesys P filename --> Read the named file, and print it to screen\n");

		exit(1);
	}

    // args validated, store locally 
	const char COMMAND = argv[1][0];
	
    ///// PROVIDED IN SEED FILE /////

	int i, j, size, noSecs, startPos;

	//open the floppy image
	FILE* floppy;
	floppy=fopen("floppya.img","r+");
	if (floppy==0)
	{
		printf("floppya.img not found\n");
		return 0;
	}

	//load the disk map from sector 256
	char map[512];
	fseek(floppy,512*256,SEEK_SET);
	for(i=0; i<512; i++)
	{
		map[i]=fgetc(floppy);
	}

	//load the directory from sector 257
	char dir[512];
	fseek(floppy,512*257,SEEK_SET);
	for (i=0; i<512; i++)
	{
		dir[i]=fgetc(floppy);
	}

	/////////////////////////////////
	
	switch(COMMAND)
	{
		case 'L':
		case 'l':
		    // call list function
			listFiles(dir);
			break;
		case 'P':
		case 'p':
			if (argc != 3)
			{
				printf("A filename must be provided for the print operation\n");
				exit(1);
			}

			// variables we'll need
			int sectorStart = -1;    // if the file is identified, this holds the starting sector 
			int sectorCount = -1;    // and this holds the number of sectors that the file spans

		    // call print function
			findPrintableFile(dir, argv[2], &sectorStart, &sectorCount);
			
			// if sectorStart still equals -1, no matching file was found
			if (sectorStart == -1)
			{
				printf("Your request to print file '%s' could not be granted because the file does not exist\n", argv[2]);

				exit(1);
			}

			printFile(floppy, sectorStart, sectorCount);
			break;
		case 'M':
		case 'm':
			if (argc != 3)
			{
				printf("A file name must be provided for the make operation\n");
				exit(1);
			}

			int directoryOffset = findFree(dir, argv[2]);

            // a free directory entry was found 
			if (directoryOffset != -1)
			{
				writeFile(directoryOffset, dir, map, floppy);
			}
			// insufficient disc space
			else
			{
				printf("Insufficient disc space; no free directory entry was found\n");
			}

			break;
		case 'D':
		case 'd':
			if (argc != 3)
			{
				printf("A filename must be provided for the delete operation\n");
				exit(1);
			}

			deleteFile(dir, map, argv[2]);
	}


	//write the map and directory back to the floppy image
    fseek(floppy,512*256,SEEK_SET);
    for (i=0; i<512; i++) fputc(map[i],floppy);

    fseek(floppy,512*257,SEEK_SET);
    for (i=0; i<512; i++) fputc(dir[i],floppy);

	fclose(floppy);
}

void discMap(char map[])
{
	//print disk map
	printf("Disk usage map:\n");
	printf("      0 1 2 3 4 5 6 7 8 9 A B C D E F\n");
	printf("     --------------------------------\n");
	for (int i=0; i<16; i++) {
		switch(i) {
			case 15: printf("0xF_ "); break;
			case 14: printf("0xE_ "); break;
			case 13: printf("0xD_ "); break;
			case 12: printf("0xC_ "); break;
			case 11: printf("0xB_ "); break;
			case 10: printf("0xA_ "); break;
			default: printf("0x%d_ ", i); break;
		}
		for (int j=0; j<16; j++) {
			if (map[16*i+j]==-1) printf(" X"); else printf(" .");
		}
		printf("\n");
	}

	return;
}

void commaSeparate(unsigned int n) 
{
	// base case: the remaining number is 3 digits
    //            print as is
    if (n < 1000) 
	{
        printf ("%d", n);
        return;
    }

    // recursive call: continue to divide the number until it is only 3 digit
    commaSeparate(n / 1000);

	// print last 3 digits with a comma
    printf (",%d", n % 1000);
}

void listFiles(char dir[])
{
	// tracks the running total of bytes stored
	unsigned int bytesUsed = 0;

	// print directory
	printf("\nDisk directory:\n");
    for (int i=0; i<512; i=i+16) 
	{
		// print the file name, if one exists
		if (dir[i]==0) break;
		for (int j=0; j<8; j++) 
		{
			if (dir[i+j]==0)
			{
				continue;
			}  
			else
			{
				printf("%c",dir[i+j]);
			} 
		}

		// append file extension
		if ((dir[i+8]=='t') || (dir[i+8]=='T')) printf(".t "); else printf(".x ");

		// add space used to the running total
		bytesUsed += 512*dir[i+10];
	}

    // display the space currently occupied and the space still available
	printf("\n\nSpace Used (bytes): ");
	commaSeparate(bytesUsed);

	printf("\nSpace Remaining (bytes): ");
	commaSeparate(TRACKED_BYTES - bytesUsed);
	printf("\n");
	
	return;
}

void findPrintableFile(char dir[], char* fileName, int* sectorStart, int* sectorCount)
{ 
	// per sector, 8 bytes are reserved for the filename, not including the file extension
	if (strlen(fileName) > 8)
	{
		printf("The filename passed is greater than the maximum length permitted (8 characters)\n");
		exit(1);
	}

	// this stores the file name of each sector, if there is one.
	// Gets overwritten for each new sector that a valid file name is found.
	char fileTemp[8] = {0};  

	for (int i=0; i<512; i=i+16) 
	{
		if (dir[i]==0) break;

		// zero out the temp string
		memset(fileTemp, 0, 8); 

		// loop through and look for the file name
		for (int j=0, k=0; j<8; j++) 
		{
			if (dir[i+j]==0)
			{
				continue;
			} 
			else 
			{
				fileTemp[k] = (char) dir[i+j];
				++k;
			}
		}
		
		// here, filetemp should contain the name of a valid file on disc
		// compare fileTemp with fileName. strcmp(arg1, arg2) == 0 *means they compared equal
		if (strcmp(fileTemp, fileName) == 0)
		{
			// they compared equal, now check that this file is printable (aka .txt)
			if (dir[i+8] == 'x' || dir[i+8] == 'X')
			{
				printf("Your request to print file '%s' could not be granted because it is an executable\n", fileName);

				exit(1);
			}

            // file was found and can be printed!

			// store the start sector and sector span
			*sectorStart = dir[i+9];
			*sectorCount = dir[i+10];

			// give some information about the file here
			printf("file '%s' was found\n", fileTemp);
			printf("the starting sector on disc is %d, ", *sectorStart);
			printf("and it spans %d sector(s)\n", *sectorCount);
		}
	}

	return;
}

void printFile(FILE *floppy, int sectorStart, int sectorCount)
{
	printf("\nfile contents >>\n");

	char contents[12288];
	fseek(floppy,512*sectorStart,SEEK_SET);
	
	// storage capacity of sector is 512 bytes, and the file spans sectorCount sectors 
	// --> i < sectorCount * 512
	for(int i=0; i<sectorCount*512; i++)
	{
		contents[i]=fgetc(floppy);
	}

    // @NOTE for the purpose of the lab, this will do. But, if the file were especially
	//       long, it would be necessary to do additional formatting to keep things neat and tidy
	printf("%s", contents);

	return;
}

int findFree(char dir[], char* fileName)
{
	// this stores the file name of each sector, if there is one.
	// Gets overwritten for each new sector that a valid file name is found.
	char fileTemp[8] = {0}; 

    // loop through to see if the file already exists
	for (int i=0; i<512; i=i+16) 
	{
		if (dir[i]==0) break;

		// zero out the temp string
		memset(fileTemp, 0, 8); 

		// loop through and look for the file name
		for (int j=0, k=0; j<8; j++) 
		{
			if (dir[i+j]==0)
			{
				continue;
			} 
			else 
			{
				fileTemp[k] = (char) dir[i+j];
				++k;
			}
		}
		
		// here, filetemp should contain the name of a valid file on disc
		// compare fileTemp with fileName. strcmp(arg1, arg2) == 0 *means they compared equal
		if (strcmp(fileTemp, fileName) == 0)
		{
			printf("Your request to create file '%s' could not be granted because it already exists\n", fileName);

			exit(1);
		}
	}

	// Now find a free a directory entry
	for (int i=0; i<512; i=i+16) 
	{
		// this is an empty directory
		if (dir[i]==0)
		{
			char tempString[8] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};

			// if fileName is shorter than 8 characters, tempString fills to 8 with zeros
			if (strlen(fileName) < 8)
			{
				strcpy(tempString, fileName);
			}
			// otherwise, only the first 8 characters of fileName are used 
			else 
			{
				strncpy(tempString, fileName, 8);
			}

			// loop through this entry and write the filename
			for (int j=0; j<8; j++)
			{
				dir[i+j] = tempString[j];
			}

            // write the file extension 
			dir[i+8] == 't';

			return i;
		}
	}

    //  if we're here, not free directory entry was found 
	return -1;
}

void writeFile(int directoryOffset, char dir[], char map[], FILE *floppy)
{
	// find a free sector on the disc by searching through the map for a zero
	for (int i=0; i<16; i++) 
	{
		for (int j=0; j<16; j++) 
		{
			// -1 represents an occupied map space
			if (map[16*i+j]!=-1) 
			{
				// set map entry to 255 (ff) to signal its taken 
				map[16*i+j] = 255;

                // add the file extension
				dir[directoryOffset + 8] = 't';

				// add the starting sector number and length (1) to the file's directory entry
				dir[directoryOffset + 9] = i + j;
				dir[directoryOffset + 10] = 1;

				// prompt and store text (512 bytes is the limit)
				char textString[512] = "";

				printf("Enter text for the contents of the file >> ");
				fgets(textString, sizeof(textString), stdin);
				
				// write the file 
				fseek(floppy,512*(i + j),SEEK_SET);
	
				// storage capacity of sector is 512 bytes, and the file spans sectorCount sectors 
				// --> i < sectorCount * 512
				for(int i=0; i<512; i++)
				{
					fputc(textString[i], floppy);
				}

				return;
			}
		}
	}

	printf("Insufficient free sectors to write the file\n");
}

void deleteFile(char dir[], char map[], char* fileName)
{
	// find the file in the directory and delete it if it exists

	// this stores the file name of each sector, if there is one.
	// Gets overwritten for each new sector that a valid file name is found.
	char fileTemp[8] = {0};  
	int isFound = 0;

	int directoryOffset = 0;
	int sectorStart = 0;
	int sectorCount = 0;

    // loop through to see if this file even exists
	for (int i=0; i<512; i=i+16) 
	{
		if (dir[i]==0) break;

		// zero out the temp string
		memset(fileTemp, 0, 8); 

		// loop through and look for the file name
		for (int j=0, k=0; j<8; j++) 
		{
			if (dir[i+j]==0)
			{
				continue;
			} 
			else 
			{
				fileTemp[k] = (char) dir[i+j];
				++k;
			}
		}
		
		// here, filetemp should contain the name of a valid file on disc
		// compare fileTemp with fileName. strcmp(arg1, arg2) == 0 *means they compared equal
		if (strcmp(fileTemp, fileName) == 0)
		{
			// a match!
			isFound = 1;

			// set the first byte of the name to 0 to erase it from the directory
			dir[i] = 0;

            // store the starting sector and file length
			directoryOffset = i;
			sectorStart = dir[i + 9];
			sectorCount = dir[i + 10];

			break;
		}
	}

	if (!isFound)
	{
		printf("Your request to delete file '%s' could not be granted because it does not exist\n", fileName);
		exit(1);
	}

	// we have verified that the file exists, proceed to delete

    // delete the map sectors that belong to the file
	int deleteCount = 0;
	while(deleteCount < sectorCount)
	{
		map[16*sectorStart + deleteCount];
        ++deleteCount;
	}

	printf("'%s' has been successfully deleted\n", fileName);
}