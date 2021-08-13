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
            print(train.loc[i][0])

    return cost


def showpic(x, y, name):
    plt.figure()
    plt.plot(x, y)
    plt.xlabel('iters')
    plt.ylabel(name)
    plt.savefig("./accuracy/" + name+".png")
    plt.show()


if __name__ == '__main__':
    address = './accuracy/da.txt'
    train = readfile(address)
    # co = cost(train)
    # acc_reward = accreward(train)
    winrate, timespent = analysis(train)
    x = np.arange(1, len(winrate)+1, 1)  # start end num x值
    # showpic(x, acc_reward, 'accreward')
    # showpic(x, co, 'cost')
    # print(train.shape)
    # showpic(x, timespent, '时间花费')
    showpic(x, winrate, 'winrate')
