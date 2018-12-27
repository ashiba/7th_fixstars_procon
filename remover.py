import sys
assert len(sys.argv)==2

filename = sys.argv[1]
with open(filename, 'rb+') as f:
    content = f.read()
    f.seek(0)
    f.write(content.replace(b'\r', b''))
    f.truncate()
