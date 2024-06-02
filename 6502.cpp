#include <iostream>
#include <cstdint>
#include <array>

// Define the maximum memory size (64 KB)
const size_t MAX_MEM = 1024 * 64;

// Type aliases for better readability
using Byte = uint8_t;  // 8-bit unsigned integer
using Word = uint16_t; // 16-bit unsigned integer
using u32 = uint32_t; // 32-bit unsigned integer
using s32 = int32_t;  // 32-bit signed integer

// Alias for a signed 8-bit integer
typedef int8_t SByte;

// Structure representing the memory
struct Mem {
    // Array to hold memory data (64 KB)
    std::array<Byte, MAX_MEM> Data;

    // Constructor to initialize memory
    Mem() {
        Initialise();
    }

    // Function to initialize memory to 0
    void Initialise() {
        Data.fill(0);
    }

    // Function to read a byte from memory at a given address
    Byte ReadByte(u32 Address) const {
        // Check if address is within memory bounds
        if (Address < MAX_MEM) {
            return Data[Address];
        }
        return 0; // Return 0 if address is out of bounds
    }

    // Function to write a byte to memory at a given address
    void WriteByte(u32 Address, Byte value) {
        // Check if address is within memory bounds
        if (Address < MAX_MEM) {
            Data[Address] = value;
        }
    }
};

// Structure representing the status flags
struct StatusFlags {
    Byte C : 1;      // Carry Flag (1 bit)
    Byte Z : 1;      // Zero Flag (1 bit)
    Byte I : 1;      // Interrupt disable (1 bit)
    Byte D : 1;      // Decimal mode (1 bit)
    Byte B : 1;      // Break (1 bit)
    Byte Unused : 1; // Unused (1 bit)
    Byte V : 1;      // Overflow (1 bit)
    Byte N : 1;      // Negative (1 bit)
};

// Structure representing the CPU
struct CPU {
    Word PC;  // Program Counter (16-bit)
    Byte SP;  // Stack Pointer (8-bit)
    Mem memory; // Memory object
    Byte A, X, Y;  // Registers (8-bit)

    // Union to access status flags either as a byte or a structure
    union {
        Byte PS;
        StatusFlags Flag;
    };

    // Member function declarations
    Byte Fetch(s32* Cycles);  // Fetch a byte from memory
    SByte FetchS(s32* Cycles); // Fetch a signed byte from memory
    Word FetchW(s32* Cycles); // Fetch a word (2 bytes) from memory
    Byte RByte(Word Address, s32* Cycles); // Read a byte from memory
    Word RWord(Word Address, s32* Cycles); // Read a word (2 bytes) from memory
    void WByte(Byte Value, Word Address, s32* Cycles); // Write a byte to memory
    void WWord(Word Value, Word Address, s32* Cycles); // Write a word (2 bytes) to memory
    Word GetSPAddress() const; // Get the address pointed to by the stack pointer
    void PWtoS(Word Value, s32* Cycles); // Push a word to the stack
    void PPCmtoS(s32* Cycles); // Push PC - 1 to the stack
    void PPCptoS(s32* Cycles); // Push PC + 1 to the stack
    void PPCtoS(s32* Cycles); // Push PC to the stack
    void PBontoS(Byte Value, s32* Cycles); // Push a byte to the stack
    Byte PBfromS(s32* Cycles); // Pop a byte from the stack
    Word PWfromS(s32* Cycles); // Pop a word from the stack

    // Constructor to initialize CPU
    CPU() {
        Reset();
    }

    // Function to reset the CPU to its initial state
    void Reset() {
        PC = 0xFFFC; // Initialize program counter
        SP = 0xFF;  // Initialize stack pointer
        Flag.C = Flag.Z = Flag.I = Flag.D = Flag.B = Flag.V = Flag.N = 0; // Clear all status flags
        A = X = Y = 0; // Initialize registers to 0
        memory.Initialise(); // Initialize memory
    }
};

// CPU member function definitions (outside the struct)
Byte CPU::Fetch(s32* Cycles) {
    // Fetch a byte from memory at the address pointed to by PC
    Byte Data = memory.ReadByte(PC);
    // Increment PC to point to the next byte
    PC++;
    // Decrement the cycle count
    (*Cycles)--;
    // Return the fetched byte
    return Data;
}

SByte CPU::FetchS(s32* Cycles) {
    // Fetch a signed byte from memory
    return static_cast<SByte>(Fetch(Cycles));
}

Word CPU::FetchW(s32* Cycles) {
    // Fetch a word (2 bytes) from memory
    Word Data = memory.ReadByte(PC);
    PC++;
    Data |= (memory.ReadByte(PC) << 8); // Combine the two bytes
    PC++;
    (*Cycles) -= 2; // Decrement cycle count for two reads
    return Data;
}

Byte CPU::RByte(Word Address, s32* Cycles) {
    // Read a byte from memory at a given address
    Byte Data = memory.ReadByte(Address);
    (*Cycles)--;
    return Data;
}

Word CPU::RWord(Word Address, s32* Cycles) {
    // Read a word (2 bytes) from memory at a given address
    Byte LSB = RByte(Address, Cycles); // Read the least significant byte
    Byte MSB = RByte(Address + 1, Cycles); // Read the most significant byte
    return LSB | (MSB << 8); // Combine the two bytes
}

void CPU::WByte(Byte Value, Word Address, s32* Cycles) {
    // Write a byte to memory at a given address
    memory.WriteByte(Address, Value);
    (*Cycles)--;
}

void CPU::WWord(Word Value, Word Address, s32* Cycles) {
    // Write a word (2 bytes) to memory at a given address
    memory.WriteByte(Address, Value & 0xFF); // Write the least significant byte
    memory.WriteByte(Address + 1, Value >> 8); // Write the most significant byte
    (*Cycles) -= 2;
}

Word CPU::GetSPAddress() const {
    // Get the address pointed to by the stack pointer
    return 0x100 | SP; // Add the stack pointer to the base address of the stack
}

void CPU::PWtoS(Word Value, s32* Cycles) {
    // Push a word to the stack
    WByte(Value >> 8, GetSPAddress(), Cycles); // Write the most significant byte to the stack
    SP--; // Decrement the stack pointer
    WByte(Value & 0xFF, GetSPAddress(), Cycles); // Write the least significant byte to the stack
    SP--; // Decrement the stack pointer
}

void CPU::PPCmtoS(s32* Cycles) {
    // Push PC - 1 to the stack
    PWtoS(PC - 1, Cycles);
}

void CPU::PPCptoS(s32* Cycles) {
    // Push PC + 1 to the stack
    PWtoS(PC + 1, Cycles);
}

void CPU::PPCtoS(s32* Cycles) {
    // Push PC to the stack
    PWtoS(PC, Cycles);
}

void CPU::PBontoS(Byte Value, s32* Cycles) {
    // Push a byte to the stack
    WByte(Value, GetSPAddress(), Cycles); // Write the byte to the stack
    SP--; // Decrement the stack pointer
    (*Cycles)--;
}

Byte CPU::PBfromS(s32* Cycles) {
    // Pop a byte from the stack
    SP++; // Increment the stack pointer
    (*Cycles)--;
    Byte Value = memory.ReadByte(GetSPAddress()); // Read the byte from the stack
    (*Cycles)--;
    return Value;
}

Word CPU::PWfromS(s32* Cycles) {
    // Pop a word from the stack
    Word ValueFromStack = RWord(GetSPAddress() + 1, Cycles); // Read the word from the stack
    SP += 2; // Increment the stack pointer by 2
    return ValueFromStack;
}

int main() {
    s32 Cycles; // Variable to store the cycle count
    CPU cpu; // Create a CPU object

    // Write the value 0xABCD to memory addresses 0x100 and 0x101
    cpu.memory.WriteByte(0x100, 0xAB);
    cpu.memory.WriteByte(0x101, 0xCD);

    // Test the Fetch function
    Cycles = 10;
    Byte fetchedByte = cpu.Fetch(&Cycles);
    std::cout << "Fetched byte: 0x" << std::hex << static_cast<int>(fetchedByte) << std::endl;

    // Test the RWord function
    Cycles = 10;
    Word readWord = cpu.RWord(0x100, &Cycles);
    std::cout << "Read word: 0x" << std::hex << readWord << std::endl;

    // Test the WWord function
    Cycles = 10;
    cpu.WWord(0x1234, 0x102, &Cycles);

    // Test the PWtoS function
    Cycles = 10;
    cpu.PWtoS(0x5678, &Cycles);

    // Test the PWfromS function
    Cycles = 10;
    Word poppedWord = cpu.PWfromS(&Cycles);
    std::cout << "Popped word: 0x" << std::hex << poppedWord << std::endl;

    return 0;
}