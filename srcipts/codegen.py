def parse_main(main_filename : str) -> list[str]:
    main_file = open(main_filename, "r")
    lines = main_file.readlines()

    libs = []

    for line in lines:
        # comment or an empty line
        if line[0] == COMMENT_SYMBOL or len(line) == 1:
            continue
        libs.append("codegen/" + line.strip() + ".codegen")

    return libs


def parse_lib_funcs(lib_filename : str) -> list[(str, int)]:
    lib_file = open(lib_filename, "r")
    lines = lib_file.readlines()
    
    lib_funcs = []
    for line in lines:
        # comment or an empty line
        if line[0] == COMMENT_SYMBOL or len(line) == 1:
            continue

        line = line.strip()
        ret_type, name, args = line.replace("(", " (").split(" ", 2)
        lib_funcs.append((ret_type, name, args))

    return lib_funcs

MAIN_FILE = "codegen/main.codegen"
COMMENT_SYMBOL = ';'

libs = parse_main(MAIN_FILE)

for lib in libs:
    lib_funcs = parse_lib_funcs(lib)
