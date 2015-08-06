;============================================================================
;  RAVM, a RISC-approximating virtual machine that fits in the L1 cache.
;  Copyright (C) 2012-2013 by Zack T Smith.
;
;  This program is free software; you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation; either version 2 of the License, or
;  (at your option) any later version.
;
;  This program is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with this program; if not, write to the Free Software
;  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
;
;  The author may be reached at 1@zsmith.co.
;=============================================================================

bits	32
cpu	ia64

global	_Interpret

extern	_putchar
extern	_printf
extern	_puts
extern	_fflush
extern	_malloc
extern	_free

%define RESULT_OK 0
%define RESULT_PROGRAM_BOUNDS 1 ; Instruction pointer went out of bounds.
%define RESULT_MEMORY_BOUNDS 2  ; Memory access ws out of bounds.
%define RESULT_STACK_BOUNDS 3   ; Stack overflow OR underflow. 
%define RESULT_STACK_UNDERFLOW 4 
%define RESULT_STACK_OVERFLOW 5 
%define RESULT_INVALID_ALLOCA_PARAM 6 
%define RESULT_DIVIDE_BY_ZERO 7
%define RESULT_CALLOUT_IMPOSSIBLE 8

%define DEST eax
%define DESTWORD ax
%define DESTBYTE al
%define DESTREG ebx
%define SRCREG ecx	; Must keep as ECX for shift instructions!
%define SRCREGBYTE cl 	; Must keep as ECX for shift instructions!
%define REGS ebp
%define TEMP edx
%define TEMPWORD dx
%define TEMPBYTE dl

%define REGIP edi 
%define REGSP esi
%define ADDRESSBUFFER esi

;-----------------------------------------------------------------------------

%macro MEMORY_BOUNDS_CHECK 1
	cmp %1, [memory_start]  
	jb error_memory_bounds 
	cmp %1, [memory_end] 
	jae error_memory_bounds
%endmacro

%macro PROGRAM_BOUNDS_CHECK 0
	cmp REGIP, dword [program_start]
	jb error_program_bounds
	cmp REGIP, dword [program_end]
	jae error_program_bounds
%endmacro

%macro STACK_BOUNDS_CHECK 0
	cmp REGIP, dword [stack_start]	
	jl error_stack_bounds
	cmp REGIP, dword [stack_end]
	jge error_stack_bounds
%endmacro

;-----------------------------------------------------------------------------
	section .text

;-----------------------------------------------------------------------------

error_callout_impossible:
	mov eax, RESULT_CALLOUT_IMPOSSIBLE
	jmp done

error_invalid_alloca_value:
	mov eax, RESULT_INVALID_ALLOCA_PARAM
	jmp done

error_divide_by_zero:
	mov eax, RESULT_DIVIDE_BY_ZERO
	jmp done

error_memory_bounds:
	mov	eax, RESULT_MEMORY_BOUNDS
	jmp done

error_program_bounds:
	mov	eax, RESULT_PROGRAM_BOUNDS
	jmp done

error_stack_bounds:
	mov	eax, RESULT_STACK_BOUNDS
	jmp done

error_stack_underflow:
	mov	eax, RESULT_STACK_UNDERFLOW
	jmp done

error_stack_overflow:
	mov	eax, RESULT_STACK_OVERFLOW
	jmp done

;-----------------------------------------------------------------------------
op_exit:
	xor eax, eax
done:
	mov [save_eax], eax

%if 0
	push dword [registers]
	call _free
	add esp, 4
%endif

	pop ebp
	pop edi
	pop esi
	pop edx
	pop ecx
	pop ebx

	push ebx
	push ecx
	push edx
	push esi
	push edi
	push ebp
	push string
	call _puts
	add esp, 4
	pop ebp
	pop edi
	pop esi
	pop edx
	pop ecx
	pop ebx

	mov eax, [save_eax]
	ret

;------------------------------------------------------------------------------
; Name:         Interpret
; Purpose:      Run some RISC bytecode.
; Params:       
;               [esp+4] = ptr to start of program bytes
;               [esp+8] = ptr to start of memory bytes
;               [esp+12] = ptr to start of stack bytes
;               [esp+16] = ptr to end of program bytes
;               [esp+20] = ptr to end of memory bytes
;               [esp+24] = ptr to end of stack bytes
;               [esp+28] = ptr to callout function
;               [esp+32] = ptr to constants area
;               [esp+36] = ptr to end of constants area
;------------------------------------------------------------------------------

        align 32
_Interpret:
        ;----------------------------------------
        ; Save memory regions to more convenient
        ; locations than on the stack.
        ;
        mov eax, [esp+4]
        mov [program_start], eax
        mov eax, [esp+16]
        mov [program_end], eax

        mov eax, [esp+8]
        mov [memory_start], eax
        mov eax, [esp+20]
        mov [memory_end], eax

        mov eax, [esp+12]
        mov [stack_start], eax
        mov eax, [esp+24]
        mov [stack_end], eax

        mov eax, [esp+28]
        mov [callout_function], eax

        mov eax, [esp+32]
        mov [constants_start], eax
        mov eax, [esp+36]
        mov [constants_end], eax

	push ebx
	push ecx
	push edx
	push esi
	push edi
	push ebp

%if 0
	push dword 256*4
	call _malloc
	add esp, 4
	mov REGS, eax
	mov dword [registers], eax
%endif
	mov REGS, registers

	; Fill the registers with increasing numbers.
	xor eax, eax
.L1
	mov dword [REGS + eax*4], eax
	inc eax
	cmp eax, 256
	jb .L1

	mov REGSP, [stack_end]
	mov REGIP, [program_start]
	jmp mainloop_post_check

do_near_branch:
	movsx SRCREG, SRCREGBYTE
	add REGIP, SRCREG
	jmp mainloop_post_check

dont_branch:
	add REGIP, 4
	jmp mainloop

do_branch:
	add REGIP, dword [REGIP]
	
mainloop_full_check:
        cmp REGIP, dword [program_start]
	jb error_program_bounds
mainloop:
        cmp REGIP, dword [program_end]
        jae error_program_bounds

mainloop_post_check:
	mov eax, [REGIP]
	add REGIP, 4
	mov edx, eax		; now get the opcode
	shr edx, 24
	movzx DESTREG, al		; obtain register-number0 (destination)
	movzx SRCREG, ah  		; obtain register-number1 (source)
	mov DEST, [REGS + DESTREG*4]	; get reg0 data

%if 0
	push eax
	push ebx
	push ecx
	push edx
	push esi
	push edi
	push ebp
	push ebp
	push ebp
	push ebp

	push edx
	push REGIP
	push dword printopcode_string
	call _printf	; ESP is 16 byte aligned
	add esp, 12

	pop ebp
	pop ebp
	pop ebp
	pop ebp
	pop edi
	pop esi
	pop edx
	pop ecx
	pop ebx
	pop eax
%endif
	jmp [edx*4 + opcode_handlers]	; now jump to the operation handler.

align 32

op_not:
	xor DEST, 0xffffffff
	mov dword [4*DESTREG + REGS], DEST
	jmp mainloop

op_logical_or:
	or DEST, dword [4*SRCREG + REGS]
	jz .L1
	mov DEST, 1
.L1
	mov dword [4*DESTREG + REGS], DEST
	jmp mainloop

op_logical_and:
	or DEST, DEST
	jz .L0
	test dword [4*SRCREG + REGS], 0xffffffff
	jz .L0
	mov DEST, 1
.L0
	mov dword [4*DESTREG + REGS], DEST
	jmp mainloop

op_logical_not:
	mov ebx, 1
	not DEST
	cmovnz DEST, ebx
	mov dword [4*DESTREG + REGS], DEST
	jmp mainloop

op_neg:
	neg DEST
	mov dword [4*DESTREG + REGS], DEST
	jmp mainloop

op_decjnz_near:
	dec DEST
	mov dword [4*DESTREG + REGS], DEST
	jz mainloop
	sub REGIP, SRCREG
	jmp mainloop

op_decjnz:
	mov SRCREG, 4
	dec DEST
	mov dword [4*DESTREG + REGS], DEST
	cmovnz SRCREG, dword [REGIP]
	add REGIP, SRCREG
	jmp mainloop_full_check

op_loop:
	; Source register contains branch destination address.
	; Destination register has counter to be decremented.
	dec eax
	mov dword [4*DESTREG + REGS], eax
	jz mainloop
	mov REGIP, dword [REGS + 4*SRCREG]
	jmp mainloop_full_check	; Must range check in case "repeat" instruction not used.

op_repeat:
	mov dword [4*DESTREG + REGS], REGIP
	jmp mainloop

op_jset:
	mov TEMP, 1
	sal TEMP, cl
	test DEST, TEMP
	jz dont_branch
	jmp do_branch 

op_jset_near:
	mov TEMP, 1
	sal TEMP, cl
	test DEST, TEMP
	jz dont_branch
	jmp do_near_branch

op_jclear:
	mov TEMP, 1
	sal TEMP, cl
	test DEST, TEMP
	jnz dont_branch
	jmp do_branch 

op_jclear_near:
	mov TEMP, 1
	sal TEMP, cl
	test DEST, TEMP
	jnz dont_branch
	jmp do_near_branch

op_jnz:
	mov edx, 4
	test eax, eax
	cmovnz edx, dword [REGIP]
	add REGIP, edx
	jmp mainloop_full_check

op_jz:
	mov SRCREG, 4
	test eax, eax
	cmovz SRCREG, dword [REGIP]
	add REGIP, SRCREG
	jmp mainloop_full_check

op_jz_near:
	test eax, eax
	jz do_near_branch
	jmp mainloop

op_jnz_near:
	test eax, eax
	jnz do_near_branch
	jmp mainloop

op_jb_near:
	cmp eax, dword [REGS + SRCREG*4]
	jb do_near_branch
	jmp dont_branch
op_ja_near:
	cmp eax, dword [REGS + SRCREG*4]
	ja do_near_branch
	jmp dont_branch
op_jbe_near:
	cmp eax, dword [REGS + SRCREG*4]
	jbe do_near_branch
	jmp dont_branch
op_jae_near:
	cmp eax, dword [REGS + SRCREG*4]
	jae do_near_branch
	jmp dont_branch
op_jl_near:
	cmp eax, dword [REGS + SRCREG*4]
	jl do_near_branch
	jmp dont_branch
op_jg_near:
	cmp eax, dword [REGS + SRCREG*4]
	jg do_near_branch
	jmp dont_branch
op_jle_near:
	cmp eax, dword [REGS + SRCREG*4]
	jle do_near_branch
	jmp dont_branch
op_jge_near:
	cmp eax, dword [REGS + SRCREG*4]
	jge do_near_branch
	jmp dont_branch
op_je_near:
	cmp eax, dword [REGS + SRCREG*4]
	je do_near_branch
	jmp dont_branch
op_jne_near:
	cmp eax, dword [REGS + SRCREG*4]
	jne do_near_branch
	jmp dont_branch

op_jb:
	cmp eax, dword [REGS + SRCREG*4]
	jb do_branch
	jmp dont_branch
op_ja:
	cmp eax, dword [REGS + SRCREG*4]
	ja do_branch
	jmp dont_branch
op_jbe:
	cmp eax, dword [REGS + SRCREG*4]
	jbe do_branch
	jmp dont_branch
op_jae:
	cmp eax, dword [REGS + SRCREG*4]
	jae do_branch
	jmp dont_branch
op_jl:
	cmp eax, dword [REGS + SRCREG*4]
	jl do_branch
	jmp dont_branch
op_jg:
	cmp eax, dword [REGS + SRCREG*4]
	jg do_branch
	jmp dont_branch
op_jle:
	cmp eax, dword [REGS + SRCREG*4]
	jle do_branch
	jmp dont_branch
op_jge:
	cmp eax, dword [REGS + SRCREG*4]
	jge do_branch
	jmp dont_branch
op_je:
	cmp eax, dword [REGS + SRCREG*4]
	je do_branch
	jmp dont_branch
op_jne:
	cmp eax, dword [REGS + SRCREG*4]
	jne do_branch
	jmp dont_branch

op_add:
	add eax, dword [REGS + SRCREG*4]		; get reg1 data
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_sub:
	sub eax, dword [REGS + SRCREG*4]		; get reg1 data
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_and:
	and eax, dword [REGS + SRCREG*4]		; get reg1 data
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_or:
	or eax, dword [REGS + SRCREG*4]		; get reg1 data
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_xor:
	xor eax, dword [REGS + SRCREG*4]		; get reg1 data
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_mov:
	mov eax, dword [4*SRCREG + REGS]
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_shl:
	mov ecx, dword [REGS + SRCREG*4]		
	shl eax, cl
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_shr:
	mov ecx, dword [REGS + SRCREG*4]		
	shr eax, cl
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_sar:
	mov ecx, dword [REGS + SRCREG*4]		
	sar eax, cl
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_shl_imm8:
	shl eax, cl
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_shr_imm8:
	shr eax, cl
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_sar_imm8:
	sar eax, cl
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_add_imm8:
	add eax, ecx
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_sub_imm8:
	sub eax, ecx
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_and_imm8:
	and eax, ecx
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_set_bit_imm8:
	mov TEMP, 1
	shl TEMP, cl
	or eax, ecx
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_clear_bit_imm8:
	mov TEMP, 1
	shl TEMP, cl
	xor TEMP, 0xffffffff
	and eax, ecx
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_invert_bit_imm8:
	mov TEMP, 1
	shl TEMP, cl
	xor eax, ecx
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_or_imm8:
	or eax, ecx
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_xor_imm8:
	xor eax, ecx
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_mul_imm8:
	mul ecx
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_mul_10:	; Faster than MUL instruction.
	lea eax, [eax + 4*eax]
	add eax, eax
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_mul_100:	; Faster than MUL instruction.
	lea eax, [eax + 4*eax]
	lea eax, [eax + 4*eax]
	shl eax, 2
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_div_imm8:
	test ecx, ecx
	jz error_divide_by_zero
	xor edx, edx
	div ecx
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_mod_imm8:
	test ecx, ecx
	jz error_divide_by_zero
	xor edx, edx
	div ecx
	mov dword [4*DESTREG + REGS], edx
	jmp mainloop

op_imul_imm8:
	imul ecx
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_idiv_imm8:
	or ecx, ecx
	jz error_divide_by_zero
	movsx ecx, cl
	xor edx, edx
	idiv ecx
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_imod_imm8:
	or ecx, ecx
	jz error_divide_by_zero
	movsx ecx, cl
	xor edx, edx
	idiv ecx
	mov dword [4*DESTREG + REGS], edx
	jmp mainloop

op_mul:
	mov ecx, [4*SRCREG + REGS]
	mul ecx
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_div:
	mov ecx, [4*SRCREG + REGS]
	test ecx, ecx
	jz error_divide_by_zero
	xor edx, edx
	div ecx
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_mod:
	mov ecx, [4*SRCREG + REGS]
	test ecx, ecx
	jz error_divide_by_zero
	xor edx, edx
	div ecx
	mov dword [4*DESTREG + REGS], edx
	jmp mainloop

op_imul:
	mov ecx, [4*SRCREG + REGS]
	imul ecx
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_idiv:
	mov ecx, [4*SRCREG + REGS]
	test ecx, ecx
	jz error_divide_by_zero
	xor edx, edx
	idiv ecx
	mov dword [4*DESTREG + REGS], eax
	jmp mainloop

op_imod:
	mov ecx, [4*SRCREG + REGS]
	test ecx, ecx
	jz error_divide_by_zero
	xor edx, edx
	idiv ecx
	mov dword [4*DESTREG + REGS], edx
	jmp mainloop

op_jump_near:
	movsx TEMP, TEMPBYTE
	add REGIP, TEMP
        jmp mainloop_full_check

op_jump:
        mov SRCREG, [REGIP]
        add REGIP, SRCREG	; This is the address of the routine being called.
        jmp mainloop_full_check

; XX Need to add call table-indirect instruction.
op_call_register_indirect:	; (As opposed to memory indirect etc.)
        sub REGSP, 4
        cmp REGSP, dword [stack_start]
        jb error_stack_overflow
	mov [REGSP], REGIP
        mov REGIP, DEST	; This is the absolute address of the routine being called.
        jmp mainloop_full_check

op_call:
op_call_relative:
        sub REGSP, 4
        cmp REGSP, dword [stack_start]
        jb error_stack_overflow
	lea DESTREG, [REGIP + 4]
	mov [REGSP], DESTREG
        add REGIP, [REGIP]  ; This is the address of the routine being called.
        jmp mainloop_full_check

op_jump_relative_near:
	movsx SRCREG, SRCREGBYTE
        sub REGSP, 4
        cmp REGSP, dword [stack_start]
        jb error_stack_overflow
        mov [REGSP], REGIP
	add REGIP, SRCREG
        jmp mainloop_full_check

op_call_relative_near_forward:
        sub REGSP, 4
        cmp REGSP, dword [stack_start]
        jb error_stack_overflow
        mov [REGSP], REGIP
	add REGIP, SRCREG
        jmp mainloop_full_check

op_call_relative_near_backward:
        sub REGSP, 4
        cmp REGSP, dword [stack_start]
        jb error_stack_overflow
        mov [REGSP], REGIP
	sub REGIP, SRCREG
        jmp mainloop_full_check

op_ret:
        cmp REGSP, dword [stack_end]
        jae error_stack_underflow
        mov REGIP, [REGSP]
        add REGSP, 4
        jmp mainloop_full_check

op_add_imm32:
	add eax, dword [REGIP]
	mov dword [4*DESTREG + REGS], eax
	add REGIP, 4
	jmp mainloop

op_mov_imm32:
	mov eax, dword [REGIP]
	mov dword [4*DESTREG + REGS], eax
	add REGIP, 4
	jmp mainloop

op_mov_imm8_signed:
	movsx ecx, cl
	mov dword [4*DESTREG + REGS], ecx
	jmp mainloop

op_mov_imm16_signed:
	mov eax, REGIP
	sub eax, 3
	movsx eax, word [eax]
	mov dword [4*SRCREG + REGS], eax	; Note! SRC is destination.
	jmp mainloop

op_alloca:
	or ecx, ecx
	jz error_invalid_alloca_value
	test ecx, 3
	jnz error_invalid_alloca_value
	sub REGSP, ecx
        cmp REGSP, dword [stack_start]
        jb error_stack_overflow
	jmp mainloop
	
op_drop:
	or ecx, ecx
	jz error_invalid_alloca_value
	test ecx, 3
	jnz error_invalid_alloca_value
	add REGSP, ecx
        cmp REGSP, dword [stack_end]
        ja error_stack_underflow	; OK to be >= stack_end.
	jmp mainloop

op_push:
	sub REGSP, 4
        cmp REGSP, dword [stack_start]
        jb error_stack_overflow
	mov [REGSP], eax
	jmp mainloop

op_pop:
        cmp REGSP, dword [stack_end]
        jae error_stack_underflow
	mov eax, dword [REGSP]
	mov dword [REGS + 4*DESTREG], eax
	add REGSP, 4
	jmp mainloop

op_get_stack_relative:
	lea TEMP, [REGSP + 4*SRCREG]
        cmp TEMP, dword [stack_end]
        jae error_stack_underflow
	mov DEST, [TEMP]
	mov dword [REGS + 4*DESTREG], DEST
	jmp mainloop

op_put_stack_relative:
	lea TEMP, [REGSP + 4*SRCREG]
        cmp TEMP, dword [stack_end]
        jae error_stack_underflow
	mov [TEMP], DEST
	jmp mainloop

op_load32:
	mov TEMP, [REGS + 4*SRCREG]
	add TEMP, [memory_start]
	MEMORY_BOUNDS_CHECK TEMP
	mov DEST, [TEMP]
	mov dword [REGS + 4*DESTREG], DEST
	jmp mainloop

op_load16_unsigned:
	mov TEMP, [REGS + 4*SRCREG]
	add TEMP, [memory_start]
	MEMORY_BOUNDS_CHECK TEMP
	movzx DEST, word [TEMP]
	mov dword [REGS + 4*DESTREG], DEST
	jmp mainloop

op_load16_signed:
	mov TEMP, [REGS + 4*SRCREG]
	add TEMP, [memory_start]
	MEMORY_BOUNDS_CHECK TEMP
	movsx DEST, word [TEMP]
	mov dword [REGS + 4*DESTREG], DEST
	jmp mainloop

op_load8_unsigned:
	mov TEMP, [REGS + 4*SRCREG]
	add TEMP, [memory_start]
	MEMORY_BOUNDS_CHECK TEMP
	movzx DEST, byte [TEMP]
	mov dword [REGS + 4*DESTREG], DEST
	jmp mainloop

op_load8_signed:
	mov TEMP, [REGS + 4*SRCREG]
	add TEMP, [memory_start]
	MEMORY_BOUNDS_CHECK TEMP
	movsx DEST, byte [TEMP]
	mov dword [REGS + 4*DESTREG], DEST
	jmp mainloop

op_write_memory32:
	mov DEST, [REGIP]
	mov TEMP, [REGIP+4]
	add REGIP, 8
	add DEST, [memory_start]
	MEMORY_BOUNDS_CHECK DEST
	mov [DEST], TEMP
	jmp mainloop

op_write_memory16:
	mov DEST, [REGIP]
	mov TEMP, [REGIP+4]
	add REGIP, 8
	add DEST, [memory_start]
	MEMORY_BOUNDS_CHECK DEST
	mov word [DEST], TEMPWORD
	jmp mainloop

op_write_memory8:	; Note! imm8 is stored in SRCREG.
	mov DEST, [REGIP]
	add REGIP, 4
	add DEST, [memory_start]
	MEMORY_BOUNDS_CHECK DEST
	mov byte [DEST], SRCREGBYTE
	jmp mainloop

op_store32:	; Note! Stores DEST into address given in SRC.
	mov TEMP, [REGS + 4*SRCREG]
	add TEMP, [memory_start]
	MEMORY_BOUNDS_CHECK TEMP
	mov dword [TEMP], DEST
	jmp mainloop

op_store16:	; Note! Stores DEST into address given in SRC.
	mov TEMP, [REGS + 4*SRCREG]
	add TEMP, [memory_start]
	MEMORY_BOUNDS_CHECK TEMP
	mov word [TEMP], DESTWORD
	jmp mainloop

op_store8:	; Note! Stores DEST into address given in SRC.
	mov TEMP, [REGS + 4*SRCREG]
	add TEMP, [memory_start]
	MEMORY_BOUNDS_CHECK TEMP
	mov byte [TEMP], DESTBYTE
	jmp mainloop

op_dump:
	mov [save_eax], eax
	mov [save_ebx], ebx
	mov [save_ecx], ecx
	mov [save_edx], edx
	mov [save_esi], esi
	mov [save_edi], edi
	mov [save_ebp], ebp

	mov eax, 0	; reg number
.L1
	push eax

	push dword 0
	push dword 0
	push dword 0

	push dword [REGS + eax*4 + 224*4]

	add eax, 224
	push eax
	sub eax, 224

	push dword [REGS + eax*4 + 192*4]

	add eax, 192
	push eax
	sub eax, 192

	push dword [REGS + eax*4 + 160*4]

	add eax, 160
	push eax
	sub eax, 160

	push dword [REGS + eax*4 + 128*4]

	add eax, 128
	push eax
	sub eax, 128

	push dword [REGS + eax*4 + 96*4]

	add eax, 96
	push eax
	sub eax, 96

	push dword [REGS + eax*4 + 64*4]

	add eax, 64
	push eax
	sub eax, 64

	push dword [REGS + eax*4 + 32*4];

	add eax, 32
	push eax
	sub eax, 32

	push dword [REGS + eax*4 + 0]

	push eax

	push dword regdump_string_line
	call _printf	; ESP is 16 byte aligned.
	add esp, 20*4

	pop eax
	inc eax
	cmp eax, 32
	jl .L1

	;push dword 0
	;call _fflush
	;add esp, 4

	mov eax, [save_eax]
	mov ebx, [save_ebx]
	mov ecx, [save_ecx]
	mov edx, [save_edx]
	mov esi, [save_esi]
	mov edi, [save_edi]
	mov ebp, [save_ebp]
	jmp mainloop

;----------------------------------------
; Dump x86 registers (for testing only).
;
dump:
	push dword 0
	push dword 0
	push dword 0
	mov [save_eax], eax
	mov [save_ebx], ebx
	mov [save_ecx], ecx
	mov [save_edx], edx
	mov [save_esi], esi
	mov [save_edi], edi
	mov [save_ebp], ebp

%if 1
	push esp
	push dword [save_ebp]
	push dword [save_edi]
	push dword [save_esi]
	push dword [save_edx]
	push dword [save_ecx]
	push dword [save_ebx]
	push dword [save_eax]
	push dword x86_regdump_string
	call _printf	; ESP is 16 byte aligned.
	add esp, 9*4

	push dword 0
	call _fflush
	add esp, 4
%endif

	mov eax, [save_eax]
	mov ebx, [save_ebx]
	mov ecx, [save_ecx]
	mov edx, [save_edx]
	mov esi, [save_esi]
	mov edi, [save_edi]
	mov ebp, [save_ebp]
	add esp, 12
	ret

;-----------------------------------------------------------------------------
; Utility functions
;-----------------------------------------------------------------------------

builtin_putchar:
	push EBX
	push ECX
	push EDX
	push ESI
	push EDI
	push EBP

	push EAX
	call _putchar
	add ESP, 4

	pop EBP
	pop EDI
	pop ESI
	pop EDX
	pop ECX
	pop EBX
	ret

op_callout:
	test dword [callout_function], 0xffffffff
	jz error_callout_impossible

	; Get function number.
	sub REGIP, 2
	movzx TEMP, byte [REGIP]
	add REGIP, 2

	; Get 2nd param.
	mov SRCREG, [SRCREG*4 + REGS]

	; DEST is the parameter.
	; SRCREG is the function.
	push dword 0
	push dword 0
	push SRCREG
	push DEST
	push TEMP
	call [callout_function]
	add esp, 5*4
	jmp mainloop

op_putchar:
	push dword 0
	push dword 0
	push EBX
	push ECX
	push EDX
	push ESI
	push EDI
	push EBP

	push eax
	call _putchar	; ESP is aligned.
	add esp, 4

	pop EBP
	pop EDI
	pop ESI
	pop EDX
	pop ECX
	pop EBX
	add esp, 8
	
	jmp mainloop

;-----------------------------------------------------------------------------
; Name: 	op_print
; Purpose:	Prints a null-terminated 8-bits per character string out 
;		of the VM memory.
;-----------------------------------------------------------------------------

op_print:
call dump
	mov TEMP, DEST
	add TEMP, [memory_start]
.L0:
	MEMORY_BOUNDS_CHECK TEMP

	; 8-bit characters only.
	movzx eax, byte [TEMP]
	test al, al
	jz .L1
	
	push dword 0
	push dword 0
	push EBX
	push ECX
	push EDX
	push ESI
	push EDI
	push EBP
	push eax
	call _putchar	; ESP is aligned.
	add esp, 4
	pop EBP
	pop EDI
	pop ESI
	pop EDX
	pop ECX
	pop EBX
	add esp, 8

	inc TEMP
	jmp .L0

.L1
	; RULE: If the immediate value is nonzero,
	; write out a newline. Else not.
	;
	test cl, cl
	jz mainloop
	mov eax, 10
	jmp op_putchar

;-----------------------------------------------------------------------------
; Name: 	op_printhex
; Purpose:	Prints an 8-digit hex value from DEST register.
;-----------------------------------------------------------------------------

op_printhex:
	mov TEMP, SRCREG
	mov ebx, eax
	mov ecx, 7

	; Convert the hex to ASCII.
.L0:
	mov al, bl
	and al, 15
	add al, 48
	cmp al, 58
	jb .L1
	add al, 39
.L1:
	shr ebx, 4
	mov byte [hexstr + ecx], al
	dec ecx
	jns .L0

	mov dword [hexstr + 8], 0

	or TEMP, TEMP
	jz .L2
	mov byte [hexstr + 8], 10

.L2:
	push dword 0
	push dword 0
	push EBX
	push ECX
	push EDX
	push ESI
	push EDI
	push EBP

	push dword hexstr
	call _printf
	add esp, 4
	
	pop EBP
	pop EDI
	pop ESI
	pop EDX
	pop ECX
	pop EBX
	add esp, 8

	jmp mainloop

;-----------------------------------------------------------------------------
; Data Section
;-----------------------------------------------------------------------------

section .data

opcode_handlers:
	dd mainloop	
	dd op_dump
	dd op_exit

	; Moves
	dd op_load16_signed
	dd op_load16_unsigned
	dd op_load32
	dd op_load8_signed
	dd op_load8_unsigned
	dd op_mov
	dd op_mov_imm16_signed	
	dd op_mov_imm32
	dd op_mov_imm8_signed
	dd op_store16
	dd op_store32
	dd op_store8
	dd op_write_memory16
	dd op_write_memory32
	dd op_write_memory8

	; Shifts
	dd op_sar
	dd op_sar_imm8
	dd op_shl
	dd op_shl_imm8
	dd op_shr
	dd op_shr_imm8

	; Arithmetic
	dd op_add
	dd op_add_imm32
	dd op_add_imm8
	dd op_div
	dd op_div_imm8
	dd op_idiv
	dd op_idiv_imm8
	dd op_imod
	dd op_imod_imm8
	dd op_imul
	dd op_imul_imm8
	dd op_mod
	dd op_mod_imm8
	dd op_mul
	dd op_mul_10
	dd op_mul_100
	dd op_mul_imm8
	dd op_neg
	dd op_sub
	dd op_sub_imm8

	; Logical operations
	dd op_logical_and
	dd op_logical_not
	dd op_logical_or

	; Bitwise operations
	dd op_and
	dd op_and_imm8
	dd op_clear_bit_imm8
	dd op_invert_bit_imm8
	dd op_not
	dd op_or
	dd op_or_imm8
	dd op_set_bit_imm8
	dd op_xor
	dd op_xor_imm8

	; Functions
	dd op_call
	dd op_call_register_indirect
	dd op_call_relative_near_backward
	dd op_call_relative_near_forward
	dd op_ret

	; Stack
	dd op_alloca
	dd op_drop
	dd op_get_stack_relative
	dd op_pop
	dd op_push
	dd op_put_stack_relative

	; Jumps
	dd op_decjnz
	dd op_decjnz_near
	dd op_ja
	dd op_ja_near
	dd op_jae
	dd op_jae_near
	dd op_jb
	dd op_jb_near
	dd op_jbe
	dd op_jbe_near
	dd op_jclear
	dd op_jclear_near
	dd op_je
	dd op_je_near
	dd op_jg
	dd op_jg_near
	dd op_jge
	dd op_jge_near
	dd op_jl
	dd op_jl_near
	dd op_jle
	dd op_jle_near
	dd op_jne
	dd op_jne_near
	dd op_jnz
	dd op_jnz_near
	dd op_jset
	dd op_jset_near
	dd op_jump
	dd op_jump_near
	dd op_jump_relative_near
	dd op_jz
	dd op_jz_near
	dd op_loop
	dd op_repeat

	; I/O
	dd op_putchar
	dd op_callout
	dd op_print
	dd op_printhex

	times 150 dd op_exit

registers:	times 256 dd 0

string:
	db 'Done.', 10, 0

memory_start    dd 0
memory_end      dd 0
stack_start     dd 0
stack_end       dd 0
program_start   dd 0
program_end     dd 0
constants_start   dd 0
constants_end     dd 0

done_string:
	db 'Done.', 10, 0

regdump_string_title: db '---RAVM Register Dump---', 10, 0
regdump_string_line: 
                db 'r%d %08lx', 9, 'r%d %08lx', 9, 'r%d %08lx', 9, 'r%d %08lx', 9
                db 'r%d %08lx', 9, 'r%d %08lx', 9, 'r%d %08lx', 9, 'r%d %08lx', 10
		db 0

x86_regdump_string:
		db '------- x86 REGISTERS: -------', 10
		db 'eax %08lx ebx %08lx ecx %08lx edx %08lx', 10, 'esi %08lx edi %08lx ebp %08lx esp %08lx', 10, 0

save_eax        dd 0
save_ebx        dd 0
save_ecx        dd 0
save_edx        dd 0
save_esi        dd 0
save_edi        dd 0
save_ebp        dd 0

hexstr	times 12 db 0

callout_function	dd 0

print_hex32_string	db '%08lx', 0

print_uint32_string	db '%lu', 0

print_int32_string	db '%ld', 0

printchar_string	db '%c', 0

printopcode_string	db 'IP=%08lx, opcode=%08lx', 10, 0

finit

