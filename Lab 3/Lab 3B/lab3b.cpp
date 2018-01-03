//
//  Lab3b.cpp
//  
//
//  Created by Rahul Sheth on 11/26/17.
//
//

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
using namespace std;
//GLOBAL VARIALBLES AND FUNCTION DECLARATIONS BEGIN
char* fileName;
struct superblock {
    int block_count;
    int inode_count;
    int block_size;
    int inode_size;
    int blocks_per_group;
    int inodes_per_group;
    int first_inode;
};

struct duplicateBlock {
    int inodeNum;
    int offset;
    int indirection;
};
struct dirEntry {
    int curInode;
    int referenceInode;
    string name;
};
vector<int> freeBlocks;
vector<int> freeInodes;
map<int, int> referenceCounts;
vector<int> allBlocks;
map<int, vector<duplicateBlock> > DuplicateMap;
vector<int> unreferencedBlocks;
vector<dirEntry> directoryEntries;
map<int, int> newRefCount;




superblock cur;
void checkSingleBlock(int curInode, int block_num);
void checkDoubleBlock(int curInode, int block_num);
void checkTripleBlock(int curInode, int block_num);
//GLOBAL VARIALBLES AND FUNCTION DECLARATIONS END

//SET UP OF GLOBAL VARIABLES BEGIN

//extract the fileName, return 1 if failed
void extractFileName(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Error, invalid number of arguments given on command line");
        exit(1);
    }
    fileName = argv[1];
    ifstream testCSV(fileName, ifstream::in);
    
    if (testCSV.fail()) {
        fprintf(stderr, "Error, could not open up file");
        exit(1);
    }

}

void setUpUnreferenceCheck() {
    //Put in all non-reserved blocks
    for (int i = 8; i < cur.block_count; i++) {
        unreferencedBlocks.push_back(i);
    }
   
}

//Extract the superBlock, with block_num, inode_num, etc
void extractSuperBlock() {
    ifstream csvFile(fileName, ifstream::in);

    string curLine;
    while (csvFile.good()) {
        getline(csvFile, curLine);
        if (curLine.find("SUPERBLOCK") != string::npos) {
            stringstream next(curLine);
            while (next.good()) {
                
                //SUPERBLOCK
                getline(next, curLine, ',');
                //Total number of blocks
                getline(next, curLine, ',');
                cur.block_count = stoi(curLine);
                //Total number of inodes
                getline(next, curLine, ',');
                cur.inode_count = stoi(curLine);

                //Block Size
                getline(next, curLine, ',');
                cur.block_size = stoi(curLine);

                //Inode size
                getline(next, curLine, ',');
                cur.inode_size = stoi(curLine);

                //Blocks per group
                getline(next, curLine, ',');
                cur.blocks_per_group = stoi(curLine);

                //inodes per group
                getline(next, curLine, ',');
                cur.inodes_per_group = stoi(curLine);
                
                //First Inode
                getline(next, curLine);
                cur.first_inode = stoi(curLine);
                
            }
        }
    }
}

//Add in all of the free blocks and free inodes
void populateBlocksAndInodes() {
    ifstream csvFile(fileName, ifstream::in);
    
    string curLine, nextLine;
    while (csvFile.good()) {
        getline(csvFile, curLine);
        if (curLine.find("BFREE") != string::npos) {
            stringstream next(curLine);
            getline(next, curLine, ',');
            getline(next, curLine, ',');
            int block_num = stoi(curLine);
            freeBlocks.push_back(block_num);
            //If it's on the freelist, it is not an unreferenced block
            if (find(unreferencedBlocks.begin(), unreferencedBlocks.end(), block_num) != unreferencedBlocks.end())
                unreferencedBlocks.erase(find(unreferencedBlocks.begin(), unreferencedBlocks.end(), block_num));
        } else if (curLine.find("IFREE") != string::npos) {
            stringstream next(curLine);
            getline(next, curLine, ',');
            getline(next, curLine, ',');
            freeInodes.push_back(stoi(curLine));
        }
        curLine = "";
        nextLine = "";
    }
    
 
}
//Populate all of the linkCounts
void populateReferenceCounts() {
    ifstream csvFile(fileName, ifstream::in);

    string curLine, nextLine;
    
    while (csvFile.good()) {
        getline(csvFile, curLine);
        if (curLine.find("INODE") != string::npos) {
            stringstream next(curLine);
            int curInode;
            for (int i = 0; i < 2; i++) {
                getline(next, curLine, ',');
            }
            curInode = stoi(curLine);
            for (int i  = 0; i < 4; i++) {
                getline(next, curLine, ',');
            }
            getline(next, curLine, ',');
            referenceCounts[curInode] = stoi(curLine);
        }
    }
}
//extract all directory entries, useful for when we are checking the parents of an inode
void populateDirectoryEntries() {
    ifstream csvFile(fileName, ifstream::in);
    
    string curLine, nextLine;
    
    while (csvFile.good()) {
        getline(csvFile, curLine);
        if (curLine.find("DIRENT") != string::npos) {
            stringstream next(curLine);
            dirEntry appendDir;
            for (int i = 0; i < 2; i++) {
                getline(next, curLine, ',');
            }
            appendDir.curInode = stoi(curLine);
            for (int i = 0; i < 2; i++) {
                getline(next, curLine, ',');
            }
            appendDir.referenceInode = stoi(curLine);
            for (int i = 0; i < 3; i++) {
                getline(next, curLine, ',');
            }
            appendDir.name = curLine;
            directoryEntries.push_back(appendDir);
        }
    }
}

//SET UP OF GLOBAL VARIABLES END


//ACTUAL CHECKING BEGIN

//The following checks are here

//1. Checking the allocated inodes against the freelist
//2. Invalid and reserved single, double, and triple indirect blocks and direct blocks
//3. This also sets up the duplicate Map where you will check to see if an inode is referenced more than once
void checkFreeInodes() {
    
    ifstream csvFile(fileName, ifstream::in);
    string curLine;
    vector<int> unallocatedInodes;
    //assume all inodes are originally unallocated
    for (int i = cur.first_inode; i < cur.inodes_per_group; i++) {
        unallocatedInodes.push_back(i);
        
    }
    
    while (csvFile.good()) {
        getline(csvFile, curLine);
        if (curLine.find("INODE") != string::npos) {
            stringstream next(curLine);
            int curInode;
            string allocated;
            //Get the inode number
            for (int i = 0; i < 2; i++) {
                getline(next, curLine, ',');
            }
            curInode = stoi(curLine);
            
            //Check and see if it's allocated
            getline(next, curLine, ',');
            allocated = curLine;
            
            
            if (allocated == "0") {
                if (find(freeInodes.begin(), freeInodes.end(), curInode) == freeInodes.end()) {
                    cout << "UNALLOCATED INODE " << curInode << " NOT ON FREELIST" << endl;
                    freeInodes.push_back(curInode);
                }
            } else {
                if (find(unallocatedInodes.begin(), unallocatedInodes.end(), curInode) != unallocatedInodes.end()) {
                    unallocatedInodes.erase(find(unallocatedInodes.begin(), unallocatedInodes.end(), curInode));
                   
                }
                
                if (find(freeInodes.begin(), freeInodes.end(), curInode) != freeInodes.end()) {
                    cout << "ALLOCATED INODE " << curInode << " ON FREELIST" << endl;
                    freeInodes.erase(find(freeInodes.begin(), freeInodes.end(), curInode));
                }
            }
            //Check the block array
            
            for (int i = 0; i < 9; i++) {
                getline(next, curLine, ',');
            }
            //Check the Direct Blocks
            for (int j = 0; j < 12; j++) {
                getline(next, curLine, ',');
                int curAddress = stoi(curLine);
                if (curAddress < 0 || curAddress > cur.block_count) {
                    cout << "INVALID BLOCK " << curAddress << " IN INODE " << curInode << " AT OFFSET " << j << endl;
                } //Put in else if for reserved as well
                else if (curAddress < 8 && curAddress != 0) {
                    cout << "RESERVED BLOCK " << curAddress << " IN INODE " << curInode << " AT OFFSET " << j << endl;
                }
                else {
                if (find(freeBlocks.begin(), freeBlocks.end(), curAddress) != freeBlocks.end()) {
                    cout << "ALLOCATED BLOCK " << curAddress << " ON FREELIST" << endl;
                }
                    if (curAddress != 0) {
                        if (find(unreferencedBlocks.begin(), unreferencedBlocks.end(), curAddress) != unreferencedBlocks.end())
                            unreferencedBlocks.erase(find(unreferencedBlocks.begin(), unreferencedBlocks.end(), curAddress));
                        duplicateBlock curDupBlock;
                        curDupBlock.inodeNum = curInode;
                        curDupBlock.offset = j;
                        curDupBlock.indirection = 0;
                        DuplicateMap[curAddress].push_back(curDupBlock);
                    }
                }
            }
            //Check the indirect blocks in here
            for (int l = 0; l < 3; l++) {
                getline(next, curLine, ',');
                int curAddress = stoi(curLine);
                
                if (l == 0) {
                    if (curAddress < 0 || curAddress > cur.block_count) {
                        cout << "INVALID INDIRECT BLOCK " << curAddress << " IN INODE " << curInode << " AT OFFSET 12" <<  endl;
                    } else if (curAddress < 8 && curAddress != 0) {
                        cout << "RESERVED INDIRECT BLOCK " << curAddress << " IN INODE " << curInode << " AT OFFSET 12" << endl;
                    }
                    //else if for reserved as well
                    else {
                        
                    if (find(freeBlocks.begin(), freeBlocks.end(), curAddress) != freeBlocks.end()) {
                        cout << "ALLOCATED BLOCK " << curAddress << " ON FREELIST" << endl;
                    } else if (curAddress != 0) {
                        
                        //Check the direct blocks in the direct block
                        checkSingleBlock(curInode, curAddress);
                        if (find(unreferencedBlocks.begin(), unreferencedBlocks.end(), curAddress) != unreferencedBlocks.end())
                            unreferencedBlocks.erase(find(unreferencedBlocks.begin(), unreferencedBlocks.end(), curAddress));
                        //Add it to the duplicate check list
                        duplicateBlock curDupBlock;
                        curDupBlock.inodeNum = curInode;
                        curDupBlock.offset = 12;
                        curDupBlock.indirection = 1;
                        DuplicateMap[curAddress].push_back(curDupBlock);
                    }
                    }
                    
                } else if (l == 1) {
                    //Doubly indirect block
                    if (curAddress < 0 || curAddress > cur.block_count) {
                        cout << "INVALID DOUBLE INDIRECT BLOCK " << curAddress << " IN INODE " << curInode << " AT OFFSET 268" << endl;
                    } //else if for reserved as well
                    else if (curAddress < 8 && curAddress != 0) {
                        cout << "RESERVED DOUBLE INDIRECT BLOCK " << curAddress << " IN INODE " << curInode << " AT OFFSET 268" << endl;
                    }
                    
                    else  {
                        
                        if (find(freeBlocks.begin(), freeBlocks.end(), curAddress) != freeBlocks.end()) {
                        cout << "ALLOCATED BLOCK " << curAddress << " ON FREELIST" << endl;
                    } else if (curAddress != 0) {
                        if (find(unreferencedBlocks.begin(), unreferencedBlocks.end(), curAddress) != unreferencedBlocks.end())
                            unreferencedBlocks.erase(find(unreferencedBlocks.begin(), unreferencedBlocks.end(), curAddress));
                        //Check the array of indirect blocks
                        checkDoubleBlock(curInode, curAddress);
                    
                        //Add to duplicate check list
                        duplicateBlock curDupBlock;
                        curDupBlock.inodeNum = curInode;
                        curDupBlock.offset = 268;
                        curDupBlock.indirection = 2;
                        DuplicateMap[curAddress].push_back(curDupBlock);
                    }
                    }
                } else {
                    //Triply indirect block
                    if (curAddress < 0 || curAddress > cur.block_count) {
                        cout << "INVALID TRIPLE INDIRECT BLOCK " << curAddress << " IN INODE " << curInode << " AT OFFSET 65804" << endl;
                    } // else if for reserved as well
                    else if (curAddress < 8 && curAddress != 0) {
                        cout << "RESERVED TRIPLE INDIRECT BLOCK " << curAddress << " IN INODE " << curInode << " AT OFFSET 65804" << endl;
                    }
                    else {
                        
                        
                        if (find(freeBlocks.begin(), freeBlocks.end(), curAddress) != freeBlocks.end()) {
                        cout << "ALLOCATED BLOCK " << curAddress << " ON FREELIST" << endl;
                    } else if (curAddress != 0) {
                        if (find(unreferencedBlocks.begin(), unreferencedBlocks.end(), curAddress) != unreferencedBlocks.end())
                            unreferencedBlocks.erase(find(unreferencedBlocks.begin(), unreferencedBlocks.end(), curAddress));
                        //Check the array of doubly indirect blocks
                        checkTripleBlock(curInode, curAddress);
                        //Add to the duplicate check array
                        duplicateBlock curDupBlock;
                        curDupBlock.inodeNum = curInode;
                        curDupBlock.offset = 65804;
                        curDupBlock.indirection = 3;
                        DuplicateMap[curAddress].push_back(curDupBlock);
                    }
                }
                }
                
            }
            
            
            
            
            
            
            
            
    }
        curLine = "";

    
}
    //Check for all unallocated inodes in the freelist
    for (unsigned long i = 0; i < unallocatedInodes.size(); i++) {
        if (find(freeInodes.begin(), freeInodes.end(), unallocatedInodes[i]) == freeInodes.end()) {
            cout << "UNALLOCATED INODE " << unallocatedInodes[i] << " NOT ON FREELIST" << endl;
            freeInodes.push_back(unallocatedInodes[i]);
        }
    }
}

//Single Allocation block check
//Also adds to the duplicateMap
void checkSingleBlock(int curInode, int block_num) {
    ifstream csvFile(fileName, ifstream::in);
    string curLine;

    while (csvFile.good()) {
        getline(csvFile, curLine);
        if (curLine.find("INDIRECT") != string::npos) {
            
            

            stringstream next(curLine);
            //Extract the inode and what kind of indirect it is
            for (int k = 0; k < 2; k++) {
                getline(next, curLine, ',');
            }
            int checkInode = stoi(curLine);
            getline(next, curLine, ',');
            int curDirection = stoi(curLine);
            int curOffset;
            //Move thru the first four to get to the current offset
            getline(next, curLine, ',');
            
            curOffset = stoi(curLine);
            
            getline(next, curLine, ',');
            int checkNum = stoi(curLine);
            
            
            //Check if valid
            if (checkNum == block_num && curDirection == 1 && checkInode == curInode) {
                
            int nextBlock;
           
                
                
            //Then obtain the nextBlock
                getline(next, curLine, ',');
                
            nextBlock = stoi(curLine);
                
                
            //Check if the nextBlock is valid or reserved
            if (nextBlock < 0 || nextBlock > cur.block_count) {
                cout << "INVALID  BLOCK " << nextBlock << " IN INODE " << curInode << " AT OFFSET " << curOffset << endl;
            } else if (nextBlock < 8 && nextBlock != 0) {
                cout << "RESERVED BLOCK " << nextBlock << " IN INODE " << curInode << " AT OFFSET " << curOffset << endl;
            }// else if for reserved as well
            
            else {
                
            if (find(freeBlocks.begin(), freeBlocks.end(), nextBlock) != freeBlocks.end()) {
                cout << "ALLOCATED BLOCK " << nextBlock << " ON FREELIST" << endl;
            }
                if (nextBlock != 0) {
                    if (find(unreferencedBlocks.begin(), unreferencedBlocks.end(), nextBlock) != unreferencedBlocks.end())
                        unreferencedBlocks.erase(find(unreferencedBlocks.begin(), unreferencedBlocks.end(), nextBlock));
                    //Add to the duplicate check map
                  //  cout << curInode << " " << block_num << " "  << nextBlock << endl;
                duplicateBlock curDupBlock;
                curDupBlock.inodeNum = curInode;
                curDupBlock.offset = curOffset;
                curDupBlock.indirection = 0;
                DuplicateMap[nextBlock].push_back(curDupBlock);
                }
                
        }
        }
            
        }
    }
}
//Double Block Check, also calls the single block check if the nextAddress != 0
void checkDoubleBlock(int curInode, int block_num) {
    ifstream csvFile(fileName, ifstream::in);
    string curLine;
    
    while (csvFile.good()) {
        getline(csvFile, curLine);
        if (curLine.find("INDIRECT") != string::npos ) {
            
            
            stringstream next(curLine);
            int curOffset, nextBlock;
            
            //Extract the inode and whether its single, double, or triply
            for (int k = 0; k < 2; k++) {
                getline(next, curLine, ',');
            }
            int checkInode = stoi(curLine);
            getline(next, curLine, ',');
            int curIndirection = stoi(curLine);
            
            getline(next, curLine, ',');
            
            curOffset = stoi(curLine);
            
            getline(next, curLine, ',');
            int checkNum = stoi(curLine);
            
            if (checkNum == block_num && curIndirection == 2 && checkInode == curInode) {
            //Move thru the first four to get to the current offset
               
            //Then obtain the nextBlock
                getline(next, curLine, ',');
                
            nextBlock = stoi(curLine);
            //Check if the nextBlock is valid or reserved
            if (nextBlock < 0 || nextBlock > cur.block_count) {
                cout << "INVALID INDIRECT BLOCK " << nextBlock << " IN INODE " << curInode << " AT OFFSET " << curOffset << endl;
            } else if (nextBlock < 8 && nextBlock != 0) {
                cout << "RESERVED INDIRECT BLOCK " << nextBlock << " IN INODE " << curInode << " AT OFFSET " << curOffset << endl;
            }// else if for reserved as well
            
            else {
                
                
                if (find(freeBlocks.begin(), freeBlocks.end(), nextBlock) != freeBlocks.end()) {
                    cout << "ALLOCATED BLOCK " << nextBlock << " ON FREELIST" << endl;
                } else if (nextBlock != 0) {
                    if (find(unreferencedBlocks.begin(), unreferencedBlocks.end(), nextBlock) != unreferencedBlocks.end())
                        unreferencedBlocks.erase(find(unreferencedBlocks.begin(), unreferencedBlocks.end(), nextBlock));
                    //Add to the duplicate check map
                    checkSingleBlock(curInode, nextBlock);
                    duplicateBlock curDupBlock;
                    curDupBlock.inodeNum = curInode;
                    curDupBlock.offset = curOffset;
                    curDupBlock.indirection = 1;
                    DuplicateMap[nextBlock].push_back(curDupBlock);
                }
                
                
                }
            
            }
        }
    }
}

//Triple block check, also calls the double block check
void checkTripleBlock(int curInode, int block_num) {
    ifstream csvFile(fileName, ifstream::in);
    string curLine;
    
    while (csvFile.good()) {
        getline(csvFile, curLine);
        if (curLine.find("INDIRECT") != string::npos) {
            stringstream next(curLine);
            int curOffset, nextBlock;
            for (int k = 0; k < 2; k++) {
                getline(next, curLine, ',');
            }
            int checkInode = stoi(curLine);
            getline(next, curLine, ',');
            int curIndirection = stoi(curLine);
            
            getline(next, curLine, ',');
            
            curOffset = stoi(curLine);
            
            getline(next, curLine, ',');
            int checkNum = stoi(curLine);
            if (checkNum == block_num && curIndirection == 3 && checkInode == curInode) {
            //Move thru the first four to get to the current offset
              
            //Then obtain the nextBlock
                getline(next, curLine, ',');
                
            nextBlock = stoi(curLine);
            //Check if the nextBlock is valid or reserved
            if (nextBlock < 0 || nextBlock > cur.block_count) {
                cout << "INVALID DOUBLE INDIRECT BLOCK " << nextBlock << " IN INODE " << curInode << " AT OFFSET " << curOffset << endl;
            } else if (nextBlock < 8 && nextBlock != 0) {
                cout << "RESERVED DOUBLE INDIRECT BLOCK " << nextBlock << " IN INODE " << curInode << " AT OFFSET " << curOffset << endl;
            } else  {
                
            if (find(freeBlocks.begin(), freeBlocks.end(), nextBlock) != freeBlocks.end()) {
                cout << "ALLOCATED BLOCK " << nextBlock << " ON FREELIST" << endl;
            } else if (nextBlock != 0) {
                if (find(unreferencedBlocks.begin(), unreferencedBlocks.end(), nextBlock) != unreferencedBlocks.end())
                    unreferencedBlocks.erase(find(unreferencedBlocks.begin(), unreferencedBlocks.end(), nextBlock));
                checkDoubleBlock(curInode, nextBlock);
                duplicateBlock curDupBlock;
                curDupBlock.inodeNum = curInode;
                curDupBlock.offset = curOffset;
                curDupBlock.indirection = 2;
                DuplicateMap[nextBlock].push_back(curDupBlock);
            }
            }
            
            
        }
        }
    }
}

//The following check is here:
//1. Duplicate inodes referenced by a file
void checkDuplicates() {
    //Iterate through all of keys
    for (map<int, vector<duplicateBlock> >::iterator it = DuplicateMap.begin(); it != DuplicateMap.end(); ++it) {
        int block_num = it->first;
        vector<duplicateBlock> curVec = it->second;
        
        
        //See if there are duplicates for a particular key
        if (curVec.size() > 1 && block_num != 0) {
            for (unsigned long i = 0; i < curVec.size(); i++) {
                duplicateBlock dupBlock = curVec[i];
                string indirectCount;
                //See if it's single, double, or triple
                switch (dupBlock.indirection) {
                case 1:
                    indirectCount = " INDIRECT";
                    break;
                case 2:
                    indirectCount = " DOUBLE INDIRECT";
                    break;
                case 3:
                    indirectCount = " TRIPLE INDIRECT";
                    break;
                default:
                    indirectCount = "";
                }
                cout << "DUPLICATE" << indirectCount << " BLOCK " << block_num << " IN INODE " << dupBlock.inodeNum << " AT OFFSET " << dupBlock.offset << endl;
            }
        }
    }
}
//This is where you check to see if the directories are valid. You also set up the check to see if the reference counts are valid
void checkDirectoryEntries() {
    ifstream csvFile(fileName, ifstream::in);

    string curLine;
    while (csvFile.good()) {
        getline(csvFile, curLine);
        if (curLine.find("DIRENT") != string::npos) {
            int curInode , referenceInode;
            string curDirName;
            stringstream next(curLine);
            //Extract CurInode
            for (int j = 0; j < 2; j++) {
                getline(next, curLine, ',');
            }
            curInode = stoi(curLine);
            for (int i = 0; i < 2; i++) {
                getline(next, curLine, ',');
            }
            referenceInode = stoi(curLine);
            for (int k = 0; k < 3; k++) {
                getline(next, curLine, ',');
            }
            curDirName = curLine;
            if (curDirName == "'.'") {
                if (curInode != referenceInode) {
                    cout << "DIRECTORY INODE " << curInode << " NAME " << curDirName << " LINK TO INODE " << referenceInode << " SHOULD BE " << curInode << endl;
                }
            } else if (curDirName == "'..'") {
                
                //If there are no inodes that are the parent of the current inode, the current inode is the current inodes parent
                bool noParent = true;
                for (vector<dirEntry>::iterator it = directoryEntries.begin(); it != directoryEntries.end(); ++it) {
                    dirEntry curDirEntry = *it;
                    if (curDirEntry.referenceInode == curInode && curDirEntry.name != "'.'" && curDirEntry.name != "'..'") {
                        //Once we confirm a parent, we shouldn't check for noParent
                        noParent = false;
                     if (curDirEntry.curInode != referenceInode) {
                        cout << "DIRECTORY INODE " << curInode << " NAME " << curDirName << " LINK TO INODE " << referenceInode << " SHOULD BE " << curDirEntry.curInode << endl;
                    }
                    }
                }
                //If there is no parent for the inode, see if the current inode is the referenced inode
                if (noParent) {
                    if (referenceInode != curInode) {
                        cout << "DIRECTORY INODE " << curInode << " NAME " << curDirName << " LINK TO INODE " << referenceInode << " SHOULD BE " << curInode << endl;
                    }
                }
            }
            //Check if the inode is valid and allocated based on modified free list
            if (referenceInode > cur.inodes_per_group || referenceInode < 1) {
                cout << "DIRECTORY INODE " << curInode << " NAME " << curDirName << " INVALID INODE " << referenceInode << endl;
            } else if (find(freeInodes.begin(), freeInodes.end(), referenceInode) != freeInodes.end()) {
                cout << "DIRECTORY INODE " << curInode << " NAME " << curDirName << " UNALLOCATED INODE " << referenceInode << endl;

            } else {
            newRefCount[referenceInode]++;
            }
        }
        curLine = "";
        
    }
    
}
       //This is where you check to see if the linkcount is valid
void checkReferenceCounts() {
    for (int i = 0; i < cur.inodes_per_group; i++) {
        if (referenceCounts[i] != newRefCount[i]) {
            cout << "INODE " << i << " HAS " << newRefCount[i] << " LINKS BUT LINKCOUNT IS " << referenceCounts[i] << endl;
        }
    }
}
//This is where you check to see the unreferenced blocks
void checkUnreferencedBlocks() {
    for (unsigned long  i = 0; i < unreferencedBlocks.size(); i++) {
        cout << "UNREFERENCED BLOCK " << unreferencedBlocks[i] << endl;
    }
}

            
            
            
//ACTUAL CHECKING END
            
//MAIN ROUTINE BEGIN



int main(int argc, char** argv) {
    
    //Get the general information to complete the tests
    extractFileName(argc, argv);
    
    extractSuperBlock();
    setUpUnreferenceCheck();
    
    //populate free inodes, free blocks, reference counts
    populateBlocksAndInodes();
    populateReferenceCounts();
    populateDirectoryEntries();
    checkFreeInodes();

    checkDuplicates();
    checkUnreferencedBlocks();
    checkDirectoryEntries();
    checkReferenceCounts();
}
            
//MAIN ROUTINE END


