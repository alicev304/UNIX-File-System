// driver file
// to redesign the Univ V6 file system to remove 16mb file limit
// developer and owner of this program: Shrey S V (ssv170001), Jiten Girdhar (jxg170021), Sushant Singh Dahiya (ssd17005)
// created by alice_v3.0.4 on 12/01/18

#include <fstream>
#include <string>
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <cstring>
#include <stdlib.h>
#include <cstring>
#include "SR_Directory.h"
#include "cns.h"

using namespace std;

class Cpin
{
	fstream file_system,e_stream;
	string external, internal;
	int nblocks_needed,file_size;
public:
	bool validator(string input)
	{
		bool valid = false;
		string e_file,i_file;
		int space[2];
		space[0] = input.find(" ");
		space[1] = input.find(" ", space[0] + 1);
		space[2] = input.find(" ", space[1] + 1);
		e_file = input.substr(space[0] + 1, space[1] - space[0] - 1);
		i_file = input.substr(space[1] + 1, space[2] - space[1] - 1);

		if ((e_file.length() <=0) | (i_file.length()<=0) | (space[0]<=0) | (space[1]<=0))
		{
			cout <<"invalid parameter" << endl;
			valid = false;
		}
		else
		{
			set_ext_file(e_file);
			set_int_file(i_file);
			valid = true;
		}
		return valid;
	}


	void copy_to_file(string input,string path)
	{
		string src,des;
		try
		{
			if(validator(input))
			{
				src = get_ext_file();
				des = get_int_file();
				file_system.open(path.c_str(),ios::binary | ios::in | ios::out);
				if(file_system.is_open())
				{
					e_stream.open(get_ext_file().c_str(),ios::in | ios::ate);
					if(e_stream.is_open())
					{
						// handle small file
						if(is_small())
						{
							copy_small();
							cout << "file copied" << endl;
						}
						// handle large file
						else
						{
							if(copy_big())	cout << "file copied" << endl;
							else cout << "error" << endl;
						}
						e_stream.clear();
						e_stream.close();
					}
					else cout << "cannot find file" << endl;
				}
				else cout << "file system does not exist" << endl;
				file_system.clear();
				file_system.close();
			}
		 }
		 catch(exception& e)
		 {
			 cout << "error Cpin :: Copy File" << endl;
		 }
		 return;
	}

	void copy_small(void) // copy small file to V6
	{
		inode new_inode;
		int sInode;
		unsigned short lsb;
		char msb, temp;

		char *file_content = new char[file_size];
		e_stream.read(file_content, file_size);
		new_inode.flags = (new_inode.flags | 0x8000);
		// storing size
		lsb = (file_size & 0xffff);
		temp = (int)(file_size >> 16);
		msb = static_cast<unsigned int>(temp);
		new_inode.size1 = lsb;
		new_inode.size0 = msb;

		for(int i = 0; i < nblocks_needed; i++) new_inode.addr[i] = get_free_block();
		//Filling 0's for remaining addr array
		for(int i = nblocks_needed; i < 8; i++) new_inode.addr[i] = 0;
		//Writing inodes
		sInode = SR_Directory::Instance() -> get_free_inode();
		file_system.seekg(0, ios::beg);
		file_system.seekg(BLOCK_SIZE + (sInode * (int)sizeof(inode)));
		file_system.write((char *)&new_inode,sizeof(inode));

		file_system.seekg(0, ios::beg);
		file_system.seekg(BLOCK_SIZE * new_inode.addr[0]);
		file_system.write(file_content, file_size);

		SR_Directory::Instance() -> file_entry(get_int_file(),sInode);

		return;
	}

	bool copy_big(void) // copy large file to V6
	{
		bool large = false;
		inode new_inode;
		int curr_inode;
		unsigned short lsb;
		char msb, temp;
		int reqd_addr = nblocks_needed/131072;

		if(nblocks_needed % 131072 != 0) reqd_addr++;

		if(reqd_addr > 8)
		{
			cout << "size limit exceeded" << endl;
			large = false;
		}
		else
		{
			new_inode.flags = (new_inode.flags | 0x9000);
			for(int i = 0; i < reqd_addr; i++) new_inode.addr[i] = get_free_block();
			// fill 0's for remaining addr array
			for(int i = reqd_addr; i < 8; i++) new_inode.addr[i] = 0;

			lsb = (file_size & 0xffff);
			temp= (int)(file_size >> 16);
			if(temp < 256)	msb = static_cast<unsigned int>(temp);
			else
			{
				temp= (int)(file_size >> 15);
				msb = static_cast<unsigned int>(temp);
				new_inode.flags |= 0x9200;
			}
			new_inode.size1 = lsb;
			new_inode.size0 = msb;

			curr_inode = SR_Directory::Instance() -> get_free_inode();
			file_system.seekg(0, ios::beg);
			file_system.seekg(BLOCK_SIZE + (curr_inode * (int)sizeof(inode)));
			file_system.write((char *)&new_inode,sizeof(inode));

			int blocksAllocated = 0; 
			unsigned short *assignBlocks;
			assignBlocks= new unsigned short[256];

			for(int i = 0; i < reqd_addr; i++)
			{
				int j = 0;
				for(; (j < 256) && (blocksAllocated <= nblocks_needed); j++)
				{
					assignBlocks[j] = get_free_block();
					blocksAllocated+ = 512;
				}

				for(; j < 256; j++)	assignBlocks[j] = 0;

				file_system.seekg(new_inode.addr[i] * BLOCK_SIZE,ios::beg);
				file_system.write((char *)&assignBlocks,256 * sizeof(unsigned short));
			}

			char *file_content = new char[file_size];

			int begin = ((new_inode.addr[reqd_addr-1])-1);
			int end = last_used_block();
			assignBlocks= new unsigned short[256];
			blocksAllocated = 0;
			int j = 0;
			for(int i = begin; i >= end; i--)
			{
				for(; (j < 256) && (blocksAllocated <= nblocks_needed); j++)
				{
					assignBlocks[j] = get_free_block();
					file_system.seekg(BLOCK_SIZE * assignBlocks[j], ios::beg);
					e_stream.seekg(BLOCK_SIZE * j);
					e_stream.read(file_content, BLOCK_SIZE);
					file_system.write(file_content, BLOCK_SIZE);
					blocksAllocated++;
				}
				for(; j < 256; j++) assignBlocks[j] = 0;
				file_system.seekg(i * BLOCK_SIZE, ios::beg);
				file_system.write((char *)&assignBlocks, 256 * sizeof(unsigned short));
			}
			large = true;
		}
		SR_Directory::Instance() -> file_entry(get_int_file(), curr_inode);
		return large;
	}

	void set_ext_file(string name)
	{
		this -> external = name;
		return;
	}


	string get_ext_file(void)
	{
		return this -> external;
	}


	void set_int_file(string name)
	{
		this -> internal = name;
		return;
	}


	string get_int_file(void)
	{
		return this -> internal;
	}


	bool is_small(void)
	{
		bool result = false;
		file_size = e_stream.tellg();
		e_stream.seekg(0, ios::beg);
		if(file_size == 0) cout <<"file empty" << endl;
		else
		{
			nblocks_needed = file_size / BLOCK_SIZE;
			if((file_size % BLOCK_SIZE) != 0) nblocks_needed++ ;
			if(file_size <= 4096) result = true;
			else result = false;
		}
		return result;
	}

	int get_free_block(void)
	{
		int free_block,free_chain_block;
		unsigned short freeHeadChain;
		super_block free_block_super;
		try
		{
			file_system.seekg(0);
			file_system.read((char *)&free_block_super,sizeof(super_block));

			if(free_block_super.nfree == 1)
			{
				free_chain_block = free_block_super.free[--free_block_super.nfree];
				file_system.seekg(free_chain_block);
				file_system.read((char *)&freeHeadChain,2);

				free_block_super.nfree = 100;

				for(int i = 0; i < 100; i++) free_block_super.free[i] = freeHeadChain + i;

				free_block = (int)free_block_super.free[--free_block_super.nfree];

				file_system.write((char *)&free_block_super,sizeof(super_block));
			}
			else
			{
				free_block = (int)free_block_super.free[--free_block_super.nfree];
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

	int last_used_block(void)
	{
		int lastBlk;
		super_block free_block_super;
		try
		{
			file_system.seekg(0);

			file_system.read((char *)&free_block_super,sizeof(super_block));

			lastBlk = (int)free_block_super.free[free_block_super.nfree];
		}
		catch(exception& e)
		{
			cout << "error get_free_block " << endl;
		}
		return lastBlk;
	}
};

class Mkdir 
{
	string dir_name;
	fstream file_system;
public:

	void create_directory(string input,string path)
	{
		if(validator(input))
		{
			file_system.open(path.c_str(),ios::binary | ios::in | ios::out);
			if(file_system.is_open())
			{
				if(search_dir_entry())
				{
					cout << "directory already exist" << endl;
					return;
				}
				else
				{
					alloc_dir_entry();
					cout << "new directory created" << endl;
				}
				file_system.clear();
				file_system.close();
			} 
			else cout << "file system does not exist" << endl;
		}
		return;
	}

	bool validator(string input)
	{
		bool valid = false;
		string name;
		int space[1];
		space[0] = input.find(" ");
		space[1] = input.find(" ",space[0] + 1);
		name = input.substr(space[0] + 1,space[1] - space[0] - 1);
		if((name.length() <= 0) | (space[0] <= 0))
		{
			cout << "invalid argument" << endl;
			valid = false;
		}
		else if(name.size() > 13)
		{
			cout << "directory name must be less than 14 char" << endl;
			set_dir_name(name);
			valid = true;
		}
		else
		{
			set_dir_name(name);
			valid = true;
		}
		return valid;
	}


	bool search_dir_entry()
	{
		bool found = false;
		int i=0;
		for(;i<=SR_Directory::Instance()->inode_count;i++)
		{
			if(SR_Directory::Instance()->entry_name[i].compare(get_dir_name())==0) 
			{
				found = true;
				break;
			}
		}
		if(i>=SR_Directory::Instance()->inode_count)
		{
			found = false;
			return found;
		}
		return found;
	}

	void alloc_dir_entry()
	{
		inode new_inode;
		Directory root_dir,curr_dir,filler;
		int dnode;
		new_inode.flags = (new_inode.flags | 0x9000);
		dnode =  SR_Directory::Instance()->get_free_inode();
		SR_Directory::Instance()->file_entry(get_dir_name(),dnode);

		file_system.seekg(0,ios::beg);
		file_system.seekg(BLOCK_SIZE + (dnode * (int)sizeof(inode)));
		file_system.write((char *)&new_inode,sizeof(inode));

		file_system.seekg(0,ios::beg);
		file_system.seekg(BLOCK_SIZE * new_inode.addr[0]);

		root_dir.curr_inode_num=1;
		strcpy(root_dir.fileName,"..");
		file_system.write((char *)&root_dir,sizeof(root_dir));

		string name = get_dir_name();
		curr_dir.curr_inode_num=dnode;
		strcpy(curr_dir.fileName,name.c_str());
		file_system.write((char *)&root_dir,sizeof(root_dir));

		for(int j=3; j<=max_dir_entry; j++){
			file_system.write((char *)&filler,sizeof(filler));
		}
		return;
	}

	void set_dir_name(string name)
	{
		this -> dir_name = name;
		return;
	}


	string get_dir_name(void)
	{
		return this -> dir_name;
	}
};

class Cpout
{
	fstream file_system,e_stream;
	string external, internal;
	int blocks;
	long file_size;

public:
	bool validator(string input)
	{
		bool valid = false;
		string e_file,i_file;
		int space[2];
		space[0] = input.find(" ");
		space[1] = input.find(" ",space[0]+1);
		space[2] = input.find(" ",space[1]+1);
		i_file = input.substr(space[0]+1,space[1]-space[0]-1);
		e_file = input.substr(space[1]+1,space[2]-space[1]-1);

		if ((e_file.length() <=0) | (i_file.length()<=0) | (space[0]<=0) | (space[1]<=0) )
		{
			cout <<"invalid parameters" << endl;
			valid = false;
		}
		else
		{
			set_int_file(i_file);
			set_ext_file(e_file);
			valid = true;
		}
		return valid;
	}

	void copy_from_file(string input,string path)
	{
		string src,des;
		try
		{
			if(validator(input))
			{
				src = get_int_file();
				des = get_ext_file();
				file_system.open(path.c_str(),ios::binary | ios::in );
				if(file_system.is_open())
				{
					if(search_file())
					{
						cout <<"file copied" << endl;
					}
					else cout <<"cannot find the file"  << endl;
				}
				else cout << "cannot find the file" << endl;
				file_system.clear();
				file_system.close();
			}
		 }
		 catch(exception& e)
		 {
			 cout <<"error Cpin :: Copy File" << endl;
		 }
		 return;
	}

	bool search_file()
	{
		bool found = false;
		string i_file = get_int_file();
		string e_file = get_ext_file();

		int i = 0,begin_addr;

		for(; i <= SR_Directory::Instance() -> inode_count; i++)
			if(SR_Directory::Instance() -> entry_name[i].compare(i_file) == 0) break;

		if(i >= SR_Directory::Instance() -> inode_count)
		{
			found = false;
			return found;
		}

		int v6_inode = SR_Directory::Instance() -> inode_list[i];

		inode v_inode;
		file_system.seekg(BLOCK_SIZE + (v6_inode*(int)sizeof(inode)),ios::beg);
		file_system.read((char *)&v_inode,sizeof(inode));

		if((v_inode.flags & 0x9200) == 0x9200) file_size =((v_inode.flags & 0x0040)| v_inode.size0 <<15 | v_inode.size1);
		else if((v_inode.flags & 0x8000) == 0x8000) file_size =(v_inode.size0 <<16 | v_inode.size1);
		else
		{
			found = false;
			return found;
		}
		cal_blocks(file_size);
		if(file_size <= 4096) begin_addr = v_inode.addr[0];
		else
		{
			unsigned short *one,*two;
			one = new unsigned short[256];
			two = new unsigned short[256];
			file_system.seekg(v_inode.addr[0]*BLOCK_SIZE,ios::beg);
			file_system.read((char *)&one,256  * sizeof(unsigned short));
			file_system.seekg(one[0]*BLOCK_SIZE,ios::beg);
			file_system.read((char *)&two,256  * sizeof(unsigned short));
			begin_addr = two[0];
			found = true;
		}

		e_stream.open(e_file.c_str(),ios::out | ios::app);
		int blk = get_blocks_read();
		if(e_stream.is_open())
		{
			for(int m=0;m<blk;m++)
			{
				file_system.seekg(BLOCK_SIZE*begin_addr,ios::beg);
				char *buffer = new char[BLOCK_SIZE];
				file_system.read(buffer,BLOCK_SIZE);
				e_stream.seekg(m * BLOCK_SIZE,ios::beg);
				cout <<e_stream.tellg() << endl;
				e_stream.write(buffer,BLOCK_SIZE);
				begin_addr--;
			}
			found = true;
			e_stream.close();
		}
		return found;
	}

	// set blocks to read
	void set_blocks_read(long n)
	{
		this -> blocks = n;
		return;
	}

	//calculate blocks needed for the file
	void cal_blocks(long fsize)
	{
		long size;
		size = fsize/BLOCK_SIZE;
		if(fsize % BLOCK_SIZE !=0) size++;
		this -> set_blocks_read(size);
		return;
	}

	int get_blocks_read()
	{
		return this -> blocks;
	}

	void set_ext_file(string name)
	{
		this -> external = name;
		return;
	}

	string get_ext_file(void)
	{
		return this -> external;
	}

	void set_int_file(string name)
	{
		this -> internal = name;
		return;
	}

	string get_int_file(void)
	{
		return this -> internal;
	}
};


class Rm
{
    fstream file_system;
	string v6_file;
	int blocks,file_size;

public:

	void removeFile(string input, string path)
	{
		v6_file = (string)input.substr(input.find(" ")+1, input.length());
        file_system.open(path.c_str(),ios::binary | ios::in | ios::out);
        if(file_system.is_open())
        {
	        int i=0;
	        //search for the inode list, to check it's name
			for(;i<=SR_Directory::Instance()->inode_count;i++)
				if(SR_Directory::Instance()->entry_name[i].compare(v6_file)==0) break;
	        //file name does not exist
			if(i>=SR_Directory::Instance()->inode_count)
				cout <<"cannot find file" << endl;

			//get inode flag, write the first bit to 0, so it is removed
			int v6_inode = SR_Directory::Instance()->inode_list[i];

			inode v_inode;
			inode v_inode2;
			file_system.seekg(BLOCK_SIZE + (v6_inode*(int)sizeof(inode)),ios::beg);
			file_system.read((char *)&v_inode,sizeof(inode));
			v_inode.flags = v_inode.flags&0x7FFF;
			file_system.seekg(0,ios::beg);
			file_system.seekg(BLOCK_SIZE + (v6_inode*(int)sizeof(inode)));
			file_system.write((char *)&v_inode,sizeof(inode));
        }
        file_system.clear();
        file_system.close();
        cout <<"file removed" << endl;
	}

};



class InitializeFS 
{
	fstream file;
	string fsPath;
	int num_blocks,num_inodes;

public:

	bool validator(string input)
	{
		bool valid=false;
		int inodes,blocks;
		string path;
		int space[2];
		space[0] = input.find(" ");
		space[1] = input.find(" ",space[0]+1);
		space[2] = input.find(" ",space[1]+1);
		path = input.substr(space[0]+1,space[1]-space[0]-1);
		blocks = atoi(input.substr(space[1]+1,space[2]-space[1]-1).c_str());
		inodes = atoi(input.substr(space[2]+1,input.length()-1).c_str());

		if ((blocks <= 0) | (inodes <=0) | (path.length()<=0) | (space[0]<=0) | (space[1]<=0) | (space[2]<=0) )
		{
			cout <<"invalid parameters" << endl;
			valid = false;
		}
		else 
		{
			this -> set_system_path(path);
			this -> set_num_blocks(blocks);
			this -> set_num_inodes(inodes);
			valid = true;
		}
		return valid;
	}
	
	void set_system_path(string path)
	{
        this -> fsPath = path;
        return;
	}

	string get_system_path(void)
	{
		return this -> fsPath;
	}

	void set_num_blocks(int n1)
	{
		this -> num_blocks = n1;
		return;
	}
	
	int get_num_blocks(void)
	{
		return this -> num_blocks;
	}

	void set_num_inodes(int n2)
	{
		this -> num_inodes = n2;
		return;
	}

	int get_num_inodes(void)
	{
		return this -> num_inodes;
	}

	int get_inode_size(void)
	{
		return sizeof(inode);
	}
	
	void create_file_system(string input)
	{
		super_block sb;
		inode node,rootNode;
		Directory root_dir,filler;
		unsigned short freeHeadChain;
		try
		{
			if(validator(input))
			{
			 	file.open(get_system_path().append("fsaccess").c_str(),ios::binary | ios::app);
				if (file.is_open())
				{
					sb.isize = get_inode_block();

					sb.fsize = get_free_block();
					sb.ninode = get_num_inodes();
					sb.nfree = 100;
					for(int i=0;i<100;i++)
						sb.free[i] = (get_free_block_index() + i);
					file.write((char *)&sb,BLOCK_SIZE);

					rootNode.flags = 0x1800;
					rootNode.addr[0] = (1+ get_inode_block())*BLOCK_SIZE;
					file.write((char *)&rootNode,get_inode_size());

					for(int i=2;i<=get_num_inodes();i++)
						file.write((char *)&node,get_inode_size());

					if(cal_inode_padding() !=0 )
					{
						char *inodeBuffer = new char[cal_inode_padding()];
						file.write((char *)&inodeBuffer,cal_inode_padding());
					}

					SR_Directory::Instance()->set_num_inode(get_num_inodes());
					SR_Directory::Instance()->set_path(get_system_path().append("fsaccess"));
					SR_Directory::Instance()->dir_state();

					root_dir.curr_inode_num=1;
					strcpy(root_dir.fileName,".");
					file.write((char *)&root_dir,sizeof(root_dir));

					root_dir.curr_inode_num=1;
					strcpy(root_dir.fileName,"..");
					file.write((char *)&root_dir,sizeof(root_dir));

					for(int j=3; j<=max_dir_entry; j++) file.write((char *)&filler,sizeof(filler));

					char *buffer = new char[BLOCK_SIZE];
					char *chain_buffer_head;

					if((get_free_block() - get_free_block_index()) < 100)
						for(int i=get_free_block_index();i<=get_free_block();i++)
							file.write((char *)&buffer,BLOCK_SIZE);
					else
					{
						for(int i=get_free_block_index();i<=get_free_block();i++)
						{
							if((i % 100 == get_free_block_index()) && ((get_free_block() - i) >= 100) )
							{
									freeHeadChain = i+100;
									file.write((char *)&freeHeadChain,2);
									chain_buffer_head = new char[BLOCK_SIZE - 2];
									file.write((char *)&chain_buffer_head,BLOCK_SIZE - 2);
							}
							else file.write((char *)&buffer,BLOCK_SIZE);
						}
					}
					cout <<"file system created" << endl;
				}
				else cout << "invalid path" << endl;
		 		file.close();
			}
			else return;
		}
		catch(exception& e)
		{
			cout <<"error create_file_system method" << endl;
		}
	}

	int get_inode_block(void)
	{
		float inode_size, n_inode_block, n_inodes,block_size;
		int inPB;
		inode_size = get_inode_size();
		n_inodes = get_num_inodes();
		block_size = BLOCK_SIZE;
		n_inode_block = (inode_size * n_inodes)/block_size;
		inPB = ceil(n_inode_block);
		return inPB;
	}

	int get_free_block(void)
	{
		int n_free_block;
		n_free_block = (get_num_blocks() - (get_inode_block() + 2));
		return n_free_block;
	}

	int get_free_block_index(void)
	{
		int ind_free_block;
		ind_free_block = get_num_blocks() - get_free_block() + 1;
		return ind_free_block;
	}
	
	int cal_inode_padding(void)
	{
		int padding_size;
		padding_size =  (get_inode_block() * BLOCK_SIZE) - (get_num_inodes() * get_inode_size());
		return padding_size;
	}

	void readBlocks(void)
	{
	    inode node;
		ifstream infile;
		infile.open("fsaccess",ios::binary);
		if(infile.is_open())
		{
			infile.seekg(BLOCK_SIZE);
			infile.read((char *)&node,get_inode_size());
			for(int k=0;k<8;k++) cout <<"addr = " << node.addr[k] << endl;
		}
		infile.close();
	}
};

bool check_command(string str);
int main() {
	string input,cmd,path;
	bool quit = false;

	InitializeFS fs;
	Cpin in;
	Mkdir mkd;
	Cpout out;
	Rm rm;

	cout <<"Commands:" << endl;
	cout <<"initfs:initfs path inodes blocks" << endl;
	cout <<"cpin:cpin external internal" << endl;
	cout <<"cpout:cpout internal external" << endl;
	cout <<"mkdir: mkdir path" << endl;
	cout <<"rm: rm filename" << endl;
	cout <<"Press 'q' to quit" << endl;

	while(!quit)
	{
		getline(cin,input);

		if(check_command(input))
		{
            string cmd = input.substr(0,input.find(" "));

            if(cmd == "initfs")
            {
                fs.create_file_system(input);
                path = (fs.get_system_path()).append("fsaccess");
                quit=false;
            }
            else if(cmd == "cpin")
            {
                in.copy_to_file(input,path);
                quit = false;
            }
            else if(cmd == "cpout")
            {
                out.copy_from_file(input,path);
                quit = false;
            }
            else if(cmd == "mkdir")
            {
                mkd.create_directory(input,path);
                quit = false;
            }
            else if(cmd == "q")
            {
                cout <<"Goodbye!" << endl;
                quit = true;
            }
            else if(cmd =="rm")
            {
                rm.removeFile(input,path);
                quit = false;
            }
		}
		else cout << "invalid" << endl;
	}
	return 0;
}

//check to see if inputs are supported
bool check_command(string str)
{
    string cmd;
	cmd = str.substr(0,str.find(" "));
	if(cmd=="initfs" || cmd=="cpin" || cmd=="cpout" || cmd=="mkdir" || cmd=="q" || cmd=="rm") return true;
	return false;
}