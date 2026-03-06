/*
 * OpenBSD RISC-V 64 linker script — merge .rodata into .text (PIE-compatible)
 *
 * The RISC-V 64 backend emits .rodata.cst8 constant-pool entries for
 * 64-bit values that cannot be cheaply materialised as immediate
 * instruction sequences.  Merging .rodata into .text keeps the
 * constant-pool data inside the extracted PIC binary.
 *
 * No explicit base address is set because the binary is linked as PIE
 * (ET_DYN) — the dynamic loader assigns the virtual address at runtime.
 * OpenBSD requires PIE on riscv64; the kernel rejects ET_EXEC with
 * ENOEXEC.
 *
 * .text input sections are listed FIRST so that --symbol-ordering-file
 * can place entry_point at offset 0 of the output section.
 */

SECTIONS
{
    .text : {
        *(.text .text.*)
        *(.rodata .rodata.*)
    }
}
