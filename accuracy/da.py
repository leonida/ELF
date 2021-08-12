import pandas as pd
import re
import numpy as np
from matplotlib import pyplot as plt
# file = pd.read_csv("./da.csv", error_bad_lines=False)
file = pd.read_csv("./accuracy/da.txt", error_bad_lines=False)
train = pd.DataFrame(file)
count = 0
opt = []

# 将best_win_rate输出到list 保存y值
for i in range(len(train)):
    if re.findall("best_win_rate*", train.loc[i][0]):
        print(count)
        count += 1
        print(train.loc[i][0])
        opt.append(float(train.loc[i][0][14:]))

for i in opt:
    print(i)
x = np.arange(1, 66, 1)  # start end num x值
# 绘图
plt.figure()
plt.plot(x, opt)
plt.xlabel('迭代轮数')
plt.ylabel('准确率')

plt.show()
# print(train.shape)
