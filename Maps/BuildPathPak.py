# BuildPathPak.py : nen Maps\Path\*.txt -> Maps\Path.pak (cung dinh dang VLHook/VLPathPak.cpp, khong can vao game)
# Cach chay:  python Maps\BuildPathPak.py [thu_muc_Maps ...]   (mac dinh: _Bin\Maps)
# Moi thu muc Maps: doc <Maps>\Path\*.txt, ghi <Maps>\Path.pak. Thu muc khong co Path\ thi lay Path\ cua thu muc dau tien
# (vd: python Maps\BuildPathPak.py _Bin\Maps _Bin\Publish\User\VLCTCPro\Maps  -> ca 2 pak cung noi dung _Bin\Maps\Path)
import os, struct, sys, time

M = 0xFFFFFFFF
PAK_SIG, PAK_KEY_IDX, PAK_KEY_DAT = 0x8E3A51C7, 0x5BD1E995, 0xC2B2AE35
PAK_MAX_FILES, PAK_MAX_LEN, PAK_MAX_OFF, PAK_CHAIN = 1024, 4096, 0xFFFFF, 256
ENT_FMT = '<iIIII116s'	# PAK_ENT: nMap, dwOfs, dwSize, dwPack, dwHash, WCHAR wszName[58]

def pak_hash(p):
	h = 0x811C9DC5
	for b in p: h = ((h ^ b) * 0x01000193) & M
	return h

def pak_crypt(p, seed):
	s = seed or 0x6D2B79F5
	out = bytearray(p)
	for i in range(len(out)):
		s ^= (s << 13) & M; s ^= s >> 17; s ^= (s << 5) & M
		out[i] ^= ((s >> 7) + i) & 0xFF
	return bytes(out)

def data_seed(nMap, dwOfs, salt):
	return (PAK_KEY_DAT ^ salt ^ ((nMap * 0x9E3779B1) & M) ^ dwOfs) & M

class BitW:
	def __init__(self): self.d = bytearray(); self.pos = 0; self.k = 8
	def bit(self, b):
		if self.k == 8: self.pos = len(self.d); self.d.append(0); self.k = 0
		if b: self.d[self.pos] |= 1 << self.k
		self.k += 1
	def gamma(self, v):
		h = v.bit_length() - 1
		for i in range(h - 1, -1, -1): self.bit((v >> i) & 1); self.bit(i > 0)

def match_len(src, a, b, nmax):
	n = 0
	while n < nmax and src[a + n] == src[b + n]: n += 1
	return n

def compress(src):
	w = BitW(); n = len(src); i = 0; last = 0; chains = {}
	def add(p):
		if p + 3 <= n: chains.setdefault(src[p:p + 3], []).append(p)
	while i < n:
		nmax = min(n - i, PAK_MAX_LEN); best = off = score = 0
		if last and last <= i:
			l = match_len(src, i, i - last, nmax)
			if l >= 2: best, off, score = l, last, l + 1
		if i + 3 <= n:
			for j in reversed(chains.get(src[i:i + 3], [])[-PAK_CHAIN:]):
				if i - j > PAK_MAX_OFF: break
				l = match_len(src, i, j, nmax)
				if l >= 3 and l > score: best, off, score = l, i - j, l
		if not best:
			w.bit(0); w.d.append(src[i]); add(i); i += 1
			continue
		w.bit(1)
		if off == last: w.gamma(2)
		else:
			w.gamma(((off - 1) >> 8) + 3); w.d.append(((off - 1) ^ 0xA5) & 0xFF); last = off
		w.gamma(best)
		for _ in range(best): add(i); i += 1
	return bytes(w.d)

def decompress(src, n):	# giong PakDecompress - kiem tra lai
	st = {'i': 0, 'k': 8, 'bb': 0}; dst = bytearray(); last = 0
	def bit():
		if st['k'] == 8: st['bb'] = src[st['i']]; st['i'] += 1; st['k'] = 0
		r = (st['bb'] >> st['k']) & 1; st['k'] += 1; return r
	def gamma():
		v = 1
		while True:
			v = v * 2 + bit()
			if not bit(): return v
	while len(dst) < n:
		if not bit(): dst.append(src[st['i']]); st['i'] += 1; continue
		v = gamma()
		if v != 2: last = (((v - 3) << 8) | (src[st['i']] ^ 0xA5)) + 1; st['i'] += 1
		for _ in range(gamma()): dst.append(dst[-last])
	return bytes(dst)

def read_maps(path_dir):
	ents = {}	# nMap -> (ten file, noi dung); trung map: uu tien <MapID>_<Ten>.txt nhu FindPathFile
	for f in sorted(os.listdir(path_dir)):
		if not f.lower().endswith('.txt'): continue
		digits = ''
		for c in f:
			if not c.isdigit(): break
			digits += c
		nMap = int(digits) if digits else 0
		if nMap <= 0: continue
		if nMap in ents and ('_' not in f or '_' in ents[nMap][0]): continue
		ents[nMap] = (f, open(os.path.join(path_dir, f), 'rb').read())
	return ents

def build(ents, out_file):
	maps = sorted(ents)[:PAK_MAX_FILES]
	salt = (int(time.time() * 1000) * 0x2C1B3C6D + len(maps)) & M
	ofs = 16 + len(maps) * struct.calcsize(ENT_FMT)
	idx, packs, raw_total = [], [], 0
	for m in maps:
		name, raw = ents[m]
		pk = pak_crypt(compress(raw), data_seed(m, ofs, salt))
		assert decompress(pak_crypt(pk, data_seed(m, ofs, salt)), len(raw)) == raw, name
		idx.append(struct.pack(ENT_FMT, m, ofs, len(raw), len(pk), pak_hash(raw), name[:57].encode('utf-16-le')))
		packs.append(pk); ofs += len(pk); raw_total += len(raw)
	idx = b''.join(idx)
	hdr = struct.pack('<4I', PAK_SIG, (len(maps) ^ (salt * 0x2545F491)) & M, salt, pak_hash(idx))
	tmp = out_file + '.tmp'
	with open(tmp, 'wb') as f: f.write(hdr + pak_crypt(idx, PAK_KEY_IDX ^ salt) + b''.join(packs))
	os.replace(tmp, out_file)
	print('%s: %d map, %d -> %d byte' % (out_file, len(maps), raw_total, ofs))

def main():
	root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
	dirs = sys.argv[1:] or [os.path.join(root, '_Bin', 'Maps')]
	src = None
	for d in dirs:
		p = os.path.join(d, 'Path')
		if os.path.isdir(p): src = src or p
		if not src: sys.exit('Khong co thu muc %s' % p)
		ents = read_maps(p if os.path.isdir(p) else src)
		if not ents: sys.exit('Khong co file .txt nao trong %s' % src)
		build(ents, os.path.join(d, 'Path.pak'))

if __name__ == '__main__':
	main()
