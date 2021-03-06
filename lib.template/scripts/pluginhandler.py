# -*- coding: koi8-r -*-

"""��������� ��������������� ��������.

������� ������������ ����� ����������� ������, ������� � ����� scripts/plugins.

������ ������ ������������ 2 �������:
def initialize():
    "�������������� ������: ������������ �������, ������������� ����������� ������� � �.�."
def terminate():
    "������� ��� �������, ������� �����������. ����� ������ ���� ������� ������ ����� ���� �������� �� ������."
"""

import pkgutil
import sys
import mud
import plugins
from utils import log_err

running_plugins = set()

def initialize():
    global running_plugins
    for loader, name, isPkg in pkgutil.iter_modules(plugins.__path__):
        if name.startswith("_"):
            continue
        try:
            plugin = __import__("plugins." + name, globals(), locals(), ("plugins",))
        except:
            log_err("Error importing plugin " + name)
            continue
        try:
            plugin.initialize()
        except:
            log_err("Error initializing plugin " + name)
            continue
        running_plugins.add(plugin)


def terminate():
    global running_plugins
    for plugin in list(running_plugins):
        running_plugins.discard(plugin)
        try:
            plugin.terminate()
        except:
            log_err("Error terminating plugin " % plugin.__name__)


def reload():
    global plugins
    terminate()
    del plugins
    mods=[k for k,v in sys.modules.iteritems() if k.startswith("plugins") and v is not None]
    for mod in mods:
        del sys.modules[mod]
    import plugins
    initialize()
