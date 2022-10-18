#include <iostream>
#include <math.h>
#include <chrono>
#include <vector>
#include <string>
#include <algorithm>
#include <bits/stdc++.h>
#include <fstream>
#include <sys/stat.h>

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

int import(void* mydisk, char* filename, unsigned short fat_links[], int fat_sectors)
{
    // Only read the irst sector initially
    jdisk_read(mydisk, 0, fat_links);

    long disk_size = jdisk_size(mydisk);

    // Getting a file size
    struct stat st;
    stat(filename, &st);
    long file_size = st.st_size;
    // This indicates whether the last sector is fully or partially occupied by a file.
    int non_full_sector = file_size % 1024;
    int buf_size = 1024 * ceil((double) file_size / (double)1024);

    if(file_size > disk_size)
    {
        fprintf(stderr, "%s (size %lu) is too big for disk of size %lu. Aborting.\n", filename, file_size, disk_size);
        return 2;
    }
    
    //printf("FILE SIZE %ld %d\n", file_size, buf_size);
    // Storing a file as a char array
    char file_buffer[buf_size];
    FILE* fp = std::fopen(filename, "r");
    char c;
    int i = 0;
    // standard C I/O file reading loop

    while ((c = std::fgetc(fp)) != EOF) 
    {
        file_buffer[i] = c;
        ++i;
    }
    
    // Place info about non full sector
    if(non_full_sector == 1023)
    {
        // Indicate we're only missing 1 byte
        file_buffer[buf_size - 1] =  0xff;
    }
    else if(non_full_sector)
    {
        // Indicate how many bytes we're using
        file_buffer[buf_size - 1] = ((non_full_sector & 0xff00) >> 8);
        file_buffer[buf_size - 2] = (non_full_sector & 0x00ff);

        //printf("BYTES OCCUPIED %d\n", non_full_sector);
        //printf("LAST BYTE %d\n", (non_full_sector & 0xff00) >> 8);

        //printf("PRE-LAST BYTE %d\n", (non_full_sector & 0x00ff));
    }
    
    int file_sectors = ceil((float) file_size / (float) 1024);

    // Walking through the fat sectors array and checking if there is enough space to fit the file.
    unsigned short iter = 0;
    int j = 0;

    // Keep track of what's been initialized
    int initialized_sectors[fat_sectors] = {};
    initialized_sectors[0] = 1;

    // Stop when the list ends or when we can fit the file.
    while(j != file_sectors)
    {

        iter = fat_links[iter];
        //printf("ITER: %d\n", iter);

        // Chek if sector has been initialized
        if (!initialized_sectors[iter / 512])
        {
            jdisk_read(mydisk, iter / 512, &fat_links[0] + 512 * (iter / 512));
            initialized_sectors[iter / 512] = 1;
        }

        if(iter == 0)
        {
            break;
        }
        ++j;
    }
    
    // List is over, no space for the file
    if(j != file_sectors)
    {
        // Nothing is changed, print the error message
        fprintf(stderr, "Not enough free sectors (%d) for %s, which needs %d.\n", j, filename, file_sectors);
        return 1;
    }

    // If we are here, the file can be fit. 
    // 2nd pass through the list using writes
    iter = 0;
    j = 0;
    printf("New file starts at sector %d\n", fat_links[0]);

    while(j != file_sectors)
    {
        iter = fat_links[iter];
        // Write into sector fat_sectors + iter - 1
        jdisk_write(mydisk, fat_sectors + iter - 1, &file_buffer[0] + j * 512);
        ++j;
    }

    // Update the first entry on the free list
    fat_links[0] = fat_links[iter];

    if(non_full_sector)
    {
        // If the file takes up a portion of a sector
        fat_links[iter] = iter;
    }
    else{
        // If the file takes up the entire sector
        fat_links[iter] = 0;
    }
    
    // Update fat links on a disk
    jdisk_write(mydisk, 0, fat_links);

    if(iter / 512 != 0)
    {
        jdisk_write(mydisk, iter / 512, &fat_links[0] + 512 * (iter / 512));
    }
    
    return 0;
}

int exprt(void* mydisk, char* filename, unsigned short fat_links[], int fat_sectors, int start_block)
{

    if(start_block == 0)
    {
        fprintf(stderr, "Error in Export: LBA is not for a data sector.\n");
        return 1;
    }
    int curr_link = start_block - fat_sectors + 1;

    // Initially read the sector associated with the link
    jdisk_read(mydisk, curr_link / 512, &fat_links[0] + 512 * (curr_link / 512));

    int prev_link;
    unsigned char buf[1024];
    int bytes_occupied = 1024;
    std::ofstream stream;
    FILE* fp = std::fopen(filename, "w");

    // Keep track of what's been initialized
    int initialized_sectors[fat_sectors] = {};
    initialized_sectors[0] = 1;

    while(curr_link != 0 && curr_link != prev_link)
    {   
        // Chek if sector has been initialized
        if (!initialized_sectors[curr_link / 512])
        {
            jdisk_read(mydisk, curr_link / 512, &fat_links[0] + 512 * (curr_link / 512));
            initialized_sectors[curr_link / 512] = 1;
        }
        
        // Read sector curr_link + S - 1 and put that in a file
        jdisk_read(mydisk, curr_link + fat_sectors - 1, buf);

        // Not occupying the full sector
        if(curr_link == fat_links[curr_link])
        {
            //printf("LAST BYTES %u %u\n", buf[1022], buf[1023]);

            // If the last byte of the sector is 255
            
            if(buf[1023] == 0xff)
            {
                //printf("HI\n");
                bytes_occupied = 1023;
            }
            else // check the last two bytes;
            {
                // Contcatenate the two and turn them into a number?
                bytes_occupied = buf[1022] | (buf[1023] << 8);
            } 
        }
        // Write "bytes_occupied" into the file
        for(int i = 0; i < bytes_occupied; ++i)
        std::fputc(buf[i], fp);

        // Advance
        prev_link = curr_link;
        curr_link = fat_links[curr_link];
    }

    return 0;
}

int main(int argc, char **argv){

    // Read the arguments    
    string filename;
    int start_block;
    
    // Create an empty disk
    Disk base;
    void* mydisk = &base;
    // Ugly af
    mydisk = jdisk_attach(argv[1]);

    long disk_size = jdisk_size(mydisk);

    long num_sectors = disk_size / 1024;
    long data_sectors = ((num_sectors * 512) - 1) / 513;
    long fat_sectors = num_sectors - data_sectors;

    // Need to read this from the disk
    unsigned short fat_links[512 * fat_sectors] = {};
    
    //FAT takes up (data_sectors + 1) * 2 bytes, hence it is
    //(data_sectors + 1) / 4 chars from the disk. Each entry in the fat_links is then
    //2 bytes. 

    //  xxd -s 0 -l 20 -g 2 -e t1.jdisk 
    // Supposed to print out FAT links
    
    /*
    for(int i = 0; i < 7000; ++i)
    {
        if (fat_links[i] < 0)
        {
            printf("%d: %u\n", i, fat_links[i]);
        }
    }

    printf("%u\n", fat_links[6711]);
    printf("%u\n", fat_links[49697]);
    */

    if (!strcmp(argv[2], "import"))
    {
        import(mydisk, argv[3], fat_links, fat_sectors);
    }
    else if (!strcmp(argv[2], "export"))
    {
        exprt(mydisk, argv[4], fat_links, fat_sectors,  std::stoi(argv[3]));
    }

    printf("Reads: %ld\n", jdisk_reads(mydisk));
    printf("Writes: %ld\n", jdisk_writes(mydisk));

    return 0;
}