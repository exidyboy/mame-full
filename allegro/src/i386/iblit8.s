/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      256 color bitmap blitting (written for speed, not readability :-)
 *
 *      By Shawn Hargreaves.
 *
 *      Stefan Schimanski optimised the reverse blitting function.
 *
 *      MMX clear code by Robert Ohannessian.
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"
#include "blit.inc"

#ifdef ALLEGRO_COLOR8

.text



/* void _linear_clear_to_color8(BITMAP *bitmap, int color);
 *  Fills a linear bitmap with the specified color.
 */
FUNC(_linear_clear_to_color8)
   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi
   pushw %es

   movl ARG1, %edx               /* edx = bmp */
   movl BMP_CT(%edx), %ebx       /* line to start at */

   movw BMP_SEG(%edx), %es       /* select segment */

   movl BMP_CR(%edx), %esi       /* width to clear */
   subl BMP_CL(%edx), %esi

#ifdef ALLEGRO_MMX               /* only use MMX if compiler supports it */

   movl GLOBL(cpu_mmx), %eax     /* if MMX is enabled (or not disabled :) */
   orl %eax, %eax
   jz clear_no_mmx

   movl %esi, %eax               /* if less than 32 pixels, use non-MMX */
   shrl $5, %eax
   orl %eax, %eax
   jz clear_no_mmx

   movb ARG2, %al                /* duplicate color 4 times */
   movb %al, %ah
   shll $16, %eax
   movb ARG2, %al
   movb %al, %ah

   pushl %eax

   pushw %ds                     /* cheap hack to get es and ds in ax and cx */
   movw %es, %cx
   popw %ax

   cmpw %ax, %cx                 /* can we use nearptr ? */
   jne clearMMXseg_loop          /* if not, then we have to decode segments...*/
				 /* else, we save one cycle per 8 pixels on PMMX/K6 */
   _align_
clearMMX_loop:
   movl %ebx, %eax
   WRITE_BANK()                  /* select bank */
   movl %eax, %edi
   addl BMP_CL(%edx), %edi       /* get line address  */

   popl %eax                     /* get eax back */

   movl %esi, %ecx               /* width to clear */

   movd %eax, %mm0               /* restore mmx reg 0 in case it's been clobbered by WRITE_BANK() */
   movd %eax, %mm1
   psllq $32, %mm0
   por %mm1, %mm0

   pushl %eax                    /* save eax */

   testl $7, %edi                /* is destination aligned on 64-bit ? */
   jz clearMMX_aligned

clearMMX_do_alignment:
   movl %edi, %eax               /* we want to adjust %ecx  (pairing: see andl) */

   movq %mm0, (%edi)             /* we clear 8 pixels */

   andl $7, %eax                 /* we calc how may pixels we actually wanted to clear (8 - %eax) (see subl) */

   andl $0xFFFFFFF8, %edi        /* instruction pairing (see inc %edi) */

   subl $8, %eax

   addl $8, %edi                 /* we set %edi to the next aligned memory address */

   addl %ecx, %eax               /* and adjust %ecx to reflect the change */

clearMMX_aligned:
   movl %ecx, %eax               /* save for later */
   shrl $5, %ecx                 /* divide by 32 for 4 * 8-byte memory move */
   jz clearMMX_finish_line       /* if there's less than 32 pixels to clear, no need for MMX */

clearMMX_continue_line:
   movq %mm0, (%edi)             /* move 4x 8 bytes */
   movq %mm0, 8(%edi)            /* MMX instructions can't pair when both write to memory */
   movq %mm0, 16(%edi)
   movq %mm0, 24(%edi)
   addl $32, %edi                /* inserting those in the MMX copy block makes no diffrence */
   decl %ecx
   jnz clearMMX_continue_line

clearMMX_finish_line:
   movl %eax, %ecx               /* get ecx back */

   testl $31, %ecx               /* check if there's any left */
   jz clearMMX_no_long
				 /* else, write trailing pixels */
   testl $16, %ecx
   jz clearMMX_finish_line2

   movq %mm0, (%edi)
   movq %mm0, 8(%edi)
   addl $16, %edi

clearMMX_finish_line2:
   testl $8, %ecx
   jz clearMMX_finish_line3

   movq %mm0, (%edi)
   addl $8, %edi

clearMMX_finish_line3:
   andl $7, %ecx
   subl $8, %ecx

   movq %mm0, (%edi, %ecx)

clearMMX_no_long:
   incl %ebx
   cmpl %ebx, BMP_CB(%edx)
   jg clearMMX_loop              /* and loop */

   popl %eax

   emms                          /* clear FPU tag word */

   jmp clear_done

clearMMXseg_loop:
   movl %ebx, %eax
   WRITE_BANK()                  /* select bank */
   movl %eax, %edi
   addl BMP_CL(%edx), %edi       /* get line address  */

   popl %eax                     /* Get eax back */

   movl %esi, %ecx               /* width to clear */

   movd %eax, %mm0               /* restore mmx reg 0 in case it's been clobbered by WRITE_BANK() */
   movd %eax, %mm1
   psllq $32, %mm0
   por %mm1, %mm0

   pushl %eax                    /* save eax */

   testl $7, %edi                /* is destination aligned on 64-bit ? */
   jz clearMMXseg_aligned

clearMMXseg_do_alignment:
   movl %edi, %eax               /* we want to adjust %ecx  (pairing: see andl) */

   movq %mm0, %es:(%edi)         /* we clear 8 pixels */

   andl $7, %eax                 /* we calc how may pixels we actually wanted to clear (8 - %eax) (see subl) */

   andl $0xFFFFFFF8, %edi        /* instruction pairing (see inc %edi) */

   subl $8, %eax

   addl $8, %edi                 /* we set %edi to the next aligned memory address */

   addl %ecx, %eax               /* and adjust %ecx to reflect the change */

clearMMXseg_aligned:
   movl %ecx, %eax               /* save for later */
   shrl $5, %ecx                 /* divide by 32 for 4 * 8-byte memory move */
   jz clearMMXseg_finish_line    /* if there's less than 32 pixels to clear, no need for MMX */

clearMMXseg_continue_line:
   movq %mm0, %es:(%edi)         /* move 4x 8 bytes */
   movq %mm0, %es:8(%edi)        /* MMX instructions can't pair when both write to memory */
   movq %mm0, %es:16(%edi)
   movq %mm0, %es:24(%edi)
   addl $32, %edi                /* inserting those in the MMX copy block makes no diffrence */
   decl %ecx
   jnz clearMMXseg_continue_line

clearMMXseg_finish_line:
   movl %eax, %ecx               /* get ecx back */

   testl $31, %ecx               /* check if there's any left */
   jz clearMMXseg_no_long
				 /* else, write trailing pixels */
   testl $16, %ecx
   jz clearMMXseg_finish_line2

   movq %mm0, %es:(%edi)
   movq %mm0, %es:8(%edi)
   addl $16, %edi

clearMMXseg_finish_line2:
   testl $8, %ecx
   jz clearMMXseg_finish_line3

   movq %mm0, %es:(%edi)
   addl $8, %edi

clearMMXseg_finish_line3:
   andl $7, %ecx
   subl $8, %ecx

   movq %mm0, %es:(%edi, %ecx)

clearMMXseg_no_long:
   incl %ebx
   cmpl %ebx, BMP_CB(%edx)
   jg clearMMXseg_loop           /* and loop */

   popl %eax

   emms                          /* clear FPU tag word */

   jmp clear_done

#endif                           /* ALLEGRO_MMX */

clear_no_mmx:
   cld

   _align_
clear_loop:
   movl %ebx, %eax
   WRITE_BANK()                  /* select bank */
   movl %eax, %edi
   addl BMP_CL(%edx), %edi       /* get line address  */

   movb ARG2, %al                /* duplicate color 4 times */
   movb %al, %ah
   shll $16, %eax
   movb ARG2, %al
   movb %al, %ah

   movl %esi, %ecx               /* width to clear */
   shrl $1, %ecx                 /* halve for 16 bit clear */
   jnc clear_no_byte
   stosb                         /* clear an odd byte */

clear_no_byte:
   shrl $1, %ecx                 /* halve again for 32 bit clear */
   jnc clear_no_word
   stosw                         /* clear an odd word */

clear_no_word:
   jz clear_no_long

   _align_
clear_x_loop:
   rep ; stosl                   /* clear the line */

clear_no_long:
   incl %ebx
   cmpl %ebx, BMP_CB(%edx)
   jg clear_loop                 /* and loop */

clear_done:
   UNWRITE_BANK()

   popw %es
   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_clear_to_color8() */




/* void _linear_blit8(BITMAP *source, BITMAP *dest, int source_x, source_y,
 *                                    int dest_x, dest_y, int width, height);
 *  Normal forwards blitting routine for linear bitmaps.
 */
FUNC(_linear_blit8)
   pushl %ebp
   movl %esp, %ebp
   pushw %es
   pushl %edi
   pushl %esi
   pushl %ebx

   movl B_DEST, %edx
   movw BMP_SEG(%edx), %es       /* load destination segment */
   movw %ds, %bx                 /* save data segment selector */
   cld                           /* for forward copy */

   shrl $1, B_WIDTH              /* halve counter for word copies */
   jz blit_only_one_byte
   jnc blit_even_bytes

   _align_
   BLIT_LOOP(words_and_byte, 1,  /* word at a time, plus leftover byte */
      rep ; movsw
      movsb
   )
   jmp blit_done

   _align_
blit_even_bytes:
   shrl $1, B_WIDTH              /* halve counter again, for long copies */
   jz blit_only_one_word
   jnc blit_even_words

   _align_
   BLIT_LOOP(longs_and_word, 1,  /* long at a time, plus leftover word */
      rep ; movsl
      movsw
   )
   jmp blit_done

   _align_
blit_even_words:
   BLIT_LOOP(even_words, 1,      /* copy a long at a time */
      rep ; movsl
   )
   jmp blit_done

   _align_
blit_only_one_byte:
   BLIT_LOOP(only_one_byte, 1,   /* copy just the one byte */
      movsb
   )
   jmp blit_done

   _align_
blit_only_one_word:
   BLIT_LOOP(only_one_word, 1,   /* copy just the one word */
      movsw
   )

   _align_
blit_done:
   movl B_SOURCE, %edx
   UNWRITE_BANK()

   movl B_DEST, %edx
   UNWRITE_BANK()

   popl %ebx
   popl %esi
   popl %edi
   popw %es
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_blit8() */




/* void _linear_blit_backward8(BITMAP *source, BITMAP *dest, int source_x,
 *                      int source_y, int dest_x, dest_y, int width, height);
 *  Reverse blitting routine, for overlapping linear bitmaps.
 */
FUNC(_linear_blit_backward8)
   pushl %ebp
   movl %esp, %ebp
   pushw %es
   pushl %edi
   pushl %esi
   pushl %ebx

   movl B_HEIGHT, %eax           /* y values go from high to low */
   decl %eax
   addl %eax, B_SOURCE_Y
   addl %eax, B_DEST_Y

   movl B_WIDTH, %eax            /* x values go from high to low */
   decl %eax
   addl %eax, B_SOURCE_X
   addl %eax, B_DEST_X

   movl B_DEST, %edx
   movw BMP_SEG(%edx), %es       /* load destination segment */
   movw %ds, %bx                 /* save data segment selector */

   movl B_SOURCE_Y, %eax         /* if different line -> fast dword blit */
   cmpl B_DEST_Y, %eax
   jne blit_backwards_loop_fast

   movl B_SOURCE_X, %eax         /* B_SOURCE_X-B_DEST_X */
   subl B_DEST_X, %eax
   cmpl $3, %eax                 /* if greater than 3 -> fast dword blit */
   jg blit_backwards_loop_fast

   _align_
blit_backwards_loop_slow:
   movl B_DEST, %edx             /* destination bitmap */
   movl B_DEST_Y, %eax           /* line number */
   WRITE_BANK()                  /* select bank */
   addl B_DEST_X, %eax           /* x offset */
   movl %eax, %edi

   movl B_SOURCE, %edx           /* source bitmap */
   movl B_SOURCE_Y, %eax         /* line number */
   READ_BANK()                   /* select bank */
   addl B_SOURCE_X, %eax         /* x offset */
   movl %eax, %esi

   std                           /* backwards */
   movl B_WIDTH, %ecx            /* x loop counter */
   movw BMP_SEG(%edx), %ds       /* load data segment */
   rep ; movsb                   /* copy the line */

   movw %bx, %ds                 /* restore data segment */
   decl B_SOURCE_Y
   decl B_DEST_Y
   decl B_HEIGHT
   jg blit_backwards_loop_slow   /* and loop */

   jmp blit_backwards_end

   _align_
blit_backwards_loop_fast:
   movl B_DEST, %edx             /* destination bitmap */
   movl B_DEST_Y, %eax           /* line number */
   WRITE_BANK()                  /* select bank */
   addl B_DEST_X, %eax           /* x offset */
   movl %eax, %edi

   movl B_SOURCE, %edx           /* source bitmap */
   movl B_SOURCE_Y, %eax         /* line number */
   READ_BANK()                   /* select bank */
   addl B_SOURCE_X, %eax         /* x offset */
   movl %eax, %esi

   std                           /* backwards */
   movl B_WIDTH, %eax            /* x loop counter */
   movw BMP_SEG(%edx), %ds       /* load data segment */

   movl %eax, %ecx
   andl $3, %ecx                 /* copy bytes */
   rep ; movsb                   /* copy the line */

   subl $3, %esi
   subl $3, %edi

   movl %eax, %ecx
   shrl $2, %ecx                 /* copy dwords */
   rep ; movsl                   /* copy the line */

   movw %bx, %ds                 /* restore data segment */
   decl B_SOURCE_Y
   decl B_DEST_Y
   decl B_HEIGHT
   jg blit_backwards_loop_fast   /* and loop */

blit_backwards_end:
   cld                           /* finished */

   movl B_SOURCE, %edx
   UNWRITE_BANK()

   movl B_DEST, %edx
   UNWRITE_BANK()

   popl %ebx
   popl %esi
   popl %edi
   popw %es
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_blit_backward8() */

FUNC(_linear_blit8_end)
   ret




/* void _linear_masked_blit8(BITMAP *source, *dest, int source_x, source_y,
 *                           int dest_x, dest_y, int width, height);
 *  Masked (skipping zero pixels) blitting routine for linear bitmaps.
 */
FUNC(_linear_masked_blit8)
   pushl %ebp
   movl %esp, %ebp
   pushw %es
   pushl %edi
   pushl %esi
   pushl %ebx

   movl B_DEST, %edx
   movw BMP_SEG(%edx), %es
   movw %ds, %bx
   cld

   _align_
   BLIT_LOOP(masked, 1,

      _align_
   masked_blit_x_loop:
      movb (%esi), %al           /* read a byte */
      incl %esi

      orb %al, %al               /* test it */
      jz masked_blit_skip

      movb %al, %es:(%edi)       /* write the pixel */
      incl %edi
      decl %ecx
      jg masked_blit_x_loop
      jmp masked_blit_x_loop_done

      _align_
   masked_blit_skip:
      incl %edi                  /* skip zero pixels */
      decl %ecx
      jg masked_blit_x_loop

   masked_blit_x_loop_done:
   )

   movl B_DEST, %edx
   UNWRITE_BANK()

   popl %ebx
   popl %esi
   popl %edi
   popw %es
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_masked_blit8() */




#endif      /* ifdef ALLEGRO_COLOR8 */
