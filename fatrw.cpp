#include <iostream>
#include <math.h>
#include <chrono>
#include <vector>
#include <string>
#include <algorithm>
#include <bits/stdc++.h>
#include <fstream>

#include "jdisk.h"

using std::string;
using std::ifstream;
using std::ios;

typedef struct {
  unsigned long size;  
  int fd;
  char *fn;
  int reads;
  int writes;
} Disk;

int import(string diskname, string filename, int fat_links[], int fat_sectors)
{
    // Read the file into a string, digure out its size
    std::ifstream t(filename);
    std::stringstream buffer;
    buffer << t.rdbuf();
    string file_data = buffer.str();
    int file_size = file_data.length();
    
    int file_sectors = ceil((float) file_size / (float) 1024);
    // This indicates whether the last sector is fully or partially occupied by a file.
    int non_full_sector = file_size % 1024;

    int fat_size = *(&fat_links + 1) - fat_links;


    // Walking through the fat sectors array and checking if there is enough space to fit the file;
    int iter = 0;
    int j = 0;
    int val = -1;

    while(val != 0)
    {
        val = fat_links[iter];
        ++j;
    }

    if(j != file_sectors)
    {
        // Nothing is changed, print the error message
        printf("Not enough free sectors (%d) for %s, which needs %d.", j, filename.c_str(), file_sectors);
        return 0;
    }

    // If we are here, the file can be fit. 
    // 2nd pass through the list using writes
    iter = 0;
    j = 0;
    // This will be used as a 1024 buffer 
    while(val != 0)
    {
        val = fat_links[iter];
        string buffer = file_data.substr(j, j + 1024 - 1);
        void * buf = &buffer;
        // Write into sector fat_sectors + val - 1
        //jdisk_write(diskname, fat_sectors + val - 1, buf);
    }

    return 1;
    // below is how this would have been implemented if FAT was sane and logical
    /*
    for(; iter < fat_size && j != file_sectors; ++iter)
    {
        if(fat_links[iter] == 0)
        ++j;
    }

    // Not enough space to fit the file on a disk
    if(j != file_sectors)
    {
        // Nothing is changed, print the error message
        printf("Not enough free sectors (%d) for %s, which needs %d.", j, filename.c_str(), file_sectors);
        return 0;
    }

    // If we're here, then we can fit the file on a disk

    int i = 0;
    j = 0;
    int prev_idx = 0;

    for(; i <= iter && j != file_sectors; ++i)
    {   
        // Check if the sector is free
        if(fat_links[i] == 0)
        {
            if(j == 0)
            {
                printf("New file starts at sector %d", j);
            }
            // If we're at a last needed empty sector, do the usual and set the next link to 0
            if (j == file_sectors - 1)
            {
                fat_links[prev_idx] = i;
                if(non_full_sector)
                {
                    // This means that the sector is partially occupied by a file
                    fat_links[i] = i;
                }
                else
                {
                    // Sector is fully occupied by a file
                    fat_links[i] == 0;
                }
            }
            // If we're occupying the sector, make sure to set the right link
            else if (j != 0)
            {
                fat_links[prev_idx] = i;
            }

            prev_idx = i;
            ++j;
        }
    }

    // Use the write to put the fat_links and the file on the disk
    */
}


int main(int argc, char **argv){

    // Read the arguments
    string diskname = argv[1];    
    string command  = argv[2];
    string filename;
    string start_block;

    if (!strcmp(command.c_str(), "import"))
    {
        filename = argv[3];
    }
    else if (!strcmp(command.c_str(), "export"))
    {
        start_block = argv[3];
        filename = argv[4];
    }
    
    // Create an empty disk
    Disk base;
    void* mydisk = &base;
    // Ugly af
    mydisk = jdisk_attach((char *) diskname.c_str());

    long disk_size = jdisk_size(mydisk);

    long num_sectors = disk_size / 1024;
    long data_sectors = ((num_sectors * 512) - 1) / 513;
    long fat_sectors = num_sectors - data_sectors;

    // Need to read this from the disk
    unsigned short fat_links[512] = {};
    
    //FAT takes up (data_sectors + 1) * 2 bytes, hence it is
    //(data_sectors + 1) / 4 chars from the disk. Each entry in the fat_links is then
    //2 bytes. 
    
    // Here, use read
    jdisk_read(mydisk, 0, fat_links);

    // Supposed to print out FAT links
    for(int i = 0; i < data_sectors + 1; ++i)
    {
        printf("%u\n", fat_links[i]);
    }

    return 0;
}