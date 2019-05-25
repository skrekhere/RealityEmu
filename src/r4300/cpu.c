#include "cpu.h"

#include "mem.h"
#include "cic.h"
#include "opcodetable.h"

void CPUInit(void* ROM, size_t ROMSize)
{
    MemoryInit(ROM, ROMSize);

    uint32_t RomType = 0;
    uint32_t ResetType = 0;
    uint32_t osVersion = 0;
    uint32_t TVType = 1;

    Regs.GPR[1].Value  = 0x0000000000000001;
    Regs.GPR[2].Value  = 0x000000000EBDA536;
    Regs.GPR[3].Value  = 0x000000000EBDA536;
    Regs.GPR[4].Value  = 0x000000000000A536;
    Regs.GPR[5].Value  = 0xFFFFFFFFC0F1D859;
    Regs.GPR[6].Value  = 0xFFFFFFFFA4001F0C;
    Regs.GPR[7].Value  = 0xFFFFFFFFA4001F08;
    Regs.GPR[8].Value  = 0x00000000000000C0;
    Regs.GPR[10].Value = 0x0000000000000040;
    Regs.GPR[11].Value = 0xFFFFFFFFA4000040;
    Regs.GPR[12].Value = 0xFFFFFFFFED10D0B3;
    Regs.GPR[13].Value = 0x000000001402A4CC;
    Regs.GPR[14].Value = 0x000000002DE108EA;
    Regs.GPR[15].Value = 0x000000003103E121;
    Regs.GPR[19].Value = RomType;
    Regs.GPR[20].Value = TVType;
    Regs.GPR[21].Value = ResetType;
    Regs.GPR[22].Value = (GetCICSeed() >> 8) & 0xFF;
    Regs.GPR[23].Value = osVersion;
    Regs.GPR[25].Value = 0xFFFFFFFF9DEBB54F;
    Regs.GPR[29].Value = 0xFFFFFFFFA4001FF0;
    Regs.GPR[31].Value = 0xFFFFFFFFA4001550;
    Regs.HI.Value      = 0x000000003FC18657;
    Regs.LO.Value      = 0x000000003103E121;
    Regs.PC.Value      = 0xA4000040;

    MemoryCopy(0xA4000000, 0xB0000000, 0x1000);

    OpcodeTableInit();
}

void CPUDeInit(void)
{
    MemoryDeInit();
}

void WriteGPR(uint64_t Value, uint8_t Index)
{
    Regs.GPR[Index].Value = Value;
    if (Regs.GPR[Index].WriteCallback) Regs.GPR[Index].WriteCallback();
}

uint64_t ReadGPR(uint8_t Index)
{
    if (Regs.GPR[Index].ReadCallback) Regs.GPR[Index].ReadCallback();
    return Regs.GPR[Index].Value;
}

void WriteFPR(uint64_t Value, uint8_t Index)
{
    Regs.FPR[Index].Value = Value;
    if (Regs.FPR[Index].WriteCallback) Regs.FPR[Index].WriteCallback();
}

uint64_t ReadFPR(uint8_t Index)
{
    if (Regs.FPR[Index].ReadCallback) Regs.FPR[Index].ReadCallback();
    return Regs.FPR[Index].Value;
}

void WritePC(uint32_t Value)
{
    Regs.PC.Value = (uint64_t)Value;
    if (Regs.PC.WriteCallback) Regs.PC.WriteCallback();
}

uint32_t ReadPC(void)
{
    if (Regs.PC.ReadCallback) Regs.PC.ReadCallback();
    return Regs.PC.Value;
}

void AdvancePC(void)
{
    WritePC(ReadPC() + 4);
}

void WriteHI(uint64_t Value)
{
    Regs.HI.Value = Value;
    if (Regs.HI.WriteCallback) Regs.HI.WriteCallback();
}

uint64_t ReadHI(void)
{
    if (Regs.HI.ReadCallback) Regs.HI.ReadCallback();
    return Regs.HI.Value;
}

void WriteLO(uint64_t Value)
{
    Regs.LO.Value = Value;
    if (Regs.LO.WriteCallback) Regs.LO.WriteCallback();
}

uint64_t ReadLO(void)
{
    if (Regs.LO.ReadCallback) Regs.LO.ReadCallback();
    return Regs.LO.Value;
}