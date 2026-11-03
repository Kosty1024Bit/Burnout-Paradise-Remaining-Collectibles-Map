import ctypes as c,sys,struct
from ctypes import wintypes as w
k=c.WinDLL('kernel32',use_last_error=True)
k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE
k.ReadProcessMemory.argtypes=[w.HANDLE,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)];k.ReadProcessMemory.restype=w.BOOL
k.CloseHandle.argtypes=[w.HANDLE]
h=k.OpenProcess(0x410,False,int(sys.argv[1]))
if not h:raise c.WinError(c.get_last_error())
def read(a,n):
 b=c.create_string_buffer(n);got=c.c_size_t()
 if not k.ReadProcessMemory(h,a,b,n,c.byref(got)):return b''
 return b.raw[:got.value]
for a in [0x4f9780,0x32c4ea0,0xa078f0,0x3215650,0x13fc8e0,0x149a1f0,0x149a1d0]:
 b=read(a,48);print(hex(a),b.hex(' '))
k.CloseHandle(h)
