"""Build a local marker cache from the user's own TRIGGERS.DAT. No game assets shipped."""
import argparse, collections, json, math, struct, zlib
from pathlib import Path

def read_markers(game):
    b=(Path(game)/'TRIGGERS.DAT').read_bytes()
    def u(p): return struct.unpack_from('<I', b, p)[0]
    if b[:4]!=b'bnd2' or u(8)!=1 or u(16)!=1:
        raise ValueError('Unsupported PC trigger bundle')
    e=u(20); start=u(24)+u(e+40); size=u(e+28)
    raw=b[start:start+size]
    d=zlib.decompress(raw) if u(36)&1 else raw
    if len(d)<128 or struct.unpack_from('<I',d)[0]!=42:
        raise ValueError('Unsupported trigger resource version')
    p,n=struct.unpack_from('<II',d,0x44)
    if n>20000 or p+n*56>len(d): raise ValueError('Invalid trigger region table')
    rows=[]
    for i in range(n):
        a=p+i*56; kind={8:0,9:1,13:2}.get(d[a+54])
        if kind is None: continue
        x,y,z=struct.unpack_from('<3f',d,a)
        own,=struct.unpack_from('<I',d,a+36)
        group,=struct.unpack_from('<I',d,a+44)
        if not all(math.isfinite(v) and abs(v)<100000 for v in (x,y,z)):
            raise ValueError('Invalid collectible position')
        rows.append((group or own,kind,x,z))
    counts=[len({r[0] for r in rows if r[1]==k}) for k in range(3)]
    if counts!=[65,475,165]: raise ValueError(f'Unexpected collectible counts: {counts}')
    return rows,counts

if __name__=='__main__':
    p=argparse.ArgumentParser(); p.add_argument('--game',default=r'D:\Burnout Paradise Remastered')
    p.add_argument('--output',default='artifacts/private/Collectibles.bin'); args=p.parse_args()
    rows,counts=read_markers(args.game)
    out=Path(args.output); out.parent.mkdir(parents=True,exist_ok=True)
    out.write_bytes(struct.pack('<4sII',b'BPRC',1,len(rows))+b''.join(struct.pack('<IIff',*r) for r in rows))
    print(json.dumps({'cache':str(out.resolve()),'jump_entrances':sum(r[1]==0 for r in rows),
                      'unique_jumps':counts[0],'smashes':counts[1],'billboards':counts[2]}))
