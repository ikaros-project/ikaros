const selector =
{
    selected_foreground: [],
    selected_connection: null,
    selected_background: null,

    showBackgroundSelection(reveal_inspector=false, rebuild_main=false)
    {
        nav.selectItem(selector.selected_background);
        breadcrumbs.selectItem(selector.selected_background);
        if(rebuild_main)
            main.selectItem([], selector.selected_background);
        else
            main.updateComponentStates();
        inspector.showInspectorForSelection(reveal_inspector);
    },

    showForegroundSelection(reveal_inspector=false, rebuild_main=false)
    {
        if(rebuild_main)
            main.selectItem(selector.selected_foreground, selector.selected_background);
        else
            main.updateComponentStates();
        inspector.showInspectorForSelection(reveal_inspector);
    },

    selectItemsAndReveal(foreground=[], background=null, toggle=false, extend=false, force_rebuild=false)
    {
        selector.selectItems(foreground, background, toggle, extend, force_rebuild, true);
    },

    selectConnectionAndReveal(connection)
    {
        selector.selectConnection(connection, true);
    },

    selectBackgroundAndReveal()
    {
        selector.selectBackground(true);
    },

    selectItems(foreground=[], background=null, toggle=false, extend=false, force_rebuild=false, reveal_inspector=false)
    {
        const previous_background = selector.selected_background;
        const previous_connection = selector.selected_connection;
        selector.selected_connection = null;

        if(background !== null)
            selector.selected_background = background;

        setCookie("selected_background", selector.selected_background);

        const background_changed = (background !== null && background !== previous_background);

        if(toggle)
            toggleStrings(selector.selected_foreground, foreground);
        else if(extend)
            selector.selected_foreground = [...new Set([...selector.selected_foreground, ...foreground])];
        else
            selector.selected_foreground = foreground;

        if(selector.selected_background === null)
            return;

        if(background_changed)
            main.applyBackgroundGridSpacing();

        if(background_changed || force_rebuild)
        {
            if(selector.selected_foreground.length == 0)
                selector.showBackgroundSelection(reveal_inspector, true);
            else
                selector.showForegroundSelection(reveal_inspector, true);
            return;
        }

        const needs_rebuild = selector.selected_foreground.some((name) => !document.getElementById(name));
        if(needs_rebuild)
        {
            selector.showForegroundSelection(reveal_inspector, true);
            return;
        }

        if(previous_connection && previous_connection !== selector.selected_connection)
            main.deselectConnection(previous_connection);

        if(selector.selected_foreground.length == 0)
            selector.showBackgroundSelection(reveal_inspector, false);
        else
            selector.showForegroundSelection(reveal_inspector, false);
    },

    selectConnection(connection, reveal_inspector=false)
    {
        const previous_connection = selector.selected_connection;
        selector.selected_foreground = [];
        selector.selected_connection = connection;
        main.selectItem(selector.selected_foreground, selector.selected_background);
        if(previous_connection && previous_connection !== connection)
            main.deselectConnection(previous_connection);
        main.selectConnection(connection);
        inspector.showInspectorForSelection(reveal_inspector);
    },

    selectBackground(reveal_inspector=false)
    {
        if(selector.selected_foreground.length === 0 && selector.selected_connection == null)
        {
            if(inspector && typeof inspector.showInspectorForSelection === "function")
                inspector.showInspectorForSelection(reveal_inspector);
            return;
        }
        selector.selectItems([], selector.selected_background, false, false, false, reveal_inspector);
    },

    selectError(error)
    {
        let c = splitAtLastDot(error);
        let d = splitAtLastDot(c[0]);
        selector.selectItems([c[0]], d[0]);
        inspector.showSingleSelection(selector.selected_foreground[0]);
        inspector.showComponent();
        main.setEditMode();
    },

    getLocalPath(s)
    {
        return removeStringFromStart(getStringUpToBracket(s), selector.selected_background + '.');
    },

    setLogLevel(level)
    {
        if(!["pause", "play", "realtime"].includes(controller.run_mode))
            return;

        let component = "";
        if(selector.selected_foreground.length == 1)
            component = selector.selected_foreground[0];
        else if(selector.selected_background)
            component = selector.selected_background;

        controller.queueCommand("control", getStringUpToBracket(component) + ".log_level", {"value":level});
    }
};
