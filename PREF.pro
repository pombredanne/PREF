TEMPLATE = subdirs

pref_ui.file = PREF/PREF.pro
preflib.file = PrefLib/PrefLib.pro

PREF.depends = preflib pref_ui

CONFIG += ordered
SUBDIRS = preflib pref_ui
