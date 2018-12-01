import numpy as np
import matplotlib.pyplot as pp

theoreticalX = [0, 39010000]
theoreticalY = [0, 0.00152382812]
linestyles = ['-', '--', '-.', ':']

def Plot(compiler):
    iccResultsOptimized = [[], [], [], [], []]
    iccResultsSimple = [[], [], [], [], []]

    for opt in range(1,5):
        for t in range(1, 13):
            fname = "toupper_variant/toupper_variant" + str(opt) + "_" + compiler + "_10000_40000000_" + str(t) + ".txt"
            file = open(fname, 'r')
            next(file)
            tmpOptimized = []
            tmpSimple = []
            for line in file:
                toks = ["Size: ", "simple: ", "optimised: "]
                ends = [" ", "\t", "\t"]
                res = []
                for (u,v) in zip(toks, ends):
                    a = line.find(u) + len(u)
                    b = line.find(v, a)
                    res.append(line[a: b])
                tmpOptimized.append((int(res[0]), float(res[2])))
                tmpSimple.append((int(res[0]), float(res[1])))
            iccResultsOptimized[opt].append(tmpOptimized)
            if t==1:
                iccResultsSimple[opt].append(tmpSimple)
    resultsOpt = iccResultsOptimized
    resultsSimple = iccResultsSimple
    fig = pp.figure()
    ax = fig.add_subplot(1,1,1)

    # compare thread scalability
    ids = []
    for t in range(0,12):
        style = linestyles[t % len(linestyles)]
        plotId, = pp.plot([u[0] for u in iccResultsOptimized[3][t]], [u[1] for u in iccResultsOptimized[3][t]], style)
        ids.append(plotId)

    theoreticalId, = pp.plot(theoreticalX, theoreticalY, linestyles[0])
    idsLegend = ids
    idsLegend.append(theoreticalId)
    legendStr = ["t=" + str(t) for t in range(1, 13)]
    legendStr.append("Theoretical")

    pp.legend(idsLegend, legendStr)
    ax.set_title(compiler + ": Thread scalability with -02")
    ax.set_xlabel("Input size (bytes)")
    ax.set_ylabel("Time (sec)")
    pp.show()

    #Compare optimization levels for the optimized with t=6
    fig = pp.figure()
    ax = fig.add_subplot(1,1,1)

    ids = []
    for q in range(1,5):
        style = linestyles[q % len(linestyles)]
        plotId, = pp.plot([u[0] for u in iccResultsOptimized[q][5]], [u[1] for u in iccResultsOptimized[q][5]], style)
        ids.append(plotId)
    idsLegend = ids
    theoreticalId, = pp.plot(theoreticalX, theoreticalY, linestyles[0])
    idsLegend.append(theoreticalId)
    legendStr = ["-O" + str(t) for t in range(0, 4)]
    legendStr.append("Theoretical")

    pp.legend(idsLegend, legendStr)
    ax.set_title(compiler + ": Comparison of compiler optimizations for optimized impl with t=6")
    ax.set_xlabel("Input size (bytes)")
    ax.set_ylabel("Time (sec)")
    pp.show()

    #Compare optimization levels for the non-optimized version
    fig = pp.figure()
    ax = fig.add_subplot(1,1,1)

    ids = []
    for q in range(1,5):
        style = linestyles[q % len(linestyles)]
        plotId, = pp.plot([u[0] for u in iccResultsSimple[q][0]], [u[1] for u in iccResultsSimple[q][0]], style)
        ids.append(plotId)
    idsLegend = ids
    theoreticalId, = pp.plot(theoreticalX, theoreticalY, linestyles[0])
    idsLegend.append(theoreticalId)
    legendStr = ["-O" + str(t) for t in range(0, 4)]
    legendStr.append("Theoretical")

    pp.legend(idsLegend, legendStr)
    ax.set_title(compiler + ": Comparison of compiler optimizations for the simple impl")
    ax.set_xlabel("Input size (bytes)")
    ax.set_ylabel("Time (sec)")
    pp.show()

    fig = pp.figure()
    ax = fig.add_subplot(1,1,1)

    #Compare optimized and non-optimized
    ids = []

    style = linestyles[0]
    plotId, = pp.plot([u[0] for u in iccResultsOptimized[3][0]], [u[1] for u in iccResultsOptimized[3][0]], style)
    ids.append(plotId)

    style = linestyles[1]
    plotId, = pp.plot([u[0] for u in iccResultsOptimized[3][5]], [u[1] for u in iccResultsOptimized[3][5]], style)
    ids.append(plotId)

    style = linestyles[2]
    plotId, = pp.plot([u[0] for u in iccResultsSimple[3][0]], [u[1] for u in iccResultsSimple[3][0]], style)
    ids.append(plotId)

    idsLegend = ids
    theoreticalId, = pp.plot(theoreticalX, theoreticalY, linestyles[0])
    idsLegend.append(theoreticalId)
    legendStr = ["Optimized t=1", "Optimized t=6", "Not-optimized"]
    legendStr.append("Theoretical")

    pp.legend(idsLegend, legendStr)
    ax.set_title(compiler + ": Comparison of optimized and non-optimized")
    ax.set_xlabel("Input size (bytes)")
    ax.set_ylabel("Time (sec)")
    pp.show()

    return (iccResultsOptimized, iccResultsSimple)

iccOpt = []
iccSimple = []
(iccOpt, iccSimple) = Plot("icc")
(gccOpt, gccSimple) = Plot("gcc")

fig = pp.figure()
ax = fig.add_subplot(1,1,1)

#Compare optimized and non-optimized
ids = []

style = linestyles[0]
plotId1, = pp.plot([u[0] for u in iccOpt[3][0]], [u[1] for u in iccOpt[3][0]], style)

style = linestyles[1]
plotId2, = pp.plot([u[0] for u in iccOpt[3][5]], [u[1] for u in iccOpt[3][5]], style)

style = linestyles[2]
plotId3, = pp.plot([u[0] for u in iccSimple[3][0]], [u[1] for u in iccSimple[3][0]], style)

style = linestyles[3]
plotId4, = pp.plot([u[0] for u in gccOpt[3][0]], [u[1] for u in gccOpt[3][0]], style)

style = linestyles[0]
plotId5, = pp.plot([u[0] for u in gccOpt[3][5]], [u[1] for u in gccOpt[3][5]], style)

style = linestyles[1]
plotId6, = pp.plot([u[0] for u in gccSimple[3][0]], [u[1] for u in gccSimple[3][0]], style)

idsLegend = [plotId1, plotId2, plotId3, plotId4, plotId5, plotId6] 
theoreticalId, = pp.plot(theoreticalX, theoreticalY, linestyles[0])
idsLegend.append(theoreticalId)
legendStr = ["icc opt t=1", "icc opt t=6", "icc not opt", "gcc opt t=1", "gcc opt t=6", "gcc not opt"]
legendStr.append("Theoretical")

pp.legend(idsLegend, legendStr)
ax.set_title("Comparison of gcc and icc")
ax.set_xlabel("Input size (bytes)")
ax.set_ylabel("Time (sec)")
pp.show()

