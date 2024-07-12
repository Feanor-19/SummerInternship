def parse_main(main_filename : str) -> list[(str, list[str])]:
    main_file = open(main_filename, "r")
    lines = main_file.readlines()

    libs = []
    cur_lib_includes = []

    for line in lines:
        # comment or an empty line
        if line[0] == COMMENT_SYMBOL or len(line) == 1:
            continue

        line = line.strip()

        if line.startswith(("#include", "extern \"C\"")):
            cur_lib_includes.append(line)
        else:
            libs.append((line, list(cur_lib_includes)))
            cur_lib_includes.clear()

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

print(libs)

for lib in libs:
    lib_funcs = parse_lib_funcs("codegen/" + lib[0] + ".codegen")
