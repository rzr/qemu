/* Each row contains 8 values for the pitch table. The lowest index value is
   stored in the MSB (i.e. big endian ordering) */
celt_uint16_t pgain_table[512] = {
0x0000, 0x0000, 0x0000, 0x0000, 
0x140d, 0x0908, 0x5b11, 0x0f07, 
0x8380, 0x7608, 0x0807, 0x0705, 
0x6078, 0x0605, 0x0706, 0x0504, 
0x8378, 0x4118, 0x520f, 0x0d06, 
0x4c06, 0x3105, 0x0706, 0x0704, 
0x0f4c, 0x0606, 0x0b0b, 0x0b06, 
0x837d, 0x4538, 0x0c09, 0x0906, 
0x7c05, 0x0425, 0x0605, 0x0604, 
0x8c87, 0x837f, 0x7064, 0x293a, 
0x8c87, 0x827a, 0x715f, 0x1008, 
0x4a05, 0x0404, 0x0504, 0x0704, 
0x8322, 0x0403, 0x0504, 0x0504, 
0x1968, 0x0b68, 0x130e, 0x0b05, 
0x710d, 0x3d08, 0x3508, 0x0804, 
0x7c46, 0x4446, 0x0d0a, 0x0705, 
0x8a81, 0x7647, 0x203f, 0x0f09, 
0x8155, 0x0637, 0x0706, 0x0504, 
0x8b84, 0x7e7a, 0x460d, 0x0b07, 
0x0a27, 0x0705, 0x0607, 0x0809, 
0x7d06, 0x2504, 0x0505, 0x0604, 
0x8980, 0x3c74, 0x684f, 0x170c, 
0x7c32, 0x3406, 0x0606, 0x0604, 
0x2905, 0x0504, 0x0505, 0x0606, 
0x151e, 0x1a6e, 0x5713, 0x0e07, 
0x6128, 0x0505, 0x0605, 0x0604, 
0x8c88, 0x8685, 0x7a6f, 0x6715, 
0x7b08, 0x4b05, 0x0507, 0x0504, 
0x8277, 0x6612, 0x0e13, 0x470b, 
0x6804, 0x0403, 0x0405, 0x0604, 
0x8241, 0x0404, 0x0405, 0x0504, 
0x7908, 0x064a, 0x0608, 0x0604, 
0x4c72, 0x450a, 0x0a08, 0x0804, 
0x4909, 0x0807, 0x3507, 0x0704, 
0x857c, 0x4871, 0x0f0c, 0x0906, 
0x7e6e, 0x0f60, 0x510f, 0x0a06, 
0x8278, 0x231a, 0x4250, 0x5514, 
0x7e28, 0x2370, 0x5944, 0x150a, 
0x7a2f, 0x0631, 0x0806, 0x0704, 
0x8981, 0x7b77, 0x1460, 0x5b14, 
0x8680, 0x0504, 0x0505, 0x0604, 
0x206d, 0x645b, 0x5c1a, 0x0e07, 
0x877d, 0x7614, 0x6763, 0x6226, 
0x8d88, 0x8482, 0x795d, 0x3b0c, 
0x4c0a, 0x0631, 0x0707, 0x0704, 
0x8481, 0x3c05, 0x0808, 0x0705, 
0x0a0b, 0x0908, 0x0b37, 0x0f08, 
0x8c86, 0x817c, 0x483f, 0x110a, 
0x494e, 0x090a, 0x4109, 0x0803, 
0x730e, 0x0909, 0x590b, 0x0a04, 
0x7f30, 0x6b73, 0x1e46, 0x4518, 
0x8a81, 0x7b4b, 0x6215, 0x0a07, 
0x7f77, 0x0a6a, 0x0908, 0x0805, 
0x7a60, 0x0d0a, 0x0a0d, 0x4107, 
0x8377, 0x3166, 0x191f, 0x4e0e, 
0x0f09, 0x3607, 0x0809, 0x0a08, 
0x8983, 0x7b23, 0x6452, 0x150d, 
0x4639, 0x3e07, 0x0909, 0x0704, 
0x0e06, 0x0731, 0x0608, 0x0908, 
0x480a, 0x6508, 0x0908, 0x0704, 
0x1c13, 0x7165, 0x190f, 0x0906, 
0x8170, 0x1c6a, 0x1c4d, 0x190d, 
0x7a0f, 0x0708, 0x0932, 0x0a04, 
0x7c40, 0x0806, 0x3107, 0x0604, 
0x7809, 0x3636, 0x0807, 0x0804, 
0x8c85, 0x817f, 0x7423, 0x0e0a, 
0x440f, 0x0967, 0x0a09, 0x0805, 
0x8442, 0x7574, 0x664f, 0x190d, 
0x1716, 0x5511, 0x4c0e, 0x0b05, 
0x4b5c, 0x0a41, 0x0908, 0x0704, 
0x7c6d, 0x0c08, 0x0a3a, 0x0905, 
0x773f, 0x0a6a, 0x0908, 0x0704, 
0x897f, 0x7753, 0x4836, 0x3b0e, 
0x720f, 0x4e70, 0x0c0c, 0x0705, 
0x8430, 0x7276, 0x6b5c, 0x5c1c, 
0x7740, 0x700a, 0x0808, 0x0704, 
0x8c86, 0x8280, 0x495d, 0x4b0d, 
0x8361, 0x0304, 0x0505, 0x0404, 
0x0c08, 0x0708, 0x090b, 0x350b, 
0x867d, 0x751d, 0x6623, 0x480e, 
0x3a33, 0x0606, 0x0707, 0x0704, 
0x8877, 0x4879, 0x6f64, 0x5e22, 
0x2467, 0x625e, 0x0c0c, 0x0a06, 
0x8b84, 0x807d, 0x6d3c, 0x5638, 
0x8303, 0x0303, 0x0405, 0x0604, 
0x887d, 0x4973, 0x5613, 0x0c08, 
0x847c, 0x6b0c, 0x1143, 0x0f08, 
0x7c08, 0x0506, 0x2b06, 0x0604, 
0x7b09, 0x0872, 0x0908, 0x0704, 
0x6f11, 0x0a50, 0x4409, 0x0904, 
0x7f5c, 0x2805, 0x0606, 0x0504, 
0x0e53, 0x3f0a, 0x0b0b, 0x0a05, 
0x0d77, 0x0a0a, 0x0a0d, 0x0a05, 
0x7e1d, 0x636b, 0x5915, 0x1208, 
0x0c0b, 0x0806, 0x2d09, 0x0a08, 
0x8a86, 0x7e79, 0x0f11, 0x0906, 
0x7f76, 0x0809, 0x3f08, 0x0704, 
0x0b0c, 0x0769, 0x0a0c, 0x0f08, 
0x8676, 0x1475, 0x6050, 0x521b, 
0x400f, 0x423c, 0x0b07, 0x0804, 
0x7423, 0x6712, 0x5611, 0x0b05, 
0x1062, 0x0f11, 0x4d0c, 0x0b05, 
0x8983, 0x7e75, 0x1a1e, 0x4510, 
0x815a, 0x5206, 0x0b08, 0x0705, 
0x8880, 0x7925, 0x1958, 0x541a, 
0x1340, 0x0b3d, 0x0b0b, 0x0a05, 
0x0e0e, 0x6a0b, 0x090b, 0x0b07, 
0x824e, 0x716c, 0x1811, 0x0a06, 
0x867e, 0x7911, 0x490e, 0x0907, 
0x8a84, 0x7f4e, 0x6f66, 0x5718, 
0x2966, 0x696b, 0x565c, 0x4c14, 
0x5e4e, 0x0505, 0x0507, 0x0504, 
0x8b84, 0x7f7b, 0x6b22, 0x490b, 
0x7719, 0x1663, 0x5856, 0x591a, 
0x3869, 0x0706, 0x0808, 0x0704, 
0x7b09, 0x7206, 0x0808, 0x0704, 
0x8981, 0x7b79, 0x1753, 0x160d, 
0x771b, 0x541c, 0x2d4c, 0x4215, 
0x8d87, 0x8583, 0x7b75, 0x6d49, 
0x1b67, 0x6f0a, 0x120c, 0x0b04, 
0x8a82, 0x7e7a, 0x3667, 0x6044, 
0x8882, 0x7a42, 0x120a, 0x0907, 
0x460c, 0x0807, 0x0a2f, 0x0905, 
0x8071, 0x1c1b, 0x5f4a, 0x130b, 
0x160d, 0x0a3f, 0x3e0c, 0x0905, 
0x750f, 0x6f44, 0x0c0b, 0x0805, 
0x827c, 0x0733, 0x0806, 0x0704, 
0x0e0f, 0x4259, 0x0c0b, 0x0b05, 
};

