#define ROOTSIG \
"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
"RootConstants(num32BitConstants=28, b0),"     /* 20 constants: w/ one int3, see PBMPM Constants */ \
"DescriptorTable(SRV(t0, numDescriptors=1)),"     /* SRV Table for g_particleCount */ \
"DescriptorTable(SRV(t1, numDescriptors=1)),"     /* SRV table for g_particles */ \
"DescriptorTable(SRV(t2, numDescriptors=1)),"     /* SRV table for g_positions */ \
"DescriptorTable(UAV(u0, numDescriptors=1))"      /* UAV table for g_bukkitCounts */