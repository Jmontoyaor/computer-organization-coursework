/*
 * main.s
 *
 * Multi-Function Shield sobre NUCLEO-L476RG
 *  - Reproduce el tema principal de Super Mario Bros con el zumbador
 *  - El display de 7 segmentos muestra la frecuencia (Hz) de la nota que suena
 *
 * El tono lo genera el TIM2 por hardware (PWM al 50 %):
 *   ARR  = 4 000 000 / frecuencia - 1   (reloj MSI de 4 MHz, sin prescaler)
 *   CCR2 = ARR / 2
 *
 * Conexiones de la shield (Arduino -> Nucleo):
 *   ZUMBADOR      = D3  = PB3  (TIM2_CH2, AF1; activo en bajo)
 *   LED D1..D4    = D13, D12, D11, D10 = PA5, PA6, PA7, PB6 (activos en bajo)
 *   74HC595 LATCH = D4 = PB5
 *   74HC595 CLOCK = D7 = PA8
 *   74HC595 DATA  = D8 = PA9
 */

.syntax unified

.global main

.equ	RCC_BASE,		0x40021000
.equ	RCC_AHB2ENR,	0x4C		// reloj de GPIOA/GPIOB (RM0351, page 251)
.equ	RCC_APB1ENR1,	0x58		// reloj de TIM2 (RM0351, page 255)

.equ	GPIOA_BASE,		0x48000000	// GPIO BASE ADDRESS (RM0351, page 78)
.equ	GPIOB_BASE,		0x48000400
.equ	GPIO_MODER,		0x00		// GPIO port mode register (RM0351, page 303)
.equ	GPIO_BSRR,		0x18		// bits 0-15 ponen el pin en 1, bits 16-31 en 0
.equ	GPIO_AFRL,		0x20		// funcion alterna de los pines 0..7 (RM0351, page 307)

.equ	TIM2_BASE,		0x40000000	// (RM0351, page 1033 en adelante)
.equ	TIM_CR1,		0x00
.equ	TIM_EGR,		0x14
.equ	TIM_CCMR1,		0x18
.equ	TIM_CCER,		0x20
.equ	TIM_PSC,		0x28
.equ	TIM_ARR,		0x2C
.equ	TIM_CCR2,		0x38

.equ	F_RELOJ,		4000000		// MSI a 4 MHz (valor de arranque)
.equ	DELAY_DIGITO,	1000		// ~1 ms encendido por digito del display
.equ	PAUSA_NOTAS,	6			// silencio corto entre notas (barridos)
.equ	PAUSA_FINAL,	400			// silencio antes de repetir la cancion

// Duracion en barridos del display (cada barrido dura ~4.2 ms)
.equ	T,				26			// tiempo basico ~0.11 s
.equ	T3,				35			// nota de tresillo ~0.15 s

// Frecuencias de las notas (Hz)
.equ	MI5,	659
.equ	SOL5,	784
.equ	LA5,	880
.equ	LAS5,	932			// La sostenido
.equ	SI5,	988
.equ	DO6,	1047
.equ	RE6,	1175
.equ	MI6,	1319
.equ	FA6,	1397
.equ	SOL6,	1568
.equ	LA6,	1760
.equ	SIL,	0			// silencio

.section .text.main
.type	main,%function
main:

// ACTIVAR RELOJ DE GPIOA, GPIOB Y TIM2
	ldr r2, =RCC_BASE
	ldr r1, [r2, #RCC_AHB2ENR]
	orr r1, #0x3				// GPIOA y GPIOB
	str r1, [r2, #RCC_AHB2ENR]
	ldr r1, [r2, #RCC_APB1ENR1]
	orr r1, #0x1				// TIM2
	str r1, [r2, #RCC_APB1ENR1]

// PA5..PA9 COMO SALIDA (01): bits 10..19 de MODER
	ldr r2, =GPIOA_BASE
	ldr r1, [r2, #GPIO_MODER]
	ldr r3, =(0x3FF << 10)
	bic r1, r3
	ldr r3, =(0x155 << 10)
	orr r1, r3
	str r1, [r2, #GPIO_MODER]

// PB5, PB6 COMO SALIDA (01) Y PB3 COMO FUNCION ALTERNA (10)
	ldr r2, =GPIOB_BASE
	ldr r1, [r2, #GPIO_MODER]
	bic r1, #(0xF << 10)
	orr r1, #(0x5 << 10)
	bic r1, #(0x3 << 6)
	orr r1, #(0x2 << 6)
	str r1, [r2, #GPIO_MODER]
	ldr r1, [r2, #GPIO_AFRL]	// PB3 -> AF1 (TIM2_CH2)
	bic r1, #(0xF << 12)
	orr r1, #(0x1 << 12)
	str r1, [r2, #GPIO_AFRL]

// ESTADO INICIAL: LEDs apagados (1), CLOCK en 0, LATCH en 1
	ldr r2, =GPIOA_BASE
	ldr r1, =((0x7 << 5) | (1 << (8+16)))
	str r1, [r2, #GPIO_BSRR]
	ldr r2, =GPIOB_BASE
	ldr r1, =((1 << 6) | (1 << 5))
	str r1, [r2, #GPIO_BSRR]

// TIM2 EN MODO PWM 1 POR EL CANAL 2
	ldr r2, =TIM2_BASE
	movs r1, #0
	str r1, [r2, #TIM_PSC]		// sin prescaler: cuenta a 4 MHz
	ldr r1, =((6 << 12) | (1 << 11))
	str r1, [r2, #TIM_CCMR1]	// OC2M = 110 (PWM 1), OC2PE = 1
	movs r1, #(1 << 4)
	str r1, [r2, #TIM_CCER]		// CC2E = 1: habilita la salida del canal 2
	bl silencio
	movs r1, #((1 << 7) | 1)
	str r1, [r2, #TIM_CR1]		// ARPE = 1, CEN = 1: arranca el timer

cancion:
	ldr r8, =partitura
nota:
	ldrh r9, [r8], #2			// r9 = frecuencia (Hz), 0 = silencio
	ldrh r10, [r8], #2			// r10 = duracion (barridos), 0 = fin
	cmp r10, #0
	beq fin_cancion

	mov r0, r9
	cbz r0, nota_silencio
	bl tono
	b nota_sonar
nota_silencio:
	bl silencio
nota_sonar:
	mov r0, r9
	mov r1, r10
	bl mostrar					// muestra la frecuencia mientras suena

	bl silencio					// separa una nota de la siguiente
	mov r0, r9
	movs r1, #PAUSA_NOTAS
	bl mostrar
	b nota

fin_cancion:
	bl silencio
	movs r0, #0
	ldr r1, =PAUSA_FINAL
	bl mostrar
	b cancion
.size	main, .-main


/*
 * tono: r0 = frecuencia en Hz
 * Programa el periodo del TIM2 y deja el ciclo de trabajo en 50 %.
 */
.type	tono,%function
tono:
	ldr r1, =F_RELOJ
	udiv r1, r1, r0
	subs r1, #1					// r1 = ARR = 4 MHz / f - 1
	ldr r2, =TIM2_BASE
	str r1, [r2, #TIM_ARR]
	lsr r1, r1, #1
	str r1, [r2, #TIM_CCR2]		// mitad del periodo en alto, mitad en bajo
	movs r1, #1
	str r1, [r2, #TIM_EGR]		// UG: carga ARR y CCR2 de inmediato
	bx lr
.size	tono, .-tono


/*
 * silencio: deja PB3 siempre en 1 (zumbador apagado).
 * En PWM 1 la salida esta en 1 mientras CNT < CCR2, y CCR2 = 0xFFFFFFFF
 * nunca se alcanza.
 */
.type	silencio,%function
silencio:
	ldr r2, =TIM2_BASE
	mov r1, #0xFFFFFFFF
	str r1, [r2, #TIM_CCR2]
	movs r1, #1
	str r1, [r2, #TIM_EGR]
	bx lr
.size	silencio, .-silencio


/*
 * mostrar: r0 = numero (0..9999), r1 = numero de barridos del display
 * Multiplexa los 4 digitos; los ceros a la izquierda quedan apagados.
 */
.type	mostrar,%function
mostrar:
	push {r4-r7, lr}
	mov r5, r0
	mov r7, r1
barrido:
	mov r6, r5					// r6 = numero que se va descomponiendo
	movs r4, #0					// r4 = posicion (0 = unidades ... 3 = miles)
digito:
	cbz r4, digito_numero		// las unidades siempre se muestran
	cbz r6, digito_apagado		// cero a la izquierda: ya no quedan cifras
digito_numero:

	movs r1, #10
	udiv r2, r6, r1				// r2 = r6 / 10
	mls r3, r2, r1, r6			// r3 = r6 - r2*10 = digito actual
	mov r6, r2
	ldr r0, =tabla_segmentos
	ldrb r0, [r0, r3]			// r0 = patron de segmentos del digito
	b digito_enviar
digito_apagado:
	movs r0, #0xFF				// todos los segmentos apagados
digito_enviar:
	ldr r1, =tabla_posicion
	ldrb r1, [r1, r4]			// r1 = seleccion del digito en el display
	bl escribir_display

	ldr r0, =DELAY_DIGITO
	bl retardo

	adds r4, #1
	cmp r4, #4
	blt digito

	subs r7, #1
	bne barrido
	pop {r4-r7, pc}
.size	mostrar, .-mostrar


/*
 * escribir_display: r0 = segmentos, r1 = seleccion de digito
 * Envia los dos bytes a los 74HC595 y luego da el pulso de LATCH.
 */
.type	escribir_display,%function
escribir_display:
	push {r4, lr}
	mov r4, r1
	ldr r2, =GPIOB_BASE
	mov r1, #(1 << (5+16))		// LATCH en 0
	str r1, [r2, #GPIO_BSRR]
	bl enviar_byte				// primero los segmentos
	mov r0, r4
	bl enviar_byte				// luego la seleccion de digito
	ldr r2, =GPIOB_BASE
	mov r1, #(1 << 5)			// LATCH en 1: los 595 muestran los datos
	str r1, [r2, #GPIO_BSRR]
	pop {r4, pc}
.size	escribir_display, .-escribir_display


/*
 * enviar_byte: r0 = byte, se envia el bit mas significativo primero
 * DATA = PA9, CLOCK = PA8 (el 595 lee el dato en el flanco de subida)
 */
.type	enviar_byte,%function
enviar_byte:
	ldr r2, =GPIOA_BASE
	movs r3, #8
bit:
	tst r0, #0x80
	ite ne
	movne r1, #(1 << 9)			// DATA = 1
	moveq r1, #(1 << (9+16))	// DATA = 0
	str r1, [r2, #GPIO_BSRR]
	mov r1, #(1 << 8)			// CLOCK = 1
	str r1, [r2, #GPIO_BSRR]
	mov r1, #(1 << (8+16))		// CLOCK = 0
	str r1, [r2, #GPIO_BSRR]
	lsl r0, r0, #1
	subs r3, #1
	bne bit
	bx lr
.size	enviar_byte, .-enviar_byte


/*
 * retardo: r0 = numero de vueltas
 */
.type	retardo,%function
retardo:
	subs r0, #1
	bne retardo
	bx lr
.size	retardo, .-retardo


.section .rodata
// Display de anodo comun: segmento encendido = 0 (bits: dp g f e d c b a)
tabla_segmentos:
	.byte 0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90	// 0..9
// Seleccion de digito: indice 0 = unidades (derecha) ... 3 = miles (izquierda)
tabla_posicion:
	.byte 0xF8, 0xF4, 0xF2, 0xF1

// Tema de Super Mario Bros: pares (frecuencia Hz, duracion); duracion 0 = fin
.balign 2
partitura:
	// Introduccion
	.hword MI6, T,  MI6, T,  SIL, T,  MI6, T,  SIL, T,  DO6, T,  MI6, T,  SIL, T
	.hword SOL6, T, SIL, 3*T,  SOL5, T, SIL, 3*T

	// Melodia (se toca dos veces)
	.rept 2
	.hword DO6, T,  SIL, 2*T,  SOL5, T,  SIL, 2*T,  MI5, T,  SIL, 2*T
	.hword LA5, T,  SIL, T,  SI5, T,  SIL, T,  LAS5, T,  LA5, T,  SIL, T
	.hword SOL5, T3,  MI6, T3,  SOL6, T3
	.hword LA6, T,  SIL, T,  FA6, T,  SOL6, T,  SIL, T,  MI6, T,  SIL, T
	.hword DO6, T,  RE6, T,  SI5, T,  SIL, 2*T
	.endr

	.hword 0, 0
