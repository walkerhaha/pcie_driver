#!/usr/bin/python3
import matplotlib.pyplot as plt
import sys
import numpy as np
import pandas as pd

if(len(sys.argv) < 2):
    print('no file name');


print('file name: ' + sys.argv[0]);

speeds_log = []

MTDMA DMA_MEM_TO_DEV total{:d} {:d}MB: speed {:3.3f}MB

with open(sys.argv[1], 'r')as input:
    for line in input:
        if 'total' in line:
            line = line.split()
            if len(line) == 10:
                speeds_log.append([int(line[4].lstrip('MTDMA')), float(line[-3].rstrip('MB/s')), float(line[-1].rstrip('MB/s'))])





df = pd.DataFrame(speeds_log,columns=['ch', 'rspeed', 'wspeed'])

df.loc['sum'] = df.apply(lambda x: x.sum())

#ch_df = df.groupby(['ch','rw'])

print(df.loc['sum'])

#p3 = plt.subplot(121)  # 22表示共4(2*2)个子图，1表示第一张子图
p4 = df.plot(kind='bar') 
#df.hist(histtype='stepfilled', bins=30, normed=True)
plt.sca(p4)
plt.show()

'''
print(ch_df['speed'].sum()/8)
print(df)
            speeds_log.append([line[4], line[5], float(line[-1].rstrip('MB'))])
plt.figure('frame time')
plt.subplot(211)
plt.plot(rangeUpdateTime, '.r',)
plt.grid(True)
plt.subplot(212)
plt.plot(rangeUpdateTime)
plt.grid(True)
plt.show()
'''