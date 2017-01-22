#include <cpu/CHIP8Cpu.h>
#include <systems/chip8/Chip8GPU.h>
#include <log.h>
#include <ymgyrch.h>
#include <input.h>
CPU_Chip8::CPU_Chip8(Ymgyrch::System * sys) : Cpu(sys)
{
	setName("CPU0");
	instructions[0x0] = &CPU_Chip8::op_0;
	instructions[0x1] = &CPU_Chip8::op_1;
	instructions[0x2] = &CPU_Chip8::op_2;
	instructions[0x3] = &CPU_Chip8::op_3;
	instructions[0x4] = &CPU_Chip8::op_4;
	instructions[0x5] = &CPU_Chip8::op_5;
	instructions[0x6] = &CPU_Chip8::op_6;
	instructions[0x7] = &CPU_Chip8::op_7;
	instructions[0x8] = &CPU_Chip8::op_8;
	instructions[0x9] = &CPU_Chip8::op_9;
	instructions[0xA] = &CPU_Chip8::op_A;
	instructions[0xB] = &CPU_Chip8::op_B;
	instructions[0xC] = &CPU_Chip8::op_C;
	instructions[0xD] = &CPU_Chip8::op_D;
	instructions[0xE] = &CPU_Chip8::op_E;
	instructions[0xF] = &CPU_Chip8::op_F;
	g_log->Log(getName().c_str(), "Initialised");
}

void CPU_Chip8::Init() {
	regs.pc = 0x200;
	regs.sp = 0;
	regs.timer_delay = 0;
	regs.timer_sound = 0;
	regs.I = 0;
}

std::string CPU_Chip8::Dissassemble(uint16_t opcode)
{
	std::string dissassembly_string = "";
	uint8_t reg = (opcode & 0x0F00) >> 8;
	uint8_t op1 = (opcode & 0x00F0) >> 4;
	switch (opcode & 0xF000) {
	case 0x0000:
		switch (opcode & 0x00FF) {
		case 0xE0:
			dissassembly_string = "cls";
			break;
		case 0xEE:
			dissassembly_string = "ret";
			break;
		default:
			break;
		}
		break;
	case 0x1000:
		dissassembly_string = fmt::format("jp 0x{0:03x}", opcode & 0x0FFF);
		break;
	case 0x2000:
		dissassembly_string = fmt::format("call 0x{0:03x}", opcode & 0x0FFF);
		break;
	case 0x3000:
		dissassembly_string = fmt::format("se v{0:x}, 0x{1:02x}", reg, opcode & 0x00FF);
		break;
	case 0x4000:
		dissassembly_string = fmt::format("sne v{0:x}, 0x{1:02x}", reg, opcode & 0x00FF);
		break;
	case 0x5000:
		dissassembly_string = fmt::format("se v{0:x}, v{1:x}", reg, op1);
		break;
	case 0x6000:
		dissassembly_string = fmt::format("ld v{0:x}, 0x{1:02x}", reg, opcode & 0x00FF);
		break;
	case 0x7000:
		dissassembly_string = fmt::format("add v{0:x}, 0x{1:02x}", reg, opcode & 0x00FF);
		break;
	case 0x8000:
		break;
	case 0x9000:
		break;
	case 0xA000:
		dissassembly_string = fmt::format("ld I, 0x{0:03X}", opcode & 0x0FFF);
		break;
	case 0xB000:
		break;
	case 0xC000:
		dissassembly_string = fmt::format("rand v{0:x}, 0x{1:02x}", reg, opcode & 0x00FF);
		break;
	case 0xD000:
		dissassembly_string = fmt::format("drw v{0:x}, v{1:x}, 0x{2:02x}", reg, op1, opcode & 0x000F);
		break;
	case 0xE000:
		break;
	case 0xF000:
		switch (opcode & 0x00FF) {
		case 0x07:
			dissassembly_string = fmt::format("ld v{0:x}, t", reg);
			break;
		case 0x15:
			dissassembly_string = fmt::format("ld t, v{0:x}", reg);
			break;
		case 0x18:
			dissassembly_string = fmt::format("ld s, v{0:x}", reg);
			break;
		case 0x1E:
			dissassembly_string = fmt::format("ld I, v{0:x}", reg);
			break;
		case 0x29:
			dissassembly_string = fmt::format("ld I, fnt (v{0:x})", reg);
			break;
		case 0x33:
			dissassembly_string = fmt::format("bcd v{0:x}", reg);
			break;
		case 0x65:
			dissassembly_string = fmt::format("ldr v0 -> v{0:x}", reg);
			break;
		}
		break;
	}

	if (dissassembly_string == "") {
		dissassembly_string = fmt::format("0x{0:04}", opcode);
	}

	return dissassembly_string;
}

void CPU_Chip8::Tick() {
	uint16_t opcode = sys->mem.ReadShort(regs.pc);
	uint16_t opcode_index = (opcode & 0xF000) >> 12;
	if (instructions[opcode_index] == nullptr) {
		op_unknown(opcode);
		return;
	}
	instructions[opcode_index](this, opcode);
	instructions_total++;
}

std::vector<cpu_reg_t> CPU_Chip8::GetRegs()
{
	std::vector<cpu_reg_t> reg_vec;
	reg_vec.push_back({ "PC", regs.pc, 2 , Ymgyrch::REG_PROGRAM_COUNTER });
	reg_vec.push_back({ "SP", regs.sp, 2 , Ymgyrch::REG_STACK_POINTER });
	reg_vec.push_back({ "I", regs.I, 2 , Ymgyrch::REG_GENERAL_PURPOSE });

	for (int i = 0; i < 16; i++) {
		reg_vec.push_back({ fmt::format("V{0:x}", i), regs.v[i], 1 , Ymgyrch::REG_GENERAL_PURPOSE });
	}

	return reg_vec;
}

std::vector<cpu_instruction_t> CPU_Chip8::GetDissassembly()
{
	std::vector<cpu_instruction_t> dis_vec;

	uint64_t current_pc = regs.pc;

	for (unsigned int i = 0; i < Ymgyrch::config->debug.numToDissassemble; i++) {
		uint16_t opcode = sys->mem.ReadWord(current_pc);
		dis_vec.push_back({ current_pc, Dissassemble(opcode) });
		current_pc += 2;
	}

	return dis_vec;
}
void CPU_Chip8::op_0(CPU_Chip8 * cpu, uint16_t opcode) {
	switch (opcode & 0x00FF) {
	case 0xE0:
		///@todo Implement
		g_log->Warn("CHIP8", "Clear not implemented yet! (pc {0:04x})", cpu->regs.pc);
		cpu->sys->gpu->Clear();
		cpu->regs.pc += 2;
		break;
	case 0xEE:
		cpu->regs.sp--;
		cpu->regs.pc = cpu->stack[cpu->regs.sp] + 2;
		break;
	default:
		cpu->op_unknown(opcode);
		return;
	}
}

void CPU_Chip8::op_1(CPU_Chip8 * cpu, uint16_t opcode) {
	cpu->regs.pc = opcode & 0x0FFF;
}

void CPU_Chip8::op_2(CPU_Chip8 * cpu, uint16_t opcode) {
	cpu->stack[cpu->regs.sp] = cpu->regs.pc;
	cpu->regs.sp++;
	cpu->regs.pc = opcode & 0x0FFF;
}

void CPU_Chip8::op_3(CPU_Chip8 * cpu, uint16_t opcode) {
	uint8_t reg = (opcode & 0x0F00) >> 8;
	uint8_t value = opcode & 0x00FF;
	if (cpu->regs.v[reg] == value) {
		cpu->regs.pc += 4;
	}
	else {
		cpu->regs.pc += 2;
	}
}

void CPU_Chip8::op_4(CPU_Chip8 * cpu, uint16_t opcode) {
	uint8_t reg = (opcode & 0x0F00) >> 8;
	uint8_t value = opcode & 0x00FF;
	if (cpu->regs.v[reg] != value) {
		cpu->regs.pc += 4;
	}
	else {
		cpu->regs.pc += 2;
	}
}

void CPU_Chip8::op_5(CPU_Chip8 * cpu, uint16_t opcode) {
	uint8_t rega = (opcode & 0x0F00) >> 8;
	uint8_t regb = (opcode & 0x00F0) >> 4;

	if (cpu->regs.v[rega] == cpu->regs.v[regb]) {
		cpu->regs.pc += 4;
	}
	else {
		cpu->regs.pc += 2;
	}
}

void CPU_Chip8::op_6(CPU_Chip8 * cpu, uint16_t opcode) {
	uint8_t reg = (opcode & 0x0F00) >> 8;
	uint8_t value = opcode & 0x00FF;
	cpu->regs.v[reg] = value;
	cpu->regs.pc += 2;
}

void CPU_Chip8::op_7(CPU_Chip8 * cpu, uint16_t opcode) {
	uint8_t reg = (opcode & 0x0F00) >> 8;
	uint8_t value = opcode & 0x00FF;
	cpu->regs.v[reg] += value;
	cpu->regs.pc += 2;
}

void CPU_Chip8::op_8(CPU_Chip8 * cpu, uint16_t opcode) {
	uint8_t rega = (opcode & 0x0F00) >> 8;
	uint8_t regb = (opcode & 0x00F0) >> 4;
	switch (opcode & 0x000F) {
	case 0x0:
		cpu->regs.v[rega] = cpu->regs.v[regb];
		break;
	case 0x1:
		cpu->regs.v[rega] = cpu->regs.v[rega] | cpu->regs.v[regb];
		break;
	case 0x2:
		cpu->regs.v[rega] = cpu->regs.v[rega] & cpu->regs.v[regb];
		break;
	case 0x3:
		cpu->regs.v[rega] = cpu->regs.v[rega] ^ cpu->regs.v[regb];
		break;
	case 0x4:
		if ((int)cpu->regs.v[rega] + (int)cpu->regs.v[regb] > 0xFF) {
			cpu->regs.v[0xf] = 1;
		}
		else {
			cpu->regs.v[0xf] = 0;
		}
		cpu->regs.v[rega] = cpu->regs.v[rega] + cpu->regs.v[regb];
		break;
	case 0x5:
		if ((int)cpu->regs.v[rega] - (int)cpu->regs.v[regb] < 0) {
			cpu->regs.v[0xf] = 0;
		}
		else {
			cpu->regs.v[0xf] = 1;
		}
		cpu->regs.v[rega] = cpu->regs.v[rega] - cpu->regs.v[regb];
		break;
	default:
		cpu->op_unknown(opcode);
		return;
	}
	cpu->regs.pc += 2;
}

void CPU_Chip8::op_9(CPU_Chip8 * cpu, uint16_t opcode) {
	cpu->op_unknown(opcode);
}

void CPU_Chip8::op_A(CPU_Chip8 * cpu, uint16_t opcode) {
	cpu->regs.I = opcode & 0x0FFF;
	cpu->regs.pc += 2;
}

void CPU_Chip8::op_B(CPU_Chip8 * cpu, uint16_t opcode) {
	cpu->regs.pc = (opcode & 0x0FFF) + cpu->regs.v[0];
}

void CPU_Chip8::op_C(CPU_Chip8 * cpu, uint16_t opcode) {
	uint8_t reg = (opcode & 0x0F00) >> 8;
	uint8_t value = opcode & 0x00FF;
	cpu->regs.v[reg] = rand() & value;
	cpu->regs.pc += 2;
}

void CPU_Chip8::op_D(CPU_Chip8 * cpu, uint16_t opcode) {
	///@todo Implement
	g_log->Warn("CHIP8", "Drawing! (pc {0:04x})", cpu->regs.pc);
	Chip8GPU* gpu = static_cast<Chip8GPU*>(cpu->sys->gpu);

	unsigned short x = cpu->regs.v[(opcode & 0x0F00) >> 8];
	unsigned short y = cpu->regs.v[(opcode & 0x00F0) >> 4];
	unsigned short height = opcode & 0x000F;

	gpu->DrawSprite(x, y, height, cpu->regs.I);
	cpu->regs.pc += 2;
}

void CPU_Chip8::op_E(CPU_Chip8 * cpu, uint16_t opcode) {
	bool shouldBePressed = (opcode && 0x000F) == 0x000E;
	uint8_t reg = (opcode & 0x0F00) >> 8;

	uint8_t keyToBePressed = cpu->regs.v[reg];

	std::string keyName = fmt::format("{0:X}", keyToBePressed);

	bool keyPressed = Ymgyrch::input->isKeyDown(keyName);

	if (keyPressed) {
		if (shouldBePressed) {
			cpu->regs.pc += 2;
		}
		else
		{
			cpu->regs.pc += 4;
		}
	}
	else
	{
		if (shouldBePressed) {
			cpu->regs.pc += 4;
		}
		else
		{
			cpu->regs.pc += 2;
		}
	}
}

void CPU_Chip8::op_F(CPU_Chip8 * cpu, uint16_t opcode) {
	uint8_t reg = (opcode & 0x0F00) >> 8;
	switch (opcode & 0x00FF) {
	case 0x07:
		cpu->regs.v[reg] = cpu->regs.timer_delay;
		break;
	case 0x15:
		cpu->regs.timer_delay = cpu->regs.v[reg];

		break;
	case 0x18:
		cpu->regs.timer_sound = cpu->regs.v[reg];
		break;
	case 0x1E:
		cpu->regs.I += cpu->regs.v[reg];
		break;
	case 0x29:
		cpu->regs.I = 0x0000 + cpu->regs.v[reg] * 5;
		break;
	case 0x33:
		cpu->sys->mem.WriteByte(cpu->regs.I + 0, cpu->regs.v[reg] / 100);
		cpu->sys->mem.WriteByte(cpu->regs.I + 1, (cpu->regs.v[reg] / 10) % 10);
		cpu->sys->mem.WriteByte(cpu->regs.I + 2, (cpu->regs.v[reg] % 100) % 10);
		break;
	case 0x55:
		for (int i = 0; i <= reg; i++) {
			cpu->sys->mem.WriteByte(cpu->regs.I + i, cpu->regs.v[i]);
		}

		break;
	case 0x65:
		for (int i = 0; i <= reg; i++) {
			cpu->regs.v[i] = cpu->sys->mem.ReadByte(cpu->regs.I + i);
		}

		break;
	default:
		cpu->op_unknown(opcode);
		return;
	}
	cpu->regs.pc += 2;
}

void CPU_Chip8::op_unknown(uint16_t opcode) {
	g_log->Log(getName().c_str(), "Unimplemented opcode 0x{0:4X}", opcode);
	sys->Stop();
	return;
}