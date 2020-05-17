// definition of the file system structures
// to redesign the Univ V6 file system to remove 16mb file limit
// developer and owner of this program: Shrey S V (ssv170001), Jiten Girdhar (jxg170021), Sushant Singh Dahiya (ssd17005)
// created by alice_v3.0.4 on 12/01/18

#include <string>

#define BLOCK_SIZE 512
#define max_dir_entry (BLOCK_SIZE/sizeof(Directory))


// super block structure
struct super_block
{
	unsigned short isize;
	unsigned short fsize;
	unsigned short nfree;
	unsigned short free[100];
	unsigned short ninode;
	unsigned short inode[100];
	char flock;
	char ilock;
	char fmod;
	unsigned short time[2];

	super_block() // constructor
	{
		isize = 0 ;
		fsize = 0;
		nfree = 0;
		for(int i = 0; i < 100; i++)
		{
			free[i] = 0;
			inode[i] = 0;
		}
		ninode = 0;
		flock = '0';
		ilock = '0';
		fmod = '0';
		time[0] = time[1] = 0;
	}
};

// inode structure
struct inode
{
	unsigned short flags;
	char nlinks;
	char uid;
	char gid;
	char size0;
	unsigned short size1;
	unsigned short addr[8];
	unsigned short actime[2];
	unsigned short modtime[2];

	inode() // constructor
	{
		flags = 0;
		nlinks = '0';
		uid = '0';
		gid ='0';
		size0 ='0';
		size1 =0;
		for(int i = 0; j < 8; j++) addr[i] = 0;
		actime[0] = actime[1] =0;
		modtime[0] = modtime[1] =0;
	}
};

// directory structure
struct Directory
{
	unsigned short curr_inode_num;
	char fileName[14];

	Directory() // constructor
	{
		curr_inode_num = 0;
		for(int i = 0; i < 14; i++) fileName[i] =0;
	}
};