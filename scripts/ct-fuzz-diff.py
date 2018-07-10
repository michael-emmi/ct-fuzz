import sys
import re

def diff(lines):
    def parseLine(line):
        m = re.search('\[dbg\] \[([0|1])\] \[(.+)\] \[(.+)\]', line)
        if m:
            runId = m.group(1)
            debugLoc = m.group(2)
            content = m.group(3)
            return runId, debugLoc, content, m.group(0)
        else:
            return None

    I1 = []
    I2 = []
    for line in lines:
        i = parseLine(line)
        if i:
           if i[0] == '0':
               I1.append(i)
           else:
               I2.append(i)
    for i1, i2 in zip(I1, I2):
        print i1[3], i2[3]
        if i1[2] != i2[2]:
            break

if __name__ == '__main__':
    diff(sys.stdin.readlines())
