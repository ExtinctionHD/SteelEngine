import sys
import os
import platform

if platform.system() == "Windows":
    import _winapi
    isWindows = True


def log(msg):
    formatted_msg = msg.replace("\\", '/')
    print("-- {0}".format(formatted_msg))


def check_python_version():
    major = sys.version_info[0]
    minor = sys.version_info[1]
    if major > 3 or major == 3 and minor >= 5:
        return True

    return False


def create_dir_link(src, link):
    src_abs = os.path.abspath(src)
    link_abs = os.path.abspath(link)
    if os.path.exists(src_abs) and os.path.isdir(src_abs):
        if not os.path.exists(link_abs) and not os.path.islink(link_abs):
            if isWindows:
                _winapi.CreateJunction(src_abs, link_abs)
            else:
                os.symlink(src_abs, link_abs, True)
            log("Link created: {0} => {1}".format(src_abs, link_abs))
        else:
            log("Directory {0} already exists".format(link_abs))
    else:
        log("Invalid source directory: {0}".format(src_abs))


def setup(project_path, build_path):
    create_dir_link(os.path.join(project_path, "Shaders/"), os.path.join(build_path, "Shaders/"))
    create_dir_link(os.path.join(project_path, "Assets/"), os.path.join(build_path, "Assets/"))


if check_python_version():
    if len(sys.argv) > 2:
        setup(project_path=sys.argv[1], build_path=sys.argv[2])
    else:
        log("Invalid arguments list: {0}".format(sys.argv))
else:
    log("Minimum required python version is 3.5")
