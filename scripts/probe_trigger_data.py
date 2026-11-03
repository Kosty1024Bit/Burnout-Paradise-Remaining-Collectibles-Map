from pathlib import Path
import zlib,struct,collections
b=Path(r'D:\Burnout Paradise Remastered\TRIGGERS.DAT').read_bytes()
u=lambda p:struct.unpack_from('<I',b,p)[0]
entry=u(20); start=u(24)+u(entry+40); size=u(entry+28)
d=zlib.decompress(b[start:start+size])
print('resource bytes',len(d),'header',[hex(x) for x in struct.unpack_from('<32I',d)])
p,n=struct.unpack_from('<II',d,0x44)
print('generic',hex(p),n)
print('types',collections.Counter(d[p+i*56+54] for i in range(n)))
for t in [8,9,13]:
 rows=[]
 for i in range(n):
  a=p+i*56
  if d[a+54]!=t:continue
  xyz=struct.unpack_from('<3f',d,a); id,=struct.unpack_from('<i',d,a+36); group,=struct.unpack_from('<i',d,a+44)
  rows.append((group or id,xyz))
 print(t,'regions',len(rows),'keys',len(set(x[0] for x in rows)),'samples',rows[:3])
save=Path.home()/r'AppData/Local/Criterion Games/Burnout Paradise Remastered/Save/Profile.BurnoutParadiseSave'
s=save.read_bytes()
print('save',len(s),'head',s[:64].hex())
