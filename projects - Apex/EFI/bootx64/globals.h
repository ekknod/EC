#ifndef GLOBALS_H
#define GLOBALS_H


typedef unsigned long DWORD;
typedef unsigned long long QWORD;
typedef int BOOL;


void  SwapMemory(QWORD BaseAddress, QWORD ImageSize, QWORD NewBase);
void  SwapMemory2(QWORD CurrentBase, QWORD NewBase);
QWORD get_caller_base(QWORD caller_address);
QWORD get_loader_block(QWORD winload_base);
void  MemCopy(void* dest, void* src, QWORD size);
typedef struct _LIST_ENTRY LIST_ENTRY;

int strcmp_imp(const char *cs, const char *ct);
int wcscmp_imp(const wchar_t *cs, const wchar_t *ct);

QWORD GetModuleEntry(LIST_ENTRY* entry, const unsigned short *name);
QWORD FindPattern(QWORD base, unsigned char* pattern, unsigned char* mask);
void  *FindPatternEx(unsigned char* base, QWORD size, unsigned char* pattern, unsigned char* mask);



//
// pe
//
BOOL  pe_resolve_imports(QWORD ntoskrnl, QWORD base);
void  pe_clear_headers(QWORD base);
QWORD get_pe_entrypoint(QWORD base);
DWORD get_pe_size(QWORD base);
QWORD get_pe_section(QWORD base, const char *section_name, DWORD *size);
QWORD GetExportByName(QWORD base, const char* export_name);


#define FILENAME L"[bootx64.efi]"
#define SERVICE_NAME L"EC"

#endif /* GLOBALS_H */

