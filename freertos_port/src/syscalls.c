#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

extern void hw_uart_putc(char c);

int _write(int file, char *ptr, int len)
{
    (void)file;

    for (int i = 0; i < len; ++i)
    {
        hw_uart_putc(ptr[i]);
    }

    return len;
}

int _close(int file)
{
    (void)file;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    (void)file;

    st->st_mode = S_IFCHR;

    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;

    return 0;
}

int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;

    return 0;
}

void _exit(int status)
{
    (void)status;

    while (1)
    {
        __asm volatile ("nop");
    }
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;

    return -1;
}

int _getpid(void)
{
    return 1;
}

void *_sbrk(int incr)
{
    extern char _end;
    extern char _estack;

    static char *heap_end;

    if (heap_end == 0)
    {
        heap_end = &_end;
    }

    char *prev_heap_end = heap_end;

    if ((heap_end + incr) >= &_estack)
    {
        return (void *)-1;
    }

    heap_end += incr;

    return prev_heap_end;
}