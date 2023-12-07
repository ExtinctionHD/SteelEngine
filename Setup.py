import sys
import os
import platform

if platform.system() == "Windows":
    import _winapi
    is_windows = True
else:
    is_windows = False


def log(msg):
    formatted_msg = msg.replace("\\", '/')
    print("-- {0}".format(formatted_msg))


def suitable_python_version():
    major = sys.version_info[0]
    minor = sys.version_info[1]
    return major > 3 or major == 3 and minor >= 5


def create_link(src, link, is_dir):
    src_abs = os.path.abspath(src)
    link_abs = os.path.abspath(link)

    if os.path.exists(src_abs):
        if not os.path.exists(link_abs):
            if is_dir:
                if is_windows:
                    _winapi.CreateJunction(src_abs, link_abs)
                else:
                    os.symlink(src_abs, link_abs, True)
            else:
                os.link(src_abs, link_abs)
            log("Link created: {0} => {1}".format(src_abs, link_abs))
        else:
            log("Link already exists".format(link_abs))
    else:
        log("Invalid link source: {0}".format(src_abs))


def setup(project_path, build_path, is_msvc):
    create_link(os.path.join(project_path, "Source/Shaders/"), os.path.join(build_path, "Shaders/"), True)
    create_link(os.path.join(project_path, "Assets/"), os.path.join(build_path, "Assets/"), True)
    create_link(os.path.join(project_path, "LocalAssets/"), os.path.join(build_path, "LocalAssets/"), True)
    create_link(os.path.join(project_path, "Config/imgui.ini"), os.path.join(build_path, "imgui.ini"), False)
    if is_msvc:
        create_link(os.path.join(project_path, "Config/SteelEngine.sln.DotSettings"),
                    os.path.join(build_path, "SteelEngine.sln.DotSettings"), False)


# Entry point
if suitable_python_version():
    if len(sys.argv) > 3:
        setup(project_path=sys.argv[1], build_path=sys.argv[2], is_msvc=sys.argv[3])
    else:
        log("Invalid arguments list: {0}".format(sys.argv))
else:
    log("Minimum required python version is 3.5")
