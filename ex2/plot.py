import matplotlib.pyplot as pl

with open("cache_line_data.txt", "r") as f:
    xV = []
    yV = []
    for line in f:
        tokens = line.split(" ")
        x = int(tokens[0])
        y = float(tokens[1])
        xV.append(x)
        yV.append(y)
    pl.plot(xV, yV)
    pl.show()
