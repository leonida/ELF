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

# 胜率


def winrate(train):

    count = 0
    winrate = []  # 胜率

    for i in range(len(train)):
        # 读取每一行找到每轮迭代的最佳胜率
        if re.findall("best_win_rate+", train.loc[i][0]):
            count += 1
            # 读取胜率的值并转化为float类型
            winrate.append(float(train.loc[i][0][14:20]))
            # 显示读取过程
            print(count)
            print(train.loc[i][0])

    return winrate  # 返回list

# 时间花费


def timespent(train):

    count = 0
    timespent = []  # 时间花费

    for i in range(len(train)):
        # 读取时间花费的值转化为float类型，单位s，保留三位小数,位置会变动
        if re.findall("Time+", train.loc[i][0]):
            if count <= 10:
                timespent.append(
                    format(float(train.loc[i][0][18:-7])/1000.0, '.3f'))
            # 显示读取过程
            elif count <= 100:
                timespent.append(
                    format(float(train.loc[i][0][19:-7])/1000.0, '.3f'))
            elif count <= 1000:
                timespent.append(
                    format(float(train.loc[i][0][20:-7])/1000.0, '.3f'))

            print(train.loc[i][0])

    print(np.array(timespent).astype(float).mean())
    return timespent  # 返回list

# 平均奖励


def avgreward(train):
    count = 0
    avgreward = []
    for i in range(len(train)):
        # 读取每一行找到每轮迭代后的累计奖励
        if re.findall(":reward+", train.loc[i][0]):
            count += 1
            # 读取胜率的值并转化为float类型
            if count <= 10:
                avgreward.append(float(train.loc[i][0][21:28]))
                print(float(train.loc[i][0][21:28]))
            # 显示读取过程
            elif count <= 100:
                avgreward.append(float(train.loc[i][0][22:29]))
                print(float(train.loc[i][0][22:29]))
            elif count <= 1000:
                avgreward.append(float(train.loc[i][0][23:30]))
                print(float(train.loc[i][0][23:30]))
            print(count)
            print(train.loc[i][0])

    return avgreward
# 累积奖励


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

    return accreward

# 花费


def cost(train):
    count = 0
    cost = []
    for i in range(len(train)):
        # 读取每一行找到每轮迭代后的累计奖励
        if re.findall("cost+", train.loc[i][0]):
            count += 1
            # 读取胜率的值并转化为float类型
            if count <= 10:
                cost.append(float(train.loc[i][0][18:25]))
                print(float(train.loc[i][0][18:25]))
            # 显示读取过程
            elif count <= 100:
                cost.append(float(train.loc[i][0][19:26]))
                print(float(train.loc[i][0][19:26]))
            elif count <= 1000:
                cost.append(float(train.loc[i][0][20:27]))
                print(float(train.loc[i][0][20:27]))
            print(count)
            print(train.loc[i][0][:-5])

    return cost

# 画图


def showpic(x, y, name):
    plt.figure()
    plt.plot(x, y)
    plt.xlabel('iters')
    plt.ylabel(name)
    plt.plot(x, y, linewidth=1, color='red',
             marker='o', markerfacecolor='orange', markersize=1)
    plt.grid(c='g', linestyle='-.')  # 网格线
    plt.savefig("./analysis/" + name+"6.png")
    plt.show()


if __name__ == '__main__':
    address = './analysis/lstm1_2pi_18jia.out'
    train = readfile(address)
    # print(train.shape)
    # avg_reward = avgreward(train)
    # co = cost(train)
    # acc_reward = accreward(train)
    win_rate = winrate(train)
    # time_spend = timespent(train)
    x = np.arange(1, len(win_rate)+1, 1)  # start end num x值
    # showpic(x, acc_reward, 'accreward')
    # showpic(x, co, 'cost')
    # showpic(x, avg_reward, 'avgreward')
    showpic(x, win_rate, 'winrate')
    # showpic(x, time_spend, 'timespend')
