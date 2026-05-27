Import("env")

from os.path import isdir, join


framework_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
if framework_dir:
    include_dirs = [
        join(framework_dir, "libraries", "WiFi", "src"),
        join(framework_dir, "libraries", "WiFiClientSecure", "src"),
    ]

    for include_dir in include_dirs:
        if isdir(include_dir):
            env.AppendUnique(CPPPATH=[include_dir])
