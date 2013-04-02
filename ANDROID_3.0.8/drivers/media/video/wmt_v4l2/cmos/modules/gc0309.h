#ifndef GC0309_H
#define GC0309_H
unsigned char gc0309_320x240[]={
0xfe,0x01,
0x54,0x22,  // 1/2 subsample 
0x55,0x03, 
0x56,0x00, 
0x57,0x00, 
0x58,0x00, 
0x59,0x00, 

0xfe,0x00,
0x46,0x80,//enable crop window mode
0x47,0x00,  
0x48,0x00,  
0x49,0x00, 
0x4a,0xf0,//240
0x4b,0x01,  
0x4c,0x40, //320
0xfe,0x00,


};
unsigned char gc0309_640x480[]={

0xfe,0x80,          

0xfe,0x00,      

0x1a,0x16,         

0xd2,0x10,   

0x22,0x55,   

0x5a,0x56, 

0x5b,0x40,

0x5c,0x4a,                  

0x22,0x57,  

0x01,0x32, 
0x02,0x70, 
0x0f,0x01, 

0xe2,0x00, 
0xe3,0x78, 

0x03,0x01, 
0x04,0x2c, 

0xe4,0x02, 
0xe5,0x58, 

0xe6,0x03, 
0xe7,0x48, 

0xe8,0x05, 
0xe9,0xa0, 

0xea,0x05, 
0xeb,0xa0, 

0x05,0x00,

0x06,0x00,

0x07,0x00, 

0x08,0x00, 

0x09,0x01, 

0x0a,0xe8, 

0x0b,0x02, 

0x0c,0x88, 

0x0d,0x02, 

0x0e,0x02, 

0x10,0x22, 

0x11,0x0d, 

0x12,0x2a, 

0x13,0x00, 

0x14,0x10, 

0x15,0x0a, 

0x16,0x05, 

0x17,0x01, 

0x1b,0x03, 

0x1c,0xc1, 

0x1d,0x08, 

0x1e,0x20, 

0x1f,0x16, 

0x20,0xff, 

0x21,0xf8, 

0x24,0xa2, 

0x25,0x0f,

0x26,0x02, 

0x2f,0x01, 

0x30,0xf7, 

0x31,0x40,

0x32,0x00, 

0x39,0x04, 

0x3a,0x20, 

0x3b,0x20, 

0x3c,0x02, 

0x3d,0x02, 

0x3e,0x02,

0x3f,0x02, 

0x50,0x24, 

0x53,0x82, 

0x54,0x80, 

0x55,0x80, 

0x56,0x82, 

0x8b,0x20, 

0x8c,0x20, 

0x8d,0x20, 

0x8e,0x10, 

0x8f,0x10, 

0x90,0x10, 

0x91,0x3c, 

0x92,0x50, 

0x5d,0x12, 

0x5e,0x1a, 

0x5f,0x24, 

0x60,0x07, 

0x61,0x0e, 

0x62,0x0c, 

0x64,0x03, 

0x66,0xe8, 

0x67,0x86, 

0x68,0xa2, 

0x69,0x20, 

0x6a,0x0f, 

0x6b,0x00, 

0x6c,0x53, 

0x6d,0x83, 

0x6e,0xac, 

0x6f,0xac, 

0x70,0x15, 

0x71,0x33, 

0x72,0xdc,  

0x73,0x80,  

0x74,0x02, 

0x75,0x3f, 

0x76,0x02, 

0x77,0x54, 

0x78,0x88, 

0x79,0x81, 

0x7a,0x81, 

0x7b,0x22, 

0x7c,0xff,

0x93,0x45, 

0x94,0x00, 

0x95,0x00, 

0x96,0x00, 

0x97,0x45, 

0x98,0xf0, 

0x9c,0x00, 

0x9d,0x03, 

0x9e,0x00, 

0xb1,0x40, 

0xb2,0x40, 

0xb8,0x20, 

0xbe,0x36, 

0xbf,0x00, 

0xd0,0xcb,  

0xd1,0x10,  

0xd3,0x50,  

0xd5,0xf2, 

0xd6,0x16,  

0xdb,0x92, 

0xdc,0xa5,  

0xdf,0x23,   

0xd9,0x00,  

0xda,0x00,  

0xe0,0x09,

0xec,0x20,  

0xed,0x04,  

0xee,0xa0,  

0xef,0x40,  



0x9F, 0x10,   //case 3:
0xA0, 0x20,
0xA1, 0x38,
0xA2, 0x4E,
0xA3, 0x63,
0xA4, 0x76,
0xA5, 0x87,
0xA6, 0xA2,
0xA7, 0xB8,
0xA8, 0xCA,
0xA9, 0xD8,
0xAA, 0xE3,
0xAB, 0xEB,
0xAC, 0xF0,
0xAD, 0xF8,
0xAE, 0xFD,
0xAF, 0xFF,

#if 0
                   
0x9F, 0x0B,    //smallest gamma curve case 1:   
0xA0, 0x16, 
0xA1, 0x29,
0xA2, 0x3C,
0xA3, 0x4F,
0xA4, 0x5F,
0xA5, 0x6F,
0xA6, 0x8A,
0xA7, 0x9F,
0xA8, 0xB4,
0xA9, 0xC6,
0xAA, 0xD3,
0xAB, 0xDD, 
0xAC, 0xE5, 
0xAD, 0xF1,
0xAE, 0xFA,
0xAF, 0xFF,	

		
0x9F, 0x0E,   //case 2:	
0xA0, 0x1C,
0xA1, 0x34,
0xA2, 0x48,
0xA3, 0x5A,
0xA4, 0x6B,
0xA5, 0x7B,
0xA6, 0x95,
0xA7, 0xAB,
0xA8, 0xBF,
0xA9, 0xCE,
0xAA, 0xD9,
0xAB, 0xE4, 
0xAC, 0xEC,
0xAD, 0xF7,
0xAE, 0xFD,
0xAF, 0xFF,


0x9F, 0x10,   //case 3:
0xA0, 0x20,
0xA1, 0x38,
0xA2, 0x4E,
0xA3, 0x63,
0xA4, 0x76,
0xA5, 0x87,
0xA6, 0xA2,
0xA7, 0xB8,
0xA8, 0xCA,
0xA9, 0xD8,
0xAA, 0xE3,
0xAB, 0xEB,
0xAC, 0xF0,
0xAD, 0xF8,
0xAE, 0xFD,
0xAF, 0xFF,



0x9F, 0x14,   //case 4:
0xA0, 0x28,
0xA1, 0x44,
0xA2, 0x5D,
0xA3, 0x72,
0xA4, 0x86,
0xA5, 0x95,
0xA6, 0xB1,
0xA7, 0xC6,
0xA8, 0xD5,
0xA9, 0xE1,
0xAA, 0xEA,
0xAB, 0xF1,
0xAC, 0xF5,
0xAD, 0xFB,
0xAE, 0xFE,
0xAF, 0xFF,

	
0x9F, 0x15,// largest gamma curve  case 5:	
0xA0, 0x2A,
0xA1, 0x4A,
0xA2, 0x67,
0xA3, 0x79,
0xA4, 0x8C,
0xA5, 0x9A,
0xA6, 0xB3,
0xA7, 0xC5,
0xA8, 0xD5,
0xA9, 0xDF,
0xAA, 0xE8,
0xAB, 0xEE,
0xAC, 0xF3,
0xAD, 0xFA,
0xAE, 0xFD,
0xAF, 0xFF,


#endif 
0xc0,0x00,

0xc1,0x0B,

0xc2,0x15,

0xc3,0x27,

0xc4,0x39,

0xc5,0x49,

0xc6,0x5A,

0xc7,0x6A,

0xc8,0x89,

0xc9,0xA8,

0xca,0xC6,

0xcb,0xE3,

0xcc,0xFF,

0xf0,0x02,

0xf1,0x01,

0xf2,0x00,  

0xf3,0x30, 

0xf7,0x04, 

0xf8,0x02, 

0xf9,0x9f,

0xfa,0x78,

0xfe,0x01,

0x00,0xf5, 

0x02,0x1a, 

0x0a,0xa0, 

0x0b,0x60, 

0x0c,0x08,

0x0e,0x4c, 

0x0f,0x39, 

0x11,0x3f, 

0x12,0x72,         

0x13,0x13, 

0x14,0x42,  

0x15,0x43, 

0x16,0xc2, 

0x17,0xa8, 

0x18,0x18,  

0x19,0x40,  

0x1a,0xd0, 

0x1b,0xf5,  

0x70,0x40, 

0x71,0x58,  

0x72,0x30,  

0x73,0x48,  

0x74,0x20,  

0x75,0x60,  

0xfe,0x00,

0xd2,0x90,  

0x8b,0x22,   

0x71,0x43,   

0x93,0x48, 

0x94,0x00, 

0x95,0x05, 

0x96,0xe8, 

0x97,0x40, 

0x98,0xf8, 

0x9c,0x00, 

0x9d,0x00, 

0x9e,0x00, 

0xd0,0xcb,  

0xd3,0x50,   // a0 

0x31,0x60, 

0x1c,0x49, 

0x1d,0x98, 

0x10,0x26, 

0x1a,0x26,  

};
#endif
