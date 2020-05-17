// function definitions for the self referenced class SR_Directory
// to redesign the Univ V6 file system to remove 16mb file limit
// developer and owner of this program: Shrey S V (ssv170001), Jiten Girdhar (jxg170021), Sushant Singh Dahiya (ssd17005)
// created by alice_v3.0.4 on 12/01/18

#include <iostream>
#include <string>
#include <fstream>
#include "SR_Directory.h" // self referenced directory class
#include "cns.h" // file system structures, viz. super block, inode, directory

using namespace std;

SR_Directory* SR_Directory :: SR_Instance = NULL;

void SR_Directory :: file_entry(string entry,int node)
{
	entry_name[inode_count] = entry;
	inode_list[inode_count] = node;
	inode_count++;
	return;
}

SR_Directory* SR_Directory :: Instance()
{
   if (!SR_Instance) // if an instance exists, return that
		SR_Instance = new SR_Directory;
   return SR_Instance;
}

void SR_Directory :: dir_state()
{
	entry_name = new string[numInodes+1];
	inode_list = new int[numInodes];
	entry_name[inode_count] = path; // file system name for every directory, including root
	inode_list[inode_count]=1; // inode of root is 1
	inode_count++;
	entry_name[inode_count] = path;
	inode_list[inode_count]=1;
	inode_count++;
	return;
}

int SR_Directory :: get_free_inode(void)
{
	int freeInode=0;
	if(inode_count == getInodeNum()) freeInode = -1;
	else freeInode = inode_count;
	return freeInode;
}

void  SR_Directory :: quit(void)
{
	file_system.open(getPath().c_str(),ios::binary | ios::in | ios::out);
	if(file_system.is_open())
	{
		if(inode_count > 256) bigRoot();
		else smallRoot();
	}
	else cout << "Could not find the file system" << endl;
	file_system.clear();
	file_system.close();
	return;
}

void SR_Directory :: smallRoot()
{
	inode rootNode;
	Directory writeDir;
	file_system.seekg(BLOCK_SIZE,ios::beg);
	file_system.read((char *)&rootNode,sizeof(inode));
	rootNode.flags = (rootNode.flags | 0x9000);
	int nodeAllocated = 0,blocksNeeded;

	blocksNeeded = inode_count / max_dir_entry;
	if(inode_count % max_dir_entry !=0) blocksNeeded++;

	for(int i=0;i<8;i++)
	{
		for(int j=0;(j<max_dir_entry) && (nodeAllocated<=inode_count);j++)
		{
			rootNode.addr[j] =get_free_block();
			nodeAllocated+=32;
		}
	}

	nodeAllocated =0;
	for(int i=0;i<8;i++)
	{
		if(rootNode.addr[i] !=0)
		{
			file_system.seekg(rootNode.addr[i] * BLOCK_SIZE,ios::beg);
			for(int k; (k<=max_dir_entry) && (nodeAllocated++<=inode_count); k++)
			{
				writeDir.curr_inode_num=inode_list[inode_count];
				strcpy(writeDir.fileName,entry_name[inode_count].c_str());
				file_system.write((char *)&writeDir,sizeof(Directory));
			}
		}
		else break;
	}
	return;
}

void SR_Directory :: bigRoot()
{
	inode rootNode;
	Directory writeDir;
	file_system.seekg(BLOCK_SIZE,ios::beg);
	file_system.read((char *)&rootNode,sizeof(inode));
	rootNode.flags = (rootNode.flags | 0xD000);
	int nodeAllocated = 0,blocksNeeded,blockAllocated=0;

	blocksNeeded = inode_count / max_dir_entry;
	if(inode_count % max_dir_entry !=0) blocksNeeded++;

	int reqd_addr = blocksNeeded/131072;
	if(blocksNeeded % 131072!=0) reqd_addr++;

	//Writing inodes
	for(int i=0;i<8;i++) rootNode.addr[i] =get_free_block();
	file_system.seekg(BLOCK_SIZE,ios::beg);
	file_system.write((char *)&rootNode,sizeof(inode));

	unsigned short *assignBlocks;
	assignBlocks= new unsigned short[256];

	for(int i=0;i<reqd_addr;i++)
	{
		int j=0;
		for(;(j<256) && (blockAllocated<=blocksNeeded);j++)
		{
			assignBlocks[j] =get_free_block();
			blockAllocated+=32;
		}

		//add zeros if less than 256
		for(;j<256;j++)	assignBlocks[j] = 0;

		file_system.seekg(rootNode.addr[i] * BLOCK_SIZE,ios::beg);
		file_system.write((char *)&assignBlocks,256 * sizeof(unsigned short));
	}

	//Writing Double indirect blocks
	int begin = ((rootNode.addr[reqd_addr-1])-1);
	int end = last_used_block();
	assignBlocks= new unsigned short[256];
	blockAllocated = 0;
	int j=0;
	for(int i=begin; i>=end; i--)
	{
		for(;(j<256) && (blockAllocated<=blocksNeeded);j++)
		{
			assignBlocks[j]=get_free_block();
			//Writing data
			file_system.seekg(BLOCK_SIZE * assignBlocks[j],ios::beg);
			for(int k; (k<=max_dir_entry) && (nodeAllocated++<=inode_count); k++)
			{
				writeDir.curr_inode_num=inode_list[inode_count];
				strcpy(writeDir.fileName,entry_name[inode_count].c_str());
				file_system.write((char *)&writeDir,sizeof(Directory));
			}
			nodeAllocated++;
		}
		for(;j<256;j++) assignBlocks[j] = 0;
		file_system.seekg(i * BLOCK_SIZE,ios::beg);
		file_system.write((char *)&assignBlocks,256 * sizeof(unsigned short));
	}
	return;
}

void SR_Directory :: set_path(string loc)
{
	path = loc;
	return;
}

string SR_Directory :: getPath(void)
{
	return path;
}

void SR_Directory :: set_num_inode(int num)
{
	numInodes = num;
	return;
}

int SR_Directory :: getInodeNum(void)
{
	return numInodes;
}

int SR_Directory :: get_free_block(void)
{
	int free_block,free_chain_block;
	unsigned short freeHeadChain;
	super_block free_block_super;
	try
	{
		//Moving the cursor to the begin of the file
		file_system.seekg(0);

		//Reading the contents of super block
		file_system.read((char *)&free_block_super,sizeof(super_block));

		if(free_block_super.nfree == 1)
		{
			free_chain_block = free_block_super.free[--free_block_super.nfree];

			//Head of the chain points to next head of chain free list
			file_system.seekg(free_chain_block);
			file_system.read((char *)&freeHeadChain,2);

			//reset nfree to 1
			free_block_super.nfree = 100;

			//reset the free array to new free list
			for(int k=0;k<100;k++) free_block_super.free[k] = freeHeadChain + k;

			free_block = (int)free_block_super.free[--free_block_super.nfree];

			//Writing after the free head chain is changed
			file_system.write((char *)&free_block_super,sizeof(super_block));
		}
		else
		{
			free_block = (int)free_block_super.free[--free_block_super.nfree];
			//Writing after the free block has been allocated
			file_system.seekg(0,ios::beg);
			file_system.write((char *)&free_block_super,sizeof(super_block));
		}
	}
	catch(exception& e)
	{
		cout << "error get_free_block " << endl;
	}
	return free_block;
}

int SR_Directory :: last_used_block(void)
{
	int lastBlk;
	super_block free_block_super;
	try
	{
		//Moving the cursor to the begin of the file
		file_system.seekg(0);

		//Reading the contents of super block
		file_system.read((char *)&free_block_super,sizeof(super_block));

		lastBlk = (int)free_block_super.free[free_block_super.nfree];
	}
	catch(exception& e)
	{
		cout << "error get_free_block " << endl;
	}
	return lastBlk;
}