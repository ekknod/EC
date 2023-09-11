#ifndef GLOBALS_H
#define GLOBALS_H


typedef unsigned long DWORD;
typedef unsigned long long QWORD;
typedef int BOOL;


void  SwapMemory(QWORD BaseAddress, QWORD ImageSize, QWORD NewBase);
QWORD get_winload_base(QWORD return_address);
QWORD get_loader_block(QWORD winload_base);
void  MemCopy(void* dest, void* src, QWORD size);
typedef struct _LIST_ENTRY LIST_ENTRY;
QWORD GetModuleEntry(LIST_ENTRY* entry, const wchar_t *name);
QWORD FindPattern(QWORD base, unsigned char* pattern, unsigned char* mask);
QWORD GetExport(QWORD base, const char *name);



//
// pe
//
BOOL pe_resolve_imports(QWORD ntoskrnl, QWORD base);
void pe_clear_headers(QWORD base);


#define FILENAME L"[bootx64.efi]"
#define SERVICE_NAME L"EC"

#endif /* GLOBALS_H */

