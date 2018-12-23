import matplotlib.pyplot as pl
import matplotlib

def readDataFromFile(f):
    xV = []
    yV = []
    for line in f:
        tokens = line.split(" ")
        x = int(tokens[0])
        y = float(tokens[1])
        xV.append(x)
        yV.append(y)
    return xV, yV
   
with open("cache_line_data.txt", "r") as f:
    xV, yV = readDataFromFile(f)
    fix1, ax1 = pl.subplots()
    pl.plot(xV, yV)
    ax1.set_xticks([0, 32, 64, 96, 128])
    ax1.set(xlabel="stride", ylabel="cycles/byte", title="Cache line size")
    pl.show()

with open("cache_size_data.txt", "r") as f:
    xV, yV = readDataFromFile(f)
    fig, ax1 = pl.subplots()
    ax1.plot(xV, yV)
    ax1.set_xscale('log')
    t = [128, 32*1024, 256*1024, 1024*1024, 6*1024*1024, 12*1024*1024, 24*1024*1024]
    tl = ['128B', '32KB', '256KB', '1MB', '6MB', '12MB', '24MB']
    ax1.set_xticklabels(tl)
    ax1.set_xticks(t)
    pl.show()

