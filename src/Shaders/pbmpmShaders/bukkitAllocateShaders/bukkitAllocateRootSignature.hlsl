#define ROOTSIG \
"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
"RootConstants(num32BitConstants=18, b0),"     /* 17 constants: w/ one int2, see PBMPM Constants */ \
"DescriptorTable(SRV(t0)),"                      /* SRV for g_bukkitCounts */ \
"DescriptorTable(UAV(u0, numDescriptors=4)),"    /* UAVs for g_bukkitThreadData, g_bukkitParticleAllocator, g_bukkitIndirectDispatch, g_bukkitIndexStart */ \