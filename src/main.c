//
//  main.m
//  listKEXTS-MachoHfromKernel_64
//
//  Created by huke on 4/28/16.
//  Copyright (c) 2016 com.cocoahuke. All rights reserved.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mach-o/loader.h>
#include <mach/machine.h>
#include <mach-o/fat.h>

char *mh_Magic = NULL;
char Magic64[] = {0xcf,0xfa,0xed,0xfe};
char Magic32[] = {0xce,0xfa,0xed,0xfe};

int real_kext;
int numberOfKextGotName;
uint64_t current_exporTsize = 0;

char *KextGetBundleID(void *bin);

uint64_t machoGetVMAddr(uint8_t firstPage[4096],char *segname,char *sectname);
uint64_t machoGetFileAddr(uint8_t firstPage[4096],char *segname,char *sectname);
uint64_t machoGetSize(uint8_t firstPage[4096],char *segname,char *sectname);

void checkMachO(const char *pathOfkernelFile,uint64_t *fileoff,uint64_t *filesize);
void getPerMachoHeaderInfo(void *firstPage);

int check_file_exist(const char *path){
    if(!access(path,F_OK)){
        if(!access(path,R_OK)){
            return 0;
        }
        return -1;
    }
    return -1;
}

int check_file_able_to_write(const char *path){
    if(!access(path,F_OK)){
        printf("%s already have same name file here\n",path);
        return -1;
    }
    return 0;
}

void usage(){
    printf("Usage: ioslistkexts <kernel file path> [-i <export item index> <export kext path>]\n\n");
}

int main(int argc, const char * argv[]) {
    
    const char *ker_path = NULL;
    int export_index = -1;
    const char *export_path = NULL;
    
    if(argc==1){
        printf("wrong args\n");usage();exit(1);
    }
    
    for(int i=0;i<argc;i++){
        if(!strcmp(argv[i],"-h")){
            usage();exit(1);
        }
        if(!strcmp(argv[i],"-i")){
            export_index = (i=i+1)>=argc?-1:(int)strtoull(argv[i],0,10);
            export_path = (i=i+1)>=argc?NULL:argv[i];
            if(export_index<0){
                printf("please specify export_index correctly\n"); exit(1);
            }
            if(check_file_able_to_write(export_path)){
                printf("export_path is not able to write\n"); exit(1);
            }
        }
    }
    
    if(check_file_exist(argv[1])){
        printf("kernel file is inexistent or not able to read\n");exit(1);
    }
    ker_path = argv[1];
    
    if(!ker_path){
        printf("kernel path not able to read\n"); exit(1);
    }
    
    numberOfKextGotName = 0;
    
    uint64_t fileoff = 0;
    uint64_t filesize = 0;
    checkMachO(ker_path,&fileoff,&filesize);
    
    FILE *fp_open = fopen(ker_path,"r");
    if(!fp_open){
        printf("file isn't exist\n");
        exit(1);
    }
    
    if(fileoff<=0||filesize<=0)
        exit(1);
    
    void *seg_kexts = malloc(filesize);
    if(fseek(fp_open,fileoff,SEEK_SET))
        exit(1);
    if(fread(seg_kexts,1,filesize,fp_open)!=filesize){
        printf("fread error\n");
        exit(1);
    }
    
    fclose(fp_open);
    
    uint64_t per_mh = (uint64_t)memmem(seg_kexts,filesize,mh_Magic,0x4);
    uint64_t last_size = 0;
    int i = 0;
    real_kext = 0;
    while(1) {
        current_exporTsize = 0;
        if(!per_mh)
            break;
        (last_size?1:last_size)?printf("interval size:0x%llx\n\n",(uint64_t)per_mh-(uint64_t)last_size):0;
        
        printf("%d items: ",i);
        printf("kr_fileoff:0x%llx ",(uint64_t)fileoff+((uint64_t)per_mh-(uint64_t)seg_kexts));
        //printf("per_mh is 0x%llx, per_mh+4: 0x%llx, per_size(大小): 0x%llx\n",per_mh,per_mh+4,filesize-(per_mh-(uint64_t)seg_kexts+4));
        getPerMachoHeaderInfo((void*)per_mh);
        if(export_index>=0&&current_exporTsize&&i==export_index){
            printf("---Export item %d---\n",i);i++;
            goto act_export;
            //NSData *data = [NSData dataWithBytes:per_mh length:0x1000];
            //[data writeToFile:@"/Desktop/Kext_AGX.mh" atomically:YES];
            break;
        }
        else if(export_index>=0&&!current_exporTsize&&i==export_index){
            last_size = per_mh;
            per_mh = (uint64_t)memmem((const void *)per_mh+4,filesize-(per_mh-(uint64_t)seg_kexts+4),mh_Magic,0x4);
            (last_size?1:last_size)?printf("interval size:0x%llx\n",(uint64_t)per_mh-(uint64_t)last_size):0;
            printf("---Export item %d---\n",i);i++;
            current_exporTsize = (uint64_t)per_mh-(uint64_t)last_size;
            goto act_export;
            break;
        }
        
        if(1+1==3){
        act_export:
            printf("export size:0x%llx\n",current_exporTsize);
            FILE *saved_fp = fopen(export_path, "w");
            if(!saved_fp)do{printf("fopen failed(%s)\n",export_path);exit(1);}while(0);
            if(fwrite((void*)per_mh,1,current_exporTsize,saved_fp)!=current_exporTsize){
                printf("fwrite failed\n");
                unlink(export_path);
                exit(1);
            }
            printf("saved successful in %s\n",export_path);
            fflush(saved_fp);
            printf("---Export end---\n");
            break;
        }
        
        last_size = per_mh;
        per_mh = (uint64_t)memmem((const void *)per_mh+4,filesize-(per_mh-(uint64_t)seg_kexts+4),mh_Magic,0x4);
        i++;
    }

    free(seg_kexts);
    printf("---prog end---\n");
    return 0;
}

void getPerMachoHeaderInfo(void *firstPage){
    
    struct mach_header *mh = (struct mach_header*)firstPage;
    
    int is32 = 0;
    if(mh->magic==MH_MAGIC||mh->magic==MH_CIGAM){
        is32 = 1;
        mh_Magic = Magic32;
    }
    else if(mh->magic==MH_MAGIC_64||mh->magic==MH_CIGAM_64){
        is32 = 0;
        mh_Magic = Magic64;
    }
    
    const uint32_t cmd_count = mh->ncmds;
    
    if(mh->flags!=1){
        //内核扩展有些是无效的,开头几个KPI,可以通过flags判断
        return;
    }
    
    struct load_command *cmds = (struct load_command*)((char*)firstPage+(is32?sizeof(struct mach_header):sizeof(struct mach_header_64)));
    struct load_command* cmd = cmds;
    for (uint32_t i = 0; i < cmd_count; ++i){
        //printf("\n");
        switch (cmd->cmd) {
            case LC_SEGMENT:
            {
                struct segment_command *seg = (struct segment_command*)cmd;
                //printf("LC_SEGMENT name:%s\n",seg->segname);
                if(!strcmp(seg->segname,"__TEXT")){
                    
                    real_kext++;
                    printf(" (Effective kext:%d)\n",real_kext);//有效存在的内核扩展
                    KextGetBundleID(firstPage);
                    //printf("|vmsize:0x%x\n",seg->vmsize);
                    //printf("|fileoff:0x%x\n",seg->fileoff);
                    //printf("|filesize:0x%x\n",seg->filesize);
                }
                
                if(!strcmp(seg->segname,"__LINKEDIT")){
                    current_exporTsize = seg->fileoff+seg->filesize;
                    printf("macho file size:0x%llx\n",current_exporTsize);
                }
            }
                break;
            case LC_SEGMENT_64:
            {
                struct segment_command_64 *seg = (struct segment_command_64*)cmd;
                //printf("LC_SEGMENT name:%s\n",seg->segname);
                if(!strcmp(seg->segname,"__TEXT")){
                    
                    real_kext++;
                    printf(" (Effective kext:%d)\n",real_kext);//有效存在的内核扩展
                    KextGetBundleID(firstPage);
                    //printf("|vmsize:0x%x\n",seg->vmsize);
                    //printf("|fileoff:0x%x\n",seg->fileoff);
                    //printf("|filesize:0x%x\n",seg->filesize);
                }
                
                if(!strcmp(seg->segname,"__LINKEDIT")){
                    current_exporTsize = seg->fileoff+seg->filesize;
                    printf("macho file size:0x%llx\n",current_exporTsize);
                }
            }
                break;
        }
        cmd = (struct load_command*)((char*)cmd + cmd->cmdsize);
    }
}

void checkMachO(const char *pathOfkernelFile,uint64_t *fileoff,uint64_t *filesize){

    FILE *fp_open = fopen(pathOfkernelFile,"r");
    if(!fp_open){
        printf("file isn't exist\n");
        exit(1);
    }
    uint8_t firstPage[4096];
    if(fread(firstPage,1,4096,fp_open)!=4096){
        printf("fread error\n");
        exit(1);
    }
    
    fclose(fp_open);
    
    struct mach_header *mh = (struct mach_header*)firstPage;
    
    int is32 = 0;
    if(mh->magic==MH_MAGIC||mh->magic==MH_CIGAM){
        is32 = 1;
        mh_Magic = Magic32;
    }
    else if(mh->magic==MH_MAGIC_64||mh->magic==MH_CIGAM_64){
        is32 = 0;
        mh_Magic = Magic64;
    }
    
    const uint32_t cmd_count = mh->ncmds;
    struct load_command *cmds = (struct load_command*)((char*)firstPage+(is32?sizeof(struct mach_header):sizeof(struct mach_header_64)));
    struct load_command* cmd = cmds;
    for (uint32_t i = 0; i < cmd_count; ++i){
        switch (cmd->cmd) {
            case LC_SEGMENT:
            {
                struct segment_command *seg = (struct segment_command*)cmd;
                //printf("LC_SEGMENT name:%s\n",seg->segname);
                if(!strcmp(seg->segname,"__PRELINK_TEXT")){
                    //printf("|size:0x%x\n",cmd->cmdsize);
                    //printf("|vmaddr:0x%x\n",seg->vmaddr);
                    //printf("|vmsize:0x%x\n",seg->vmsize);
                    //printf("|fileoff:0x%x\n",seg->fileoff);
                    //printf("|filesize:0x%x\n",seg->filesize);
                    *fileoff = seg->fileoff;
                    *filesize = seg->filesize;
                }
            }
                break;
            case LC_SEGMENT_64:
            {
                struct segment_command_64 *seg = (struct segment_command_64*)cmd;
                //printf("LC_SEGMENT name:%s\n",seg->segname);
                if(!strcmp(seg->segname,"__PRELINK_TEXT")){
                    //printf("|size:0x%x\n",cmd->cmdsize);
                    //printf("|vmaddr:0x%llx\n",seg->vmaddr);
                    //printf("|vmsize:0x%llx\n",seg->vmsize);
                    //printf("|fileoff:0x%llx\n",seg->fileoff);
                    //printf("|filesize:0x%llx\n",seg->filesize);
                    *fileoff = seg->fileoff;
                    *filesize = seg->filesize;
                }
            }
                break;
        }
        cmd = (struct load_command*)((char*)cmd + cmd->cmdsize);
    }
}

uint64_t machoGetVMAddr(uint8_t firstPage[4096],char *segname,char *sectname){
    if(!segname){
        printf("machoH missing segname,it must need segname\n");
        exit(1);
    }
    
    struct fat_header* fileStartAsFat = (struct fat_header*)firstPage;
    if(fileStartAsFat->magic==FAT_CIGAM||fileStartAsFat->magic==FAT_MAGIC){
        printf("machoH is fat\n");
        exit(1);
    }
    
    struct mach_header *mh = (struct mach_header*)firstPage;
    
    int is32 = 1;
    
    if(mh->magic==MH_MAGIC||mh->magic==MH_CIGAM){
        is32 = 1;
    }
    else if(mh->magic==MH_MAGIC_64||mh->magic==MH_CIGAM_64){
        is32 = 0;
    }
    
    const uint32_t cmd_count = mh->ncmds;
    struct load_command *cmds = (struct load_command*)((char*)firstPage+(is32?sizeof(struct mach_header):sizeof(struct mach_header_64)));
    struct load_command* cmd = cmds;
    for (uint32_t i = 0; i < cmd_count; ++i){
        switch (cmd->cmd) {
            case LC_SEGMENT_64:
            {
                struct segment_command_64 *seg = (struct segment_command_64*)cmd;
                if(memcmp(seg->segname,segname,strlen(seg->segname))==0){
                    if(!sectname){
                        //如果没有sectname,代表该seg的VM起始地址
                        return seg->vmaddr;
                    }
                    
                    const uint32_t sec_count = seg->nsects;
                    struct section_64 *sec = (struct section_64*)((char*)seg + sizeof(struct segment_command_64));
                    for(uint32_t ii = 0; ii <sec_count; ++ii){
                        if(memcmp(sec->sectname,sectname,strlen(sec->sectname))==0){
                            return sec->addr;
                        }
                        sec = (struct section_64*)((char*)sec + sizeof(struct section_64));
                    }
                    
                }
                
            }
            case LC_SEGMENT:
            {
                struct segment_command *seg = (struct segment_command*)cmd;
                if(memcmp(seg->segname,segname,strlen(seg->segname))==0){
                    if(!sectname){
                        //如果没有sectname,代表该seg的VM起始地址
                        return seg->vmaddr;
                    }
                    
                    const uint32_t sec_count = seg->nsects;
                    struct section *sec = (struct section*)((char*)seg + sizeof(struct segment_command));
                    for(uint32_t ii = 0; ii <sec_count; ++ii){
                        if(memcmp(sec->sectname,sectname,strlen(sec->sectname))==0){
                            return sec->addr;
                        }
                        sec = (struct section*)((char*)sec + sizeof(struct section));
                    }
                    
                }
                
            }
                break;
        }
        cmd = (struct load_command*)((char*)cmd + cmd->cmdsize);
    }
    return -1;
}

uint64_t machoGetFileAddr(uint8_t firstPage[4096],char *segname,char *sectname){
    if(!segname){
        printf("machoH missing segname,it must need segname\n");
        exit(1);
    }
    
    struct fat_header* fileStartAsFat = (struct fat_header*)firstPage;
    if(fileStartAsFat->magic==FAT_CIGAM||fileStartAsFat->magic==FAT_MAGIC){
        printf("machoH is fat\n");
        exit(1);
    }
    
    struct mach_header *mh = (struct mach_header*)firstPage;
    
    int is32 = 1;
    
    if(mh->magic==MH_MAGIC||mh->magic==MH_CIGAM){
        is32 = 1;
    }
    else if(mh->magic==MH_MAGIC_64||mh->magic==MH_CIGAM_64){
        is32 = 0;
    }
    
    const uint32_t cmd_count = mh->ncmds;
    struct load_command *cmds = (struct load_command*)((char*)firstPage+(is32?sizeof(struct mach_header):sizeof(struct mach_header_64)));
    struct load_command* cmd = cmds;
    for (uint32_t i = 0; i < cmd_count; ++i){
        switch (cmd->cmd) {
            case LC_SEGMENT_64:
            {
                struct segment_command_64 *seg = (struct segment_command_64*)cmd;
                if(memcmp(seg->segname,segname,strlen(seg->segname))==0){
                    if(!sectname){
                        return seg->fileoff;
                    }
                    
                    const uint32_t sec_count = seg->nsects;
                    struct section_64 *sec = (struct section_64*)((char*)seg + sizeof(struct segment_command_64));
                    for(uint32_t ii = 0; ii <sec_count; ++ii){
                        if(memcmp(sec->sectname,sectname,strlen(sec->sectname))==0){
                            return sec->offset;
                        }
                        sec = (struct section_64*)((char*)sec + sizeof(struct section_64));
                    }
                    
                }
                
            }
            case LC_SEGMENT:
            {
                struct segment_command *seg = (struct segment_command*)cmd;
                if(memcmp(seg->segname,segname,strlen(seg->segname))==0){
                    if(!sectname){
                        return seg->fileoff;
                    }
                    
                    const uint32_t sec_count = seg->nsects;
                    struct section *sec = (struct section*)((char*)seg + sizeof(struct segment_command));
                    for(uint32_t ii = 0; ii <sec_count; ++ii){
                        if(memcmp(sec->sectname,sectname,strlen(sec->sectname))==0){
                            return sec->offset;
                        }
                        sec = (struct section*)((char*)sec + sizeof(struct section));
                    }
                    
                }
                
            }
                break;
        }
        cmd = (struct load_command*)((char*)cmd + cmd->cmdsize);
    }
    return -1;
}

uint64_t machoGetSize(uint8_t firstPage[4096],char *segname,char *sectname){
    if(!segname){
        printf("machoH missing segname,it must need segname\n");
        exit(1);
    }
    
    struct fat_header* fileStartAsFat = (struct fat_header*)firstPage;
    if(fileStartAsFat->magic==FAT_CIGAM||fileStartAsFat->magic==FAT_MAGIC){
        printf("machoH is fat\n");
        exit(1);
    }
    
    struct mach_header *mh = (struct mach_header*)firstPage;
    
    int is32 = 1;
    
    if(mh->magic==MH_MAGIC||mh->magic==MH_CIGAM){
        is32 = 1;
    }
    else if(mh->magic==MH_MAGIC_64||mh->magic==MH_CIGAM_64){
        is32 = 0;
    }
    
    const uint32_t cmd_count = mh->ncmds;
    struct load_command *cmds = (struct load_command*)((char*)firstPage+(is32?sizeof(struct mach_header):sizeof(struct mach_header_64)));
    struct load_command* cmd = cmds;
    for (uint32_t i = 0; i < cmd_count; ++i){
        switch (cmd->cmd) {
            case LC_SEGMENT_64:
            {
                struct segment_command_64 *seg = (struct segment_command_64*)cmd;
                if(memcmp(seg->segname,segname,strlen(seg->segname))==0){
                    if(!sectname){
                        return seg->filesize;
                    }
                    
                    const uint32_t sec_count = seg->nsects;
                    struct section_64 *sec = (struct section_64*)((char*)seg + sizeof(struct segment_command_64));
                    for(uint32_t ii = 0; ii <sec_count; ++ii){
                        if(memcmp(sec->sectname,sectname,strlen(sec->sectname))==0){
                            return sec->size;
                        }
                        sec = (struct section_64*)((char*)sec + sizeof(struct section_64));
                    }
                    
                }
                
            }
            case LC_SEGMENT:
            {
                struct segment_command *seg = (struct segment_command*)cmd;
                if(memcmp(seg->segname,segname,strlen(seg->segname))==0){
                    if(!sectname){
                        return seg->filesize;
                    }
                
                    const uint32_t sec_count = seg->nsects;
                    struct section *sec = (struct section*)((char*)seg + sizeof(struct segment_command));
                    for(uint32_t ii = 0; ii <sec_count; ++ii){
                        if(memcmp(sec->sectname,sectname,strlen(sec->sectname))==0){
                            return sec->size;
                        }
                        sec = (struct section*)((char*)sec + sizeof(struct section));
                    }
                    
                }
                
            }
                break;
        }
        cmd = (struct load_command*)((char*)cmd + cmd->cmdsize);
    }
    return -1;
}

char *KextGetBundleID(void *bin){
    uint64_t dataSecStart = machoGetFileAddr(bin,"__DATA","__data");
    uint64_t dataSecSize = machoGetSize(bin,"__DATA","__data");
    
    //printf("\n__DATA is 0x%llx-0x%llx\n",dataSecStart,dataSecStart+dataSecSize);
    
    char mh_Magic[] = {'c','o','m','.'};
    uint64_t per_mh = (uint64_t)memmem(bin+dataSecStart,dataSecSize,mh_Magic,0x4);
    if(per_mh){
        numberOfKextGotName++;
        printf("BundleID: %s\n",(char*)per_mh);
        return (char*)per_mh;
    }
    return NULL;
}

