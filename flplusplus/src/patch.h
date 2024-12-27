#pragma once

namespace patch {
    
void detour(unsigned char* pOFunc, void* pHkFunc, unsigned char* originalData, bool jmp = true);
void undetour(unsigned char* pOFunc, unsigned char* originalData);
void patch_bytes(unsigned int address, void* pData, unsigned int pSize);
void set_execute_read_write(unsigned int address, unsigned int size);

inline void patch_uint32(unsigned int address, unsigned int data)
{
    patch_bytes(address, (void*)&data, 4);
}
inline void patch_uint16(unsigned int address, unsigned short data)
{
    patch_bytes(address, (void*)&data, 2);
}
inline void patch_uint8(unsigned int address, unsigned char data)
{
    patch_bytes(address, (void*)&data, 1);
}
inline void patch_float(unsigned int address, float data)
{
    patch_bytes(address, (void*)&data, 4);
}
inline void patch_x3(unsigned int address, unsigned char a, unsigned char b, unsigned char c)
{
    unsigned char bytes[] = { a, b, c };
    patch_bytes(address, (void*)bytes, 3);
}
}

