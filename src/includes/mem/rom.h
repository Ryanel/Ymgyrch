#pragma once
#include <memory.h>
#include <mem/ram.h>
class CMem_ROM : public CMem_RAM {
private:
	uint8_t * memory;
	uint64_t mem_size = 0;
public:
	explicit CMem_ROM(uint64_t size);
	~CMem_ROM();
	CMem_ROM(uint64_t size, std::string name);
	uint8_t ReadByte(uint64_t address);
	uint16_t ReadShort(uint64_t address);
	uint32_t ReadWord(uint64_t address);
	uint64_t ReadLong(uint64_t address);
	void WriteByte(uint64_t address, uint8_t  data);
	void WriteShort(uint64_t address, uint16_t data);
	void WriteWord(uint64_t address, uint32_t data);
	void WriteLong(uint64_t address, uint64_t data);
	uint64_t GetSize();
	void LoadData(uint8_t * buffer, uint64_t sz);
};