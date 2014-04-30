#define LINKER_VERSION "1.0.0"
/*
NOTICE:
Signatures are done in in x86 standard byte conventions NOT in
byte wise patterns. This is to aid in transcription due to x86
casting mechanics.

for instance:
E_MAGIC is [45 5A] when put in the file however is is stored as
the short [5A 45]. This means bytes are transcribed in order of
lowest byte to highest (right to left) opposite of how they are
read (left to right).

Redundant zeros indicate bytes that are part of the signature
but simply have no value.
*/
enum SUBSYSTEMS { SUBSYSTEM_UNKOWN, SUBSYSTEM_NATIVE, SUBSYSTEM_GUI, SUBSYSTEM_CUI };

//DOS HEADER
#define E_MAGIC 0x5A4D
#define PE_HEADER_OFFSET 0x00000040 

//PE HEADER
#define PE_SIGNATURE 0x00004550
#define TARGET_MACHINE 0x14C
#define SEGMENT_COUNT 0x3
#define SIZE_OF_OPTIONAL_HEADER 0xE0
#define CHARACTERISTICS 0x102

//OPTIONAL HEADER
#define MAGIC_32_BIT 0x10B
#define IMAGE_BASE 0x400000
#define MAJOR_SUBSYSTEM_VER 0x4
#define SUBSYSTEM SUBSYSTEM_CUI
#define FILE_ALIGNMET 0x8000
#define SECTION_ALIGNMENT FILE_ALIGNMENT << 1
#define IMAGE_SIZE SELECTION_ALIGNMENT << 2
#define ADDRESS_ENTRY_POINT SECTION_ALIGNMENT
#define SIZE_OF_HEADER 0x200
#define NUMBER_OF_RVA_SIZES 0x20

//DATA DIRECTORIES
#define IMPORTS_VA SECTION_ALIGNMENT * 2

//SECTION TABLE
#define VIRTUAL_SIZE SECTION_ALIGNMENT
#define SIZE_RAW_DATA FILE_ALIGNMENT
#define TEXT_ADRESS SECTION_ALIGNMENT
#define RDATA_ADRESS SECTION_ALIGNMENT * 2
#define DATA_ADRESS SECTION_ALIGNMENT * 3
#define TEXT_RAW FILE_ALIGNMENT
#define RDATA_RAW FILE_ALIGNMENT * 2
#define DATA_RAW FILE_ALIGNMENT * 3
#define TEXT_CHARACTER 0x60000002
#define RDATA_CHARACTER 0x40000004
#define DATA_CHARACTER 0x0C000004