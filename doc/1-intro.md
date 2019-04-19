# Title

Towards Read-Optimized Key-value Stores on Flash Storage

# Abstract

键值存储已经在大规模数据存储应用中扮演着十分重要的角色。基于LSM-Tree结构的键值存储在频繁写（write-intensive）的workload上能够取得很好的性能，主要因为是它能够把一系列的随机写转换为一批（batch）的顺序写，并通过Compaction操作形成层次结构。但是，在读写共存的workload或全读的workload上，LSM-Tree需要花费更高的延迟。造成这个问题的主要原因在于LSM-Tree结构中的层次查找机制和Compaction机制，但因为这两个机制难以被改变，所以我们所面临的关键挑战在于如何在已有机制的基础上提出新的策略以提高LSM-Tree的读性能，同时也保持几乎一样的写性能。

本文提出FSLSM-Tree，一种上浮可伸展的日志结构合并树（<u>*F*</u>loating <u>*S*</u>play <u>*LSM-Tree*</u>）。FSLSM-Tree首先允许频繁访问的SSTable从低层移动到高层，以减少查找延迟。其次还支持树为了适应不同的workload而改变自己的形状，使其性能达到最大化。目的是提高LSM-Tree在读紧张（read-intensive）的workload上的性能，并使LSM-Tree的结构得到最大化使用。为了验证FSLSM-Tree的可行性，我们使用数据库标准测试工具设计了一系列的实验。实验结果表明，相比于具有代表性的方案，FSLSM-Tree能够明显的提高读性能，减少读放大。

# Intro

