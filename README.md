# ACM-ICN-2021-FWUPDATE

[![Paper][paper-badge]][paper-link]

Code and documentation to reproduce experimental results of the paper **[Reliable Firmware Updates for the Information-Centric Internet of Things][paper-link]** published in Proc. of ACM ICN 2021.

* Cenk Gündogan, Christian Amsüss, Thomas C. Schmidt, Matthias Wählisch,
**Reliable Firmware Updates for the Information-Centric Internet of Things**,
In: Proc. of 8th ACM Conference on Information-Centric Networking (ICN), ACM : New York, September 2021.

    **Abstract**
    > Security in the Internet of Things (IoT) requires ways to regularly update firmware  in the field. These demands ever increase with new, agile concepts such as security as code and should be considered a regular operation. Hosting massive firmware roll-outs present a crucial challenge for the constrained wireless environment.
    > In this paper, we explore how information-centric networking can ease reliable firmware updates. We start from the recent standards developed by  the IETF SUIT working group and contribute a system that allows for a timely discovery of new firmware versions by using cryptographically protected manifest files.
    > Our design enables a cascading firmware roll-out from a gateway towards leaf nodes in a low-power multi-hop network.
    > While a chunking mechanism prepares firmware images for typically low-sized maximum transmission units (MTUs), an early Denial-of-Service (DoS) detection prevents the distribution of tampered or malformed chunks.
    > In experimental evaluations on a real-world IoT testbed, we demonstrate feasible  strategies with adaptive bandwidth consumption and a high resilience to connectivity loss when replicating firmware images into the IoT edge.

Please follow our [Getting Started](getting-started.md) instructions for further information on how to compile and execute the code.

<!-- TODO: update URLs -->
[paper-link]:https://github.com/inetrg/ACM-ICN-2021-FWUPDATE
<!-- [paper-badge]:https://img.shields.io/badge/Paper-IEEE%20Xplore-green -->
[paper-badge]:https://img.shields.io/badge/Paper-ACM%20DL-gray
