"""Read-only executable inspection; no game data is copied to the repository."""
import struct, re, importlib.util
from pathlib import Path
p = Path(r'D:\Burnout Paradise Remastered\BurnoutPR.exe')
b = p.read_bytes()
print('modules', {m: bool(importlib.util.find_spec(m)) for m in ['capstone','pefile']})
nt=struct.unpack_from('<I',b,0x3c)[0]
n=struct.unpack_from('<H',b,nt+6)[0]
opt=struct.unpack_from('<H',b,nt+20)[0]
base=struct.unpack_from('<I',b,nt+24+28)[0]
sections=[]
for i in range(n):
 o=nt+24+opt+i*40
 name=b[o:o+8].rstrip(b'\0').decode()
 vs,va,rs,ro=struct.unpack_from('<IIII',b,o+8)
 sections.append((name,base+va,ro,rs))
print(sections)
for m in re.finditer(rb'[\x20-\x7e]{5,}',b):
 s=m.group().decode()
 if re.search(r'billboard|smashgate|superjump|smash.?gate|super.?jump|mapicon|mapmarker',s,re.I):
  va=next((v+m.start()-r for _,v,r,z in sections if r<=m.start()<r+z),0)
  print(hex(va), s[:220])
