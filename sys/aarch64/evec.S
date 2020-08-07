#include <aarch64/asm.h>

#define VECT_EMPTY                                                             \
        B       .;                                                             \
        .align 7

        .section .text
        .global exception_vectors

        .align 11
exception_vectors:
        VECT_EMPTY  /* Synchronous EL1t */
        VECT_EMPTY  /* IRQ EL1t */
        VECT_EMPTY  /* FIQ EL1t */
        VECT_EMPTY  /* Error EL1t */

        VECT_EMPTY  /* Synchronous EL1h */
        VECT_EMPTY  /* IRQ EL1h */
        VECT_EMPTY  /* FIQ EL1h */
        VECT_EMPTY  /* Error EL1h */

        VECT_EMPTY  /* Synchronous 64-bit EL0 */
        VECT_EMPTY  /* IRQ 64-bit EL0 */
        VECT_EMPTY  /* FIQ 64-bit EL0 */
        VECT_EMPTY  /* Error 64-bit EL0 */

        VECT_EMPTY  /* Synchronous 32-bit EL0 */
        VECT_EMPTY  /* IRQ 32-bit EL0 */
        VECT_EMPTY  /* FIQ 32-bit EL0 */
        VECT_EMPTY  /* Error 32-bit EL0 */

        .section .boot.text
        .global hypervisor_vectors

        .align 11
hypervisor_vectors:
        VECT_EMPTY  /* Synchronous EL2t */
        VECT_EMPTY  /* IRQ EL2t */
        VECT_EMPTY  /* FIQ EL2t */
        VECT_EMPTY  /* Error EL2t */

        VECT_EMPTY  /* Synchronous EL2h */
        VECT_EMPTY  /* IRQ EL2h */
        VECT_EMPTY  /* FIQ EL2h */
        VECT_EMPTY  /* Error EL2h */

        VECT_EMPTY  /* Synchronous 64-bit EL1 */
        VECT_EMPTY  /* IRQ 64-bit EL1 */
        VECT_EMPTY  /* FIQ 64-bit EL1 */
        VECT_EMPTY  /* Error 64-bit EL1 */

        VECT_EMPTY  /* Synchronous 32-bit EL1 */
        VECT_EMPTY  /* IRQ 32-bit EL1 */
        VECT_EMPTY  /* FIQ 32-bit EL1 */
        VECT_EMPTY  /* Error 32-bit EL1 */

# vim: sw=8 ts=8 et
