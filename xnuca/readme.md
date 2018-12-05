//double-free always has more influence than use after free on me~~
0x9d0 for set timer: val = 1 
0x9dc for auth:
```
  //init
  *(_DWORD *)(a1 + 0x9D0) = 0;
  *(_BYTE *)(a1 + 0x9D4) = 88;
  *(_BYTE *)(a1 + 0x9D5) = 110;
  *(_BYTE *)(a1 + 0x9D6) = 117;
  *(_BYTE *)(a1 + 0x9D7) = 99;
  *(_BYTE *)(a1 + 0x9D8) = 97;
  *(_DWORD *)(a1 + 0x9DC) = 0;
  *(_QWORD *)(a1 + 0x9E0) = &mem_buf;
  memset((void *)(a1 + 0x9E8), 0, 0x18uLL);
```

//result = (unsigned __int8)addr;
result = xnuca_send_request(
                 XnucaState,
                 (unsigned __int64)(addr & 0xF00) >> 8,
                 (unsigned __int64)((unsigned __int16)addr & 0xF000) >> 12,
                 (_addr & 0xFF0000u) >> 16,
                 (unsigned __int8)val);
  *(_DWORD *)(a1 + 0x9E8) = a2; //index
  *(_DWORD *)(a1 + 0x9EC) = a3; //case
  *(_DWORD *)(a1 + 0x9F0) = a4; //malloc size
  *(_QWORD *)(a1 + 0x9F8) = val; //write value
  *(_DWORD *)(a1 + 0x9D0) |= 4u; //for timer_func