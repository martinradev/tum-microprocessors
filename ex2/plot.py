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

with open("l1_dtlb_size.txt", "r") as f, open("l1_dtlb2mb_size.txt") as f2:
    xV, yV = readDataFromFile(f)
    fix1, ax1 = pl.subplots()
    dtlb4k, = pl.plot(xV, yV)
    xV, yV = readDataFromFile(f2)
    dtlb2mb, = pl.plot(xV,yV);
    ax1.set_xticks([0, 32, 64, 256, 512, 1024])
    ax1.set(xlabel="Num pages", ylabel="cycles/access", title="D-TLB size")
    pl.legend([dtlb4k, dtlb2mb], ["4KB page", "2MB page"])
    pl.show()

with open("itlb_size_data.txt", "r") as f, open("itlb2mb_size_data.txt", "r") as f2:
    xV, yV = readDataFromFile(f)
    fix1, ax1 = pl.subplots()
    itlb4k, = pl.plot(xV, yV)
    xV, yV = readDataFromFile(f2)
    itlb2mb, = pl.plot(xV, yV)
    ax1.set_xticks([0, 8, 64, 128, 256])
    ax1.set(xlabel="Num pages", ylabel="cycles/access", title="I-TLB size")
    pl.legend([dtlb4k, dtlb2mb], ["4KB page", "2MB page"])
    pl.show()

with open("cache_line_data.txt", "r") as f:
    xV, yV = readDataFromFile(f)
    fix1, ax1 = pl.subplots()
    pl.plot(xV, yV)
    ax1.set_xticks([0, 32, 64, 96, 128])
    ax1.set(xlabel="stride", ylabel="cycles/byte", title="Cache line size")
    pl.show()

with open("cache_size_data.txt", "r") as f, open("cache_size_data_2mb.txt", "r") as f2:
    xV, yV = readDataFromFile(f)
    fig, ax1 = pl.subplots()
    size4k, = ax1.plot(xV, yV)
    xV, yV = readDataFromFile(f2)
    size2mb, = ax1.plot(xV, yV)
    ax1.set_xscale('log')
    pl.legend([size4k, size2mb], ["4KB page", "2MB page"])
    t = [128, 32*1024, 128*1024, 256*1024, 1024*1024, 6*1024*1024, 12*1024*1024, 24*1024*1024]
    tl = ['128B', '32KB', '128KB', '256KB', '1MB', '6MB', '12MB', '24MB']
    ax1.set_xticklabels(tl)
    ax1.set_xticks(t)
    ax1.set(xlabel="work set size", ylabel="cycles/access", title="Data cache size")
    pl.show()

with open("icache_size_data.txt", "r") as f:
    xV, yV = readDataFromFile(f)
    fig, ax1 = pl.subplots()
    ax1.plot(xV, yV)
    ax1.set_xscale('log')
    t = [ 32*1024, 256*1024]
    tl = [ '32KB', '256KB']
    ax1.set_xticklabels(tl)
    ax1.set_xticks(t)
    ax1.set(xlabel="Work set size", ylabel="cycles/access", title="Instruction cache size")
    pl.show()


with open("l1_assoc_data.txt", "r") as f:
    xV, yV = readDataFromFile(f)
    fig, ax1 = pl.subplots()
    ax1.plot(xV, yV)
    ax1.set_xscale('log')
    t = [2**i for i in range(0,7)]
    tl = [str(i) + "way" for i in t]
    ax1.set_xticklabels(tl)
    ax1.set_xticks(t)
    pl.show()

with open("gpu_cache_line_size_data.txt", "r") as f:
    xV, yV = readDataFromFile(f)
    fix1, ax1 = pl.subplots()
    pl.plot(xV, yV)
    ax1.set(xlabel="stride", ylabel="cycles/byte", title="Cache line size")
    pl.show()


