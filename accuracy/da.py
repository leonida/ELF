from datetime import time
from os import read
from typing import Counter
import pandas as pd
import re
import numpy as np
from matplotlib import pyplot as plt

# 读取文件导出为dataframe


def readfile(address):
    file = pd.read_csv(address, encoding='utf-8', header=None, sep='\t')
    train = pd.DataFrame(file)
    return train


def analysis(train):

    count = 0
    winrate = []  # 胜率
    timespent = []  # 时间花费

    for i in range(len(train)):
        # 读取每一行找到每轮迭代的最佳胜率
        if re.findall("best_win_rate+", train.loc[i][0]):
            count += 1
            # 读取胜率的值并转化为float类型
            winrate.append(float(train.loc[i][0][14:]))
            # 显示读取过程
            print(count)
            print(train.loc[i][0])

        # 读取时间花费的值转化为float类型，单位s，保留三位小数,位置会变动
        elif re.findall("Time+", train.loc[i][0]):
            if count <= 10:
                timespent.append(
                    format(float(train.loc[i][0][18:-3])/1000.0, '.3f'))
            # 显示读取过程
            elif count <= 100:
                timespent.append(
                    format(float(train.loc[i][0][19:-3])/1000.0, '.3f'))
            elif count <= 1000:
                timespent.append(
                    format(float(train.loc[i][0][20:-3])/1000.0, '.3f'))

            print(train.loc[i][0])

    return winrate, timespent  # 返回list

# 累计奖励


def accreward(train):
    count = 0
    accreward = []
    for i in range(len(train)):
        # 读取每一行找到每轮迭代后的累计奖励
        if re.findall("acc_reward+", train.loc[i][0]):
            count += 1
            # 读取胜率的值并转化为float类型
            if count <= 10:
                accreward.append(float(train.loc[i][0][24:32]))
                print(float(train.loc[i][0][24:32]))
            # 显示读取过程
            elif count <= 100:
                accreward.append(float(train.loc[i][0][25:33]))
                print(float(train.loc[i][0][25:33]))
            elif count <= 1000:
                accreward.append(float(train.loc[i][0][26:34]))
                print(float(train.loc[i][0][26:34]))
            print(count)
            print(train.loc[i][0])


def showpic(x, y, name):
    plt.figure()
    plt.plot(x, y)
    plt.xlabel('迭代轮数')
    plt.ylabel(name)
    plt.show()


if __name__ == '__main__':
    address = './accuracy/da.txt'
    train = readfile(address)

    winrate, timespent = analysis(train)
    x = np.arange(1, len(timespent)+1, 1)  # start end num x值
    showpic(x, timespent, '时间花费')
    showpic(x, winrate, '胜率')
    # acc_reward = accreward(train)
    # print(train.shape)
