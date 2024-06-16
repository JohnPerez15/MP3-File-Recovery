#include "projinc.h"
#include "mbr.h"
#include "superblock.h"
#include "blockdefault.h"
#include "BlkByte.h"

#define SIZE_OF_MBR 512
#define SIZE_OF_SUPERBLK 1024
#define BLK_SIZE 4096
#define MAGIC_VAL1 0x55
#define MAGIC_VAL2 0xAA
#define ID3_VAL1 0x49
#define ID3_VAL2 0x44
#define ID3_VAL3 0x33


uint32_t getPartAddr( int fd ) {
	
	int retVal;
	int temp;
	uint32_t addr;
	//integer used to check if the machine is little or big endian
	int endianCheck = 1;
	
	
	struct disk_mbr Disk_MBR;
	
	retVal = read(fd, &Disk_MBR, SIZE_OF_MBR);
	
	if( retVal <= 0) {
		fprintf(stderr, "unable to read disk, retVal = %d\n", retVal);
		return;
	}
	
	
	//used to check number of bits in Disk_MBR
	temp = sizeof(Disk_MBR);
	
	//test outputs for size and signature
	fprintf(stdout, "struct size: %d\n", temp);
	fprintf(stdout, "sign1: 0x%X\n", Disk_MBR.signature[0]);
	fprintf(stdout, "sign2: 0x%X\n", Disk_MBR.signature[1]);
	
	
	
	//converts to little endian if the machine is formatted in big endian
	if( *(char *)&endianCheck != 1) {
		Disk_MBR.pt[0].first_sector_lba = ntohl(Disk_MBR.pt[0].first_sector_lba);
	}
	
	//calculates the address of the partition and prints it
	addr = Disk_MBR.pt[0].first_sector_lba * 512;
	fprintf(stdout, "Partition address: 0x%X\n", addr);
	
	return addr;
	
}

uint32_t printSuperblock( int fd ) {
	int retVal;
	int temp;
	int endianCheck = 1;
	//uint8_t *b;
	
	struct disk_superblock Disk_Superblock;
	
	//b = (uint8_t *)&Disk_Superblock;
	
	
	temp = sizeof(Disk_Superblock);
	
	
	retVal = read(fd, &Disk_Superblock, SIZE_OF_SUPERBLK);
	
	//error message if can not read the disk
	if( retVal <= 0) {
		fprintf(stderr, "unable to read disk, retVal = %d\n", retVal);
	}
	
	//Outputs requested on assignment
	fprintf(stdout, "Magic Signature: 0x%X\n", Disk_Superblock.s_magic);
	fprintf(stdout, "Block sizes: %d\n", Disk_Superblock.s_log_block_size);
	fprintf(stdout, "total number of blocks: %d\n", Disk_Superblock.s_blocks_count);
	fprintf(stdout, "total number of inodes: %d\n", Disk_Superblock.s_inodes_count);
	
	
	fprintf(stdout, "Superblock size: %d\n", temp);
	
	return Disk_Superblock.s_blocks_count;

}

void printIndirectBlk(int fd, uint32_t numBlks) {
	int retVal;
	int numIndirects = 0;
	int numConsec = 0;
	uint32_t currentVal;
	uint32_t nextVal;
	
	struct block_default buf;
	
	//fprintf(stdout, "num blocks: %d\n", numBlks);
	
	
	//main loop to iterate through the blocks in the partition
	for(int blkIndex = 0; blkIndex < numBlks; blkIndex++) {
		
		//reads next block of Partition
		retVal = read(fd, &buf, BLK_SIZE);
		
		
		//error message if can not read the disk
		if( retVal < 0) {
			fprintf(stdout, "blkIndex: %d\n", blkIndex);
			fprintf(stderr, "unable to read disk, retVal = %d\n", retVal);
		}
		
		//loop used to count the number of consecutive values in a block
		for(int i = 0; i < 1024; i++) {
		
			currentVal = buf.buf1[i];
			nextVal = buf.buf1[i+1];
			
			
			//Takes into consideration a indirect block with only one val
			//Had example file with such instance
			if(i == 0 && currentVal > 0 && nextVal== 0 && buf.buf1[i + 2] == 0) {
				numIndirects++;
				break;
			}
			
			
			if (currentVal + 1 == nextVal) {
				numConsec++;
				//fprintf(stdout, "number of Consecutive: %d\n", numConsec);
			}
			else if(numConsec > 3) {
				//fprintf(stdout, "Indirect Blk: %d\n", blkIndex+1);
				break;
			}
			//used for incompleted second and third indirect blocks
			else if(numConsec >= 1 && nextVal == 0) {
				numIndirects++;
				//fprintf(stdout, "Indirect Blk: %d\n", blkIndex+1);
				break;
			}
			else {
				//break found resets the number of
				//consecutive integers
				numConsec = 0;
			}
		}
		
		//increments the number of Indirects if there
		//are four or more consequtive numbers
		if(numConsec > 3) {
			numIndirects++;
		}
		
		numConsec = 0;
		
	}
	
	fprintf(stdout, "Number of Indirect Blocks Found = %d\n", numIndirects);
}

uint32_t findFirstBlock(int fd, uint32_t numBlks) {
	int retVal;
	uint32_t nextVal;
	uint8_t Val1_temp;
	uint8_t Val2_temp;
	uint8_t Val3_temp;
	uint32_t firstBlk;
	//uint32_t firstBlkAddr;

	struct Blk_Byte buf;
	
	fprintf(stdout, "Possible Index of first Block:\n");
	
	//main loop to iterate through the blocks in the partition
	for(int blkIndex = 0; blkIndex < numBlks; blkIndex++) {
		
		//reads next block of Partition
		retVal = read(fd, &buf, BLK_SIZE);
		
		//error message if can not read the disk
		if( retVal < 0) {
			fprintf(stdout, "blkIndex: %d\n", blkIndex);
			fprintf(stderr, "unable to read disk, retVal = %d\n", retVal);
		}
		
		
		for(int byteIndex = 0; byteIndex < BLK_SIZE; byteIndex++) {
			Val1_temp = buf.buf1[byteIndex];
			//Checks to see if the three byte vals are free
			//this is the size of the ID3 identifier
			if(byteIndex + 2 < BLK_SIZE) {
				Val1_temp = buf.buf1[byteIndex];
				Val2_temp = buf.buf1[byteIndex + 1];
				Val3_temp = buf.buf1[byteIndex + 2];
			
			}
			else {
				break;
			}
		
			if( Val1_temp == ID3_VAL1 && Val2_temp == ID3_VAL2 && Val3_temp == ID3_VAL3) {
				//Add 1 because an array index start at 0
				//the block number starts at 1
				//add back the offset
				firstBlk = blkIndex + 1;
				fprintf(stdout, "ID3 Tag found: %d\n", firstBlk);
			}
		
		}//end of inner loop
	
	}//end of outer loop
	
	//firstBlkAddr = firstBlk * BLK_SIZE;
	
	return firstBlk;
}

int getFirstIndirBlk(int fd, uint32_t numBlks, uint32_t lastDirBlk) {
	int retVal;
	int numIndirects = 0;
	int numConsec = 0;
	uint32_t currentVal;
	uint32_t nextVal;
	
	uint32_t firstEntryVal = lastDirBlk + 1;
	
	struct block_default buf;	
	
	//main loop to iterate through the blocks in the partition
	for(int blkIndex = 0; blkIndex < numBlks; blkIndex++) {
		
		//reads next block of Partition
		retVal = read(fd, &buf, BLK_SIZE);
		
		
		//error message if can not read the disk
		if( retVal < 0) {
			fprintf(stdout, "blkIndex: %d\n", blkIndex);
			fprintf(stderr, "unable to read disk, retVal = %d\n", retVal);
			return false;
		}
		
		//loop used to count the number of consecutive values in a block
		for(int i = 0; i < 1024; i++) {
		
			if(i == 0 && buf.buf1[i] != firstEntryVal) {
				break;			
			}
		
			currentVal = buf.buf1[i];
			nextVal = buf.buf1[i+1];
			
			
			//Takes into consideration a indirect block with only one val
			//Had example file with such instance
			if(i == 0 && currentVal > 0 && nextVal== 0 && buf.buf1[i + 2] == 0) {
				return blkIndex;
			}
			
			
			if (currentVal + 1 == nextVal) {
				numConsec++;
			}
			else if(numConsec > 3) {
				return blkIndex;
			}
			//used for uncompleted second and third indirect blocks
			else if(numConsec >= 1 && nextVal == 0) {
				return blkIndex;
			}
			else {
				//break found resets the number of
				//consecutive integers
				numConsec = 0;
			}
		}
		
		numConsec = 0;
	}
	return 0;
}

//candBlk is my first entry of my consideration of by second indirect blk
//checking to see if this block is a first indirect blk
//if this is a first indirect blk ---> my second indirect block is correct
bool checkIndir(int fd, uint32_t numBlks, uint32_t lastDirBlk, uint32_t startPartAddr, uint32_t candBlk) {
	int retVal;
	int numIndirects = 0;
	int numConsec = 0;
	uint32_t currentVal;
	
	uint32_t nextVal;
	uint32_t candBlkAddr;
	uint32_t firstEntry = lastDirBlk + 1;
	
	candBlkAddr = candBlk * BLK_SIZE;
	
	struct block_default buf;
	
	
	
	//sets fd to the start of Partition
	retVal = lseek64(fd, startPartAddr, SEEK_SET);
	
	if( retVal < 0 ) {
		fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
		return;
	}
	
	
	//sets fd to the start of possible first indirect blk
	retVal = lseek64(fd, candBlkAddr, SEEK_CUR);
	
	if( retVal < 0 ) {
		fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
		return;
	}
		
	//reads next block of Partition
	retVal = read(fd, &buf, BLK_SIZE);
	
	//fprintf(stdout, "firstEntry: %d\n", firstEntry);
	//fprintf(stdout, "buf content: %d\n", buf.buf1[0]);
	
	if(buf.buf1[0] != firstEntry) {
		return false;
	}
	
	
	//error message if can not read the disk
	if( retVal < 0) {
		fprintf(stderr, "unable to read disk, retVal = %d\n", retVal);
	}
	
	//loop used to count the number of consecutive values in a block
	for(int i = 0; i < 1024; i++) {
	
		currentVal = buf.buf1[i];
		nextVal = buf.buf1[i+1];
		
		
		//Takes into consideration a indirect block with only one val
		//Had example file with such instance
		if(i == 0 && currentVal > 0 && nextVal== 0 && buf.buf1[i + 2] == 0) {
			return true;
		}
		
		
		if (currentVal + 1 == nextVal) {
			numConsec++;
			//fprintf(stdout, "number of Consecutive: %d\n", numConsec);
		}
		else if(numConsec > 3) {
			return true;
		}
		//used for incompleted second and third indirect blocks
		else if(numConsec >= 1 && nextVal == 0) {
			return true;
		}
		else {
			//break found resets the number of
			//consecutive integers
			numConsec = 0;
		}
	}//iterates through the four bytes of the blk
	
	return false;
}

int getSecondIndirBlk(int fd, uint32_t numBlks, uint32_t lastDirBlk, uint32_t startPartAddr) {
	int retVal;
	int numIndirects = 0;
	int numConsec = 0;
	uint32_t currentVal;
	uint32_t currentBlkAddr;
	uint32_t nextVal;
	bool SIndirBlkFound = false;
	bool SIndirBlkFound2 = false;
	
	
	struct block_default SIndirBuf;
	struct block_default buf;
	
	
	//main loop to iterate through the blocks in the partition
	for(int blkIndex = 0; blkIndex < numBlks; blkIndex++) {
		
		
		
		currentBlkAddr = blkIndex * BLK_SIZE;
		
		//sets fd to the start of Partition
		retVal = lseek64(fd, startPartAddr, SEEK_SET);
		
		if( retVal < 0 ) {
			fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
			return;
		}
		
		//sets fd to the start of Partition
		retVal = lseek64(fd, currentBlkAddr, SEEK_CUR);
		
		if( retVal < 0 ) {
			fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
			return;
		}
		
		
		
		//reads next block of Partition
		retVal = read(fd, &SIndirBuf, BLK_SIZE);
		
		
		//error message if can not read the disk
		if( retVal < 0) {
			fprintf(stdout, "blkIndex: %d\n", blkIndex);
			fprintf(stderr, "unable to read disk, retVal = %d\n", retVal);
		}
		
		//loop used to count the number of consecutive values in a block
		for(int i = 0; i < 1024; i++) {
			
		
			currentVal = SIndirBuf.buf1[i];
			nextVal = SIndirBuf.buf1[i+1];
			
			
			//Takes into consideration a indirect block with only one val
			//Had example file with such instance
			if(i == 0 && currentVal > 0 && nextVal== 0 && SIndirBuf.buf1[i + 2] == 0) {
				//fprintf(stdout, "Indirect Blk: %d\n", blkIndex);
				
				if(currentVal == 0 || SIndirBuf.buf1[0] > numBlks) {
					break;
				}
				
				SIndirBlkFound = checkIndir(fd, numBlks, lastDirBlk, startPartAddr, SIndirBuf.buf1[0]);
				if (SIndirBlkFound) {
					//fprintf(stdout, "Return 1\n");
					return blkIndex;
				}
				break;
			}
			
			
			if (currentVal + 1 == nextVal) {
				numConsec++;
			}
			else if(numConsec > 3) {
				//fprintf(stdout, "Indirect Blk: %d\n", blkIndex);
				
				//checks if the contents are either the superblk or past the partition
				//breaks if it is either case
				if(SIndirBuf.buf1[0] == 0 || SIndirBuf.buf1[0] > numBlks) {
					break;
				}
				
				SIndirBlkFound = checkIndir(fd, numBlks, lastDirBlk, startPartAddr, SIndirBuf.buf1[0]);
				if (SIndirBlkFound == true) {
					//fprintf(stdout, "Return 2\n");
					return blkIndex;
				}
				
				break;
			}
			//used for uncompleted second and third indirect blocks
			else if(numConsec >= 1 && nextVal == 0) {
				//fprintf(stdout, "Indirect Blk: %d\n", blkIndex);
				
				//checks if the contents are either the superblk or past the partition
				//breaks if it is either case
				if(SIndirBuf.buf1[0] == 0 || SIndirBuf.buf1[0] > numBlks) {
					break;
				}
				
				
				SIndirBlkFound = checkIndir(fd, numBlks, lastDirBlk, startPartAddr, SIndirBuf.buf1[0]);
				if (SIndirBlkFound == true) {
					//fprintf(stdout, "Return 3\n");
					return blkIndex;
				}
				
				break;
			}
			else {
				//break found resets the number of
				//consecutive integers
				numConsec = 0;
			}
		} //end of inner loop
		
		numConsec = 0;
		
	} //end of outer loop
	
	//fprintf(stdout, "Return 4\n");
	return 0;
}


void fileRecovery(int fd, int outfd, uint32_t startPartAddr, uint32_t firstBlkFile, uint32_t numBlks){
	struct block_default buf;
	struct block_default FirstIndirBuf;
	struct block_default SecondIndirBuf;
	int retVal;
	uint32_t firstIndBlk;
	uint32_t secondIndBlk;
	uint32_t firstIndBlkAddr;
	uint32_t secondIndBlkAddr;
	uint32_t DirBlkAddr;
	uint32_t IndirBlkAddr;
	uint32_t refVal1;
	uint32_t refVal2;
	int numDirBlks = 12;
	uint32_t lastDirBlk = firstBlkFile + 11;
	
	bool endDirBlk = false;
	bool endFIndirBlk = false;
	bool endSIndirBlk = false;
	bool completedFile = false;
	
	fprintf(stdout, "Start of Recovering File:\n");


	for(int dirBlkIndex = 0; dirBlkIndex < numDirBlks; dirBlkIndex++) {
		//reads next block of file
		retVal = read(fd, &buf, BLK_SIZE);
		
		
		//error message if can not read the disk
		if( retVal < 0) {
			fprintf(stderr, "unable to read disk, retVal = %d\n", retVal);
		}
		
		for(int fileBlkIndex = 0; fileBlkIndex < 1024; fileBlkIndex++) {
			//sets two reference values to be the next 8 bytes
			if(fileBlkIndex + 1 < 1024) {
				refVal1 = buf.buf1[fileBlkIndex];
				refVal2 = buf.buf1[fileBlkIndex + 1];
			
			}
			
			
			//checks to see if the next 8 bytes are 0 
			if(refVal1 == 0 && refVal2 == 0) {
				//continues to check if the rest of the block is zero
				//the file is completed if it is zeroed out
				for(int checkIndex = fileBlkIndex; checkIndex < 1024; checkIndex++) {
					if(buf.buf1[checkIndex] == 0) {
						completedFile = true;
					
					}
					else {
						completedFile = false;
						break;
					}
				
				}
			}
			
			if(completedFile == true) {
				break;
			}
			
			//writes four bytes to the file
			write(outfd, &buf.buf1[fileBlkIndex], 4);
			
			
		
		}//end of file block loop
	
		if(completedFile == true) {
			break;
		}
	}//direct blocks loop
	
	if(completedFile == true) {
		fprintf(stdout, "End of Recovering File:\n");
		return;
	}
	
	//sets fd to the start of Partition
	retVal = lseek64(fd, startPartAddr, SEEK_SET);
	
	if( retVal < 0 ) {
		fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
		return;
	}
	
	
	//Gets the first Indirect block of File
	firstIndBlk = getFirstIndirBlk(fd, numBlks, lastDirBlk);
	
	
	fprintf(stdout, "First Direct Block: %d\n", firstIndBlk);
	
	//Message if the First Indirect is Not found
	if(firstIndBlk == 0) {
		fprintf(stdout, "No first Indirect Block Found\n");
		fprintf(stdout, "End of Recovering File:\n");
		return;
	}
	
	
	
	//Addr of address of First Indirect Block
	//Does not include Start of Partition
	firstIndBlkAddr = firstIndBlk * BLK_SIZE;
	
	retVal = lseek64(fd, startPartAddr, SEEK_SET);
	
	if( retVal < 0 ) {
		fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
		return;
	}
	
	
	//Sets fd to the start of the first Indirect Blk
	retVal = lseek64(fd, firstIndBlkAddr, SEEK_CUR);
	
	if( retVal < 0 ) {
		fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
		return;
	}
	
	
	
	retVal = read(fd, &FirstIndirBuf, BLK_SIZE);
		
	//error message if can not read the disk
	if( retVal < 0) {
		fprintf(stderr, "unable to read disk, retVal = %d\n", retVal);
	}	
	
	//Iterate through the First Indirect Blocks
	for(int FIndirIndex = 0; FIndirIndex < 1024; FIndirIndex++) {
	
		//file has been completed in this point
		if(FIndirIndex + 1 < 1024) {
			if( FirstIndirBuf.buf1[FIndirIndex + 1] == 0) {
				endFIndirBlk = true;
			}
		
		}
		
		//fprintf(stdout, "first Indirect Buffer: %d\n", FirstIndirBuf.buf1[FIndirIndex]);
		
		
		//Used to calculate the offset of direct block
		DirBlkAddr = FirstIndirBuf.buf1[FIndirIndex] * BLK_SIZE;
		
		//fd to start of Partition
		retVal = lseek64(fd, startPartAddr, SEEK_SET);
	
		if( retVal < 0 ) {
			fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
			return;
		}
		//Moves to the direct block
		retVal = lseek64(fd, DirBlkAddr, SEEK_CUR);
	
		if( retVal < 0 ) {
			fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
			return;
		}
		
		//Sets buf to be the direct block
		retVal = read(fd, &buf, BLK_SIZE);
		
		//error message if can not read the disk
		if( retVal < 0) {
			fprintf(stderr, "unable to read disk, retVal = %d\n", retVal);
		}
		
		
		for(int FBIndex = 0; FBIndex < 1024; FBIndex++) {
			//sets two reference values to be the next 8 bytes
			if(FBIndex + 1 < 1024) {
				refVal1 = buf.buf1[FBIndex];
				refVal2 = buf.buf1[FBIndex + 1];
			
			}
			
			
			///checks to see if the next 8 bytes are 0 
			if(endFIndirBlk == true && refVal1 == 0 && refVal2 == 0) {
				//continues to check if the rest of the block is zero
				//the file is completed if it is zeroed out
				for(int checkIndex = FBIndex; checkIndex < 1024; checkIndex++) {
					if(buf.buf1[FBIndex] == 0) {
						completedFile = true;
					
					}
					else {
						completedFile = false;
						break;
					}
				
				}
			}
			
			if(completedFile == true) {
				break;
			}
			
			write(outfd, &buf.buf1[FBIndex], 4);
		
		}//end of file block loop
		
		if(endFIndirBlk == true) {
			break;
		
		}
	
	}//end of first indirect loop
	
	
	if(endFIndirBlk == true ||  completedFile == true) {
		fprintf(stdout, "End of Recovering File:\n");
		return;
	}
	
	retVal = lseek64(fd, startPartAddr, SEEK_SET);
	
	if( retVal < 0 ) {
		fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
		return;
	}
	
	secondIndBlk = getSecondIndirBlk(fd, numBlks, FirstIndirBuf.buf1[1023], startPartAddr);
	
	fprintf(stdout, "Second Indirect Blk: %d\n", secondIndBlk);
	
	secondIndBlkAddr = secondIndBlk * BLK_SIZE;
	
	//fd to start of Partition
	retVal = lseek64(fd, startPartAddr, SEEK_SET);

	if( retVal < 0 ) {
		fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
		return;
	}
	
	//Moves to the direct block
	retVal = lseek64(fd, secondIndBlkAddr, SEEK_CUR);

	if( retVal < 0 ) {
		fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
		return;
	}
	
	//Sets buf to be the direct block
	retVal = read(fd, &SecondIndirBuf, BLK_SIZE);
	
	//error message if can not read the disk
	if( retVal < 0) {
		fprintf(stderr, "unable to read disk, retVal = %d\n", retVal);
	}
	
	
	//Main loop going through the second indirect block
	for(int SIndirIndex = 0; SIndirIndex < 1024; SIndirIndex++) {
	
	
		//Used to calculate the offset of direct block
		IndirBlkAddr = SecondIndirBuf.buf1[SIndirIndex] * BLK_SIZE;
		
		//fd to start of Partition
		retVal = lseek64(fd, startPartAddr, SEEK_SET);
	
		if( retVal < 0 ) {
			fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
			return;
		}
		//Moves to the first IndirBlkAddr block
		retVal = lseek64(fd, IndirBlkAddr, SEEK_CUR);
	
		if( retVal < 0 ) {
			fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
			return;
		}
		
		//Sets the FirstIndirBuf to an indirect block
		retVal = read(fd, &FirstIndirBuf, BLK_SIZE);
		//error message if can not read the disk
		if( retVal < 0) {
			fprintf(stderr, "unable to read disk, retVal = %d\n", retVal);
		}
		
		
		//fprintf(stdout, "Error occured prior to OBlk:%d\n", SIndirIndex);
	
		//Iterate through the First Indirect Blocks
		for(int FIndirIndex = 0; FIndirIndex < 1024; FIndirIndex++) {
		
			if(FIndirIndex + 1 < 1024) {
				if( FirstIndirBuf.buf1[FIndirIndex + 1] == 0) {
					endFIndirBlk = true;
				}
			}
	
			//fprintf(stdout, "first Indirect Buffer: %d\n", FirstIndirBuf.buf1[FIndirIndex]);
			
			//fprintf(stdout, "inner Blk:%d\n", FirstIndirBuf.buf1[FIndirIndex]);
			
			//Used to calculate the offset of direct block
			DirBlkAddr = FirstIndirBuf.buf1[FIndirIndex] * BLK_SIZE;
			
			//fd to start of Partition
			retVal = lseek64(fd, startPartAddr, SEEK_SET);
		
			if( retVal < 0 ) {
				fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
				return;
			}
			//Moves to the direct block
			retVal = lseek64(fd, DirBlkAddr, SEEK_CUR);
		
			if( retVal < 0 ) {
				fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
				return;
			}
			
			//Sets buf to be the direct block
			retVal = read(fd, &buf, BLK_SIZE);
			
			//error message if can not read the disk
			if( retVal < 0) {
				fprintf(stderr, "unable to read disk, retVal = %d\n", retVal);
			}
			
			
			for(int FBIndex = 0; FBIndex < 1024; FBIndex++) {
			
				//sets two reference values to be the next 8 bytes
				if(FBIndex + 1 < 1024) {
					refVal1 = buf.buf1[FBIndex];
					refVal2 = buf.buf1[FBIndex + 1];
				
				}
				
				
				///checks to see if the next 8 bytes are 0 
				if(endFIndirBlk == true && refVal1 == 0 && refVal2 == 0) {
					//continues to check if the rest of the block is zero
					//the file is completed if it is zeroed out
					for(int checkIndex = FBIndex; checkIndex < 1024; checkIndex++) {
						if(buf.buf1[FBIndex] == 0) {
							completedFile = true;
						
						}
						else {
							completedFile = false;
							break;
						}
					
					}
				}
			
				if(completedFile == true) {
					break;
				}
				write(outfd, &buf.buf1[FBIndex], 4);
			
			}//end of file block loop
		
	
		//file has been completed in this point
		if(endFIndirBlk == true) {
			break;
		}
		
	}//end of first indirect loop
		
		if(endFIndirBlk == true) {
			break;
		}
		else if(SIndirIndex + 1 < 1024) {
			if( SecondIndirBuf.buf1[SIndirIndex + 1] == 0) {
				break;
			}
		}
	
	} //end of iterating through second indirect block
	

	fprintf(stdout, "End of Recovering File:\n");
} 

//Referenced from previously provided code
int main(int argc, char** argv) {

	int fd;
	int outfd;
	int retVal;
	uint32_t superblkAddr;
	uint32_t startPartAddr;
	//used to save the address of the first block after the superblock
	uint32_t firstBlkPartAddr;
	//used to save the address of the first block of the file recovered
	uint32_t firstBlkFile;
	uint32_t firstBlkFileAddr;
	uint32_t totalBlks;
	
	//incorrect number of arguments
	if(argc != 3) {
		fprintf(stderr, "USAGE: %s /dev/sdx\n", argv[0]);
		return -1;
	}
	
	
	//opens the drive and the written file
	fd = open(argv[1], O_RDONLY);
	outfd = open(argv[2], O_WRONLY);
	
	
	//condition if the drive failed to open
	if(fd <= 0) {
		fprintf(stderr, "File not opened %x %s\n", strerror(errno));
		return -1;
	
	}
	
	//condition if the Recovered.mp3 file failed to open
	if(outfd <= 0) {
		fprintf(stderr, "File not opened %x %s\n", strerror(errno));
		return -1;
	}

	
	
	startPartAddr = getPartAddr( fd );
	//sets position to the start of first block after superblock
	firstBlkPartAddr = startPartAddr + 4096;
	//sets position to the start of superblock
	superblkAddr = startPartAddr + 1024;
	
	retVal = lseek64(fd, superblkAddr, SEEK_SET);
	
	if( retVal < 0 ) {
		fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
		return -1;
	}
	
	totalBlks = printSuperblock( fd );
	
	
	//sets address to the first block of partition
	retVal = lseek64(fd, firstBlkPartAddr, SEEK_SET);
	
	if( retVal < 0 ) {
		fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
		return -1;
	}
	
	printIndirectBlk(fd, totalBlks);
	
	//sets address to the first block of partition
	retVal = lseek64(fd, firstBlkPartAddr, SEEK_SET);
	
	if( retVal < 0 ) {
		fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
		return -1;
	}
	
	firstBlkFile = findFirstBlock(fd, totalBlks);
	firstBlkFileAddr = firstBlkFile * BLK_SIZE;
	
	//moving fd to the begining of the Partition
	
	retVal = lseek64(fd, startPartAddr, SEEK_SET);
	
	if( retVal < 0 ) {
		fprintf(stderr, "unable to move to partition address, retVal = %d\n", retVal);
		return -1;
	}
	
	//Moves the fd to the start of the file
	retVal = lseek64(fd, firstBlkFileAddr, SEEK_CUR);
	
	if( retVal < 0 ) {
		fprintf(stderr, "unable to move to file address, retVal = %d\n", retVal);
		return -1;
	}
	
	
	fileRecovery(fd, outfd, startPartAddr, firstBlkFile, totalBlks);
	
	close( fd );
	close( outfd );
	
	return 0;
}
