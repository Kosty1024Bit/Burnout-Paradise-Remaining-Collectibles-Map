import sys, struct, re
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'third_party/re_tools'))
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
pe=pefile.PE(r'D:\Burnout Paradise Remastered\BurnoutPR.exe')
base=pe.OPTIONAL_HEADER.ImageBase
data=pe.get_memory_mapped_image()
cs=Cs(CS_ARCH_X86,CS_MODE_32)
def dump(a,n=180):
 first=next(cs.disasm(data[a-base:a-base+15],a),None)
 if first and first.mnemonic=='jmp' and first.op_str.startswith('0x'):
  print('THUNK',hex(a),'->',first.op_str); a=int(first.op_str,16)
 for i in cs.disasm(data[a-base:a-base+n],a):
  if i.mnemonic!='int3':print(f'{i.address:08x} {i.mnemonic:9} {i.op_str}')
def refs(a):
 needle=struct.pack('<I',a)
 return [base+m.start() for m in re.finditer(re.escape(needle),data) if any(s.Characteristics & 0x20000000 and s.VirtualAddress<=m.start()<s.VirtualAddress+s.Misc_VirtualSize for s in pe.sections)]
if sys.argv[1]=='refs':
 for x in sys.argv[2:]:
  a=int(x,16); print('REFS',hex(a),[hex(v) for v in refs(a)])
  for v in refs(a): dump(v-20,100)
elif sys.argv[1]=='dump': dump(int(sys.argv[2],16),int(sys.argv[3]) if len(sys.argv)>3 else 400)
elif sys.argv[1]=='strings':
 for m in re.finditer(rb'[\x20-\x7e]{4,}',data):
  if re.search(sys.argv[2],m.group().decode(),re.I):print(hex(base+m.start()),m.group().decode()[:180])
elif sys.argv[1]=='calls':
 target=int(sys.argv[2],16)
 for s in pe.sections:
  if not s.Characteristics & 0x20000000:continue
  start=s.VirtualAddress; blob=data[start:start+s.Misc_VirtualSize]
  for m in re.finditer(b'[\xe8\xe9]',blob):
   p=start+m.start()
   if p+5<len(data) and base+p+5+struct.unpack_from('<i',data,p+1)[0]==target:print(hex(base+p))
elif sys.argv[1]=='refs_only':
 for a in sys.argv[2:]:print(a,[hex(x) for x in refs(int(a,16))])
elif sys.argv[1]=='range':
 a,z=int(sys.argv[2],16),int(sys.argv[3],16)
 for i in cs.disasm(data[a-base:z-base],a):
  if i.mnemonic=='jmp' or ('0x5c0' in i.op_str) or ('0x5d0' in i.op_str): print(hex(i.address),i.mnemonic,i.op_str)
elif sys.argv[1]=='prologs':
 a,z=int(sys.argv[2],16),int(sys.argv[3],16)
 for m in re.finditer(rb'\x55\x8b\xec|\x53\x8b\xdc',data[a-base:z-base]):print(hex(a+m.start()))
elif sys.argv[1]=='returns':
 a,z=int(sys.argv[2],16),int(sys.argv[3],16)
 for i in cs.disasm(data[a-base:z-base],a):
  if i.mnemonic.startswith('ret'):print(hex(i.address),i.mnemonic,i.op_str)
