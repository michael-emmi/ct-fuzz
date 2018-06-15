import sys
import struct

def getTypeId(t):
  if 'int8' in t:
    return 'B'
  elif 'int16' in t:
    return 'H'
  elif 'int32' in t:
    return 'I'
  elif 'int64' in t:
    return 'L'
  elif 'char' in t:
    return 'B'
  elif 'short' in t:
    return 'H'
  elif 'int' in t:
    return 'I'
  elif 'long' in t:
    return 'L'
  elif 'unsigned' == t:
    return 'I'
  else:
    raise RuntimeError("Type not supported: {0}".format(t))

def splitAndStrip(lst, delim=None):
    return map(lambda x: x.strip(), (lst.split(delim) if delim else lst.split()))

def parse(declStr):
  decls = map(splitAndStrip, (splitAndStrip(declStr, ",")))
  return decls

def generate_data(decls):
  data = ''
  for decl in decls:
    t = decl[0]
    n = decl[1]
    if '[' in t and ']' in t:
        et = splitAndStrip(t, '[')[0]
        le = splitAndStrip(splitAndStrip(t, '[')[1], ']')[0]
        data += repr(struct.pack('<'+getTypeId('int16'), int(le)))[1:-1]
        for i in range(int(le)):
            data += repr(struct.pack('<'+getTypeId(et), i+1))[1:-1]
    else:
        data += repr(struct.pack('<'+getTypeId(et), 0))[1:-1]
  return data
     

if __name__ == '__main__':
  ''' usage:
      script 'uint32_t[2] v, uint32_t[4] k'
  '''
  declStr = sys.argv[1]  
  decls = parse(declStr)
  print generate_data(decls)
