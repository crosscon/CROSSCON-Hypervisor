## CROSSCON Hypervisor - A Lightweight Hypervisor

![code quality workflow](https://github.com/bao-project/bao-hypervisor/actions/workflows/code-quality.yaml/badge.svg)
![arm build workflow](https://github.com/bao-project/bao-hypervisor/actions/workflows/build-arm.yaml/badge.svg)
![riscv build workflow](https://github.com/bao-project/bao-hypervisor/actions/workflows/build-riscv.yaml/badge.svg)
![rh850 build workflow](https://github.com/bao-project/bao-hypervisor/actions/workflows/build-v850.yaml/badge.svg)
![tricore build workflow](https://github.com/bao-project/bao-hypervisor/actions/workflows/build-tricore.yaml/badge.svg)
Here you can find the initial version of the _CROSSCON Hypervisor_:  an Hypervisor developed as part of the [CROSSCON project](https://crosscon.eu/).  The repository contains an overview of the CROSSCON Hypervisor, featuring support for QEMU virt. For detailed description of the CROSSCON Hypervisor, please refer to Section 3.2 on the deliverable [D3.1: CROSSCON Open Security Stack Documentation ‐ Draft.](https://crosscon.eu/library/deliverables). This repository is part of the deliverable D3.2: CROSSCON Open Security Stack - Initial Version.

### Overview

**CROSSCON Hypervisor**, being built upon [Bao](https://github.com/bao-project/bao-hypervisor), is grounded in a lightweight, open-source and static partitioning hypervisor that aims to provide strong isolation and real-time guarantees. Its static partitioning designs targets mixed-criticality systems focusing on isolation for fault-containment and real-time behavior. To implement the static partitioning hypervisor architecture: (i) resources are statically partitioned and assigned at VM instantiation time; (ii) memory is statically assigned using 2-stage translation; (iii) IO is pass-through only; (iv) virtual interrupts are directly mapped to physical ones; and (v) it follows a 1-1 mapping of virtual to physical CPUs (with no need for a scheduler). In addition to the inherited architectural isolation features, CROSSCON Hypervisor also offers microarchitectural-level isolation, implementing mechanisms, such as cache coloring, to maintain the isolation of shared resources such as last‐level caches. However, the static partitioning hypervisor design come with its own set of challenges that limit their applicability in IoT systems. Such limitations encompass (i) the absence of dynamic VM creation and management, and (ii) the incapacity to deliver per‐VM TEE services. These challenges will be addressed by enhancing the static partitioning design of the Bao.


**Dynamic-VM feature:**
For dynamic VM, the CROSSCON Hypervisor aims to provide multiple VMs executing concurrently on a single CPU. For this, CROSSCON  Hypervisor leverage VM-stack mechanism detailed in D3.1. Additionally, to enable the creation of new VMs during run-time VM execution, VMs need to access to the hypervisor interface that enables them to send a config file to the hypervisor, i.e., through the VM create hypercall. The CROSSCON Hypervisor then parses this file to instantiate the new child VM.  After this, the hypervisor proceeds to instantiate the child VM, while removing all resources, except for the physical CPUs, from the parent VM. This process, illustrated in Figure 1, ensures the integrity and isolation of each VM, crucial for system security.

<p align="center">
    <img src="./imgs/crossconhyp-dynamicvm.drawio.png" width=70% height=70%>
</p>
<p align="center">Figure 1: Diagram for Dynamic VM creation.<p align="center">

Dynamic VMs require that a CROSSCON Hypervisor driver on the host OS interacts with the CROSSCON Hypervisor through the CROSSCON Hypervisor hypervisor call interface. This interface serves three main objectives: VM Creation, VM Destruction, and VM Invocation. For more detailed explanation refer to D3.1.

**Per-VM TEE feature:**

For Per-VM TEE feature, the CROSSCON Hypervisor aims to host trusted OSes, that typically rely on TEE technologies, within a VM. In addition, this features allows moving from a single trusted OS in the secure world to multiple trusted OS VMs running in the normal world. Similar to the Dynamic-VM feature, for per-VM TEE support, CROSSCON Hypervisor requires VM-stack mechanism to link/bind a GPOS VM to a Trusted OS. Figure 2 illustrates how the system establishes per VM TEEs. At boot-time the hypervisor reads the configuration file to establishe multiple GPOS/Trusted OS pairs.

<p align="center">
    <img src="./imgs/crossconhyp-pervmtee.drawio.png" width=70% height=70%>
</p>
<p align="center">Figure 2: Per VM TEEs.<p align="center">


During the CROSSCON project, OP-TEE has been integrated with CROSSCON Hypervisor to execute within VMs in both Arm and RISC-V. This feature enhances security by enabling per-VM TEE services and splitting a single TEE system into multiple isolated TEEs. For more detailed explanation about this process refer to D3.1.

**NOTE**: This is work in progress! Don't expect things to be complete. Use at your own risk.


Supported Platforms
-------------------

The full list of supported (and work in progress)
platforms is presented below:

**Armv8-A AArch64**
- [x] Xilinx Zynq UltraScale+ MPSoC ZCU102/4
- [ ] Ultra96 Zynq UltraScale+ ZU3EG
- [ ] NXP MCIMX8QM-CPU
- [ ] NVIDIA Jetson TX2
- [ ] 96Boards HiKey 960
- [x] Raspberry Pi 4
- [x] QEMU virt
<<<<<<< HEAD
- [x] Arm Fixed Virtual Platforms
- [x] Toradex Verdin iMX8M Plus (w/ Dahlia Carrier Board)
- [x] NXP S32G3
=======
- [ ] Arm Fixed Virtual Platforms
- [ ] Toradex Verdin iMX8M Plus (w/ Dahlia Carrier Board)
- [ ] BeagleBone AI-64
- [ ] NXP MCIMX8M-EVK
- [ ] 96Boards ROCK960
>>>>>>> b0b2808 (Rebase with up-to-date Bao)

**Armv7-A / Armv8-A AArch32**
- [ ] Arm Fixed Virtual Platforms
- [ ] QEMU virt

**Armv8-R AArch64**
- [ ] Arm Fixed Virtual Platforms

**Armv8-R AArch32**
<<<<<<< HEAD
- [x] Arm Fixed Virtual Platforms
- [x] Arm MPS3-AN536
- [x] Semidrive E3650
- [x] NXP S32Z270
=======
- [ ] Arm Fixed Virtual Platforms
- [ ] NXP S32Z/E
>>>>>>> b0b2808 (Rebase with up-to-date Bao)
- [ ] Renesas RZT2M

**RISC-V RV64**
- [x] QEMU virt 
- [ ] Rocket w/ H-extension 
- [x] CVA6 w/ H-extension 

**RISC-V RV32**
- [x] QEMU virt

**Infineon Tricore 1.8**
- [x] TC4x Communication Board

**Renesas RH850**
- [x] RH850 U2A16



Demos
------------

Please refer to [the example demos](https://github.com/crosscon/CROSSCON-Hypervisor-and-TEE-Isolation-Demos).



References
------------

1. José Martins, Adriano Tavares, Marco Solieri, Marko Bertogna, and Sandro Pinto.
"**Bao: A Lightweight Static Partitioning Hypervisor for Modern Multi-Core Embedded
Systems**". In Workshop on Next Generation Real-Time Embedded Systems (NG-RES 2020).
Schloss Dagstuhl-Leibniz-Zentrum für Informatik. 2020.
https://drops.dagstuhl.de/opus/volltexte/2020/11779/

2. José Martins and Sandro Pinto. "**Bao: a modern lightweight embedded hypervisor**".
In Proceedings of the Embedded World Conference, Nuremberg, Germany, 2020.

3. José Martins and Sandro Pinto. "**Static Partitioning Virtualization on RISC-V**".
In RISC-V Summit, virtual, 2020. https://www.youtube.com/watch?v=yuxMn4ZApEM

4. Bruno Sá, José Martins and Sandro Pinto. "**A First Look at RISC-V Virtualization from an Embedded Systems Perspective**".
In IEEE Transactions on Computers, doi: 10.1109/TC.2021.3124320.

5. Samuel Pereira, João Sousa, Sandro Pinto, José Martins, David Cerdeira "**Bao-Enclave: Virtualization-based Enclaves for Arm**.
In https://arxiv.org/abs/2209.05572

6. José Martins and Sandro Pinto. "**Shedding Light on Static Partitioning Hypervisors for Arm-based Mixed-Criticality Systems**".
In RTAS 2023, San Antonio, Texas, 2023. https://arxiv.org/abs/2303.11186

7. José Martins and Sandro Pinto. "**Porting of a Static Partitioning Hypervisor to Arm’s Cortex-R52**"
In Embedded Open Source Summit 2023, Prague, Czech Republic, 2023. https://www.youtube.com/watch?v=GmeOikZJRas

8. David Cerdeira and José Martins. "**"Hello 'Bao' World" Tutorial**"
In Bao Half-Day, Virtual Workshop, 2023. https://www.youtube.com/watch?v=6c8_MG-OHYo

9. João Peixoto, José Martins, David Cerdeira and Sandro Pinto. "**BiRtIO: VirtIO for Real-Time Network Interface Sharing on the Bao Hypervisor**"
In IEEE Access, 2024. https://ieeexplore.ieee.org/document/10781314

### Community Resources

Project website:
- [https://crosscon.eu/](https://crosscon.eu/)

Source code:
- [https://github.com/crossconhyp-project/crossconhyp-hypervisor.git](https://github.com/crossconhyp-project/crossconhyp-hypervisor.git)
- [git@github.com](mailto:git@github.com):crossconhyp-project/crossconhyp-hypervisor.git


## License

See LICENSE file.

### Acknowledgments
The work presented in this repository is part of the [CROSSCON project](https://crosscon.eu/) that received funding from the European Union’s Horizon Europe research and innovation programme under grant agreement No 101070537.

[![](https://camo.githubusercontent.com/de8763a5a302b59dde046459283cfad6b985da63dc38e220385e9e339d991bcf/68747470733a2f2f63726f7373636f6e2e65752f73697465732f63726f7373636f6e2f7468656d65732f63726f7373636f6e2f696d616765732f65752e737667)](https://camo.githubusercontent.com/de8763a5a302b59dde046459283cfad6b985da63dc38e220385e9e339d991bcf/68747470733a2f2f63726f7373636f6e2e65752f73697465732f63726f7373636f6e2f7468656d65732f63726f7373636f6e2f696d616765732f65752e737667)

[![](https://camo.githubusercontent.com/7a17468e4cb9149ee035eb5a0b98e4675563a46e9402d7eb8ab0a08e32ce0e14/68747470733a2f2f63726f7373636f6e2e65752f73697465732f63726f7373636f6e2f66696c65732f7075626c69632f7374796c65732f6c617267655f313038305f2f7075626c69632f636f6e74656e742d696d616765732f6d656469612f323032332f63726f7373636f6e5f6c6f676f2e706e673f69746f6b3d4c554833656a7a4f)](https://camo.githubusercontent.com/7a17468e4cb9149ee035eb5a0b98e4675563a46e9402d7eb8ab0a08e32ce0e14/68747470733a2f2f63726f7373636f6e2e65752f73697465732f63726f7373636f6e2f66696c65732f7075626c69632f7374796c65732f6c617267655f313038305f2f7075626c69632f636f6e74656e742d696d616765732f6d656469612f323032332f63726f7373636f6e5f6c6f676f2e706e673f69746f6b3d4c554833656a7a4f)
