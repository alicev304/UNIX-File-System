// definition of self referenced class SR_Directory
// to redesign the Univ V6 file system to remove 16mb file limit
// developer and owner of this program: Shrey S V (ssv170001), Jiten Girdhar (jxg170021), Sushant Singh Dahiya (ssd17005)
// created by alice_v3.0.4 on 12/01/18

#include <string>
#include <cstring>

using namespace std;

class SR_Directory
{
private:
	static SR_Directory* SR_Instance;

public:
	string *entry_name;
	string path;
	int *inode_list;
	int inode_count,numInodes;
	fstream file_system;

	SR_Directory()
	{
		entry_name = NULL;
		path = "";
		inode_count = numInodes =0;
		inode_list = new int[0];
	}

	static SR_Directory* Instance(); //Public static access function

	void set_num_inode(int num);
	string getPath(void);
	int getInodeNum(void);
	int get_free_inode(void);
	void quit(void);
	void smallRoot(void);
	void dir_state(void);
	void file_entry(string,int);
	void set_path(string path);
	void bigRoot(void);
	int last_used_block(void);
	int get_free_block(void);
};