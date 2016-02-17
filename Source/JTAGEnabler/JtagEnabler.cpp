#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

//#define BCM2708_PERI_BASE	0x20000000
#define BCM2708_PERI_BASE  	0x3F000000
#define GPIO_BASE          			(BCM2708_PERI_BASE + 0x200000)

class GpioFunctionSelector
{
private:
	volatile unsigned *m_pAltFunctionRegisters;

public:
	GpioFunctionSelector()
		: m_pAltFunctionRegisters((volatile unsigned *)MAP_FAILED)
	{
		int fd = open("/dev/mem", O_RDWR | O_SYNC);

		if (fd < 0)
			return;

		m_pAltFunctionRegisters = (volatile unsigned *)mmap(NULL, 0x18, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO_BASE);
		if (m_pAltFunctionRegisters == MAP_FAILED)
			return;

		close(fd);
	}

	~GpioFunctionSelector()
	{
		if (m_pAltFunctionRegisters != MAP_FAILED)
			munmap((void *)m_pAltFunctionRegisters, 0x18);
	}

	bool IsValid()
	{
		return m_pAltFunctionRegisters != MAP_FAILED;
	}

	void SetGPIOFunction(int GPIO, int functionCode)
	{
		int registerIndex = GPIO / 10;
		int bit = (GPIO % 10) * 3;

		unsigned oldValue = m_pAltFunctionRegisters[registerIndex];
		unsigned mask = 0b111 << bit;
		printf("Changing function of GPIO%d from %x to %x\n", GPIO, (oldValue >> bit) & 0b111, functionCode);
		m_pAltFunctionRegisters[registerIndex] = (oldValue & ~mask) | ((functionCode << bit) & mask);
	}
};

#define GPIO_ALT_FUNCTION_4 0b011
#define GPIO_ALT_FUNCTION_5 0b010

int main()
{
	GpioFunctionSelector selector;
	if (!selector.IsValid())
	{
		printf("Cannot open /dev/mem! Re-run me as root!\n");
		return 1;
	}

	selector.SetGPIOFunction(22, GPIO_ALT_FUNCTION_4);
	selector.SetGPIOFunction(4,  GPIO_ALT_FUNCTION_5);
	selector.SetGPIOFunction(27, GPIO_ALT_FUNCTION_4);
	selector.SetGPIOFunction(25, GPIO_ALT_FUNCTION_4);
	selector.SetGPIOFunction(23, GPIO_ALT_FUNCTION_4);
	selector.SetGPIOFunction(24, GPIO_ALT_FUNCTION_4);

	printf("Successfully enabled JTAG pins. You can start debugging now.\n");

	return 0;
}