TEMPLATE = subdirs

SUBDIRS = \
        ads \
        dark_style \
        graphics_engine \
        cycles_wrapper \
        game_core \
        dashboard_gui


cycles_wrapper.depends = graphics_engine

game_core.depends = graphics_engine
game_core.depends = cycles_wrapper

dashboard_gui.depends = ads
dashboard_gui.depends = dark_style
dashboard_gui.depends = graphics_engine
dashboard_gui.depends = cycles_wrapper
dashboard_gui.depends = game_core
