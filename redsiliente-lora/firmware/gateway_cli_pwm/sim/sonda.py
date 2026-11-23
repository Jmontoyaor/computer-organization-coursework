# =============================================================================
#  sonda.py - "Analizador logico" dentro de Renode
# -----------------------------------------------------------------------------
#  Registra cada cambio de nivel de PB5 (salida PWM) con el tiempo VIRTUAL del
#  microcontrolador emulado, y marcas con el nombre de cada caso de prueba.
#  Se carga desde simulacion.resc con "include @sonda.py"; las funciones mc_*
#  quedan como comandos del monitor de Renode.
# =============================================================================
eventos = []  # (tiempo_s, tipo, valor): tipo "pb5" o "marca"


def _ahora():
    return monitor.Machine.ElapsedVirtualTime.TimeElapsed.TotalSeconds


def mc_sonda_conectar():
    from Antmicro.Renode.Peripherals.Miscellaneous import ILed
    sonda = list(monitor.Machine.GetPeripheralsOfType[ILed]())[0]  # la unica: sonda_pb5

    def al_cambiar(fuente, estado):
        eventos.append((_ahora(), "pb5", 1 if estado else 0))

    sonda.StateChanged += al_cambiar


def mc_sonda_marcar(nombre):
    eventos.append((_ahora(), "marca", nombre))


def mc_sonda_guardar(ruta):
    with open(ruta, "w") as f:
        f.write("t_s,tipo,valor\n")
        for t, tipo, valor in eventos:
            f.write("%.9f,%s,%s\n" % (t, tipo, valor))
    print("%d eventos guardados en %s" % (len(eventos), ruta))
