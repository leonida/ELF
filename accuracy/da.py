import pandas as pd
import re
import numpy as np
from matplotlib import pyplot as plt

file = pd.read_csv("./accuracy/da.txt", error_bad_lines=False)
train = pd.DataFrame(file)
count = 0
opt = []

# 将best_win_rate输出到list 保存y值
for i in range(len(train)):
    # 读取每一行找到每轮迭代的最佳准确率
    if re.findall("best_win_rate*", train.loc[i][0]):
        count += 1
        # 读取准确率的值并转化为float类型
        opt.append(float(train.loc[i][0][14:]))
        print(count)
        print(train.loc[i][0])

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
