//#define ROOTSIG \
//"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
//"RootConstants(num32BitConstants=3, b0)"

#define ROOTSIG \
"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
"RootConstants(num32BitConstants=32, b0),"   /* For view and projection matrices */ \
"DescriptorTable(SRV(t0))"                   /* SRV for model matrices buffer */