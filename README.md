# Implementation-of-Stabilized-RED-in-ns-3
## Overview 
Stabilized RED is an Active Queue Management algorithm which proactively drops the packets using a load-dependent probability when a buffer appears congested [1]. It stabilizes the buffer occupation independent of the number of active connections. This is done by estimating the number of active flows without collecting the state information about individual flows. This is also used to identify the misbehaving flows which consume more bandwidth than their fair share. SRED is already implemented in ns-2 [2] and this repository contains the implementation of SRED in ns-3 [3]. 
## References
[1] Teunis J., Ott T.V. Lakshman.(1999).SRED: Stabilized RED. INFOCOM '99. Eighteenth Annual Joint Conference of the IEEE Computer and Communications Societies. Proceedings. IEEE.

[2] http://www.isi.edu/nsnam/ns/

[3] http://www.nsnam.org/
