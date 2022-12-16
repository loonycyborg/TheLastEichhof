env = Environment()

env.ParseConfig("allegro-config --cflags --libs")

env.Append(CFLAGS = "-fcommon")

env.Program("beer", Split("""
    fileman.c
    xmodeasm.c
    xmodec.c
    xmodedef.c
    soundc.c
    gameasm.c
    support.c
    gameplay.c
    shop.c
    menu.c
    baller.c
    hiscore.c
    intro.c
    """)
)
