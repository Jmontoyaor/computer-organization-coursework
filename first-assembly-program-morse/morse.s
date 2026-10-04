# =============================================================================
#  morse.s  -  Baliza Morse en ensamblador RISC-V (RV32I) para el SoC FemtoRV32
# =============================================================================
#
#  QUE HACE
#  --------
#  Transmite un mensaje en codigo Morse haciendo parpadear TODOS los LEDs de la
#  placa a la vez (todos encendidos = "senal", todos apagados = "silencio").
#  El mensaje por defecto es "1 2 3 4" y se repite indefinidamente. Para que
#  diga "HOLA MUNDO" basta con cambiar la cadena `mensaje` (ver el final).
#
#  HARDWARE OBJETIVO
#  -----------------
#  - Placa Tang Primer 20K (FPGA Gowin GW2A-18, reloj de 27 MHz).
#  - CPU FemtoRV32 Quark, juego de instrucciones RV32I (sin multiplicar, sin
#    dividir, sin instrucciones comprimidas).
#  - RAM (BRAM) de 1 KB en 0x00000000..0x000003FF: contiene codigo, datos y pila.
#  - Registro de LEDs mapeado en memoria en 0x00010000 (solo se usan los 4 bits
#    bajos; el SoC invierte la salida porque los LEDs de la placa son activos
#    en bajo, de modo que aqui 1 = encendido y 0 = apagado).
#
#  CODIGO MORSE (norma UIT) - todo se mide en "unidades" de tiempo:
#    punto (.)                  = 1 unidad encendido
#    raya  (-)                  = 3 unidades encendido
#    espacio entre elementos    = 1 unidad apagado (dentro de una misma letra)
#    espacio entre letras       = 3 unidades apagado
#    espacio entre palabras     = 7 unidades apagado
#
#  COMO SE CODIFICA CADA CARACTER (tabla `tabla`)
#  ----------------------------------------------
#  Cada caracter ocupa un byte. Se leen los bits desde el menos significativo:
#    bit 0 = primer elemento, bit 1 = segundo, ...   (0 = punto, 1 = raya)
#  Por encima del ultimo elemento hay un bit "centinela" en 1 que marca el
#  final. Asi un solo byte codifica la longitud y el patron:
#      '1' = .----  ->  centinela 1 | bits (LSB primero) 0,1,1,1,1  = 0b111110
#      'A' = .-     ->  centinela 1 | bits (LSB primero) 0,1        = 0b000110
#  El algoritmo de envio es:  mascara = 1;  mientras (2*mascara <= codigo) {
#  enviar raya si (codigo & mascara) sino punto;  mascara = 2*mascara; }
#  (2*mascara <= codigo es falso justo cuando la mascara llega al centinela.)
#
#  CONVENCION DE REGISTROS (no hay pila ni llamadas desde C, main no retorna)
#  -----------------------------------------------------------------------
#    s0 = direccion del registro de LEDs (constante)
#    s1 = puntero al caracter actual del mensaje
#    s2 = iteraciones del bucle de retardo por unidad de tiempo (constante)
#    s3 = codigo Morse del caracter en curso
#    t0 = mascara del bit en curso (tambien guarda el caracter al traducirlo)
#    t3 = siguiente mascara (2 * t0)
#    a0 = argumento de `esperar` (numero de unidades)
#    t1, t2, t4 = temporales
#
#  COMO COMPILARLO
#  ---------------
#  Este archivo lo recoge automaticamente CMake (sw/*.s). Flujo normal:
#      cmake --build build --target compile_hls      (ensamblar -> firmware.hex)
#      cmake --build build --target build_bitstream  (sintesis + place&route)
#      cmake --build build --target flash            (cargar en la SRAM)
# =============================================================================

# -----------------------------------------------------------------------------
#  Constantes
# -----------------------------------------------------------------------------
        .equ    LED_REG,    0x00010000  # registro de LEDs mapeado en memoria
        .equ    LED_ON,     0xFF        # todos los LEDs encendidos a la vez
                                        # (el SoC ignora los bits por encima del 3)
        .equ    TABLA_LEN,  43          # entradas de la tabla: de '0'(0x30) a 'Z'(0x5A)

# Iteraciones del bucle de retardo que equivalen a UNA unidad de tiempo Morse.
# Calibracion (medida en simulacion RTL del SoC, ver informe): cada iteracion
# del bucle (addi + bnez) cuesta 6 ciclos de reloj a 27 MHz, mas ~22 ciclos
# fijos por unidad. Para 0,2 s por unidad: 0,2 s * 27e6 / 6 = 900000.
# Se puede sobreescribir al ensamblar con -Wa,--defsym,UNIT_LOOPS=<n>
# (asi se simula el diseno completo en segundos en vez de minutos).
        .ifndef UNIT_LOOPS
        .equ    UNIT_LOOPS, 900000
        .endif

# =============================================================================
#  CODIGO
# =============================================================================
        .section .text
        .globl  main

# -----------------------------------------------------------------------------
#  main - punto de entrada (lo invoca boot.s con `jal main`). No retorna.
# -----------------------------------------------------------------------------
main:
        li      s0, LED_REG             # s0 <- direccion del registro de LEDs
        li      s2, UNIT_LOOPS          # s2 <- iteraciones por unidad de tiempo
        sw      zero, 0(s0)             # arranca con todos los LEDs apagados

repetir:
        la      s1, mensaje             # s1 <- inicio del mensaje (reinicia el puntero)

# -- Bucle exterior: un caracter del mensaje por iteracion ---------------------
siguiente_caracter:
        lbu     t0, 0(s1)               # t0 <- caracter ASCII actual (sin signo)
        addi    s1, s1, 1               # avanza el puntero al siguiente caracter
        beqz    t0, fin_mensaje         # caracter 0 = fin de la cadena

        li      t1, ' '                 # es un espacio entre palabras?
        bne     t0, t1, buscar_codigo   # no: es un caracter normal
        li      a0, 4                   # si: 3 (ya dadas tras la letra) + 4 = 7 unidades
        jal     ra, esperar             # silencio de palabra
        j       siguiente_caracter      # continua con el caracter siguiente

# -- Traduce el caracter ASCII a su codigo Morse mediante la tabla -------------
buscar_codigo:
        li      t1, 'a'                 # minuscula? (ASCII >= 'a')
        bltu    t0, t1, 1f              # no: se deja tal cual
        addi    t0, t0, -32             # si: pasa a mayuscula ('a'-32 = 'A')
1:      addi    t0, t0, -'0'            # indice de tabla = caracter - '0'
        li      t1, TABLA_LEN           # tamano de la tabla
        bgeu    t0, t1, siguiente_caracter  # fuera de rango (la comparacion sin signo
                                            # tambien descarta caracteres < '0')
        la      t1, tabla               # t1 <- base de la tabla
        add     t1, t1, t0              # t1 <- direccion de la entrada
        lbu     s3, 0(t1)               # s3 <- codigo Morse del caracter
        beqz    s3, siguiente_caracter  # entrada 0 = caracter sin equivalente Morse

# -- Bucle interior: un elemento (punto o raya) por iteracion ------------------
# Se recorre el codigo con una MASCARA (t0) que va probando el bit 0, 1, 2...
# en vez de desplazar el codigo. Se evita a proposito cualquier instruccion de
# desplazamiento (slli/srli): el desplazador de esta CPU tiene un fallo y
# desplaza una posicion de menos (ver informe). Multiplicar por 2 = sumar.
        li      t0, 1                   # t0 <- mascara del bit en curso (1, 2, 4, ...)
emitir_elemento:
        add     t3, t0, t0              # t3 <- 2 * mascara (siguiente mascara)
        bltu    s3, t3, fin_caracter    # codigo < 2*mascara: solo queda el centinela

        and     t1, s3, t0              # t1 <- bit en curso del codigo (0 o distinto de 0)
        sltu    t1, zero, t1            # t1 <- 1 si es raya, 0 si es punto
        add     a0, t1, t1              # a0 <- 0 (punto) o 2 (raya)
        addi    a0, a0, 1               # a0 <- 1 unidad (punto) o 3 unidades (raya)

        li      t2, LED_ON              # ---- SENAL ----
        sw      t2, 0(s0)               # enciende TODOS los LEDs a la vez
        jal     ra, esperar             # los mantiene a0 unidades
        sw      zero, 0(s0)             # apaga todos los LEDs

        li      a0, 1                   # 1 unidad de silencio entre elementos
        jal     ra, esperar
        mv      t0, t3                  # mascara <- 2 * mascara (t3 sobrevive a esperar)
        j       emitir_elemento         # siguiente elemento de la misma letra

# -- Fin de una letra: completa el silencio entre letras -----------------------
fin_caracter:
        li      a0, 2                   # 1 unidad ya dada + 2 = 3 unidades entre letras
        jal     ra, esperar
        j       siguiente_caracter

# -- Fin del mensaje: pausa larga y vuelta a empezar ---------------------------
fin_mensaje:
        li      a0, 14                  # pausa entre repeticiones del mensaje
        jal     ra, esperar
        j       repetir

# -----------------------------------------------------------------------------
#  esperar - retardo de a0 unidades de tiempo Morse
#
#  Entrada : a0 = numero de unidades (0 no espera nada)
#  Usa     : t4 (contador interno); modifica a0 (queda en 0)
#  Espera  : a0 * UNIT_LOOPS iteraciones de un bucle de dos instrucciones.
# -----------------------------------------------------------------------------
esperar:
        beqz    a0, 3f                  # nada que esperar
1:      mv      t4, s2                  # t4 <- iteraciones de UNA unidad
2:      addi    t4, t4, -1              # bucle interno: cuenta hacia atras...
        bnez    t4, 2b                  # ...hasta llegar a 0
        addi    a0, a0, -1              # una unidad menos por esperar
        bnez    a0, 1b                  # repite mientras queden unidades
3:      ret                             # vuelve al llamador (jalr zero, 0(ra))

# =============================================================================
#  DATOS DE SOLO LECTURA
# =============================================================================
        .section .rodata

# Mensaje a transmitir (terminado en 0). Solo se soportan 0-9, A-Z, a-z y
# espacio. Para el otro mensaje, comentar la primera linea y descomentar la
# segunda:
mensaje:
        .asciz  "1 2 3 4"
#       .asciz  "HOLA MUNDO"

# Tabla Morse indexada por (ASCII - '0'), de '0' (0x30) a 'Z' (0x5A).
# Formato de cada byte: ver la cabecera (bit 0 = primer elemento, 1 = raya,
# con un bit centinela en 1 encima del ultimo elemento). Las posiciones
# 0x3A..0x40 (:;<=>?@) no tienen codigo y valen 0.
tabla:
        .byte   0x3F                    # 0  -----
        .byte   0x3E                    # 1  .----
        .byte   0x3C                    # 2  ..---
        .byte   0x38                    # 3  ...--
        .byte   0x30                    # 4  ....-
        .byte   0x20                    # 5  .....
        .byte   0x21                    # 6  -....
        .byte   0x23                    # 7  --...
        .byte   0x27                    # 8  ---..
        .byte   0x2F                    # 9  ----.
        .byte   0x00, 0x00, 0x00, 0x00  # : ; < =      (sin codigo)
        .byte   0x00, 0x00, 0x00        # > ? @        (sin codigo)
        .byte   0x06                    # A  .-
        .byte   0x11                    # B  -...
        .byte   0x15                    # C  -.-.
        .byte   0x09                    # D  -..
        .byte   0x02                    # E  .
        .byte   0x14                    # F  ..-.
        .byte   0x0B                    # G  --.
        .byte   0x10                    # H  ....
        .byte   0x04                    # I  ..
        .byte   0x1E                    # J  .---
        .byte   0x0D                    # K  -.-
        .byte   0x12                    # L  .-..
        .byte   0x07                    # M  --
        .byte   0x05                    # N  -.
        .byte   0x0F                    # O  ---
        .byte   0x16                    # P  .--.
        .byte   0x1B                    # Q  --.-
        .byte   0x0A                    # R  .-.
        .byte   0x08                    # S  ...
        .byte   0x03                    # T  -
        .byte   0x0C                    # U  ..-
        .byte   0x18                    # V  ...-
        .byte   0x0E                    # W  .--
        .byte   0x19                    # X  -..-
        .byte   0x1D                    # Y  -.--
        .byte   0x13                    # Z  --..
