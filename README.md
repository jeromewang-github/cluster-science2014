cluster-science2014
===================

Clustering algorithm appeared on Science in 2014 is implemented in C++

/************************************************************
FileName: cluster_sci14.cpp
Author: Yunfei WANG
E-mail: wangyunfeiysm@163.com
Date: Nov.23,2014
***********************************************************/


OPTIONS

    --input     Requests that all the samples are stored in the given file
                in which each row is a sample.
    --clusters  Specify the number of clusters.
                If clusters<=0, the program will estimate the appropriate
                number of clusters automatically.
    --mode      Specify the approach to compute density.
                0-Gauss Kernel(default)
                1-Cutoff Kernel
                2-K Nearest Neighbors
    --neighbors Specify the number of nearest neighbors, which is
                used to estiamte the density of each sample. It works
                only when the option '--mode 2'  is used.(default 5)
    --metric    Specify the metric used to compute distance two samples.
                0-Euclidean(default)
                1-Cosine
    --output    Specify the name of output files(default:the same as inputfile).
                The file named output.decisiongraph stores the decision graph,
                the two columns correspond to rho and delta respectively for each sample.
                The file named output.result stores the clustering result,
                in which the first column indicates the index of each sample
                and the second column indicate the index of its cluster.
    --help
