from os import read
import pandas as pd
import re
import numpy as np
from matplotlib import pyplot as plt

# 读取文件导出为dataframe


def readfile(address):
    file = pd.read_csv(address, error_bad_lines=False)
    train = pd.DataFrame(file)
    return train


def best_winrate(train):
    count = 0
    opt = []
    # 将best_win_rate输出到list 保存y值
    for i in range(len(train)):
        # 读取每一行找到每轮迭代的最佳胜率
        if re.findall("best_win_rate*", train.loc[i][0]):
            count += 1
            # 读取胜率的值并转化为float类型
            opt.append(float(train.loc[i][0][14:]))
            # 显示读取过程
            print(count)
            print(train.loc[i][0])
    return opt  # 返回list


def timespent(train):
    count = 0
    opt = []
    # 将best_win_rate输出到list 保存y值
    for i in range(len(train)):
        # 读取每一行找到每轮迭代的最佳准确率
        if re.findall("Time*", train.loc[i][0]):
            count += 1
            # 读取时间花费的值转化为float类型，单位s，保留三位小数
            opt.append(format(float(train.loc[i][0][18:-3])/1000.0, '.3f'))
            # 显示读取过程
            print(count)
            print(format(float(train.loc[i][0][18:-3])/1000.0, '.3f'))
    return opt  # 返回list


def showpic(x, y, name):
    plt.figure()
    plt.plot(x, y)
    plt.xlabel('迭代轮数')
    plt.ylabel(name)
    plt.show()


if __name__ == '__main__':
    address = './accuracy/da.txt'
    train = readfile(address)
    winrate = best_winrate(train)
    time_spent = timespent(train)
    x = np.arange(1, len(time_spent)+1, 1)  # start end num x值
    showpic(x, time_spent, '时间花费')
    showpic(x, winrate, '胜率')
