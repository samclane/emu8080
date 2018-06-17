#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <stddef.h>
#include "SDL2/SDL.h"

//Screen dimension constants
const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 224;

typedef struct ConditionCodes {
    uint8_t z:1;
    uint8_t s:1;
    uint8_t p:1;
    uint8_t cy:1;
    uint8_t ac:1;
    uint8_t pad:3;
} ConditionCodes;

// Space invaders I/O ports
typedef struct Ports {
    uint8_t     read1;
    uint8_t     read2;
    uint8_t     shift1;
    uint8_t     shift0;

    uint8_t     write2; // shift amount (bits 0,1,2)
    uint8_t     write4; // shift data
} Ports;

typedef struct State8080 {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
    uint16_t sp;
    uint16_t pc;
    uint8_t *memory;
    struct ConditionCodes cc;
    struct Ports port;
    uint8_t int_enable;
} State8080;

void LogicFlagsA(State8080 *state) {
    state->cc.cy = state->cc.ac = 0;
    state->cc.z = (state->a == 0);
    state->cc.s = (0x80 == (state->a & 0x80));
    state->cc.p = parity(state->a, 8);
}

static void ArithFlagsA(State8080 *state, uint16_t res)
{
    state->cc.cy = (res > 0xff);
    state->cc.z = ((res&0xff) == 0);
    state->cc.s = (0x80 == (res & 0x80));
    state->cc.p = parity(res&0xff, 8);
}

void FlagsZSP(State8080 *state, uint8_t value) {
    state->cc.z = (value == 0);
    state->cc.s = (0x80 == (value & 0x80));
    state->cc.p = parity(state->a, 8);
    state->cc.ac = ((value & 0xf) == 0xf);
}

void UnimplementedInstruction(State8080 *state) {
    //pc will have advanced one, so undo that
    state->pc--;
    unsigned char *opcode = &state->memory[state->pc];
    printf("Error: Unimplemented instruction. Opcode: <0x%x>\n", *opcode);
    exit(1);
}

int parity(int x, int size) {
    int i;
    int p = 0;
    x = (x & ((1 << size) - 1));
    for (i = 0; i < size; i++) {
        if (x & 0x1) p++;
        x = x >> 1;
    }
    return (0 == (p & 0x1));
}


int Disassemble8080Op(unsigned char *codebuffer, int pc) {
    unsigned char *code = &codebuffer[pc];
    int opbytes = 1;
    printf("%04x ", pc);
    switch (*code) {
        case 0x00:
            printf("NOP");
            break;
        case 0x01:
            printf("LXI    B,#$%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0x02:
            printf("STAX   B");
            break;
        case 0x03:
            printf("INX    B");
            break;
        case 0x04:
            printf("INR    B");
            break;
        case 0x05:
            printf("DCR    B");
            break;
        case 0x06:
            printf("MVI    B,#$%02x", code[1]);
            opbytes = 2;
            break;
        case 0x07:
            printf("RLC");
            break;
        case 0x08:
            printf("NOP");
            break;
        case 0x09:
            printf("DAD    B");
            break;
        case 0x0a:
            printf("LDAX   B");
            break;
        case 0x0b:
            printf("DCX    B");
            break;
        case 0x0c:
            printf("INR    C");
            break;
        case 0x0d:
            printf("DCR    C");
            break;
        case 0x0e:
            printf("MVI    C,#$%02x", code[1]);
            opbytes = 2;
            break;
        case 0x0f:
            printf("RRC");
            break;

        case 0x10:
            printf("NOP");
            break;
        case 0x11:
            printf("LXI    D,#$%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0x12:
            printf("STAX   D");
            break;
        case 0x13:
            printf("INX    D");
            break;
        case 0x14:
            printf("INR    D");
            break;
        case 0x15:
            printf("DCR    D");
            break;
        case 0x16:
            printf("MVI    D,#$%02x", code[1]);
            opbytes = 2;
            break;
        case 0x17:
            printf("RAL");
            break;
        case 0x18:
            printf("NOP");
            break;
        case 0x19:
            printf("DAD    D");
            break;
        case 0x1a:
            printf("LDAX   D");
            break;
        case 0x1b:
            printf("DCX    D");
            break;
        case 0x1c:
            printf("INR    E");
            break;
        case 0x1d:
            printf("DCR    E");
            break;
        case 0x1e:
            printf("MVI    E,#$%02x", code[1]);
            opbytes = 2;
            break;
        case 0x1f:
            printf("RAR");
            break;

        case 0x20:
            printf("NOP");
            break;
        case 0x21:
            printf("LXI    H,#$%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0x22:
            printf("SHLD   $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0x23:
            printf("INX    H");
            break;
        case 0x24:
            printf("INR    H");
            break;
        case 0x25:
            printf("DCR    H");
            break;
        case 0x26:
            printf("MVI    H,#$%02x", code[1]);
            opbytes = 2;
            break;
        case 0x27:
            printf("DAA");
            break;
        case 0x28:
            printf("NOP");
            break;
        case 0x29:
            printf("DAD    H");
            break;
        case 0x2a:
            printf("LHLD   $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0x2b:
            printf("DCX    H");
            break;
        case 0x2c:
            printf("INR    L");
            break;
        case 0x2d:
            printf("DCR    L");
            break;
        case 0x2e:
            printf("MVI    L,#$%02x", code[1]);
            opbytes = 2;
            break;
        case 0x2f:
            printf("CMA");
            break;

        case 0x30:
            printf("NOP");
            break;
        case 0x31:
            printf("LXI    SP,#$%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0x32:
            printf("STA    $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0x33:
            printf("INX    SP");
            break;
        case 0x34:
            printf("INR    M");
            break;
        case 0x35:
            printf("DCR    M");
            break;
        case 0x36:
            printf("MVI    M,#$%02x", code[1]);
            opbytes = 2;
            break;
        case 0x37:
            printf("STC");
            break;
        case 0x38:
            printf("NOP");
            break;
        case 0x39:
            printf("DAD    SP");
            break;
        case 0x3a:
            printf("LDA    $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0x3b:
            printf("DCX    SP");
            break;
        case 0x3c:
            printf("INR    A");
            break;
        case 0x3d:
            printf("DCR    A");
            break;
        case 0x3e:
            printf("MVI    A,#$%02x", code[1]);
            opbytes = 2;
            break;
        case 0x3f:
            printf("CMC");
            break;

        case 0x40:
            printf("MOV    B,B");
            break;
        case 0x41:
            printf("MOV    B,C");
            break;
        case 0x42:
            printf("MOV    B,D");
            break;
        case 0x43:
            printf("MOV    B,E");
            break;
        case 0x44:
            printf("MOV    B,H");
            break;
        case 0x45:
            printf("MOV    B,L");
            break;
        case 0x46:
            printf("MOV    B,M");
            break;
        case 0x47:
            printf("MOV    B,A");
            break;
        case 0x48:
            printf("MOV    C,B");
            break;
        case 0x49:
            printf("MOV    C,C");
            break;
        case 0x4a:
            printf("MOV    C,D");
            break;
        case 0x4b:
            printf("MOV    C,E");
            break;
        case 0x4c:
            printf("MOV    C,H");
            break;
        case 0x4d:
            printf("MOV    C,L");
            break;
        case 0x4e:
            printf("MOV    C,M");
            break;
        case 0x4f:
            printf("MOV    C,A");
            break;

        case 0x50:
            printf("MOV    D,B");
            break;
        case 0x51:
            printf("MOV    D,C");
            break;
        case 0x52:
            printf("MOV    D,D");
            break;
        case 0x53:
            printf("MOV    D.E");
            break;
        case 0x54:
            printf("MOV    D,H");
            break;
        case 0x55:
            printf("MOV    D,L");
            break;
        case 0x56:
            printf("MOV    D,M");
            break;
        case 0x57:
            printf("MOV    D,A");
            break;
        case 0x58:
            printf("MOV    E,B");
            break;
        case 0x59:
            printf("MOV    E,C");
            break;
        case 0x5a:
            printf("MOV    E,D");
            break;
        case 0x5b:
            printf("MOV    E,E");
            break;
        case 0x5c:
            printf("MOV    E,H");
            break;
        case 0x5d:
            printf("MOV    E,L");
            break;
        case 0x5e:
            printf("MOV    E,M");
            break;
        case 0x5f:
            printf("MOV    E,A");
            break;

        case 0x60:
            printf("MOV    H,B");
            break;
        case 0x61:
            printf("MOV    H,C");
            break;
        case 0x62:
            printf("MOV    H,D");
            break;
        case 0x63:
            printf("MOV    H.E");
            break;
        case 0x64:
            printf("MOV    H,H");
            break;
        case 0x65:
            printf("MOV    H,L");
            break;
        case 0x66:
            printf("MOV    H,M");
            break;
        case 0x67:
            printf("MOV    H,A");
            break;
        case 0x68:
            printf("MOV    L,B");
            break;
        case 0x69:
            printf("MOV    L,C");
            break;
        case 0x6a:
            printf("MOV    L,D");
            break;
        case 0x6b:
            printf("MOV    L,E");
            break;
        case 0x6c:
            printf("MOV    L,H");
            break;
        case 0x6d:
            printf("MOV    L,L");
            break;
        case 0x6e:
            printf("MOV    L,M");
            break;
        case 0x6f:
            printf("MOV    L,A");
            break;

        case 0x70:
            printf("MOV    M,B");
            break;
        case 0x71:
            printf("MOV    M,C");
            break;
        case 0x72:
            printf("MOV    M,D");
            break;
        case 0x73:
            printf("MOV    M.E");
            break;
        case 0x74:
            printf("MOV    M,H");
            break;
        case 0x75:
            printf("MOV    M,L");
            break;
        case 0x76:
            printf("HLT");
            break;
        case 0x77:
            printf("MOV    M,A");
            break;
        case 0x78:
            printf("MOV    A,B");
            break;
        case 0x79:
            printf("MOV    A,C");
            break;
        case 0x7a:
            printf("MOV    A,D");
            break;
        case 0x7b:
            printf("MOV    A,E");
            break;
        case 0x7c:
            printf("MOV    A,H");
            break;
        case 0x7d:
            printf("MOV    A,L");
            break;
        case 0x7e:
            printf("MOV    A,M");
            break;
        case 0x7f:
            printf("MOV    A,A");
            break;

        case 0x80:
            printf("ADD    B");
            break;
        case 0x81:
            printf("ADD    C");
            break;
        case 0x82:
            printf("ADD    D");
            break;
        case 0x83:
            printf("ADD    E");
            break;
        case 0x84:
            printf("ADD    H");
            break;
        case 0x85:
            printf("ADD    L");
            break;
        case 0x86:
            printf("ADD    M");
            break;
        case 0x87:
            printf("ADD    A");
            break;
        case 0x88:
            printf("ADC    B");
            break;
        case 0x89:
            printf("ADC    C");
            break;
        case 0x8a:
            printf("ADC    D");
            break;
        case 0x8b:
            printf("ADC    E");
            break;
        case 0x8c:
            printf("ADC    H");
            break;
        case 0x8d:
            printf("ADC    L");
            break;
        case 0x8e:
            printf("ADC    M");
            break;
        case 0x8f:
            printf("ADC    A");
            break;

        case 0x90:
            printf("SUB    B");
            break;
        case 0x91:
            printf("SUB    C");
            break;
        case 0x92:
            printf("SUB    D");
            break;
        case 0x93:
            printf("SUB    E");
            break;
        case 0x94:
            printf("SUB    H");
            break;
        case 0x95:
            printf("SUB    L");
            break;
        case 0x96:
            printf("SUB    M");
            break;
        case 0x97:
            printf("SUB    A");
            break;
        case 0x98:
            printf("SBB    B");
            break;
        case 0x99:
            printf("SBB    C");
            break;
        case 0x9a:
            printf("SBB    D");
            break;
        case 0x9b:
            printf("SBB    E");
            break;
        case 0x9c:
            printf("SBB    H");
            break;
        case 0x9d:
            printf("SBB    L");
            break;
        case 0x9e:
            printf("SBB    M");
            break;
        case 0x9f:
            printf("SBB    A");
            break;

        case 0xa0:
            printf("ANA    B");
            break;
        case 0xa1:
            printf("ANA    C");
            break;
        case 0xa2:
            printf("ANA    D");
            break;
        case 0xa3:
            printf("ANA    E");
            break;
        case 0xa4:
            printf("ANA    H");
            break;
        case 0xa5:
            printf("ANA    L");
            break;
        case 0xa6:
            printf("ANA    M");
            break;
        case 0xa7:
            printf("ANA    A");
            break;
        case 0xa8:
            printf("XRA    B");
            break;
        case 0xa9:
            printf("XRA    C");
            break;
        case 0xaa:
            printf("XRA    D");
            break;
        case 0xab:
            printf("XRA    E");
            break;
        case 0xac:
            printf("XRA    H");
            break;
        case 0xad:
            printf("XRA    L");
            break;
        case 0xae:
            printf("XRA    M");
            break;
        case 0xaf:
            printf("XRA    A");
            break;

        case 0xb0:
            printf("ORA    B");
            break;
        case 0xb1:
            printf("ORA    C");
            break;
        case 0xb2:
            printf("ORA    D");
            break;
        case 0xb3:
            printf("ORA    E");
            break;
        case 0xb4:
            printf("ORA    H");
            break;
        case 0xb5:
            printf("ORA    L");
            break;
        case 0xb6:
            printf("ORA    M");
            break;
        case 0xb7:
            printf("ORA    A");
            break;
        case 0xb8:
            printf("CMP    B");
            break;
        case 0xb9:
            printf("CMP    C");
            break;
        case 0xba:
            printf("CMP    D");
            break;
        case 0xbb:
            printf("CMP    E");
            break;
        case 0xbc:
            printf("CMP    H");
            break;
        case 0xbd:
            printf("CMP    L");
            break;
        case 0xbe:
            printf("CMP    M");
            break;
        case 0xbf:
            printf("CMP    A");
            break;

        case 0xc0:
            printf("RNZ");
            break;
        case 0xc1:
            printf("POP    B");
            break;
        case 0xc2:
            printf("JNZ    $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xc3:
            printf("JMP    $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xc4:
            printf("CNZ    $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xc5:
            printf("PUSH   B");
            break;
        case 0xc6:
            printf("ADI    #$%02x", code[1]);
            opbytes = 2;
            break;
        case 0xc7:
            printf("RST    0");
            break;
        case 0xc8:
            printf("RZ");
            break;
        case 0xc9:
            printf("RET");
            break;
        case 0xca:
            printf("JZ     $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xcb:
            printf("JMP    $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xcc:
            printf("CZ     $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xcd:
            printf("CALL   $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xce:
            printf("ACI    #$%02x", code[1]);
            opbytes = 2;
            break;
        case 0xcf:
            printf("RST    1");
            break;

        case 0xd0:
            printf("RNC");
            break;
        case 0xd1:
            printf("POP    D");
            break;
        case 0xd2:
            printf("JNC    $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xd3:
            printf("OUT    #$%02x", code[1]);
            opbytes = 2;
            break;
        case 0xd4:
            printf("CNC    $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xd5:
            printf("PUSH   D");
            break;
        case 0xd6:
            printf("SUI    #$%02x", code[1]);
            opbytes = 2;
            break;
        case 0xd7:
            printf("RST    2");
            break;
        case 0xd8:
            printf("RC");
            break;
        case 0xd9:
            printf("RET");
            break;
        case 0xda:
            printf("JC     $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xdb:
            printf("IN     #$%02x", code[1]);
            opbytes = 2;
            break;
        case 0xdc:
            printf("CC     $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xdd:
            printf("CALL   $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xde:
            printf("SBI    #$%02x", code[1]);
            opbytes = 2;
            break;
        case 0xdf:
            printf("RST    3");
            break;

        case 0xe0:
            printf("RPO");
            break;
        case 0xe1:
            printf("POP    H");
            break;
        case 0xe2:
            printf("JPO    $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xe3:
            printf("XTHL");
            break;
        case 0xe4:
            printf("CPO    $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xe5:
            printf("PUSH   H");
            break;
        case 0xe6:
            printf("ANI    #$%02x", code[1]);
            opbytes = 2;
            break;
        case 0xe7:
            printf("RST    4");
            break;
        case 0xe8:
            printf("RPE");
            break;
        case 0xe9:
            printf("PCHL");
            break;
        case 0xea:
            printf("JPE    $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xeb:
            printf("XCHG");
            break;
        case 0xec:
            printf("CPE     $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xed:
            printf("CALL   $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xee:
            printf("XRI    #$%02x", code[1]);
            opbytes = 2;
            break;
        case 0xef:
            printf("RST    5");
            break;

        case 0xf0:
            printf("RP");
            break;
        case 0xf1:
            printf("POP    PSW");
            break;
        case 0xf2:
            printf("JP     $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xf3:
            printf("DI");
            break;
        case 0xf4:
            printf("CP     $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xf5:
            printf("PUSH   PSW");
            break;
        case 0xf6:
            printf("ORI    #$%02x", code[1]);
            opbytes = 2;
            break;
        case 0xf7:
            printf("RST    6");
            break;
        case 0xf8:
            printf("RM");
            break;
        case 0xf9:
            printf("SPHL");
            break;
        case 0xfa:
            printf("JM     $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xfb:
            printf("EI");
            break;
        case 0xfc:
            printf("CM     $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xfd:
            printf("CALL   $%02x%02x", code[2], code[1]);
            opbytes = 3;
            break;
        case 0xfe:
            printf("CPI    #$%02x", code[1]);
            opbytes = 2;
            break;
        case 0xff:
            printf("RST    7");
            break;
    }

    printf("\n");

    return opbytes;
}

int Emulate8080Op(State8080 *state) {

    unsigned char *opcode = &state->memory[state->pc];
    Disassemble8080Op(state->memory, state->pc);

    state->pc += 1;
    switch (*opcode) {
        case 0x00:
            break;    //NOP
        case 0x01: //LXI B ,word
            state->c = opcode[1];
            state->b = opcode[2];
            state->pc += 2;
            break;
        case 0x02: // STAX B
        {
            uint16_t offset = (state->b << 8) | state->c;
            state->memory[offset] = state->a;
        }
            break;
        case 0x03: // INX B
            state->c++;
            if (state->c == 0)
                state->b++;
            break;
        case 0x04: // INR B
            state->b++;
            FlagsZSP(state, state->b);
            break;
        case 0x05: //DCR B
            state->b--;
            FlagsZSP(state, state->b);
            break;
        case 0x06: //MVI B, byte
            state->b = opcode[1];
            state->pc++;
            break;
        case 0x07: //RLC
        {
            uint8_t x = state->a;
            state->a = ((x & 0x80) >> 7) | (x << 1);
            state->cc.cy = (0x80 == (x&0x80));
        }
            break;
        case 0x08:
            UnimplementedInstruction(state);
            break;
        case 0x09: //DAD B
        {
            uint32_t hl = (state->h << 8) | state->l;
            uint32_t bc = (state->b << 8) | state->c;
            uint32_t res = hl + bc;
            state->h = (res & 0xff00) >> 8;
            state->l = res & 0xff;
            state->cc.cy = ((res & 0xffff0000) > 0);
        }
            break;
        case 0x0a: // LDAX B
        {
            uint16_t offset=(state->b<<8) | state->c;
            state->a = state->memory[offset];
        }
            break;
        case 0x0b: // DCX B
            state->c -= 1;
            if (state->c==0xff)
                state->b-=1;
            break;
        case 0x0c:
            UnimplementedInstruction(state);
            break;
        case 0x0d: //DCR C
        {
            uint8_t res = state->c - 1;
            state->cc.z = (res == 0);
            state->cc.s = (0x80 == (res & 0x80));
            state->cc.p = parity(res, 8);
            state->c = res;
        }
            break;
        case 0x0e: //MVI C,byte
            state->c = opcode[1];
            state->pc++;
            break;
        case 0x0f: //RRC
        {
            uint8_t x = state->a;
            state->a = ((x & 1) << 7) | (x >> 1);
            state->cc.cy = (1 == (x & 1));
        }
            break;
        case 0x10:
            UnimplementedInstruction(state);
            break;
        case 0x11:                            //LXI	D,word
            state->e = opcode[1];
            state->d = opcode[2];
            state->pc += 2;
            break;
        case 0x12:
        {
            uint16_t offset=(state->d<<8) | state->e;
            state->memory[offset] = state->a;
        }
            break;
        case 0x13:                            //INX    D
            state->e++;
            if (state->e == 0)
                state->d++;
            break;
        case 0x14:
            state->d += 1;
            FlagsZSP(state,state->d);
            break;
        case 0x15:
            state->d -= 1;
            FlagsZSP(state,state->d);
            break;
        case 0x16:
            state->d = opcode[1];
            state->pc++;
            break;
        case 0x17:
        {
            uint8_t x = state->a;
            state->a = state->cc.cy  | (x << 1);
            state->cc.cy = (0x80 == (x&0x80));
        }
            break;
        case 0x18:
            UnimplementedInstruction(state);
            break;
        case 0x19:                            //DAD    D
        {
            uint32_t hl = (state->h << 8) | state->l;
            uint32_t de = (state->d << 8) | state->e;
            uint32_t res = hl + de;
            state->h = (res & 0xff00) >> 8;
            state->l = res & 0xff;
            state->cc.cy = ((res & 0xffff0000) != 0);
        }
            break;
        case 0x1a:                            //LDAX	D
        {
            uint16_t offset = (state->d << 8) | state->e;
            state->a = state->memory[offset];
        }
            break;
        case 0x1b:
            state->e -= 1;
            if (state->e==0xff)
                state->d-=1;
            break;
        case 0x1c:
            state->e += 1;
            FlagsZSP(state,state->e);
            break;
        case 0x1d:
            state->e -= 1;
            FlagsZSP(state,state->e);
            break;
        case 0x1e:
            state->e = opcode[1];
            state->pc++;
            break;
        case 0x1f:
        {
            uint8_t x = state->a;
            state->a = (state->cc.cy << 7) | (x >> 1);
            state->cc.cy = (1 == (x & 1));
        }
            break;
        case 0x20:
            UnimplementedInstruction(state);
            break;
        case 0x21:                            //LXI	H,word
            state->l = opcode[1];
            state->h = opcode[2];
            state->pc += 2;
            break;
        case 0x22:
        {
            uint16_t offset = opcode[1] | (opcode[2] << 8);
            state->memory[offset] = state->l;
            state->memory[offset+1] = state->h;
            state->pc += 2;
        }
            break;
        case 0x23:                            //INX    H
            state->l++;
            if (state->l == 0)
                state->h++;
            break;
        case 0x24:
            state->h++;
            FlagsZSP(state, state->h);
            break;
        case 0x25:
            state->h--;
            FlagsZSP(state, state->h);
            break;
        case 0x26:                            //MVI H,byte
            state->h = opcode[1];
            state->pc++;
            break;
        case 0x27:
            if ((state->a &0xf) > 9)
                state->a += 6;
            if ((state->a&0xf0) > 0x90)
            {
                uint16_t res = (uint16_t) state->a + 0x60;
                state->a = res & 0xff;
                ArithFlagsA(state, res);
            }
            break;
        case 0x28:
            UnimplementedInstruction(state);
            break;
        case 0x29:                                //DAD    H
        {
            uint32_t hl = (state->h << 8) | state->l;
            uint32_t res = hl + hl;
            state->h = (res & 0xff00) >> 8;
            state->l = res & 0xff;
            state->cc.cy = ((res & 0xffff0000) != 0);
        }
            break;
        case 0x2a:
            UnimplementedInstruction(state);
            break;
        case 0x2b:
            UnimplementedInstruction(state);
            break;
        case 0x2c:
            UnimplementedInstruction(state);
            break;
        case 0x2d:
            UnimplementedInstruction(state);
            break;
        case 0x2e:
            UnimplementedInstruction(state);
            break;
        case 0x2f:
            state->a = ~state->a;
            break;
        case 0x30:
            UnimplementedInstruction(state);
            break;
        case 0x31:                            //LXI	SP,word
            state->sp = (opcode[2] << 8) | opcode[1];
            state->pc += 2;
            break;
        case 0x32:                            //STA    (word)
        {
            uint16_t offset = (opcode[2] << 8) | (opcode[1]);
            state->memory[offset] = state->a;
            state->pc += 2;
        }
            break;
        case 0x33:
        {
            state->sp += 1;
            state->pc += 1;
        }
            break;
        case 0x34:
        {
            //AC set if lower nibble of h was zero prior to dec
            uint16_t offset = (state->h << 8) | state->l;
            state->memory[offset] += 1;
            FlagsZSP(state, state->memory[offset]);
            state->pc++;
        }
            break;
        case 0x35:
        {
            //AC set if lower nibble of h was zero prior to dec
            uint16_t offset = (state->h << 8) | state->l;
            state->memory[offset] -= 1;
            FlagsZSP(state, state->memory[offset]);
            state->pc++;
        }
            break;
        case 0x36:                            //MVI	M,byte
        {
            //AC set if lower nibble of h was zero prior to dec
            uint16_t offset = (state->h << 8) | state->l;
            state->memory[offset] = opcode[1];
            state->pc++;
        }
            break;
        case 0x37:
            UnimplementedInstruction(state);
            break;
        case 0x38:
            UnimplementedInstruction(state);
            break;
        case 0x39:
            UnimplementedInstruction(state);
            break;
        case 0x3a:                            //LDA    (word)
        {
            uint16_t offset = (opcode[2] << 8) | (opcode[1]);
            state->a = state->memory[offset];
            state->pc += 2;
        }
            break;
        case 0x3b:
            UnimplementedInstruction(state);
            break;
        case 0x3c:
            UnimplementedInstruction(state);
            break;
        case 0x3d:
            UnimplementedInstruction(state);
            break;
        case 0x3e:                            //MVI    A,byte
            state->a = opcode[1];
            state->pc++;
            break;
        case 0x3f:
            UnimplementedInstruction(state);
            break;
        case 0x40:
            UnimplementedInstruction(state);
            break;
        case 0x41:
            UnimplementedInstruction(state);
            break;
        case 0x42:
            UnimplementedInstruction(state);
            break;
        case 0x43:
            UnimplementedInstruction(state);
            break;
        case 0x44:
            UnimplementedInstruction(state);
            break;
        case 0x45:
            UnimplementedInstruction(state);
            break;
        case 0x46: // MOV B, M
        {
            uint16_t offset = (state->h << 8) | (state->l);
            state->b = state->memory[offset];
        }
            break;
        case 0x47:
            UnimplementedInstruction(state);
            break;
        case 0x48:
            UnimplementedInstruction(state);
            break;
        case 0x49:
            UnimplementedInstruction(state);
            break;
        case 0x4a:
            UnimplementedInstruction(state);
            break;
        case 0x4b:
            UnimplementedInstruction(state);
            break;
        case 0x4c:
            UnimplementedInstruction(state);
            break;
        case 0x4d:
            UnimplementedInstruction(state);
            break;
        case 0x4e:
            UnimplementedInstruction(state);
            break;
        case 0x4f:
            UnimplementedInstruction(state);
            break;
        case 0x50:
            UnimplementedInstruction(state);
            break;
        case 0x51:
            UnimplementedInstruction(state);
            break;
        case 0x52:
            UnimplementedInstruction(state);
            break;
        case 0x53:
            UnimplementedInstruction(state);
            break;
        case 0x54:
            UnimplementedInstruction(state);
            break;
        case 0x55:
            UnimplementedInstruction(state);
            break;
        case 0x56:                            //MOV D,M
        {
            uint16_t offset = (state->h << 8) | (state->l);
            state->d = state->memory[offset];
        }
            break;
        case 0x57:
            UnimplementedInstruction(state);
            break;
        case 0x58:
            UnimplementedInstruction(state);
            break;
        case 0x59:
            UnimplementedInstruction(state);
            break;
        case 0x5a:
            UnimplementedInstruction(state);
            break;
        case 0x5b:
            UnimplementedInstruction(state);
            break;
        case 0x5c:
            UnimplementedInstruction(state);
            break;
        case 0x5d:
            UnimplementedInstruction(state);
            break;
        case 0x5e:                            //MOV E,M
        {
            uint16_t offset = (state->h << 8) | (state->l);
            state->e = state->memory[offset];
        }
            break;
        case 0x5f: // MOV E, A
            state->e = state->a;
            break;
        case 0x60: // MOV H, B
            state->h = state->b;
            break;
        case 0x61: // MOV H, C
            state->h = state->c;
            break;
        case 0x62: // MOV H, D
            state->h = state->d;
            break;
        case 0x63: // MOV H, E
            state->h = state->e;
            break;
        case 0x64: // MOV H, H
            state->h = state->h;
            break;
        case 0x65: // MOV H, L
            state->h = state->l;
            break;
        case 0x66: //MOV H,M
        {
            uint16_t offset = (state->h << 8) | (state->l);
            state->h = state->memory[offset];
        }
            break;
        case 0x67: // MOV H, A
            state->h = state->a;
            break;
        case 0x68:
            UnimplementedInstruction(state);
            break;
        case 0x69:
            UnimplementedInstruction(state);
            break;
        case 0x6a:
            UnimplementedInstruction(state);
            break;
        case 0x6b:
            UnimplementedInstruction(state);
            break;
        case 0x6c:
            UnimplementedInstruction(state);
            break;
        case 0x6d:
            UnimplementedInstruction(state);
            break;
        case 0x6e:
            UnimplementedInstruction(state);
            break;
        case 0x6f:
            state->l = state->a;
            break; //MOV L,A
        case 0x70:
            UnimplementedInstruction(state);
            break;
        case 0x71:
            UnimplementedInstruction(state);
            break;
        case 0x72:
            UnimplementedInstruction(state);
            break;
        case 0x73:
            UnimplementedInstruction(state);
            break;
        case 0x74:
            UnimplementedInstruction(state);
            break;
        case 0x75:
            UnimplementedInstruction(state);
            break;
        case 0x76: // HLT
            exit(0);
        case 0x77: //MOV M,A
        {
            uint16_t offset = (state->h << 8) | (state->l);
            state->memory[offset] = state->a;
        }
            break;
        case 0x78: // MOV A, B
            state->a = state->b;
            break;
        case 0x79: // MOV A, C
            state->a = state->c;
            break;
        case 0x7A: // MOV A, D
            state->a = state->d;
            break;
        case 0x7B: // MOV A, E
            state->a = state->e;
            break;
        case 0x7C: // MOV A, H
            state->a = state->h;
            break;
        case 0x7D: // MOV A, L
            state->a = state->l;
            break;
        case 0x7e:                            //MOV A,M
        {
            uint16_t offset = (state->h << 8) | (state->l);
            state->a = state->memory[offset];
        }
            break;
        case 0x7f:
            UnimplementedInstruction(state);
            break;
        case 0x80: {
            // do the math with higher precision so we can capture the
            // carry out
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->b;

            // Zero flag: if the result is zero,
            // set the flag to zero
            // else clear the flag
            if ((answer & 0xff) == 0)
                state->cc.z = 1;
            else
                state->cc.z = 0;

            // Sign flag: if bit 7 is set,
            // set the sign flag
            // else clear the sign flag
            if (answer & 0x80)
                state->cc.s = 1;
            else
                state->cc.s = 0;

            // Carry flag
            if (answer > 0xff)
                state->cc.cy = 1;
            else
                state->cc.cy = 0;

            // Parity is handled by a subroutine
            state->cc.p = parity(answer & 0xff, 8);

            state->a = answer & 0xff;
        }
            break;
        case 0x81: {

            uint16_t answer = (uint16_t) state->a + (uint16_t) state->c;
            state->cc.z = ((answer & 0xff) == 0);
            state->cc.s = ((answer & 0x80) != 0);
            state->cc.cy = (answer > 0xff);
            state->cc.p = parity(answer & 0xff, 8);
            state->a = answer & 0xff;
        }
            break;
        case 0x82:
            UnimplementedInstruction(state);
            break;
        case 0x83:
            UnimplementedInstruction(state);
            break;
        case 0x84:
            UnimplementedInstruction(state);
            break;
        case 0x85:
            UnimplementedInstruction(state);
            break;
        case 0x86: {
            uint16_t offset = (state->h << 8) | (state->l);
            uint16_t answer = (uint16_t) state->a + state->memory[offset];
            state->cc.z = ((answer & 0xff) == 0);
            state->cc.s = ((answer & 0x80) != 0);
            state->cc.cy = (answer > 0xff);
            state->cc.p = parity(answer & 0xff, 8);
            state->a = answer & 0xff;
        }
            break;
        case 0x87:
            UnimplementedInstruction(state);
            break;
        case 0x88:
            UnimplementedInstruction(state);
            break;
        case 0x89:
            UnimplementedInstruction(state);
            break;
        case 0x8a:
            UnimplementedInstruction(state);
            break;
        case 0x8b:
            UnimplementedInstruction(state);
            break;
        case 0x8c:
            UnimplementedInstruction(state);
            break;
        case 0x8d:
            UnimplementedInstruction(state);
            break;
        case 0x8e:
            UnimplementedInstruction(state);
            break;
        case 0x8f:
            UnimplementedInstruction(state);
            break;
        case 0x90:
            UnimplementedInstruction(state);
            break;
        case 0x91:
            UnimplementedInstruction(state);
            break;
        case 0x92:
            UnimplementedInstruction(state);
            break;
        case 0x93:
            UnimplementedInstruction(state);
            break;
        case 0x94:
            UnimplementedInstruction(state);
            break;
        case 0x95:
            UnimplementedInstruction(state);
            break;
        case 0x96:
            UnimplementedInstruction(state);
            break;
        case 0x97:
            UnimplementedInstruction(state);
            break;
        case 0x98:
            UnimplementedInstruction(state);
            break;
        case 0x99:
            UnimplementedInstruction(state);
            break;
        case 0x9a:
            UnimplementedInstruction(state);
            break;
        case 0x9b:
            UnimplementedInstruction(state);
            break;
        case 0x9c:
            UnimplementedInstruction(state);
            break;
        case 0x9d:
            UnimplementedInstruction(state);
            break;
        case 0x9e:
            UnimplementedInstruction(state);
            break;
        case 0x9f:
            UnimplementedInstruction(state);
            break;
        case 0xa0:
            UnimplementedInstruction(state);
            break;
        case 0xa1:
            UnimplementedInstruction(state);
            break;
        case 0xa2:
            UnimplementedInstruction(state);
            break;
        case 0xa3:
            UnimplementedInstruction(state);
            break;
        case 0xa4:
            UnimplementedInstruction(state);
            break;
        case 0xa5:
            UnimplementedInstruction(state);
            break;
        case 0xa6:
            UnimplementedInstruction(state);
            break;
        case 0xa7:
            state->a = state->a & state->a;
            LogicFlagsA(state);
            break; //ANA A
        case 0xa8:
            UnimplementedInstruction(state);
            break;
        case 0xa9:
            UnimplementedInstruction(state);
            break;
        case 0xaa:
            UnimplementedInstruction(state);
            break;
        case 0xab:
            UnimplementedInstruction(state);
            break;
        case 0xac:
            UnimplementedInstruction(state);
            break;
        case 0xad:
            UnimplementedInstruction(state);
            break;
        case 0xae:
            UnimplementedInstruction(state);
            break;
        case 0xaf:
            state->a = state->a ^ state->a;
            LogicFlagsA(state);
            break; //XRA A
        case 0xb0:
            UnimplementedInstruction(state);
            break;
        case 0xb1:
            UnimplementedInstruction(state);
            break;
        case 0xb2:
            UnimplementedInstruction(state);
            break;
        case 0xb3:
            UnimplementedInstruction(state);
            break;
        case 0xb4:
            UnimplementedInstruction(state);
            break;
        case 0xb5:
            UnimplementedInstruction(state);
            break;
        case 0xb6:
            UnimplementedInstruction(state);
            break;
        case 0xb7:
            UnimplementedInstruction(state);
            break;
        case 0xb8:
            UnimplementedInstruction(state);
            break;
        case 0xb9:
            UnimplementedInstruction(state);
            break;
        case 0xba:
            UnimplementedInstruction(state);
            break;
        case 0xbb:
            UnimplementedInstruction(state);
            break;
        case 0xbc:
            UnimplementedInstruction(state);
            break;
        case 0xbd:
            UnimplementedInstruction(state);
            break;
        case 0xbe:
            UnimplementedInstruction(state);
            break;
        case 0xbf:
            UnimplementedInstruction(state);
            break;
        case 0xc0:
            UnimplementedInstruction(state);
            break;
        case 0xc1:                        //POP    B
        {
            state->c = state->memory[state->sp];
            state->b = state->memory[state->sp + 1];
            state->sp += 2;
        }
            break;
        case 0xc2:                        //JNZ address
            if (0 == state->cc.z)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
        case 0xc3:                        //JMP address
            state->pc = (opcode[2] << 8) | opcode[1];
            break;
        case 0xc4: // CNZ address
        {
            if (state->cc.z == 0)
            {
                uint16_t ret = state->pc + 2;
                state->memory[state->sp-1] = (ret >> 8) & 0xff;
                state->memory[state->sp-2] = (ret & 0xff);
                state->sp = state->sp - 2;
                state->pc = (opcode[2] << 8) | opcode[1];
            } else
                state->pc += 2;
        }
            break;
        case 0xc5:                        //PUSH   B
        {
            state->memory[state->sp - 1] = state->b;
            state->memory[state->sp - 2] = state->c;
            state->sp = state->sp - 2;
        }
            break;
        case 0xc6: //ADI    byte
        {
            uint16_t x = (uint16_t) state->a + (uint16_t) opcode[1];
            state->cc.z = ((x & 0xff) == 0);
            state->cc.s = (0x80 == (x & 0x80));
            state->cc.p = parity((x & 0xff), 8);
            state->cc.cy = (x > 0xff);
            state->a = (uint8_t) x;
            state->pc++;
        }
            break;
        case 0xc7:
            UnimplementedInstruction(state);
            break;
        case 0xc8:
            UnimplementedInstruction(state);
            break;
        case 0xc9:                        //RET
            state->pc = state->memory[state->sp] | (state->memory[state->sp + 1] << 8);
            state->sp += 2;
            break;
        case 0xca:
            if (state->cc.z)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
        case 0xcb:
            UnimplementedInstruction(state);
            break;
        case 0xcc:
            UnimplementedInstruction(state);
            break;
        case 0xcd:                        //CALL adr
#ifdef FOR_CPUDIAG
            if (5 ==  ((opcode[2] << 8) | opcode[1]))
            {
                if (state->c == 9)
                {
                    uint16_t offset = (state->d<<8) | (state->e);
                    char *str = &state->memory[offset+3];  //skip the prefix bytes
                    while (*str != '$')
                        printf("%c", *str++);
                    printf("\n");
                }
                else if (state->c == 2)
                {
                    //saw this in the inspected code, never saw it called
                    printf ("print char routine called\n");
                }
            }
            else if (0 ==  ((opcode[2] << 8) | opcode[1]))
            {
                exit(0);
            }
            else
#endif
        {
            uint16_t ret = state->pc + 2;
            state->memory[state->sp - 1] = (ret >> 8) & 0xff;
            state->memory[state->sp - 2] = (ret & 0xff);
            state->sp = state->sp - 2;
            state->pc = (opcode[2] << 8) | opcode[1];
        }
            break;
        case 0xce:
            state->a = state->a + opcode[1] + state->cc.cy;
            LogicFlagsA(state);
            state->pc++;
            break;
        case 0xcf:
            UnimplementedInstruction(state);
            break;
        case 0xd0:
            UnimplementedInstruction(state);
            break;
        case 0xd1:                        //POP    D
        {
            state->e = state->memory[state->sp];
            state->d = state->memory[state->sp + 1];
            state->sp += 2;
        }
            break;
        case 0xd2:
            if (~state->cc.cy)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
        case 0xd3:
            //Don't know what to do here (yet)
            state->pc++;
            break;
        case 0xd4:
            UnimplementedInstruction(state);
            break;
        case 0xd5:                        //PUSH   D
        {
            state->memory[state->sp - 1] = state->d;
            state->memory[state->sp - 2] = state->e;
            state->sp = state->sp - 2;
        }
            break;
        case 0xd6:
            UnimplementedInstruction(state);
            break;
        case 0xd7:
            UnimplementedInstruction(state);
            break;
        case 0xd8:
            UnimplementedInstruction(state);
            break;
        case 0xd9:
            UnimplementedInstruction(state);
            break;
        case 0xda:
            if (state->cc.cy)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
        case 0xdb:
            UnimplementedInstruction(state);
            break;
        case 0xdc:
            UnimplementedInstruction(state);
            break;
        case 0xdd:
            UnimplementedInstruction(state);
            break;
        case 0xde:
            UnimplementedInstruction(state);
            break;
        case 0xdf:
            UnimplementedInstruction(state);
            break;
        case 0xe0:
            UnimplementedInstruction(state);
            break;
        case 0xe1:                    //POP    H
        {
            state->l = state->memory[state->sp];
            state->h = state->memory[state->sp + 1];
            state->sp += 2;
        }
            break;
        case 0xe2:
            if (state->cc.p == 0)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
        case 0xe3:
            UnimplementedInstruction(state);
            break;
        case 0xe4:
            UnimplementedInstruction(state);
            break;
        case 0xe5:                        //PUSH   H
        {
            state->memory[state->sp - 1] = state->h;
            state->memory[state->sp - 2] = state->l;
            state->sp = state->sp - 2;
        }
            break;
        case 0xe6:                        //ANI    byte
        {
            uint8_t x = state->a & opcode[1];
            state->cc.z = (x == 0);
            state->cc.s = (0x80 == (x & 0x80));
            state->cc.p = parity(x, 8);
            state->cc.cy = 0;           //Data book says ANI clears CY
            state->a = x;
            state->pc++;                //for the data byte
        }
            break;
        case 0xe7:
            UnimplementedInstruction(state);
            break;
        case 0xe8:
            UnimplementedInstruction(state);
            break;
        case 0xe9:
            UnimplementedInstruction(state);
            break;
        case 0xea:
            if (state->cc.p)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
        case 0xeb:                    //XCHG
        {
            uint8_t save1 = state->d;
            uint8_t save2 = state->e;
            state->d = state->h;
            state->e = state->l;
            state->h = save1;
            state->l = save2;
        }
            break;
        case 0xec:
            UnimplementedInstruction(state);
            break;
        case 0xed:
            UnimplementedInstruction(state);
            break;
        case 0xee:
            UnimplementedInstruction(state);
            break;
        case 0xef:
            UnimplementedInstruction(state);
            break;
        case 0xf0:
            UnimplementedInstruction(state);
            break;
        case 0xf1:                    //POP PSW
        {
            state->a = state->memory[state->sp + 1];
            uint8_t psw = state->memory[state->sp];
            state->cc.z = (0x01 == (psw & 0x01));
            state->cc.s = (0x02 == (psw & 0x02));
            state->cc.p = (0x04 == (psw & 0x04));
            state->cc.cy = (0x05 == (psw & 0x08));
            state->cc.ac = (0x10 == (psw & 0x10));
            state->sp += 2;
        }
            break;
        case 0xf2:
            if (state->cc.p == 1)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
        case 0xf3:
            UnimplementedInstruction(state);
            break;
        case 0xf4:
            UnimplementedInstruction(state);
            break;
        case 0xf5:                        //PUSH   PSW
        {
            state->memory[state->sp - 1] = state->a;
            uint8_t psw = (state->cc.z |
                           state->cc.s << 1 |
                           state->cc.p << 2 |
                           state->cc.cy << 3 |
                           state->cc.ac << 4);
            state->memory[state->sp - 2] = psw;
            state->sp = state->sp - 2;
        }
            break;
        case 0xf6:
            UnimplementedInstruction(state);
            break;
        case 0xf7:
            UnimplementedInstruction(state);
            break;
        case 0xf8:
            UnimplementedInstruction(state);
            break;
        case 0xf9:
            UnimplementedInstruction(state);
            break;
        case 0xfa:
            if (state->cc.s == 1) // M (s=1)
                state->pc = (opcode[2] << 8) | opcode[1];
            else
                state->pc += 2;
            break;
        case 0xfb:
            state->int_enable = 1;
            break;    //EI
        case 0xfc:
            UnimplementedInstruction(state);
            break;
        case 0xfd:
            UnimplementedInstruction(state);
            break;
        case 0xfe:                        //CPI  byte
        {
            uint8_t x = state->a - opcode[1];
            state->cc.z = (x == 0);
            state->cc.s = (0x80 == (x & 0x80));
            state->cc.p = parity(x, 8);
            state->cc.cy = (state->a < opcode[1]);
            state->pc++;
        }
            break;
        case 0xff:
            UnimplementedInstruction(state);
            break;
    }
    printf("\t");
    printf("%c", state->cc.z ? 'z' : '.');
    printf("%c", state->cc.s ? 's' : '.');
    printf("%c", state->cc.p ? 'p' : '.');
    printf("%c", state->cc.cy ? 'c' : '.');
    printf("%c  ", state->cc.ac ? 'a' : '.');
    printf("A $%02x B $%02x C $%02x D $%02x E $%02x H $%02x L $%02x SP %04x\n", state->a, state->b, state->c,
           state->d, state->e, state->h, state->l, state->sp);
    return 0;
}

static void Push(State8080* state, uint8_t high, uint8_t low)
{
    state->memory[state->sp-1] = high;
    state->memory[state->sp-2] = low;
    state->sp = state->sp - 2;
}

static void Pop(State8080* state, uint8_t *high, uint8_t *low)
{
    *low = state->memory[state->sp];
    *high = state->memory[state->sp+1];
    state->sp += 2;
}

void ReadFileIntoMemoryAt(State8080 *state, char *filename, uint32_t offset) {
    FILE *f = fopen(filename, "rb");
    if (f == NULL) {
        printf("error: Couldn't open %s\n", filename);
        exit(1);
    }
    fseek(f, 0L, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);

    uint8_t *buffer = &state->memory[offset];
    fread(buffer, fsize, 1, f);
    fclose(f);
}

uint8_t MachineIN(State8080* state, uint8_t port)
{
    uint8_t a;
    switch(port)
    {
        case 1:
            a = state->port.read1;
            break;
        case 2:
            a = state->port.read2;
            break;
        case 3:
        {
            uint16_t v = (state->port.shift1<<8) | state->port.shift0;
            a = ((v >> (8 - state->port.write2)) & 0xff);
        }
            break;
        default:
            UnimplementedInstruction(state);
            break;
    }
    return a;
}

void MachineOUT(State8080* state, uint8_t port)
{
    switch(port)
    {
        case 2:
            state->port.write2 = state->a & 0x7;
            break;
        case 4:
            state->port.shift0 = state->port.shift1;
            state->port.shift1 = state->a;
            break;
    }
}

/*MachineKeyDown(char key)
{
    switch(key)
    {
        case LEFT:
            port[1] |= 0x20;  //Set bit 5 of port 1
            break;
        case RIGHT:
            port[1] |= 0x40;  //Set bit 6 of port 1
            break;
            *//*....*//*
    }
}

PlatformKeyUp(char key)
{
    switch(key)
    {
        case LEFT:
            port[1] &= 0xDF //Clear bit 5 of port 1
            break;
        case RIGHT:
            port[1] &= 0xBF //Clear bit 6 of port 1
            break;
            *//*....*//*
    }
}*/

void GenerateInterrupt(State8080* state, int interrupt_num)
{
    //perform "PUSH PC"
    Push(state, (state->pc & 0xFF00) >> 8, (state->pc & 0xff));

    //Set the PC to the low memory vector.
    //This is identical to an "RST interrupt_num" instruction.
    state->pc = 8 * interrupt_num;
}

State8080 *Init8080(void) {
    State8080 *state = calloc(1, sizeof(State8080));
    state->memory = malloc(0x10000);  //16K
    return state;
}

void set_pixel(Uint32* pixels, int x, int y, Uint8 pixel)
{
    pixels[x + y * SCREEN_WIDTH] = pixel;
}

int main(int argc, char **argv) {
    int done = 0;
    int vblankcycles = 0;
    State8080 *state = Init8080();
    time_t lastInterrupt = 0;

    ReadFileIntoMemoryAt(state, "invaders.h", 0);
    ReadFileIntoMemoryAt(state, "invaders.g", 0x800);
    ReadFileIntoMemoryAt(state, "invaders.f", 0x1000);
    ReadFileIntoMemoryAt(state, "invaders.e", 0x1800);

/*
    //Fix the first instruction to be JMP 0x100
    state->memory[0] = 0xc3;
    state->memory[1] = 0;
    state->memory[2] = 0x01;

    //Fix the stack pointer from 0x6ad to 0x7ad
    // this 0x06 byte 112 in the code, which is
    // byte 112 + 0x100 = 368 in memory
    state->memory[368] = 0x7;

    //Skip DAA test
    state->memory[0x59c] = 0xc3; //JMP
    state->memory[0x59d] = 0xc2;
    state->memory[0x59e] = 0x05;
*/

    // Initialize SDL
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
        printf("SDL couldn't initialize! SDL_Error: %s\n", SDL_GetError());
    // The window we'll be rendering to
    SDL_Window* window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC,
                                             SCREEN_WIDTH, SCREEN_HEIGHT);

    Uint32 pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
    memset(pixels, 255, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Uint32));

    // Initialize SDL
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
        printf("SDL couldn't initialize! SDL_Error: %s\n", SDL_GetError());
    else
    {
        if(window == NULL)
        {
            printf("Window couldn't be created! SDL_Error %s\n, SDL_GetError()", SDL_GetError());
        }
    }

    while (!done)
    {
        done = Emulate8080Op(state);

        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            for (int y = 0; y < SCREEN_HEIGHT; y++)
            {
                int offset = 0x2400 + (x + y * SCREEN_WIDTH);
                set_pixel(pixels, x, y, state->memory[offset]);
            }
        }
        SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(Uint32));

        if ( time(NULL) - lastInterrupt > 1.0/60.0) // 1/60 seconds has elapsed
        {
            // only do an interrupt if they are enabled
            if (state->int_enable)
            {
                GenerateInterrupt(state, 2); // Interrupt 2

                // Save the time we did this
                lastInterrupt = time(NULL);
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
    free(pixels);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    //Destroy window
    SDL_DestroyWindow( window );
    //Quit SDL subsystems
    SDL_Quit();
    return 0;
}

