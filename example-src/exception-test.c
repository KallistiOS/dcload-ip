void main(void)
{
    __builtin_sh_set_fpscr(0x40001 | (1 << 10) ); // enable FPU exception (divide by 0)
    asm volatile ("fldi0 fr0\n\t" // load 0.0f
    "fldi1 fr1\n\t" // load 1.0f
    "fdiv fr0, fr1\n\t" // divide by 0
  );
}