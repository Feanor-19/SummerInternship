import re

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


def parse_lib_funcs(lib_filename : str) -> list[(str, str, str)]:
    lib_file = open(lib_filename, "r")
    lines = lib_file.readlines()
    
    lib_funcs = []
    for line in lines:
        # comment or an empty line
        if line[0] == COMMENT_SYMBOL or len(line) == 1:
            continue

        line = line.strip()
        ret_type, name, args = re.split("@", line)
        lib_funcs.append((ret_type, name, args))

    return lib_funcs


def start_link_h():
    FILE_LINK_H.write("#pragma once\n\n")


def start_link_cpp():
    FILE_LINK_CPP.write("#include \"link.h\"\n#include <dlfcn.h>\n\n")


def append_link_h(lib_name : str):
    FILE_LINK_H.write("bool dlink_open_"+lib_name+"(const char **err_msg_p = nullptr);\n\n")
    FILE_LINK_H.write("bool dlink_close_"+lib_name+"(const char **err_msg_p = nullptr);\n\n")


def gen_inc_lines(includes : list[str]):
    inc_lines = []
    for inc in includes:
        inc_lines.append(inc.replace("{", "{\n").replace("}", "}\n").replace(">", ">\n"))

    return inc_lines


def append_link_cpp(lib_name : str, includes : list[str], lib_funcs : list[(str, str, str)]):
    # includes
    FILE_LINK_CPP.writelines(gen_inc_lines(includes))
    FILE_LINK_CPP.write("\n\n")

    # global vars
    for lib_func in lib_funcs:
        FILE_LINK_CPP.write(lib_func[0] + " (*DL_" + lib_func[1] + ")" + lib_func[2] + " = nullptr;\n")
        FILE_LINK_CPP.write("typedef " + lib_func[0] + " (*DL_" + lib_func[1] + "_t)" + lib_func[2] + ";\n\n")

    FILE_LINK_CPP.write("\nvoid *DL_handle_" + lib_name + " = nullptr;\n\n")

    # func open
    func_open_lines = ["bool dlink_open_"+lib_name+"(const char **err_msg_p)\n",
                       "{\n",
                       "    void *handle = dlopen(\"lib"+lib_name+".so\", RTLD_LAZY);\n",
                       "    if (!handle) goto Failed;\n\n",
                       "#pragma GCC diagnostic push\n",
                       "#pragma GCC diagnostic ignored \"-Wconditionally-supported\"\n"]

    for lib_func in lib_funcs:
        dl_func_name = "DL_"+lib_func[1]
        dl_func_t    = "DL_"+lib_func[1]+"_t"
        func_open_lines.append("    "+dl_func_name+" = ("+dl_func_t+") dlsym(handle, \""+lib_func[1]+"\");\n")
        func_open_lines.append("    if (!"+dl_func_name+") goto Failed;\n\n")

    func_open_lines.extend(["#pragma GCC diagnostic pop\n",
                           "    DL_handle_"+lib_name+" = handle;\n",
                           "    return true;\n",
                           "Failed:\n",
                           "    if (err_msg_p) \n",
                           "        *err_msg_p = dlerror();\n",
                           "    if (handle) dlclose(handle);\n",
                           "    return false;\n",
                           "}\n\n"])
    FILE_LINK_CPP.writelines(func_open_lines)

    # func close
    func_close_lines = ["bool dlink_close_"+lib_name+"(const char **err_msg_p)\n",
                        "{\n",
                        "   if (DL_handle_"+lib_name+")\n",
                        "   {\n",
                        "       if (dlclose(DL_handle_"+lib_name+") != 0)\n",
                        "       {\n",
                        "           if (err_msg_p)\n",
                        "               *err_msg_p = dlerror();\n",
                        "           return false;\n",
                        "       }\n",
                        "       DL_handle_"+lib_name+" = nullptr;\n",
                        "   }\n",
                        "   return true;\n",
                        "}\n\n"]
    FILE_LINK_CPP.writelines(func_close_lines)


def create_header_file(lib_name : str, includes : list[str], lib_funcs : list[(str, str, str)]):
    h_file = open(INC_DIR+"/dl_"+lib_name+".h", "w")

    h_file.write("#pragma once\n\n")

    # includes
    h_file.writelines(gen_inc_lines(includes))
    h_file.write("\n\n")

    # funcs
    for lib_func in lib_funcs:
        h_file.write("extern "+lib_func[0]+" (*DL_"+lib_func[1]+")"+lib_func[2] + ";\n\n")

    h_file.close()

CODEGEN_DIR = "codegen"
SRC_DIR     = "src"
INC_DIR     = "inc"

CODEGEN_EXT = ".codegen"

MAIN_FILE = CODEGEN_DIR + "/main" + CODEGEN_EXT
LINK_CPP  = SRC_DIR+"/link.cpp"
LINK_H    = INC_DIR+"/link.h"

COMMENT_SYMBOL = ';'

libs = parse_main(MAIN_FILE)

#print(libs)

FILE_LINK_H   = open(LINK_H, "w")
FILE_LINK_CPP = open(LINK_CPP, "w")

start_link_h()
start_link_cpp()

for lib in libs:
    lib_funcs = parse_lib_funcs(CODEGEN_DIR + '/' + lib[0] + CODEGEN_EXT)
    #print(lib_funcs)

    append_link_h(lib[0])
    append_link_cpp(lib[0], lib[1], lib_funcs)
    create_header_file(lib[0], lib[1], lib_funcs)

FILE_LINK_H.close()
FILE_LINK_CPP.close()

print("Code generation done!")