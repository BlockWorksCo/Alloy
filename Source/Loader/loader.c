//
// Originally from https://github.com/embedded2014/elf-loader.git
//


#include "loader.h"
#include "elf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include "CoreServices.h"

#define DBG(...) printf("ELF: " __VA_ARGS__)
#define ERR(msg) do { perror("ELF: " msg); exit(-1); } while(0)
#define MSG(msg) puts("ELF: " msg)

#define IS_FLAGS_SET(v, m) ((v&m) == m)
#define SECTION_OFFSET(e, n) (e->sectionTable + n * sizeof(Elf32_Shdr))

#define swab16(x) ( (((x) << 8) & 0xFF00) | (((x) >> 8) & 0x00FF) )



//
//
//
typedef struct
{
    void*       data;
    uint32_t    physicalAddress;
    int         secIdx;
    off_t       relSecIdx;

} ELFSection_t;


//
//
//
typedef struct
{
    int fd;

    size_t sections;
    off_t sectionTable;
    off_t sectionTableStrings;

    size_t symbolCount;
    off_t symbolTable;
    off_t symbolTableStrings;
    off_t entry;

    ELFSection_t text;
    ELFSection_t rodata;
    ELFSection_t data;
    ELFSection_t bss;

    void* stack;

    const ELFEnv_t* env;
} ELFExec_t;



//
//
//
typedef enum
{
    FoundERROR = 0,
    FoundSymTab = (1 << 0),
    FoundStrTab = (1 << 2),
    FoundText = (1 << 3),
    FoundRodata = (1 << 4),
    FoundData = (1 << 5),
    FoundBss = (1 << 6),
    FoundRelText = (1 << 7),
    FoundRelRodata = (1 << 8),
    FoundRelData = (1 << 9),
    FoundRelBss = (1 << 10),
    FoundValid = FoundSymTab | FoundStrTab,
    FoundExec = FoundValid | FoundText,
    FoundAll = FoundSymTab | FoundStrTab | FoundText | FoundRodata | FoundData
               | FoundBss | FoundRelText | FoundRelRodata | FoundRelData | FoundRelBss
} FindFlags_t;



//
//
//
void *do_alloc(size_t size, size_t align, ELFSecPerm_t perm, uint32_t* physicalAddress );
void arch_jumpTo(entry_t entry);





//
//
//
static int ReadSectionName(ELFExec_t* e, off_t off, char* buf, size_t max)
{
    int ret = -1;
    off_t offset = e->sectionTableStrings + off;
    off_t old = lseek(e->fd, 0, SEEK_CUR);

    if (lseek(e->fd, offset, SEEK_SET) != -1)
    {
        if (read(e->fd, buf, max) == 0)
        {
            ret = 0;
        }
    }

    (void) lseek(e->fd, old, SEEK_SET);
    return ret;
}


//
//
//
static int readSymbolName(ELFExec_t* e, off_t off, char* buf, size_t max)
{
    int ret = -1;
    off_t offset = e->symbolTableStrings + off;
    off_t old = lseek(e->fd, 0, SEEK_CUR);

    if (lseek(e->fd, offset, SEEK_SET) != -1)
    {
        if (read(e->fd, buf, max) == 0)
        {
            ret = 0;
        }
    }

    (void) lseek(e->fd, old, SEEK_SET);

    return ret;
}


//
//
//
static uint32_t swabo(uint32_t hl)
{
    return ((((hl) >> 24)) | /* */
            (((hl) >> 8) & 0x0000ff00) | /* */
            (((hl) << 8) & 0x00ff0000) | /* */
            (((hl) << 24))); /* */
}




//
//
//
static void LoadSectionData(ELFExec_t* e, ELFSection_t* s, Elf32_Shdr* h)
{
    if (h->sh_size == 0)
    {
        //
        // NO data for section.
        //
        MSG(" No data for section");
    }
    else
    {
        //
        // This section has data, so attempt to load it.
        //
        uint32_t    physicalAddress;
        s->data = do_alloc(h->sh_size, h->sh_addralign, h->sh_flags, &s->physicalAddress);

        if (s->data == NULL)
        {
            ERR("memory allocation failure.");
        }

        if (lseek(e->fd, h->sh_offset, SEEK_SET) == -1)
        {
            ERR("    seek fail");
        }

        if (read(e->fd, s->data, h->sh_size) != h->sh_size)
        {
            ERR("     read data fail");
        }        
    }
}



//
//
//
static void LoadBSSSectionData(ELFExec_t* e, ELFSection_t* s, Elf32_Shdr* h)
{
    if (h->sh_size == 0)
    {
        //
        // NO data for section.
        //
        MSG(" No data for section");
    }
    else
    {
        //
        // This section has data, so attempt to load it.
        //
        uint32_t    physicalAddress;
        s->data = do_alloc(h->sh_size, h->sh_addralign, h->sh_flags, &s->physicalAddress);

        if (s->data == NULL)
        {
            ERR("memory allocation failure.");
        }

        if (lseek(e->fd, h->sh_offset, SEEK_SET) == -1)
        {
            ERR("    seek fail");
        }

        //
        // Zero the BSS data.
        //
        memset(s->data, 0x00, h->sh_size);
        printf("Zeroed %d bytes of .bss data.\n", h->sh_size);
    }
}


//
//
//
static void ReadSectionHeader(ELFExec_t* e, int n, Elf32_Shdr* h)
{
    off_t offset = SECTION_OFFSET(e, n);

    if (lseek(e->fd, offset, SEEK_SET) == -1)
    {
        ERR("Can't seek to section start.");
    }

    if (read(e->fd, h, sizeof(Elf32_Shdr)) != sizeof(Elf32_Shdr))
    {
        ERR("Can't read section header.");
    }
}


//
//
//
static int ReadSection(ELFExec_t* e, int n, Elf32_Shdr* h, char* name, size_t nlen)
{
    ReadSectionHeader(e, n, h);

    if (h->sh_name)
    {
        return ReadSectionName(e, h->sh_name, name, nlen);
    }

    return 0;
}


//
//
//
static int readSymbol(ELFExec_t* e, int n, Elf32_Sym* sym, char* name, size_t nlen)
{
    int     ret     = -1;
    off_t   old     = lseek(e->fd, 0, SEEK_CUR);
    off_t   pos     = e->symbolTable + n * sizeof(Elf32_Sym);

    if (lseek(e->fd, pos, SEEK_SET) != -1)
    {
        if (read(e->fd, sym, sizeof(Elf32_Sym)) == sizeof(Elf32_Sym))
        {
            if (sym->st_name)
            {
                ret = readSymbolName(e, sym->st_name, name, nlen);
            }
            else
            {
                Elf32_Shdr shdr;
                ret = ReadSection(e, sym->st_shndx, &shdr, name, nlen);
            }
        }
        else
        {
            ERR("Can't read symbol.");
        }
    }

    (void) lseek(e->fd, old, SEEK_SET);
    return ret;
}






static void relR_ARM_THM_JUMP24(Elf32_Addr relAddr, int type, Elf32_Addr symAddr, Elf32_Addr relPhysAddr)
{
    uint32_t upper = ((uint16_t*) relAddr)[0];
    uint32_t lower = ((uint16_t*) relAddr)[1];
    
    /*
    * 25 bit signed address range (Thumb-2 BL and B.W
    * instructions):
    *   S:I1:I2:imm10:imm11:0
    * where:
    *   S     = upper[10]   = offset[24]
    *   I1    = ~(J1 ^ S)   = offset[23]
    *   I2    = ~(J2 ^ S)   = offset[22]
    *   imm10 = upper[9:0]  = offset[21:12]
    *   imm11 = lower[10:0] = offset[11:1]
    *   J1    = lower[13]
    *   J2    = lower[11]
    */
    uint32_t sign = (upper >> 10) & 1;
    uint32_t j1 = (lower >> 13) & 1;
    uint32_t j2 = (lower >> 11) & 1;
    int32_t offset = (sign << 24) | ((~(j1 ^ sign) & 1) << 23) |
             ((~(j2 ^ sign) & 1) << 22) |
             ((upper & 0x03ff) << 12) |
             ((lower & 0x07ff) << 1);

    if (offset & 0x01000000)
    {
        offset -= 0x02000000;
    }

    printf("offset before = %08x, symAddr = %08x, relPhysAddr = %08x\n", offset, symAddr, relPhysAddr);
    offset += symAddr - relPhysAddr;
    printf("offset after = %08x\n", offset);

    if (    offset <= (int32_t)0xff000000 ||
            offset >= (int32_t)0x01000000)
    {
        printf("offset = %08x\n",offset);
        ERR("relocation out of range.");
    }

    sign = (offset >> 24) & 1;
    j1 = sign ^ (~(offset >> 23) & 1);
    j2 = sign ^ (~(offset >> 22) & 1);
    upper = (uint16_t)((upper & 0xf800) | (sign << 10) |
                  ((offset >> 12) & 0x03ff));
    lower = (uint16_t)((lower & 0xd000) |
                  (j1 << 13) | (j2 << 11) |
                  ((offset >> 1) & 0x07ff));

    ((uint16_t*) relAddr)[0] = upper;
    ((uint16_t*) relAddr)[1] = lower;
}




//
//
//
static void relR_ARM_THM_MOVW_ABS_NC(Elf32_Addr relAddr, int type, Elf32_Addr symAddr, Elf32_Addr relPhysAddr)
{
    uint32_t    upper_insn = ( ((uint16_t*) relAddr)[0] );
    uint32_t    lower_insn = ( ((uint16_t*) relAddr)[1] );
    int32_t     offset;
    offset =    ((upper_insn & 0x000f) << 12) |
                ((upper_insn & 0x0400) << 1) |
                ((lower_insn & 0x7000) >> 4) | (lower_insn & 0x00ff);
    offset =    (offset ^ 0x8000) - 0x8000;
    offset +=   symAddr;
    upper_insn = (uint16_t)((upper_insn & 0xfbf0) |
                            ((offset & 0xf000) >> 12) |
                            ((offset & 0x0800) >> 1));
    lower_insn = (uint16_t)((lower_insn & 0x8f00) |
                            ((offset & 0x0700) << 4) |
                            (offset & 0x00ff));
    ((uint16_t*) relAddr)[0] = ( upper_insn );
    ((uint16_t*) relAddr)[1] = ( lower_insn );
}




//
//
//
static void relR_ARM_THM_MOVT_ABS(Elf32_Addr relAddr, int type, Elf32_Addr symAddr, Elf32_Addr relPhysAddr)
{
    uint32_t    upper_insn = ( ((uint16_t*) relAddr)[0] );
    uint32_t    lower_insn = ( ((uint16_t*) relAddr)[1] );
    int32_t     offset;
    offset =    ((upper_insn & 0x000f) << 12) |
                ((upper_insn & 0x0400) << 1) |
                ((lower_insn & 0x7000) >> 4) | (lower_insn & 0x00ff);
    offset =    (offset ^ 0x8000) - 0x8000;
    offset +=   symAddr;
    offset >>= 16;
    upper_insn = (uint16_t)((upper_insn & 0xfbf0) |
                            ((offset & 0xf000) >> 12) |
                            ((offset & 0x0800) >> 1));
    lower_insn = (uint16_t)((lower_insn & 0x8f00) |
                            ((offset & 0x0700) << 4) |
                            (offset & 0x00ff));
    ((uint16_t*) relAddr)[0] = ( upper_insn );
    ((uint16_t*) relAddr)[1] = ( lower_insn );
}




//
//
//
static void relR_ARM_MOVW_ABS_NC(Elf32_Addr relAddr, int type, Elf32_Addr symAddr, Elf32_Addr relPhysAddr)
{
    int32_t     offset;
    uint32_t    tmp;

    printf("<%08x>\n", symAddr - relPhysAddr);

    tmp = *(uint32_t*)relAddr;
    offset = tmp;

    offset = ((offset & 0xf0000) >> 4) | (offset & 0xfff);
    offset = (offset ^ 0x8000) - 0x8000;

    offset += symAddr;
    if (type == R_ARM_MOVT_ABS)
        offset >>= 16;

    tmp &= 0xfff0f000;
    tmp |= ((offset & 0xf000) << 4) | (offset & 0x0fff);

    *(uint32_t*)relAddr = tmp;
}




//
//
//
static void relR_ARM_CALL(Elf32_Addr relAddr, int type, Elf32_Addr symAddr, Elf32_Addr relPhysAddr)
{
    uint32_t    insn = ( ((uint32_t*) relAddr)[0] );
    int32_t     offset;
    uint32_t    tmp;

    if (symAddr & 3) {
        return;
    }

    offset = ( ((uint32_t*) relAddr)[0] );
    offset = (offset & 0x00ffffff) << 2;
    if (offset & 0x02000000)
        offset -= 0x04000000;

    offset += symAddr - relPhysAddr;
    if (offset <= (int32_t)0xfe000000 ||
        offset >= (int32_t)0x02000000) {
        return;
    }

    offset >>= 2;
    offset &= 0x00ffffff;

    *(uint32_t *)relAddr &= (0xff000000);
    *(uint32_t *)relAddr |= (offset);
}





//
//
//
static void relR_ARM_JUMP24(Elf32_Addr relAddr, int type, Elf32_Addr symAddr, Elf32_Addr relPhysAddr)
{
    uint32_t    insn = ( ((uint32_t*) relAddr)[0] );
    int32_t     offset;
    uint32_t    tmp;

    if (symAddr & 3) {
        return;
    }

    offset = ( ((uint32_t*) relAddr)[0] );
    offset = (offset & 0x00ffffff) << 2;
    if (offset & 0x02000000)
        offset -= 0x04000000;

    offset += symAddr - relPhysAddr;
    if (offset <= (int32_t)0xfe000000 ||
        offset >= (int32_t)0x02000000) {
        return;
    }

    offset >>= 2;
    offset &= 0x00ffffff;

    *(uint32_t *)relAddr &= (0xff000000);
    *(uint32_t *)relAddr |= (offset);
}


//
// Modify the value to be relocated according to the relocation-type.
//
static void relocateSymbol(Elf32_Addr relAddr, int type, Elf32_Addr symAddr, Elf32_Addr relPhysAddr)
{
    printf("Symbol relocation type %d\n", type);

    switch (type)
    {
        case R_ARM_ABS32:
            *((uint32_t*) relAddr) += symAddr;
            DBG("  R_ARM_ABS32 relocated is 0x%08X\n", (unsigned int) * ((uint32_t*)relAddr));
            break;

        case R_ARM_THM_PC22:
        case R_ARM_THM_JUMP24:
            relR_ARM_THM_JUMP24(relAddr, type, symAddr, relPhysAddr);
            DBG("  R_ARM_THM_CALL/JMP relocated is 0x%08X\n", (unsigned int) * ((uint32_t*)relAddr));
            break;

        case R_ARM_THM_MOVW_ABS_NC:
            relR_ARM_THM_MOVW_ABS_NC(relAddr, type, symAddr, relPhysAddr);
            DBG("  R_ARM_THM_MOVW_ABS_NC relocated is 0x%08X\n", (unsigned int) * ((uint32_t*)relAddr));
            break;

        case R_ARM_THM_MOVT_ABS:
            relR_ARM_THM_MOVT_ABS(relAddr, type, symAddr, relPhysAddr);
            DBG("  R_ARM_THM_MOVT_ABS relocated is 0x%08X\n", (unsigned int) * ((uint32_t*)relAddr));
            break;

        case R_ARM_MOVT_ABS:
        case R_ARM_MOVW_ABS_NC:
            relR_ARM_MOVW_ABS_NC(relAddr, type, symAddr, relPhysAddr);
            DBG("  R_ARM_MOVW_ABS_NC relocated is 0x%08X\n", (unsigned int) * ((uint32_t*)relAddr));
            break;

        case R_ARM_CALL:
            relR_ARM_CALL(relAddr, type, symAddr, relPhysAddr);
            DBG("  R_ARM_CALL relocated is 0x%08X\n", (unsigned int) * ((uint32_t*)relAddr));
            break;

        case R_ARM_JUMP24:
            relR_ARM_JUMP24(relAddr, type, symAddr, relPhysAddr);
            DBG("  R_ARM_JUMP24 relocated is 0x%08X\n", (unsigned int) * ((uint32_t*)relAddr));
            break;

        case R_ARM_TLS_IE32:
            DBG("  R_ARM_TLS_IE32  *not*relocated. 0x%08X\n", (unsigned int) * ((uint32_t*)relAddr));
            break;

        default:
            ERR("  Undefined relocation type.");
            break;
    }
}


//
// Return the section with the matching index value.
//
static ELFSection_t* SectionFromIndex(ELFExec_t* e, int index)
{
    ELFSection_t*   section     = NULL;

    if(e->text.secIdx == index)
    {
        section     = &e->text;
    }

    if(e->rodata.secIdx == index)
    {
        section     = &e->rodata;
    }

    if(e->data.secIdx == index)
    {
        section     = &e->data;
    }

    if(e->bss.secIdx == index)
    {
        section     = &e->bss;
    }

    if(section == NULL)
    {
        printf("Unknown section %d.\n", index);
    }

    return section;
}



//
// Determine the address of the given symbol within the sections or the 
// enviromment if undefined.
//
static Elf32_Addr addressOf(ELFExec_t* e, Elf32_Sym* sym, const char* sName)
{
    uint32_t    address     = 0xffffffff;

    if (sym->st_shndx == SHN_UNDEF)
    {
        printf("<undefined symbol %s>\n", sName);
    }
    else if(strcmp(sName, "__libc_errno") == 0)
    {
        return ALLOY_RAM_BASE;
    }
    else
    {
        //
        // Defined symbol so get the section that contains it and work out the address.
        //
        ELFSection_t* symSec = SectionFromIndex(e, sym->st_shndx);
        //address = ((Elf32_Addr) symSec->data) + sym->st_value;
        address     = ((Elf32_Addr) symSec->physicalAddress) + sym->st_value;
    }

    if( address == 0xffffffff)
    {
        ERR("  Can not find address for symbol.");
    }

    return address;
}


//
//
//
static void relocate(ELFExec_t* e, Elf32_Shdr* h, ELFSection_t* s, const char* name)
{
    if (s->data != NULL)
    {
        Elf32_Rel   rel;
        size_t      relEntries = h->sh_size / sizeof(rel);
        size_t      relCount;

        (void) lseek(e->fd, h->sh_offset, SEEK_SET);

        //
        // For every relocation entry...
        //
        DBG(" Offset   Info     Type             Name\n");

        for (relCount = 0; relCount < relEntries; relCount++)
        {
            if (read(e->fd, &rel, sizeof(rel)) == sizeof(rel))
            {
                Elf32_Sym   sym;
                Elf32_Addr  symAddr;
                char        name[33]    = "<unnamed>";
                int         symEntry    = ELF32_R_SYM(rel.r_info);
                int         relType     = ELF32_R_TYPE(rel.r_info);
                Elf32_Addr  relAddr     = ((Elf32_Addr) s->data) + rel.r_offset;
                Elf32_Addr  relPhysAddr = ((Elf32_Addr) s->physicalAddress) + rel.r_offset;

                printf("\n");
                readSymbol(e, symEntry, &sym, name, sizeof(name));
                DBG(" %08X %08X %d %s\n", (unsigned int) rel.r_offset, (unsigned int) rel.r_info, relType, name);

                //
                // Get the address to relocate to.
                //
                symAddr = addressOf(e, &sym, name);

                //
                // Do the relocation.
                //
                DBG("  symAddr=%08X relAddr=%08X relPhysAddr=%08x\n", (unsigned int) symAddr, (unsigned int) relAddr, relPhysAddr);
                relocateSymbol(relAddr, relType, symAddr, relPhysAddr);
            }
        }
    }
    else
    {
        ERR("Section not loaded");
    }
}


//
//
//
int ParseSectionHeader(ELFExec_t* e, Elf32_Shdr* sh, const char* name, int n)
{
    int type    = 0;

    if (strcmp(name, ".symtab") == 0)
    {
        e->symbolTable = sh->sh_offset;
        e->symbolCount = sh->sh_size / sizeof(Elf32_Sym);
        type = FoundSymTab;
    }
    else if (strcmp(name, ".strtab") == 0)
    {
        e->symbolTableStrings = sh->sh_offset;
        type = FoundStrTab;
    }
    else if (strcmp(name, ".text") == 0)
    {
        LoadSectionData(e, &e->text, sh);
        e->text.secIdx = n;
        type = FoundText;
    }
    else if (strcmp(name, ".rodata") == 0)
    {
        LoadSectionData(e, &e->rodata, sh);
        e->rodata.secIdx = n;
        type = FoundRodata;
    }
    else if (strcmp(name, ".data") == 0)
    {
        LoadSectionData(e, &e->data, sh);
        e->data.secIdx = n;
        type = FoundData;
    }
    else if (strcmp(name, ".bss") == 0)
    {
        LoadBSSSectionData(e, &e->bss, sh);
        e->bss.secIdx = n;
        type = FoundBss;
    }
    else if (strcmp(name, ".rel.text") == 0)
    {
        e->text.relSecIdx = n;
        type = FoundRelText;
    }
    else if (strcmp(name, ".rel.rodata") == 0)
    {
        e->rodata.relSecIdx = n;
        type =  FoundRelText;
    }
    else if (strcmp(name, ".rel.data") == 0)
    {
        e->data.relSecIdx = n;
        type = FoundRelText;
    }

    return type;
}


//
//
//
static bool loadSymbols(ELFExec_t* e)
{
    int n;
    int founded = false;

    MSG("Scan ELF indexs...");

    //
    // For all sections...
    //
    for (n = 1; n < e->sections; n++)
    {
        Elf32_Shdr  sectHdr;
        char        name[33] = "<unamed>";

        //
        // Read the section header.
        //
        ReadSectionHeader(e, n, &sectHdr);

        if (sectHdr.sh_name)
        {
            ReadSectionName(e, sectHdr.sh_name, name, sizeof(name));
        }

        DBG("Examining section %d %s\n", n, name);
        founded |= ParseSectionHeader(e, &sectHdr, name, n);

        if (IS_FLAGS_SET(founded, FoundAll))
        {
            //
            // We have all the requried sections, so break out of the loop.
            //
            founded     = true;
            break;
        }
    }

    MSG("Done");
    return founded;
}


//
//
//
static void initElf(ELFExec_t* e, int f)
{
    Elf32_Ehdr h;
    Elf32_Shdr sH;

    if (f == -1)
    {
        ERR("Invalid file handle.");
    }

    memset(e, 0, sizeof(ELFExec_t));

    if (read(f, &h, sizeof(h)) != sizeof(h))
    {
        ERR("Can't read the file header.");
    }

    e->fd = f;

    if (lseek(e->fd, h.e_shoff + h.e_shstrndx * sizeof(sH), SEEK_SET) == -1)
    {
        ERR("Cant seek to section start.");
    }

    if (read(e->fd, &sH, sizeof(Elf32_Shdr)) != sizeof(Elf32_Shdr))
    {
        ERR("Can't read section header.");
    }

    e->entry                = h.e_entry;
    e->sections             = h.e_shnum;
    e->sectionTable         = h.e_shoff;
    e->sectionTableStrings  = sH.sh_offset;
}


//
//
//
static void relocateSection(ELFExec_t* e, ELFSection_t* s, const char* name)
{
    DBG("Relocating section %s\n", name);

    if (s->relSecIdx)
    {
        Elf32_Shdr sectHdr;

        ReadSectionHeader(e, s->relSecIdx, &sectHdr);
        relocate(e, &sectHdr, s, name);
    }
    else
    {
        MSG("No relocation index");    /* Not an error */
    }
}


//
//
//
static void relocateSections(ELFExec_t* e)
{
    relocateSection(e, &e->text, ".text");
    relocateSection(e, &e->rodata, ".rodata");
    relocateSection(e, &e->data, ".data");
}

static int jumpTo(ELFExec_t* e)
{
    if (e->entry)
    {
        entry_t* entry = (entry_t*)(e->text.physicalAddress + e->entry);
        arch_jumpTo(entry);
        return 0;
    }

    else
    {
        MSG("No entry defined.");
        return -1;
    }
}



//
// Top level call to load a .elf file.
//
void exec_elf(const char* path, const ELFEnv_t* env)
{
    ELFExec_t exec;

    //
    // Setup file handle and read file header and initial section header.
    //
    initElf(&exec, open(path, O_RDONLY) );

    //
    // Store the environment symbols.
    //
    exec.env = env;

    //
    // If we have all the required sections,then relocate the sections.
    //
    bool     r   = loadSymbols(&exec);
    if (r == true)
    {
        //
        // Relocate all the sections.
        //
        relocateSections(&exec);
        printf("ELF loaded... executing offset %08x PC @ %08x\n", (uint32_t)(exec.entry), (uint32_t)(exec.text.data + exec.entry) );
        
        //
        // Call the newly loaded executable.
        //
        jumpTo(&exec);
    }
    else
    {
        ERR("Invalid EXEC");
    }
}
