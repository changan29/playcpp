#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <assert.h>

#define MAX_BUF_SIZE 1024
void* get_module_base(pid_t pid, const char* module_name)
{
    FILE* fp;
    long addr = 0;
    char* pch;
    char filename[32];
    char line[MAX_BUF_SIZE];
    
    // 格式化字符串得到 "/proc/pid/maps"
    if(pid < 0){
        snprintf(filename, sizeof(filename), "/proc/self/maps");
    }else{
        snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    }

    // 打开文件/proc/pid/maps，获取指定pid进程加载的内存模块信息
    fp = fopen(filename, "r");
    if(fp != NULL){
        // 每次一行，读取文件 /proc/pid/maps中内容
        while(fgets(line, sizeof(line), fp))
        {
            printf("/proc/self/maps: %s \n" , line);
            // 查找指定的so模块
            if(strstr(line, module_name)){
                // 分割字符串
                pch = strtok(line, "-");
                // 字符串转长整形
                addr = strtoul(pch, NULL, 16);
                break;
            }
        }
    }
    fclose(fp);
    printf("get_module_base %p\n", (void*) addr);
    return (void*)addr;
}

struct rel_info {
    uint64_t rel_plt_addr;
    uint32_t rel_plt_size;
    uint64_t str_tab_addr;
    uint64_t sym_tab_addr;
    uint32_t rel_type;
};

struct dynamice_segment {
    Elf64_Dyn *addr;
    uint32_t size;
};

static int get_dyn_section(void *base_addr, struct dynamice_segment *dyn);
static void get_rel_info(Elf64_Dyn *dynamic_table, uint32_t dynamic_size, struct rel_info *info);
static void update_entry(uint64_t *addr, uint64_t *old_entry, uint64_t target_entry);

static int hook_entry (void *base_addr, 
                const char *func_name_1,
                uint64_t *old_entry,
                uint64_t target_entry)
{
    struct dynamice_segment dyn;
    struct rel_info info;

    get_dyn_section(base_addr, &dyn);
    printf("dyn.addr: %p , dyn.size:%d\n" , dyn.addr , dyn.size);
    get_rel_info(dyn.addr, dyn.size, &info);

    Elf64_Rela *rel_table = (Elf64_Rela *)info.rel_plt_addr;
    printf("rel_plt_addr: %p\n" , rel_table);
    Elf64_Sym *sym_table = (Elf64_Sym *)info.sym_tab_addr;
    printf("sym_table_addr: %p\n" , sym_table);
    char *str_table = (char *)info.str_tab_addr;
    printf("strtab_addr: %p\n" , str_table);
    for(int i = 0;i < info.rel_plt_size / sizeof(Elf64_Rela);i++)
    {
        uint32_t number = ELF64_R_SYM(rel_table[i].r_info);
        uint32_t index = sym_table[number].st_name;
        char* func_name = &str_table[index];
        printf("func_name: %s ,\n " , func_name);
        if(strcmp(func_name, func_name_1) == 0)
        {
            //uint64_t *addr = (uint64_t *)(rel_table[i].r_offset + (uint64_t) base_addr);
            uint64_t *addr = (uint64_t *)(rel_table[i].r_offset );
            update_entry(addr, old_entry, target_entry);
            break;
        }
    }

    return 0;
}

static int get_dyn_section(void *base_addr, struct dynamice_segment *dyn)
{
    if (base_addr == NULL || dyn == NULL) {
        return 0;
    }

    //计算program header table实际地址
    Elf64_Ehdr *header = (Elf64_Ehdr*)(base_addr);
    if (memcmp(header->e_ident, "\177ELF", 4) != 0) {
        printf("not a program header \n");
        return -1;
    }

    Elf64_Phdr* phdr_table = (Elf64_Phdr*)(( Elf64_Off ) base_addr + header->e_phoff);
    if (phdr_table == 0) {
        printf(" phdr_table == NULl \n");
        return -1;
    }
    size_t phdr_count = header->e_phnum;


    //遍历program header table，ptype等于PT_DYNAMIC即为dynameic，获取到p_offset
    for (int j = 0; j < phdr_count; j++)
    {
        if (phdr_table[j].p_type == PT_DYNAMIC)
        {
            printf("dynameic phdr_table[j].p_vaddr: %x base_addr: %x size: %d\n", phdr_table[j].p_vaddr , base_addr, phdr_table[j].p_memsz);
            //dyn->addr = (Elf64_Dyn *)(phdr_table[j].p_vaddr + (uint64_t)base_addr);
            dyn->addr = (Elf64_Dyn *)(phdr_table[j].p_vaddr );
            dyn->size = phdr_table[j].p_memsz;
            break;
        }
    }
    return 0;
}

static void get_rel_info(Elf64_Dyn *dynamic_table, uint32_t dynamic_size, struct rel_info *info)
{
    if (info == NULL) {
        return;
    }

    for(int i = 0;i < dynamic_size / sizeof(Elf64_Dyn);i ++)
    {
        uint64_t val = dynamic_table[i].d_un.d_val;
        printf("dynamic_table[%d].d_un.d_val: %ld dynamic_table[i].d_tag: %ld \n" , i , val ,dynamic_table[i].d_tag );
        if (dynamic_table[i].d_tag == DT_JMPREL)
        {
            info->rel_plt_addr = dynamic_table[i].d_un.d_ptr;
        }
        if (dynamic_table[i].d_tag == DT_STRTAB)
        {
            info->str_tab_addr = dynamic_table[i].d_un.d_ptr;
        }
        if (dynamic_table[i].d_tag == DT_PLTRELSZ)
        {
            info->rel_plt_size = dynamic_table[i].d_un.d_val;
        }
        if (dynamic_table[i].d_tag == DT_SYMTAB)
        {
            info->sym_tab_addr = dynamic_table[i].d_un.d_ptr;
        }
        if (dynamic_table[i].d_tag == DT_PLTREL)
        {
            // DT_RELA = 7
            // DT_REL = 17
            info->rel_type = dynamic_table[i].d_un.d_val;
        }
    }
}

static void update_entry(uint64_t *addr, uint64_t *old_entry, uint64_t target_entry)
{
    // 获取当前内存分页的大小
    uint64_t page_size = getpagesize();
    // 获取内存分页的起始地址（需要内存对齐）
    uint64_t mem_page_start = (uint64_t)addr & (~(page_size - 1));
    mprotect((void *)mem_page_start, page_size, PROT_READ | PROT_WRITE | PROT_EXEC);
    if (old_entry != NULL) {
        *old_entry = *addr;
    }
    *addr = target_entry;
}

static int get_exe_name(char *buf, int size)
{
    int count; 
    count = readlink( "/proc/self/exe", buf, size); 
    if (count < 0 || count >= size) { 
        return -1;
    } 
 
    buf[count] = '\0'; 
    return 0;
}

typedef int (*FuncPuts)();
static FuncPuts old_rand = NULL;

int my_rand()
{
    if (old_rand != NULL) {
        printf("[fucking pass in fake func :) ]\n ");
        //old_rand();
        return 42;
    }
    return 0;
}

int main(void)
{
    char buf[MAX_BUF_SIZE] = {0}; 
    int err = get_exe_name(buf, sizeof(buf));
    assert(err == 0);

    printf("exe name: %s\n", buf);
    void* base_addr = get_module_base(getpid(), buf);
    hook_entry(base_addr, "rand", (uint64_t *)&old_rand, (uint64_t)my_rand );
    printf("after hook \n");
    printf("main call rand: %d\n" , rand() );
    return 0;
}
